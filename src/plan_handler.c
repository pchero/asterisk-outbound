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
#include "ami_handler.h"


/**
 *
 * @return
 */
bool init_plan(void)
{
    struct ast_json* j_plans;
    struct ast_json* j_plan;
    int i;
    int size;
    int ret;

    j_plans = get_plans_all();
    size = ast_json_array_size(j_plans);
    for(i = 0; i < size; i++) {
        j_plan = ast_json_array_get(j_plans, i);
        ret = delete_plan_extension(j_plan);

        ret = create_plan_extension(j_plan);
        if(ret == false) {
            ast_log(LOG_ERROR, "Could not create outbound dialplan. plan_uuid[%s]\n", ast_json_string_get(ast_json_object_get(j_plan, "uuid")));
            ast_json_unref(j_plans);
            return false;
        }
    }
    ast_json_unref(j_plans);

    return true;
}

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
    j_tmp = get_plan(uuid);
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
struct ast_json* get_plan(const char* uuid)
{
    char* sql;
    struct ast_json* j_res;
    db_res_t* db_res;

    if(uuid == NULL) {
        ast_log(LOG_WARNING, "Invalid input parameters.\n");
        return NULL;
    }
    ast_asprintf(&sql, "select * from plan where in_use=1 and uuid=\"%s\";", uuid);

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
struct ast_json* get_plans_all(void)
{
    char* sql;
    struct ast_json* j_res;
    struct ast_json* j_tmp;
    db_res_t* db_res;

    ast_asprintf(&sql, "%s", "select * from plan where in_use=1;");

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

int update_plan(struct ast_json* j_plan)
{
    char* tmp;
    char* sql;
    struct ast_json* j_tmp;
    int ret;
    const char* uuid;

    uuid = ast_json_string_get(ast_json_object_get(j_plan, "uuid"));
    if(uuid == NULL) {
        ast_log(LOG_WARNING, "Could not get uuid.\n");
        return false;
    }

    tmp = db_get_update_str(j_plan);
    if(tmp == NULL) {
        ast_log(LOG_WARNING, "Could not get update str.\n");
        return false;
    }

    ast_asprintf(&sql, "update plan set %s where in_use=1 and uuid=\"%s\";", tmp, uuid);
    ast_free(tmp);

    ret = db_exec(sql);
    ast_free(sql);
    if(ret == false) {
        ast_log(LOG_WARNING, "Could not update plan info. uuid[%s]\n", uuid);
        return false;
    }

    j_tmp = get_plan(uuid);
    if(j_tmp == NULL) {
        ast_log(LOG_WARNING, "Could not get update plan info. uuid[%s]\n", uuid);
        return false;
    }
    send_manager_evt_plan_update(j_tmp);
    ast_json_unref(j_tmp);

    return true;
}

bool create_plan_extension(struct ast_json* j_plan)
{
    int i;
    char* tmp;
    E_DIAL_MODE dial_mode;
    struct ast_json* j_tmp;
    struct ast_json* j_ret;

    i = 0;

    // Add queue
    dial_mode = ast_json_integer_get(ast_json_object_get(j_plan, "dial_mode"));
    switch(dial_mode) {
        case E_DIAL_MODE_PREDICTIVE: {
            // add to the queue
            ast_asprintf(&tmp, "%d", i);
            j_tmp = ast_json_pack("{s:s, s:s, s:s, s:s, s:s}",
                    "context",          PLAN_CONTEXT,
                    "extension",        ast_json_string_get(ast_json_object_get(j_plan, "uuid")),
                    "priority",         tmp,
                    "application",      "Queue",
                    "application_data", ast_json_string_get(ast_json_object_get(j_plan, "queue_name"))
                    );
            ast_free(tmp);
            if(j_tmp == NULL) {
                ast_log(LOG_WARNING, "Could not create plan extension req. plan_uuid[%s]\n", ast_json_string_get(ast_json_object_get(j_plan, "uuid")));
                return false;
            }

            j_ret = ami_cmd_dialplan_extension_add(j_tmp);
            ast_json_unref(j_tmp);
            if(j_ret == NULL) {
                delete_plan_extension(j_plan);
                return false;
            }
            ast_json_unref(j_ret);
        }
        break;

        default: {
            ast_log(LOG_WARNING, "Could not find correspond dial_mode. plan_uuid[%s], dial_mode[%d]\n",
                    ast_json_string_get(ast_json_object_get(j_plan, "uuid")), dial_mode);
        }
    }

    return true;
}

bool delete_plan_extension(struct ast_json* j_plan)
{
    struct ast_json* j_ret;

    j_ret = ami_cmd_dialplan_extension_remove(
            PLAN_CONTEXT,
            ast_json_string_get(ast_json_object_get(j_plan, "uuid")),
            -1
            );
    if(j_ret == NULL) {
        ast_log(LOG_WARNING, "Could not delete dialplan extension. context[%s], extension[%s]\n",
                PLAN_CONTEXT,
                ast_json_string_get(ast_json_object_get(j_plan, "uuid"))
                );
        return false;
    }

    ast_json_unref(j_ret);

    return true;
}
