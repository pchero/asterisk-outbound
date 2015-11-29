/*
 * db_handler.c
 *
 *  Created on: Nov 7, 2015
 *      Author: pchero
 */


#include "db_handler.h"

#include "asterisk.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <mysql/mysql.h>

#include "asterisk/utils.h"
#include "asterisk/module.h"
#include "asterisk/json.h"
#include "asterisk/lock.h"


static MYSQL* g_db = NULL;
AST_MUTEX_DEFINE_STATIC(g_mysql_mutex);



#define MAX_BIND_BUF 4096
#define DELIMITER   0x02

/**
 Connect to db.

 @return Success:TRUE, Fail:FALSE
 */
int db_connect(const char* host, int port, const char* user, const char* pass, const char* dbname)
{
    int ret;
    ret = mysql_thread_safe();
    ast_log(LOG_DEBUG, "Thread safe. ret[%d]\n", ret);

    if(g_db == NULL) {
        g_db = mysql_init(NULL);
        ast_log(LOG_DEBUG, "Initiated mysql. g_db[%p]\n", g_db);
    }

    if(g_db == NULL) {
        ast_log(LOG_ERROR, "Could not initiate mysql. err[%d:%s]\n", mysql_errno(g_db), mysql_error(g_db));
        return false;
    }

    // connect
    g_db = mysql_real_connect(g_db, host, user, pass, dbname, port, NULL, 0);
    if(g_db == NULL) {
        ast_log(LOG_ERROR, "Could not connect to mysql. err[%d:%s]\n", mysql_errno(g_db), mysql_error(g_db));
        mysql_close(g_db);
        return false;
    }
    ast_log(LOG_VERBOSE, "Connected to mysql. host[%s], user[%s], dbname[%s]\n", host, user, dbname);

    return true;
}

/**
 Disconnect to db.
 */
void db_exit(void)
{
    if(g_db == NULL) {
        ast_log(LOG_WARNING, "Nothing to release.\n");
        return;
    }
    ast_log(LOG_NOTICE, "Release database context. g_db[%p]\n", g_db);

    mysql_close(g_db);
    g_db = NULL;
}

/**
 database query function. (select)
 @param query
 @return Success:, Fail:NULL
 */
db_res_t* db_query(char* query)
{
    int ret;
    db_res_t*   db_ctx;
    MYSQL_RES*  result;

    if(query == NULL) {
        ast_log(LOG_WARNING, "Could not execute NULL query.\n");
        return NULL;
    }

    if(g_db == NULL) {
        ast_log(LOG_WARNING, "Wrong DB context.\n");
        return NULL;
    }

    ast_mutex_lock(&g_mysql_mutex);
    ret = mysql_query(g_db, query);
    if(ret != 0) {
        ast_mutex_unlock(&g_mysql_mutex);
        ast_log(LOG_ERROR, "Could not query to db. sql[%s], err[%d:%s]\n", query, mysql_errno(g_db), mysql_error(g_db));
        return NULL;
    }

    result = mysql_store_result(g_db);
    if(result == NULL) {
        ast_mutex_unlock(&g_mysql_mutex);
        ast_log(LOG_ERROR, "Could not store result. sql[%s], err[%d:%s]\n", query, mysql_errno(g_db), mysql_error(g_db));
        return NULL;
    }
    ast_mutex_unlock(&g_mysql_mutex);

    db_ctx = ast_calloc(1, sizeof(db_res_t));
    db_ctx->result = result;

    return db_ctx;
}

/**
 * database query execute function. (update, delete, insert)
 * @param query
 * @return  success:true, fail:false
 */
int db_exec(char* query)
{
    int ret;

    ast_mutex_lock(&g_mysql_mutex);
    ret = mysql_query(g_db, query);
    ast_mutex_unlock(&g_mysql_mutex);
    if(ret != 0) {
        ast_log(LOG_ERROR, "Could not execute query. qeury[%s], err[%d:%s]\n", query, ret, mysql_error(g_db));
        return false;
    }
    return true;
}

