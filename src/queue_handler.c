/*
 * queue_handler.c
 *
 *  Created on: Dec 3, 2015
 *	  Author: pchero
 */

#include "asterisk.h"
#include "asterisk/json.h"
#include "asterisk/logger.h"

#include "queue_handler.h"
#include "event_handler.h"
#include "db_handler.h"
#include "campaign_handler.h"
#include "cli_handler.h"
#include "utils.h"
#include "dl_handler.h"

#include <stdbool.h>

int create_queue(struct ast_json* j_queue)
{
	int ret;
	char* uuid;
	struct ast_json* j_tmp;

	if(j_queue == NULL) {
		return false;
	}

	j_tmp = ast_json_deep_copy(j_queue);
	uuid = gen_uuid();
	ast_json_object_set(j_tmp, "uuid", ast_json_string_create(uuid));

	ast_log(LOG_NOTICE, "Create queue. uuid[%s], name[%s]\n",
			ast_json_string_get(ast_json_object_get(j_tmp, "uuid")),
			ast_json_string_get(ast_json_object_get(j_tmp, "name"))
			);
	ret = db_insert("queue", j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(ret == false) {
		ast_free(uuid);
		return false;
	}

	// send ami event
	j_tmp = get_queue(uuid);
	send_manager_evt_out_queue_create(j_tmp);
	AST_JSON_UNREF(j_tmp);

	return true;
}


int delete_queue(const char* uuid)
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
	ast_json_object_set(j_tmp, "in_use", ast_json_integer_create(E_DL_USE_NO));
	ast_free(tmp);

	tmp = db_get_update_str(j_tmp);
	AST_JSON_UNREF(j_tmp);
	ast_asprintf(&sql, "update queue set %s where uuid=\"%s\";", tmp, uuid);
	ast_free(tmp);

	ret = db_exec(sql);
	ast_free(sql);
	if(ret == false) {
		ast_log(LOG_WARNING, "Could not delete queue. uuid[%s]\n", uuid);
		return false;
	}

	// send notification
	send_manager_evt_out_queue_delete(uuid);

	return true;
}

struct ast_json* get_queue(const char* uuid)
{
	char* sql;
	struct ast_json* j_res;
	db_res_t* db_res;

	if(uuid == NULL) {
		ast_log(LOG_WARNING, "Invalid input parameters.\n");
		return NULL;
	}
	ast_asprintf(&sql, "select * from queue where in_use=%d and uuid=\"%s\";", E_DL_USE_OK, uuid);

	db_res = db_query(sql);
	ast_free(sql);
	if(db_res == NULL) {
		ast_log(LOG_ERROR, "Could not get queue info. uuid[%s]\n", uuid);
		return NULL;
	}

	j_res = db_get_record(db_res);
	db_free(db_res);

	return j_res;
}

struct ast_json* get_queues_all(void)
{
	char* sql;
	struct ast_json* j_res;
	struct ast_json* j_tmp;
	db_res_t* db_res;

	ast_asprintf(&sql, "select * queue where in_use=%d;", E_DL_USE_OK);

	db_res = db_query(sql);
	ast_free(sql);
	if(db_res == NULL) {
		ast_log(LOG_ERROR, "Could not get queue all info.\n");
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

int update_queue(struct ast_json* j_queue)
{
	char* tmp;
	char* sql;
	struct ast_json* j_tmp;
	int ret;
	const char* uuid;

	uuid = ast_json_string_get(ast_json_object_get(j_queue, "uuid"));
	if(uuid == NULL) {
		ast_log(LOG_WARNING, "Could not get uuid.\n");
		return false;
	}

	tmp = db_get_update_str(j_queue);
	if(tmp == NULL) {
		ast_log(LOG_WARNING, "Could not get update str.\n");
		return false;
	}

	ast_asprintf(&sql, "update queue set %s where in_use=%d and uuid=\"%s\";", tmp, E_DL_USE_OK, uuid);
	ast_free(tmp);

	ret = db_exec(sql);
	ast_free(sql);
	if(ret == false) {
		ast_log(LOG_WARNING, "Could not update queue info. uuid[%s]\n", uuid);
		return false;
	}

	j_tmp = get_queue(uuid);
	if(j_tmp == NULL) {
		ast_log(LOG_WARNING, "Could not get update queue info. uuid[%s]\n", uuid);
		return false;
	}
	send_manager_evt_out_queue_update(j_tmp);
	AST_JSON_UNREF(j_tmp);

	return true;
}
