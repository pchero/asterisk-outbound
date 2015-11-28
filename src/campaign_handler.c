/*
 * campaign_handler.c
 *
 *  Created on: Nov 26, 2015
 *      Author: pchero
 */

#include "asterisk.h"
#include "asterisk/json.h"
#include "asterisk/logger.h"
#include "asterisk/utils.h"
#include "asterisk/uuid.h"

#include <stdbool.h>

#include "db_handler.h"
#include "campaign_handler.h"
#include "cli_handler.h"

int create_campaign(struct ast_json* j_camp)
{
    int ret;
    char* uuid;
    struct ast_json* j_tmp;

    if(j_camp == NULL) {
        return false;
    }

    j_tmp = ast_json_deep_copy(j_camp);
    uuid = gen_uuid();
    ast_json_object_set(j_tmp, "uuid", ast_json_string_create(uuid));

    ast_log(LOG_NOTICE, "Create campaign. uuid[%s], name[%s]\n",
            ast_json_string_get(ast_json_object_get(j_tmp, "uuid")),
            ast_json_string_get(ast_json_object_get(j_tmp, "name"))
            );
    ret = db_insert("campaign", j_tmp);
    ast_json_unref(j_tmp);
    if(ret == false) {
        ast_free(uuid);
        return false;
    }

    // send ami event
    j_tmp = get_campaign_info(uuid);
    send_manager_evt_campaign_create(j_tmp);
    ast_json_unref(j_tmp);

    return true;
}

/**
 * Get all campaigns
 * @return
 */
struct ast_json* get_campaign_info(const char* uuid)
{
    struct ast_json* j_res;
    db_res_t* db_res;
    char* sql;

    if(uuid == NULL) {
        return NULL;
    }
    ast_log(LOG_DEBUG, "Get campaign info. uuid[%s]\n", uuid);

    // get all campaigns
    ast_asprintf(&sql, "select * from campaign where uuid = \"%s\";", uuid);

    db_res = db_query(sql);
    ast_free(sql);
    if(db_res == NULL) {
        ast_log(LOG_WARNING, "Could not get campaign info.\n");
        return NULL;
    }

    j_res = db_get_record(db_res);
    db_free(db_res);

    return j_res;
}

/**
 * Get all campaigns
 * @return
 */
struct ast_json* get_campaign_info_all(void)
{
    struct ast_json* j_res;
    struct ast_json* j_tmp;
    db_res_t* db_res;
    char* sql;

    // get all campaigns
    ast_asprintf(&sql, "%s", "select * from campaign");

    db_res = db_query(sql);
    ast_free(sql);
    if(db_res == NULL) {
        ast_log(LOG_WARNING, "Could not get campaign info.\n");
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

/**
 * Update campaign status info.
 * @param uuid
 * @param status
 * @return
 */
int update_campaign_info_status(const char* uuid, E_CAMP_STATUS_T status)
{
    char* sql;
    int ret;
    char* tmp_status;

    if(uuid == NULL) {
        ast_log(LOG_WARNING, "Invalid input parameters.\n");
        return false;
    }
    ast_log(LOG_NOTICE, "Update campaign status. uuid[%s], status[%d]\n", uuid, status);

    if(status == E_CAMP_START) tmp_status = "run";
    else if(status == E_CAMP_STOP) tmp_status = "stop";
    else if(status == E_CAMP_PAUSE) tmp_status = "pause";
    else if(status == E_CAMP_STARTING) tmp_status = "running";
    else if(status == E_CAMP_STOPPING) tmp_status = "stopping";
    else if(status == E_CAMP_PAUSING) tmp_status = "pausing";
    else {
        ast_log(LOG_WARNING, "Invalid input parameters.\n");
        return false;
    }

    ast_asprintf(&sql, "update campaign set status = %d where uuid = \"%s\";", status, uuid );
    ret = db_exec(sql);
    ast_free(sql);
    if(ret == false) {
        ast_log(LOG_ERROR, "Could not update campaign status info. uuid[%s], status[%s]\n", uuid, tmp_status);
        return false;
    }

    return true;
}

/**
 * Get campaign for dialing.
 * @return
 */
struct ast_json* get_campaigns_info_by_status(E_CAMP_STATUS_T status)
{
    struct ast_json* j_res;
    struct ast_json* j_tmp;
    db_res_t* db_res;
    char* sql;

    // get "start" status campaign only.
    ast_asprintf(&sql, "select * from campaign where status = %d;",
            status
            );

    db_res = db_query(sql);
    ast_free(sql);
    if(db_res == NULL) {
        ast_log(LOG_WARNING, "Could not get campaign info.\n");
        return NULL;
    }

    j_res = ast_json_array_create();
    while(1){
        j_tmp = db_get_record(db_res);
        if(j_tmp == NULL) {
            break;
        }
        ast_json_array_append(j_res, j_tmp);
    }
    db_free(db_res);

    return j_res;
}

/**
 * Generate uuid.
 * Return value should be free after used.
 * @param prefix
 * @return
 */
char* gen_uuid(void)
{
    char tmp[AST_UUID_STR_LEN];
    char* res;

    ast_uuid_generate_str(tmp, sizeof(tmp));
    res = ast_strdup(tmp);

    return res;
}

