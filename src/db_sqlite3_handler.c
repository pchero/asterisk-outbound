/*
 * sqlite3_handler.c
 *
 *  Created on: Oct 11, 2016
 *      Author: pchero
 */


#include "asterisk.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sqlite3.h>

#include "asterisk/utils.h"
#include "asterisk/module.h"
#include "asterisk/json.h"
#include "asterisk/lock.h"

#include "res_outbound.h"
#include "db_sqlite3_handler.h"
#include "db_sql_create.h"
#include "utils.h"

static sqlite3* g_db = NULL;

#define MAX_BIND_BUF 4096
#define DELIMITER   0x02
#define MAX_MEMDB_LOCK_RELEASE_TRY 100
#define MAX_MEMDB_LOCK_RELEASE_TRY 100

typedef struct {
	int max_retry;  /* Max retry times. */
	int sleep_ms;   /* Time to sleep before retry again. */
} busy_handler_attr;


static bool db_sqlite3_connect(const char* filename);
//static bool db_sqlite3_lock(void);
//static bool db_sqlite3_release(void);
//static void db_sqlite3_msleep(unsigned long milisec);
static int db_sqlite3_busy_handler(void *data, int retry);

bool db_sqlite3_init(void)
{
	int ret;
	struct ast_json* j_database;
	struct ast_json* j_res;
	char* sql;
	db_res_t* db_res;

	// get [database]
	j_database = ast_json_object_get(g_app->j_conf, "database");
	if(j_database == NULL) {
		ast_log(LOG_ERROR, "Could not get database configuration.\n");
		return false;
	}

	// db connect
	ret = db_sqlite3_connect(ast_json_string_get(ast_json_object_get(j_database, "db_sqlite3_data")));
	if(ret == false) {
		ast_log(LOG_ERROR, "Could not initiate sqlite3 database.\n");
		return false;
	}

	// check table exist(campaign)
	ast_asprintf(&sql, "SELECT name FROM sqlite_master WHERE type='table' AND name='%s';", "campaign");
	db_res = db_sqlite3_query(sql);
	ast_free(sql);
	j_res = db_sqlite3_get_record(db_res);
	db_sqlite3_free(db_res);
	if(j_res != NULL) {
		AST_JSON_UNREF(j_res);
		return true;
	}

	// create new
	ast_log(LOG_NOTICE, "Could not find correct table info. Create tables.\n");

	// plan
	ret = db_sqlite3_exec(g_db_sql_plan);
	if(ret == false) {
		ast_log(LOG_ERROR, "Could not create table. table[%s]\n", "plan");
		return false;
	}

	// queue
	ret = db_sqlite3_exec(g_db_sql_queue);
	if(ret == false) {
		ast_log(LOG_ERROR, "Could not create table. table[%s]\n", "queue");
		return false;
	}

	// dial_list
	ret = db_sqlite3_exec(g_db_sql_dial_list);
	if(ret == false) {
		ast_log(LOG_ERROR, "Could not create table. table[%s]\n", "dial_list");
		return false;
	}

	// dial_list_ma
	ret = db_sqlite3_exec(g_db_sql_dl_list_ma);
	if(ret == false) {
		ast_log(LOG_ERROR, "Could not create table. table[%s]\n", "dl_list_ma");
		return false;
	}

	// campaign
	ret = db_sqlite3_exec(g_db_sql_campaign);
	if(ret == false) {
		ast_log(LOG_ERROR, "Could not create table. table[%s]\n", "campaign");
		return false;
	}

	// dl_result
	ret = db_sqlite3_exec(g_sql_dl_result);
	if(ret == false) {
		ast_log(LOG_ERROR, "Could not create table. table[%s]\n", "dl_result");
		return false;
	}

	// destination
	ret = db_sqlite3_exec(g_db_sql_destination);
	if(ret == false) {
		ast_log(LOG_ERROR, "Could not create table. table[%s]\n", "destination");
		return false;
	}

	return true;
}

/**
 Connect to db.

 @return Success:TRUE, Fail:FALSE
 */
static bool db_sqlite3_connect(const char* filename)
{
	int ret;

	if(g_db != NULL) {
		ast_log(LOG_NOTICE, "Database is already connected.\n");
		return true;
	}

	ret = sqlite3_open(filename, &g_db);
	if(ret != SQLITE_OK) {
		ast_log(LOG_ERROR, "Could not initiate database. err[%s]\n", sqlite3_errmsg(g_db));
		return false;
	}
	ast_log(LOG_VERBOSE, "Connected to database. filename[%s]\n", filename);

	return true;
}

/**
 Disconnect to db.
 */