/**
 * Return 1 record info by json.
 * If there's no more record or error happened, it will return NULL.
 * @param res
 * @return  success:json_t*, fail:NULL
 */
struct ast_json* db_get_record(db_res_t* ctx)
{
    struct ast_json* j_res;
    MYSQL_RES* res;
    MYSQL_ROW row;
    MYSQL_FIELD* field;
    int field_cnt;
    int i;

    res = (MYSQL_RES*)ctx->result;

    row = mysql_fetch_row(res);
    if(row == NULL) {
        return NULL;
    }

    field = mysql_fetch_fields(res);
    field_cnt = mysql_num_fields(res);

    j_res = ast_json_object_create();
    for(i = 0; i < field_cnt; i++) {
        if(row[i] == NULL) {
            ast_json_object_set(j_res, field[i].name, ast_json_null());
            continue;
        }

        switch(field[i].type) {
            case MYSQL_TYPE_DECIMAL:
            case MYSQL_TYPE_TINY:
            case MYSQL_TYPE_SHORT:
            case MYSQL_TYPE_LONG:
            case MYSQL_TYPE_LONGLONG: {
                ast_json_object_set(j_res, field[i].name, ast_json_integer_create(atoi(row[i])));
            }
            break;

            case MYSQL_TYPE_FLOAT:
            case MYSQL_TYPE_DOUBLE: {
                ast_json_object_set(j_res, field[i].name, ast_json_real_create(atof(row[i])));
            }
            break;

            case MYSQL_TYPE_NULL: {
                ast_json_object_set(j_res, field[i].name, ast_json_null());
            }
            break;

            default: {
                ast_json_object_set(j_res, field[i].name, ast_json_string_create(row[i]));
            }
            break;
        }
    }
    return j_res;
}

/**
 *
 * @param ctx
 */
void db_free(db_res_t* ctx)
{
    if(ctx == NULL) {
        return;
    }
    mysql_free_result(ctx->result);
    ast_free(ctx);

    return;
}

/**
 * Insert j_data into table.
 * @param table
 * @param j_data
 * @return
 */
int db_insert(const char* table, const struct ast_json* j_data)
{
    char*               sql;
    char*               tmp;
    struct ast_json*    j_val;
    struct ast_json*    j_data_cp;
    const char*         key;
    bool                is_first;
    int                 ret;
    enum ast_json_type  type;
    char*               sql_keys;
    char*               sql_values;
    char*               tmp_sub;
    struct ast_json_iter    *iter;


    // copy original.
    j_data_cp = ast_json_deep_copy(j_data);

    // set keys
    is_first = true;
    tmp = NULL;
    sql_keys    = NULL;
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
            case AST_JSON_INTEGER:
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
    ast_json_unref(j_data_cp);

    ret = ast_asprintf(&sql, "insert into %s(%s) values (%s);", table, sql_keys, sql_values);
    ast_free(sql_keys);
    ast_free(sql_values);

    ret = db_exec(sql);
    ast_free(sql);
    if(ret == false) {
        ast_log(LOG_ERROR, "Could not insert dialing info.\n");
        return false;
    }

    return true;
}

/**
 * Return part of update sql.
 * @param j_data
 * @return
 */
char* db_get_update_str(const struct ast_json* j_data)
{
    char*       res;
    char*       tmp;
    struct ast_json*     j_val;
    struct ast_json*     j_data_cp;
    const char* key;
    bool        is_first;
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
                ast_asprintf(&res, "%s%s = %ld", tmp, key, ast_json_integer_get(j_val));
            }
            break;

            case AST_JSON_REAL: {
                ast_asprintf(&res, "%s%s = %f", tmp, key, ast_json_real_get(j_val));
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

    ast_json_unref(j_data_cp);

    return res;
}
