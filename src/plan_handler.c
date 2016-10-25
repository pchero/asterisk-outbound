/*
 * plan_handler.c
 *
 *  Created on: Nov 30, 2015
 *	  Author: pchero
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
#include "utils.h"

/**
 *
 * @return
 */
bool init_plan(void)
{
	return true;
}

/**
 * Create plan.
 * @param j_plan
 * @return
 */
bool create_plan(const struct ast_json* j_plan)
{
	int ret;
	char* uuid;
	struct ast_json* j_tmp;
	char* tmp;

	if(j_plan == NULL) {
		return false;
	}

	j_tmp = ast_json_deep_copy(j_plan);
	uuid = gen_uuid();
	ast_json_object_set(j_tmp, "uuid", ast_json_string_create(uuid));

	tmp = get_utc_timestamp();
	ast_json_object_set(j_tmp, "tm_create", ast_json_string_create(tmp));
	ast_free(tmp);

	ast_log(LOG_NOTICE, "Create plan. uuid[%s], name[%s]\n",
			ast_json_string_get(ast_json_object_get(j_tmp, "uuid")),
			ast_json_string_get(ast_json_object_get(j_tmp, "name"))? : "<unknown>"
			);
	ret = db_insert("plan", j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(ret == false) {
		ast_free(uuid);
		return false;
	}
	ast_log(LOG_VERBOSE, "Finished insert.\n");

	// send ami event
	j_tmp = get_plan(uuid);
	ast_log(LOG_VERBOSE, "Check plan info. uuid[%s]\n",
			ast_json_string_get(ast_json_object_get(j_tmp, "uuid"))
			);
	ast_free(uuid);
	if(j_tmp == NULL) {
		ast_log(LOG_ERROR, "Could not get created plan info.");
		return false;
	}
	send_manager_evt_out_plan_create(j_tmp);
	AST_JSON_UNREF(j_tmp);

	return true;
}

/**
 * Delete plan.
 * @param uuid
 * @return
 */
bool delete_plan(const char* uuid)
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
	AST_JSON_UNREF(j_tmp);
	ast_asprintf(&sql, "update plan set %s where uuid=\"%s\";", tmp, uuid);
	ast_free(tmp);

	ret = db_exec(sql);
	ast_free(sql);
	if(ret == false) {
		ast_log(LOG_WARNING, "Could not delete plan. uuid[%s]\n", uuid);
		return false;
	}

	// send notification
	send_manager_evt_out_plan_delete(uuid);

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

/**
 * Update plan
 * @param j_plan
 * @return
 */
bool update_plan(const struct ast_json* j_plan)
{
	char* tmp;
	const char* tmp_const;
	char* sql;
	struct ast_json* j_tmp;
	int ret;
	char* uuid;

	if(j_plan == NULL) {
		return false;
	}

	j_tmp = ast_json_deep_copy(j_plan);
	if(j_tmp == NULL) {
		return false;
	}

	tmp_const = ast_json_string_get(ast_json_object_get(j_tmp, "uuid"));
	if(tmp_const == NULL) {
		ast_log(LOG_WARNING, "Could not get uuid.\n");
		AST_JSON_UNREF(j_tmp);
		return false;
	}
	uuid = ast_strdup(tmp_const);

	tmp = get_utc_timestamp();
	ast_json_object_set(j_tmp, "tm_update", ast_json_string_create(tmp));
	ast_free(tmp);

	tmp = db_get_update_str(j_tmp);
	if(tmp == NULL) {
		ast_log(LOG_WARNING, "Could not get update str.\n");
		AST_JSON_UNREF(j_tmp);
		ast_free(uuid);
		return false;
	}
	AST_JSON_UNREF(j_tmp);

	ast_asprintf(&sql, "update plan set %s where in_use=1 and uuid=\"%s\";", tmp, uuid);
	ast_free(tmp);

	ret = db_exec(sql);
	ast_free(sql);
	if(ret == false) {
		ast_log(LOG_WARNING, "Could not update plan info. uuid[%s]\n", uuid);
		ast_free(uuid);
		return false;
	}

	j_tmp = get_plan(uuid);
	ast_free(uuid);
	if(j_tmp == NULL) {
		ast_log(LOG_WARNING, "Could not get updated plan info.\n");
		return false;
	}
	send_manager_evt_out_plan_update(j_tmp);
	AST_JSON_UNREF(j_tmp);

	return true;
}

struct ast_json* create_dial_plan_info(struct ast_json* j_plan)
{
	struct ast_json* j_res;

	if(j_plan == NULL) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return NULL;
	}

	j_res = ast_json_pack("{s:i}",
			"dial_timeout", ast_json_integer_get(ast_json_object_get(j_plan, "dial_timeout"))
			);

	if(ast_json_string_get(ast_json_object_get(j_plan, "caller_id")) != NULL) {
		ast_json_object_set(j_res, "callerid", ast_json_ref(ast_json_object_get(j_plan, "caller_id")));
	}

	return j_res;

}

/**
 * Return the is nonstop dl handle
 * \param j_plan
 * \return
 */
bool is_nonstop_dl_handle(struct ast_json* j_plan)
{
	int ret;

	if(j_plan == NULL) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return true;
	}

	ret = ast_json_integer_get(ast_json_object_get(j_plan, "dl_end_handle"));
	if(ret == E_PLAN_DL_END_NONSTOP) {
		return true;
	}
	return false;
}

/**
 * Return the is stoppable plan or not.
 * \param j_plan
 * \return
 */
bool is_endable_plan(struct ast_json* j_plan)
{
	int ret;

	if(j_plan == NULL) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return true;
	}

	ret = is_nonstop_dl_handle(j_plan);
	if(ret == true) {
		ast_log(LOG_VERBOSE, "The plan dl_end_handle is nonstop. plan_uuid[%s], dl_end_handle[%d]\n",
				ast_json_string_get(ast_json_object_get(j_plan, "uuid"))? : "",
				ret
				);
		return false;
	}

	return true;
}