void db_sqlite3_exit(void)
{
	int ret;

	if(g_db == NULL) {
		ast_log(LOG_NOTICE, "Released database context already.\n");
		return;
	}

	ret = sqlite3_close(g_db);
	if(ret != SQLITE_OK) {
		ast_log(LOG_WARNING, "Could not close the database correctly. err[%s]\n", sqlite3_errmsg(g_db));
		return;
	}

	ast_log(LOG_NOTICE, "Released database context.\n");
	g_db = NULL;
}

/**
 database query function. (select)
 @param query
 @return Success:, Fail:NULL
 */
db_res_t* db_sqlite3_query(const char* query)
{
	int ret;
	sqlite3_stmt* result;
	db_res_t* db_res;

	if(query == NULL) {
		ast_log(LOG_WARNING, "Could not query NULL query.\n");
		return NULL;
	}

	ret = sqlite3_prepare_v2(g_db, query, -1, &result, NULL);
	if(ret != SQLITE_OK) {
		ast_log(LOG_ERROR, "Could not prepare query. query[%s], err[%s]\n", query, sqlite3_errmsg(g_db));
		return NULL;
	}

	db_res = ast_calloc(1, sizeof(db_res_t));
	db_res->res = result;

	return db_res;
}

/**
 * database query execute function. (update, delete, insert)
 * @param query
 * @return  success:true, fail:false
 */
bool db_sqlite3_exec(const char* query)
{
	int ret;
	char* err;
	busy_handler_attr bh_attr;

	if(query == NULL) {
		ast_log(LOG_WARNING, "Could not execute NULL query.\n");
		return false;
	}

	/* Setup busy handler for all following operations. */
	bh_attr.max_retry = 100;  /* Max retry times */
	bh_attr.sleep_ms = 100;   /* Sleep 100ms before each retry */
	sqlite3_busy_handler(g_db, db_sqlite3_busy_handler, &bh_attr);

	// execute
	ret = sqlite3_exec(g_db, query, NULL, 0, &err);
	if(ret != SQLITE_OK) {
		ast_log(LOG_ERROR, "Could not execute query. query[%s], err[%s]\n", query, err);
		sqlite3_free(err);
		return false;
	}
	sqlite3_free(err);

	return true;
}

/**
 * Return 1 record info by json.
 * If there's no more record or error happened, it will return NULL.
 * @param res
 * @return  success:json_t*, fail:NULL
 */
struct ast_json* db_sqlite3_get_record(db_res_t* ctx)
{
	int ret;
	int cols;
	int i;
	struct ast_json* j_res;
	struct ast_json* j_tmp;
	int type;

	ret = sqlite3_step(ctx->res);
	if(ret != SQLITE_ROW) {
		if(ret != SQLITE_DONE) {
			ast_log(LOG_ERROR, "Could not patch the result. ret[%d], err[%s]", ret, sqlite3_errmsg(g_db));
		}
		return NULL;
	}

	cols = sqlite3_column_count(ctx->res);
	j_res = ast_json_object_create();
	for(i = 0; i < cols; i++) {
		j_tmp = NULL;
		type = sqlite3_column_type(ctx->res, i);
		switch(type) {
			case SQLITE_INTEGER: {
				j_tmp = ast_json_integer_create(sqlite3_column_int(ctx->res, i));
			}
			break;

			case SQLITE_FLOAT: {
				j_tmp = ast_json_real_create(sqlite3_column_double(ctx->res, i));
			}
			break;

			case SQLITE_NULL: {
				j_tmp = ast_json_null();
			}
			break;

			case SQLITE3_TEXT:
			{
				j_tmp = ast_json_string_create((const char*)sqlite3_column_text(ctx->res, i));
			}
			break;

			case SQLITE_BLOB:
			default:
			{
				// not done yet.
				ast_log(LOG_WARNING, "Not supported type. type[%d]\n", type);
				j_tmp = ast_json_null();
			}
			break;
		}

		if(j_tmp == NULL) {
			ast_log(LOG_WARNING, "Could not parse result column. name[%s], type[%d]",
					sqlite3_column_name(ctx->res, i), type);
			j_tmp = ast_json_null();
		}
		ast_json_object_set(j_res, sqlite3_column_name(ctx->res, i), j_tmp);
	}

	return j_res;
}

/**
 *
 * @param ctx
 */
void db_sqlite3_free(db_res_t* db_res)
{
	if(db_res == NULL) {
		return;
	}

	sqlite3_finalize(db_res->res);
	ast_free(db_res);

	return;
}

/**
 * Insert j_data into table.
 * @param table
 * @param j_data
 * @return
 */
