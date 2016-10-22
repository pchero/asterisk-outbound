/*
 * db_handler.c
 *
 *  Created on: Nov 7, 2015
 *	  Author: pchero
 */



#include "asterisk.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
//#include <mysql/mysql.h>

#include "asterisk/utils.h"
#include "asterisk/module.h"
#include "asterisk/json.h"
#include "asterisk/lock.h"

#include "res_outbound.h"
#include "db_sqlite3_handler.h"
//#include "db_mysql_handler.h"
#include "db_handler.h"


/**
 * database type
 */
typedef enum _E_DB_TYPE
{
	E_DB_NONE			= 0,
	E_DB_SQLITE3,				///< sqlite3
//	E_DB_MYSQL,  				///< mysql, maria
} E_DB_TYPE;

E_DB_TYPE g_db_type = E_DB_NONE;



const char* g_db_func[3][1] =
{
		{""},

		// E_DB_SQLITE3
		{"random()"},

		// E_DB_MYSQL
		{"rand()"}
};



static void set_db_type(E_DB_TYPE type) {
	g_db_type = type;
}

static E_DB_TYPE get_db_type(void) {
	return g_db_type;
}

bool db_init(void)
{
	const char* tmp_const;
	struct ast_json* j_database;
	E_DB_TYPE type;
	int ret;

	// get [database]
	j_database = ast_json_object_get(g_app->j_conf, "database");
	if(j_database == NULL) {
		ast_log(LOG_ERROR, "Could not get database configuration.\n");
		return false;
	}

	// get database type.
	tmp_const = ast_json_string_get(ast_json_object_get(j_database, "db_type"));
	if(tmp_const == NULL) {
		ast_log(LOG_ERROR, "Could not get database configure option. option[%s]\n", "db_type");
		return false;
	}

	// set database type
	ret = atoi(tmp_const);
	set_db_type(ret);

	// get db type
	type = get_db_type();

	switch(type) {
		case E_DB_SQLITE3: {
			return db_sqlite3_init();
		}
		break;

//		case E_DB_MYSQL: {
//			return db_mysql_init();
//		}
//		break;

		default: {
			ast_log(LOG_ERROR, "Unsupported database type. type[%d]\n", type);
			return false;
		}
	}

	// should not reach to here.
	ast_log(LOG_ERROR, "Could not initiate database.\n");

	return false;
}

/**
 Disconnect to db.
 */
void db_exit(void)
{
	E_DB_TYPE type;

	type = get_db_type();

	switch(type) {
		case E_DB_SQLITE3: {
			return db_sqlite3_exit();
		}
		break;

//		case E_DB_MYSQL: {
//			return db_mysql_exit();
//		}
//		break;

		default: {
			ast_log(LOG_ERROR, "Unsupported database type. type[%d]\n", type);
			return;
		}
	}
	return;
}

/**
 database query function. (select)
 @param query
 @return Success:, Fail:NULL
 */
db_res_t* db_query(const char* query)
{
	E_DB_TYPE type;

	if(query == NULL) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return NULL;
	}

	type = get_db_type();

	switch(type) {
		case E_DB_SQLITE3: {
			return db_sqlite3_query(query);
		}
		break;

//		case E_DB_MYSQL: {
//			return db_mysql_query(query);
//		}
//		break;

		default: {
			ast_log(LOG_ERROR, "Unsupported database type. type[%d]\n", type);
			return NULL;
		}
	}

	// Should not reach to here.
	ast_log(LOG_ERROR, "Could not call the correct database handler.\n");

	return NULL;
}

/**
 * database query execute function. (update, delete, insert)
 * @param query
 * @return  success:true, fail:false
 */
bool db_exec(const char* query)
{
	E_DB_TYPE type;

	type = get_db_type();

	if(query == NULL) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return false;
	}

	switch(type) {
		case E_DB_SQLITE3: {
			return db_sqlite3_exec(query);
		}
		break;

//		case E_DB_MYSQL: {
//			return db_mysql_exec(query);
//		}
//		break;

		default: {
			ast_log(LOG_ERROR, "Unsupported database type. type[%d]\n", type);
			return false;
		}
	}

	// Should not reach to here.
	ast_log(LOG_ERROR, "Could not call the correct database handler.\n");

	return false;
}

/**
 * Return 1 record info by json.
 * If there's no more record or error happened, it will return NULL.
 * @param res
 * @return  success:json_t*, fail:NULL
 */
struct ast_json* db_get_record(db_res_t* ctx)
{
	E_DB_TYPE type;

	type = get_db_type();

	switch(type) {
		case E_DB_SQLITE3: {
			return db_sqlite3_get_record(ctx);
		}
		break;

//		case E_DB_MYSQL: {
//			return db_mysql_get_record(ctx);
//		}
//		break;

		default: {
			ast_log(LOG_ERROR, "Unsupported database type. type[%d]\n", type);
			return NULL;
		}
	}

	// Should not reach to here.
	ast_log(LOG_ERROR, "Could not call the correct database handler.\n");

	return NULL;
}

/**
 *
 * @param ctx
 */
void db_free(db_res_t* db_res)
{
	E_DB_TYPE type;

	if(db_res == NULL) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return;
	}

	type = get_db_type();

	switch(type) {
		case E_DB_SQLITE3: {
			return db_sqlite3_free(db_res);
		}
		break;

//		case E_DB_MYSQL: {
//			return db_mysql_free(db_res);
//		}
//		break;

		default: {
			ast_log(LOG_ERROR, "Unsupported database type. type[%d]\n", type);
			return;
		}
	}

	// Should not reach to here.
	ast_log(LOG_ERROR, "Could not call the correct database handler.\n");

	return;
}

/**
 * Insert j_data into table.
 * @param table
 * @param j_data
 * @return
 */
bool db_insert(const char* table, const struct ast_json* j_data)
{
	E_DB_TYPE type;

	type = get_db_type();

	switch(type) {
		case E_DB_SQLITE3: {
			return db_sqlite3_insert(table, j_data);
		}
		break;

//		case E_DB_MYSQL: {
//			return db_mysql_insert(table, j_data);
//		}
//		break;

		default: {
			ast_log(LOG_ERROR, "Unsupported database type. type[%d]\n", type);
			return false;
		}
	}

	// Should not reach to here.
	ast_log(LOG_ERROR, "Could not call the correct database handler.\n");

	return false;
}

/**
 * Return part of update sql.
 * @param j_data
 * @return
 */
char* db_get_update_str(const struct ast_json* j_data)
{
	E_DB_TYPE type;

	type = get_db_type();

	switch(type) {
		case E_DB_SQLITE3: {
			return db_sqlite3_get_update_str(j_data);
		}
		break;

//		case E_DB_MYSQL: {
//			return db_mysql_get_update_str(j_data);
//		}
//		break;

		default: {
			ast_log(LOG_ERROR, "Unsupported database type. type[%d]\n", type);
			return NULL;
		}
	}

	// Should not reach to here.
	ast_log(LOG_ERROR, "Could not call the correct database handler.\n");

	return NULL;
}

const char* db_translate_function(E_DB_FUNC e_func) {
	E_DB_TYPE type;

	type = get_db_type();

	if(e_func < 0) {
		ast_log(LOG_ERROR, "Wrong input parameter.\n");
		return NULL;
	}

	if(e_func >= sizeof(g_db_func[type])) {
		ast_log(LOG_ERROR, "Could not find correct function translate. e_func[%d]\n", e_func);
		return NULL;
	}

	return g_db_func[type][e_func];
}
