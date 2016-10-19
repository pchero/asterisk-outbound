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


static bool create_plan_extension(struct ast_json* j_plan);
static bool delete_plan_extension(struct ast_json* j_plan);
static bool update_plan_extension(struct ast_json* j_plan);
static bool set_exten(const char* context, const char* exten, int priority, const char* application, const char* application_data);
static int set_amd_mode(const char* exten, E_AMD_MODE amd_mode, int priority);


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
	ast_json_unref(j_tmp);
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
	ast_json_unref(j_tmp);

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
		ast_json_unref(j_tmp);
		return false;
	}
	uuid = ast_strdup(tmp_const);

	tmp = get_utc_timestamp();
	ast_json_object_set(j_tmp, "tm_update", ast_json_string_create(tmp));
	ast_free(tmp);

	tmp = db_get_update_str(j_tmp);
	if(tmp == NULL) {
		ast_log(LOG_WARNING, "Could not get update str.\n");
		ast_json_unref(j_tmp);
		ast_free(uuid);
		return false;
	}
	ast_json_unref(j_tmp);

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

	// update plan extension
	update_plan_extension(j_tmp);
	ast_json_unref(j_tmp);

	return true;
}

/**
 *
 * @param j_plan
 * @return
 */
static bool create_plan_extension(struct ast_json* j_plan)
{
	int priority;
	E_DIAL_MODE dial_mode;
	int amd_mode;
	int ret;
	const char* plan_uuid;

	priority = 1;
	plan_uuid = ast_json_string_get(ast_json_object_get(j_plan, "uuid"));

	// dialplan start
	ret = set_exten(PLAN_CONTEXT, plan_uuid, priority, "NoOp", plan_uuid);
	if(ret == false) {
		ast_log(LOG_WARNING, "Could not set amd_setting to dialplan. exten[%s]\n", plan_uuid);
		return -1;
	}
	priority++;

	// AMD setting
	amd_mode = ast_json_integer_get(ast_json_object_get(j_plan, "amd_mode"));
	ret = set_amd_mode(plan_uuid, amd_mode, priority);
	if(ret < 0) {
		ast_log(LOG_WARNING, "Could not set amd setting. plan_uuid[%s]\n", plan_uuid);
		return false;
	}
	priority = ret;

	// Add queue
	dial_mode = ast_json_integer_get(ast_json_object_get(j_plan, "dial_mode"));
	switch(dial_mode) {
		case E_DIAL_MODE_PREDICTIVE: {
			// add to the queue
			ret = set_exten(PLAN_CONTEXT, plan_uuid, priority, "Queue", ast_json_string_get(ast_json_object_get(j_plan, "queue_name")));
			if(ret == false) {
				ast_log(LOG_WARNING, "Could not set amd_setting to dialplan. exten[%s]\n", plan_uuid);
				return -1;
			}
			priority++;
		}
		break;

		default: {
			ast_log(LOG_WARNING, "Could not find correspond dial_mode. plan_uuid[%s], dial_mode[%d]\n", plan_uuid, dial_mode);
		}
		break;
	}
	return true;
}

static bool delete_plan_extension(struct ast_json* j_plan)
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

static bool update_plan_extension(struct ast_json* j_plan)
{
	int ret;

	ret = delete_plan_extension(j_plan);
	if(ret == false) {
		ast_log(LOG_WARNING, "Could not delete dialplan extension.\n");
		return false;
	}

	ret = create_plan_extension(j_plan);
	if(ret == false) {
		ast_log(LOG_WARNING, "Could not create dialplan extension.\n");
		return false;
	}
	return true;
}


static bool set_exten(const char* context, const char* exten, int priority, const char* application, const char* application_data)
{
	struct ast_json* j_tmp;
	struct ast_json* j_ret;
	char* tmp;


	ast_asprintf(&tmp, "%d", priority);
	j_tmp = ast_json_pack("{s:s, s:s, s:s, s:s, s:s}",
			"context",		  context,
			"extension",		exten,
			"priority",		 tmp,
			"application",	  application,
			"application_data", application_data
			);
	ast_free(tmp);
	j_ret = ami_cmd_dialplan_extension_add(j_tmp);
	ast_json_unref(j_tmp);
	if(j_ret == NULL) {
		ast_log(LOG_WARNING, "Could not set extension to dialplan. context[%s], exten[%s], priority[%d], applicatioin[%s], application_data[%s]\n",
				context, exten, priority, application, application_data);
		return false;
	}
	ast_json_unref(j_ret);
	return true;
}

/**
 * Set the dialplan for AMD mode.
 * @param exten
 * @param amd_mode
 * @param priority
 * @return
 */
static int set_amd_mode(const char* exten, E_AMD_MODE amd_mode, int priority)
{
	int tmp_priority;
	int ret;

	tmp_priority = priority;

	if(exten == NULL) {
		ast_log(LOG_WARNING, "The extension is null.\n");
		return tmp_priority;
	}

	if(amd_mode == E_AMD_MODE_NONE) {
		ast_log(LOG_NOTICE, "No AMD setting.\n");
		return tmp_priority;
	}

	// set AMD
	ret = set_exten(PLAN_CONTEXT, exten, priority, "AMD", "");
	if(ret == false) {
		ast_log(LOG_WARNING, "Could not set amd_setting to dialplan. exten[%s]\n", exten);
		return -1;
	}
	priority++;

	// print AMD
	ret = set_exten(PLAN_CONTEXT, exten, priority, "NoOp", "AMD result. status[${AMDSTATUS}], cause[${AMDCAUSE}]");
	if(ret == false) {
		ast_log(LOG_WARNING, "Could not set amd_setting to dialplan. exten[%s]\n", exten);
		return -1;
	}
	priority++;

	// HUMAN
	if((amd_mode & E_AMD_MODE_HUMAN) == 0)
	{
		ret = set_exten(PLAN_CONTEXT, exten, priority, "ExecIf", "${AMDSTATUS} == HUMAN?HANGUP():");
		if(ret == false) {
			ast_log(LOG_WARNING, "Could not set amd_setting to dialplan. exten[%s]\n", exten);
			return -1;
		}
		priority++;
	}

	// MACHINE
	if((amd_mode & E_AMD_MODE_MACHINE) == 0)
	{
		ret = set_exten(PLAN_CONTEXT, exten, priority, "ExecIf", "${AMDSTATUS} == MACHINE?HANGUP():");
		if(ret == false) {
			ast_log(LOG_WARNING, "Could not set amd_setting to dialplan. exten[%s]\n", exten);
			return -1;
		}
		priority++;
	}

	// NOTSURE
	if((amd_mode & E_AMD_MODE_NOTSURE) == 0)
	{
		ret = set_exten(PLAN_CONTEXT, exten, priority, "ExecIf", "${AMDSTATUS} == NOTSURE?HANGUP():");
		if(ret == false) {
			ast_log(LOG_WARNING, "Could not set amd_setting to dialplan. exten[%s]\n", exten);
			return -1;
		}
		priority++;
	}

	return priority;
}