bool db_sqlite3_insert(const char* table, const struct ast_json* j_data)
{
	char*			   sql;
	char*			   tmp;
	struct ast_json*	j_val;
	struct ast_json*	j_data_cp;
	const char*		 key;
	bool				is_first;
	int				 ret;
	enum ast_json_type  type;
	char*			   sql_keys;
	char*			   sql_values;
	char*			   tmp_sub;
	struct ast_json_iter	*iter;

	ast_log(LOG_VERBOSE, "db_insert.\n");
	if((table == NULL) || (j_data == NULL)) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return false;
	}

	// copy original.
	j_data_cp = ast_json_deep_copy(j_data);

	// set keys
	is_first = true;
	tmp = NULL;
	sql_keys	= NULL;
	sql_values  = NULL;
	iter = ast_json_object_iter(j_data_cp);
	while(iter) {
		key = ast_json_object_iter_key(iter);
		if(is_first == true) {
			is_first = false;
			ret = ast_asprintf(&tmp, "%s", key);
		}
		else {
			ret = ast_asprintf(&tmp, "%s, %s", sql_keys, key);
		}
		ast_free(sql_keys);
		ret = ast_asprintf(&sql_keys, "%s", tmp);

		ast_free(tmp);
		iter = ast_json_object_iter_next(j_data_cp, iter);
	}

	// set values
	is_first = true;
	tmp = NULL;
	iter = ast_json_object_iter(j_data_cp);
	while(iter) {
		if(is_first == true) {
			is_first = false;
			ret = ast_asprintf(&tmp_sub, "%s", " ");
		}
		else {
			ret = ast_asprintf(&tmp_sub, "%s, ", sql_values);
		}

		// get type.
		j_val = ast_json_object_iter_value(iter);
		type = ast_json_typeof(j_val);
		switch(type) {
			// string
			case AST_JSON_STRING: {
				ret = ast_asprintf(&tmp, "%s\'%s\'", tmp_sub, ast_json_string_get(j_val));
			}
			break;

			// numbers
			case AST_JSON_INTEGER: {
				ret = ast_asprintf(&tmp, "%s%"PRIdMAX"", tmp_sub, ast_json_integer_get(j_val));
			}
			break;

			case AST_JSON_REAL: {
				ret = ast_asprintf(&tmp, "%s%f", tmp_sub, ast_json_real_get(j_val));
			}
			break;

			// true
			case AST_JSON_TRUE: {
				ret = ast_asprintf(&tmp, "%s\"%s\"", tmp_sub, "true");
			}
			break;

			// false
			case AST_JSON_FALSE: {
				ret = ast_asprintf(&tmp, "%s\"%s\"", tmp_sub, "false");
			}
			break;

			case AST_JSON_NULL: {
				ret = ast_asprintf(&tmp, "%s\"%s\"", tmp_sub, "null");
			}
			break;

			// object
			// array
			default: {
				// Not done yet.

				// we don't support another types.
				ast_log(LOG_WARNING, "Wrong type input. We don't handle this.\n");
				ret = ast_asprintf(&tmp, "%s\"%s\"", tmp_sub, "null");
			}
			break;
		}

		ast_free(tmp_sub);
		ast_free(sql_values);
		ret = ast_asprintf(&sql_values, "%s", tmp);

		ast_free(tmp);

		iter = ast_json_object_iter_next(j_data_cp, iter);
	}
	AST_JSON_UNREF(j_data_cp);

	ret = ast_asprintf(&sql, "insert into %s(%s) values (%s);", table, sql_keys, sql_values);
	ast_free(sql_keys);
	ast_free(sql_values);

	ret = db_exec(sql);
	ast_free(sql);
	if(ret == false) {
		ast_log(LOG_ERROR, "Could not insert data.\n");
		return false;
	}

	return true;
}

/**
 * Return part of update sql.
 * @param j_data
 * @return
 */
