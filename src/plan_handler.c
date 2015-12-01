/*
 * plan_handler.c
 *
 *  Created on: Nov 30, 2015
 *      Author: pchero
 */


#include "asterisk.h"
#include "asterisk/logger.h"
#include "asterisk/json.h"
#include "asterisk/utils.h"

#include <stdbool.h>

#include "plan_handler.h"
#include "event_handler.h"
#include "db_handler.h"
#include "campaign_handler.h"
#include "cli_handler.h"

/**
 * Create plan.
 * @param j_plan
 * @return
 */
int create_plan(struct ast_json* j_plan)
{
    int ret;
    char* uuid;
    struct ast_json* j_tmp;

    if(j_plan == NULL) {
        return false;
    }

    j_tmp = ast_json_deep_copy(j_plan);
    uuid = gen_uuid();
    ast_json_object_set(j_tmp, "uuid", ast_json_string_create(uuid));

    ast_log(LOG_NOTICE, "Create plan. uuid[%s], name[%s]\n",
            ast_json_string_get(ast_json_object_get(j_tmp, "uuid")),
            ast_json_string_get(ast_json_object_get(j_tmp, "name"))
            );
    ret = db_insert("plan", j_tmp);
    ast_json_unref(j_tmp);
    if(ret == false) {
        ast_free(uuid);
        return false;
    }

    // send ami event
    j_tmp = get_plan_info(uuid);
    send_manager_evt_plan_create(j_tmp);
    ast_json_unref(j_tmp);

    return true;
}

/**
 * Delete plan.
 * @param uuid
 * @return
 */
int delete_plan(const char* uuid)
{
    struct ast_json* j_tmp;
    char* tmp;
    char* sql;
    int ret;

    if(uuid == NULL) {
        // invalid parameter.
        return false;
    }

    j_tmp = ast_json_object_create();
    tmp = get_utc_timestamp();
    ast_json_object_set(j_tmp, "tm_delete", ast_json_string_create(tmp));
    ast_json_object_set(j_tmp, "in_use", ast_json_integer_create(0));
    ast_free(tmp);

    tmp = db_get_update_str(j_tmp);
    ast_json_unref(j_tmp);
    ast_asprintf(&sql, "update plan set %s where uuid=\"%s\";", tmp, uuid);
    ast_free(tmp);

    ret = db_exec(sql);
    ast_free(sql);
    if(ret == false) {
        ast_log(LOG_WARNING, "Could not delete plan. uuid[%s]\n", uuid);
        return false;
    }

    // send notification
    send_manager_evt_plan_delete(uuid);

    return true;
}


/**
 * Get plan record info.
 * @param uuid
 * @return
 */
struct ast_json* get_plan_info(const char* uuid)
{
    char* sql;
    struct ast_json* j_res;
    db_res_t* db_res;

    if(uuid == NULL) {
        ast_log(LOG_WARNING, "Invalid input parameters.\n");
        return NULL;
    }

    ast_asprintf(&sql, "select * from plan where uuid = \"%s\";", uuid);

    db_res = db_query(sql);
    ast_free(sql);
    if(db_res == NULL) {
        ast_log(LOG_ERROR, "Could not get plan info. uuid[%s]\n", uuid);
        return NULL;
    }

    j_res = db_get_record(db_res);
    db_free(db_res);

    return j_res;
}

/**
 * Get all plan info.
 * @return
 */
struct ast_json* get_plan_info_all(void)
{
    char* sql;
    struct ast_json* j_res;
    struct ast_json* j_tmp;
    db_res_t* db_res;

    ast_asprintf(&sql, "%s", "select * from plan;");

    db_res = db_query(sql);
    ast_free(sql);
    if(db_res == NULL) {
        ast_log(LOG_ERROR, "Could not get plan all info.\n");
        return NULL;
    }

    j_res = ast_json_array_create();
    while(1) {
        j_tmp = db_get_record(db_res);
        if(j_tmp == NULL) {
            break;
        }
        ast_json_array_append(j_res, j_tmp);
    }
    db_free(db_res);

    return j_res;
}