char* db_sqlite3_get_update_str(const struct ast_json* j_data)
{
	char*	   res;
	char*	   tmp;
	struct ast_json*	 j_val;
	struct ast_json*	 j_data_cp;
	const char* key;
	bool		is_first;
	__attribute__((unused)) int ret;
	enum ast_json_type   type;
	struct ast_json_iter *iter;

	// copy original data.
	j_data_cp = ast_json_deep_copy(j_data);

	is_first = true;
	res = NULL;
	tmp = NULL;

	iter = ast_json_object_iter(j_data_cp);
	while(iter) {
		// copy/set previous sql.
		if(is_first == true) {
			ast_asprintf(&tmp, "%s", " ");
			is_first = false;
		}
		else {
			ast_asprintf(&tmp, "%s, ", res);
		}
		ast_free(res);

		j_val = ast_json_object_iter_value(iter);
		key = ast_json_object_iter_key(iter);
		type = ast_json_typeof(j_val);
		switch(type) {
			// string
			case AST_JSON_STRING: {
				ast_asprintf(&res, "%s%s = \'%s\'", tmp, key, ast_json_string_get(j_val));
			}
			break;

			// numbers
			case AST_JSON_INTEGER: {
				ast_asprintf(&res, "%s%s = %"PRIdMAX"", tmp, key, ast_json_integer_get(j_val));
			}
			break;

			case AST_JSON_REAL: {
				ast_asprintf(&res, "%s%s = %lf", tmp, key, ast_json_real_get(j_val));
			}
			break;

			// true
			case AST_JSON_TRUE: {
				ast_asprintf(&res, "%s%s = \"%s\"", tmp, key, "true");
			}
			break;

			// false
			case AST_JSON_FALSE: {
				ast_asprintf(&res, "%s%s = \"%s\"", tmp, key, "false");
			}
			break;

			case AST_JSON_NULL: {
				ast_asprintf(&res, "%s%s = %s", tmp, key, "null");
			}
			break;

			// object
			// array
			default: {
				// Not done yet.
				// we don't support another types.
				ast_log(LOG_WARNING, "Wrong type input. We don't handle this.\n");
				ast_asprintf(&res, "%s%s = %s", tmp, key, key);
			}
			break;
		}
		ast_free(tmp);
		iter = ast_json_object_iter_next(j_data_cp, iter);
	}

	AST_JSON_UNREF(j_data_cp);

	return res;
}

///**
// * Do the database lock.
// * Keep try MAX_DB_ACCESS_TRY.
// * Each try has 100 ms delay.
// */
//static bool db_sqlite3_lock(void)
//{
//	int ret;
//	int i;
//	char* err;
//
//	for(i = 0; i < MAX_MEMDB_LOCK_RELEASE_TRY; i++) {
//		ret = sqlite3_exec(g_db, "BEGIN IMMEDIATE", NULL, 0, &err);
//		sqlite3_free(err);
//		if(ret == SQLITE_OK) {
//			break;
//		}
//		db_sqlite3_msleep(100);
//	}
//
//	if(ret != SQLITE_OK) {
//		return false;
//	}
//	return true;
//}
//
//
///**
// * Release database lock.
// * Keep try MAX_MEMDB_LOCK_RELEASE_TRY.
// * Each try has 100 ms delay.
// */
//static bool db_sqlite3_release(void)
//{
//	int ret;
//	int i;
//	char* err;
//
//	for(i = 0; i < MAX_MEMDB_LOCK_RELEASE_TRY; i++) {
//		ret = sqlite3_exec(g_db, "COMMIT", NULL, 0, &err);
//		sqlite3_free(err);
//		if(ret == SQLITE_OK) {
//			break;
//		}
//		db_sqlite3_msleep(100);
//	}
//
//	if(ret != SQLITE_OK) {
//		return false;
//	}
//	return true;
//}

///**
// * Millisecond sleep.
// * @param milisec
// */
//static void db_sqlite3_msleep(unsigned long milisec)
//{
//    struct timespec req = {0, 0};
//    time_t sec = (int)(milisec/1000);
//    milisec = milisec - (sec * 1000);
//
//    req.tv_sec = sec;
//    req.tv_nsec = milisec * 1000000L;
//
//    while(nanosleep(&req, &req) == -1)
//    {
//        continue;
//    }
//    return;
//}

/*
 * Busy callback handler for all operations. If the busy callback handler is
 * NULL, then SQLITE_BUSY or SQLITE_IOERR_BLOCKED is returned immediately upon
 * encountering the lock. If the busy callback is not NULL, then the callback
 * might be invoked.
 *
 * This callback will be registered by SQLite's API:
 *    int sqlite3_busy_handler(sqlite3*, int(*)(void*,int), void*);
 * That's very useful to deal with SQLITE_BUSY event automatically. Otherwise,
 * you have to check the return code, reset statement and do retry manually.
 *
 * We've to use ANSI C declaration here to eliminate warnings in Visual Studio.
 */
static int db_sqlite3_busy_handler(void *data, int retry)
{
	busy_handler_attr* attr;

	attr = (busy_handler_attr*)data;

	if (retry < attr->max_retry) {
		/* Sleep a while and retry again. */
		ast_log(LOG_VERBOSE, "Hits SQLITE_BUSY %d times, retry again.\n", retry);
		sqlite3_sleep(attr->sleep_ms);

		/* Return non-zero to let caller retry again. */
		return 1;
	}

	/* Return zero to let caller return SQLITE_BUSY immediately. */
	ast_log(LOG_ERROR, "Retried too many, exit. retry[%d]\n", retry);
	return 0;
}
