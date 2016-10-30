/*
 * campaign_handler.c
 *
 *  Created on: Nov 26, 2015
 *	  Author: pchero
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
#include "event_handler.h"
#include "utils.h"
#include "dl_handler.h"
#include "plan_handler.h"

static struct ast_json* get_campaign_deleted(const char* uuid);

static bool is_startable_campaign_schedule(struct ast_json* j_camp);
static bool is_startable_campaign_schedule_day(struct ast_json* j_camp, int day);
static bool is_startable_campaign_schedule_check(struct ast_json* j_camp, const char* cur_date, const char* cur_time, int cur_day);
static bool is_stopable_campaign_schedule_day_date_list(struct ast_json* j_camp);
static bool is_stoppable_campaign_schedule_check(struct ast_json* j_camp, const char* cur_date, const char* cur_time, int cur_day);


/**
 * Create campaign.
 * @param j_camp
 * @return
 */
bool create_campaign(const struct ast_json* j_camp)
{
	int ret;
	char* uuid;
	char* tmp;
	struct ast_json* j_tmp;

	if(j_camp == NULL) {
		return false;
	}
	j_tmp = ast_json_deep_copy(j_camp);

	uuid = gen_uuid();
	ast_json_object_set(j_tmp, "uuid", ast_json_string_create(uuid));

	tmp = get_utc_timestamp();
	ast_json_object_set(j_tmp, "tm_create", ast_json_string_create(tmp));
	ast_free(tmp);

	ast_log(LOG_NOTICE, "Create campaign. uuid[%s], name[%s]\n",
			ast_json_string_get(ast_json_object_get(j_tmp, "uuid")),
			ast_json_string_get(ast_json_object_get(j_tmp, "name"))
			);
	ret = db_insert("campaign", j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(ret == false) {
		ast_free(uuid);
		return false;
	}

	// send ami event
	j_tmp = get_campaign(uuid);
	ast_free(uuid);
	send_manager_evt_out_campaign_create(j_tmp);
	AST_JSON_UNREF(j_tmp);

	return true;
}

/**
 * Delete campaign.
 * @param uuid
 * @return
 */
bool delete_campaign(const char* uuid)
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
	ast_json_object_set(j_tmp, "status", ast_json_integer_create(E_CAMP_STOP));
	ast_json_object_set(j_tmp, "tm_delete", ast_json_string_create(tmp));
	ast_json_object_set(j_tmp, "in_use", ast_json_integer_create(E_DL_USE_NO));
	ast_free(tmp);

	tmp = db_get_update_str(j_tmp);
	AST_JSON_UNREF(j_tmp);
	ast_asprintf(&sql, "update campaign set %s where uuid=\"%s\";", tmp, uuid);
	ast_free(tmp);

	ret = db_exec(sql);
	ast_free(sql);
	if(ret == false) {
		ast_log(LOG_WARNING, "Could not delete campaign. uuid[%s]\n", uuid);
		return false;
	}

	// send notification
	j_tmp = get_campaign_deleted(uuid);
	send_manager_evt_out_campaign_delete(j_tmp);
	AST_JSON_UNREF(j_tmp);

	return true;
}

/**
 * Get specified campaign
 * @return
 */
struct ast_json* get_campaign(const char* uuid)
{
	struct ast_json* j_res;
	db_res_t* db_res;
	char* sql;

	if(uuid == NULL) {
		return NULL;
	}
	ast_log(LOG_DEBUG, "Get campaign info. uuid[%s]\n", uuid);

	// get specified campaign
	ast_asprintf(&sql, "select * from campaign where uuid=\"%s\" and in_use=%d;", uuid, E_DL_USE_OK);

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
 * Get deleted campaign.
 * @return
 */
static struct ast_json* get_campaign_deleted(const char* uuid)
{
	struct ast_json* j_res;
	db_res_t* db_res;
	char* sql;

	if(uuid == NULL) {
		return NULL;
	}
	ast_log(LOG_DEBUG, "Get deleted campaign info. uuid[%s]\n", uuid);

	// get specified campaign
	ast_asprintf(&sql, "select * from campaign where uuid=\"%s\" and in_use=%d;", uuid, E_DL_USE_NO);

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
struct ast_json* get_campaigns_all(void)
{
	struct ast_json* j_res;
	struct ast_json* j_tmp;
	db_res_t* db_res;
	char* sql;

	// get all campaigns
	ast_asprintf(&sql, "select * from campaign where in_use=%d", E_DL_USE_OK);

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
 * Get all start-able campaigns by schedule.
 * \return
 */
struct ast_json* get_campaigns_schedule_start(void)
{
	char* sql;
	struct ast_json* j_res;
	struct ast_json* j_tmp;
	db_res_t* db_res;
	int ret;

	ast_asprintf(&sql, "select * from campaign where status == %d and sc_mode == %d;",
			E_CAMP_STOP,
			E_CAMP_SCHEDULE_ON
			);

	db_res = db_query(sql);
	ast_free(sql);
	if(db_res == NULL) {
		return NULL;
	}

	j_res = ast_json_array_create();
	while(1) {
		j_tmp = db_get_record(db_res);
		if(j_tmp == NULL) {
			break;
		}

		ret = is_startable_campaign_schedule(j_tmp);
		if(ret == false) {
			AST_JSON_UNREF(j_tmp);
			continue;
		}

		ast_json_array_append(j_res, j_tmp);
	}
	db_free(db_res);

	return j_res;
}

/**
 * Get all stop-able campaigns by schedule.
 * \return
 */
struct ast_json* get_campaigns_schedule_end(void)
{
	char* sql;
	struct ast_json* j_res;
	struct ast_json* j_tmp;
	db_res_t* db_res;
	int ret;

	ast_asprintf(&sql, "select * from campaign where status == %d and sc_mode == %d;",
			E_CAMP_START,
			E_CAMP_SCHEDULE_ON
			);

	db_res = db_query(sql);
	ast_free(sql);
	if(db_res == NULL) {
		return NULL;
	}

	j_res = ast_json_array_create();
	while(1) {
		j_tmp = db_get_record(db_res);
		if(j_tmp == NULL) {
			break;
		}

		ret = is_stopable_campaign_schedule_day_date_list(j_tmp);
		if(ret == false) {
			AST_JSON_UNREF(j_tmp);
			continue;
		}

		ast_json_array_append(j_res, j_tmp);
	}
	db_free(db_res);

	return j_res;
}



/**
 * Update campaign
 * @param j_camp
 * @return
 */
bool update_campaign(const struct ast_json* j_camp)
{
	char* tmp;
	const char* tmp_const;
	char* sql;
	struct ast_json* j_tmp;
	char* uuid;

	if(j_camp == NULL) {
		return false;
	}

	j_tmp = ast_json_deep_copy(j_camp);
	if(j_tmp == NULL) {
		return false;
	}

	tmp_const = ast_json_string_get(ast_json_object_get(j_tmp, "uuid"));
	if(tmp_const == NULL) {
		ast_log(LOG_WARNING, "Could not get uuid info.\n");
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

	ast_asprintf(&sql, "update campaign set %s where uuid=\"%s\" and in_use=%d;", tmp, uuid, E_DL_USE_OK);
	ast_free(tmp);

	db_exec(sql);
	ast_free(sql);

	j_tmp = get_campaign(uuid);
	ast_free(uuid);
	if(j_tmp == NULL) {
		ast_log(LOG_WARNING, "Could not get updated campaign info.\n");
		return false;
	}
	send_manager_evt_out_campaign_update(j_tmp);
	AST_JSON_UNREF(j_tmp);

	return true;
}

/**
 * Update campaign status info.
 * @param uuid
 * @param status
 * @return
 */
bool update_campaign_status(const char* uuid, E_CAMP_STATUS_T status)
{
	int ret;
	char* tmp_status;
	struct ast_json* j_tmp;

	if(uuid == NULL) {
		ast_log(LOG_WARNING, "Invalid input parameters.\n");
		return false;
	}

	if(status == E_CAMP_START)		  tmp_status = "run";
	else if(status == E_CAMP_STOP)	  tmp_status = "stop";
	else if(status == E_CAMP_PAUSE)	 tmp_status = "pause";
	else if(status == E_CAMP_STARTING)  tmp_status = "running";
	else if(status == E_CAMP_STOPPING)  tmp_status = "stopping";
	else if(status == E_CAMP_PAUSING)   tmp_status = "pausing";
//	else if(status == E_CAMP_SCHEDULE_STOP)   		tmp_status = "schedule_stop";
//	else if(status == E_CAMP_SCHEDULE_STOPPING)   tmp_status = "schedule_stopping";
	else {
		ast_log(LOG_WARNING, "Invalid input parameters.\n");
		return false;
	}
	ast_log(LOG_NOTICE, "Update campaign status. uuid[%s], status[%d], status_string[%s]\n", uuid, status, tmp_status);

	// get campaign
	j_tmp = get_campaign(uuid);
	if(j_tmp == NULL) {
		ast_log(LOG_WARNING, "Could not find campaign. camp_uuid[%s]\n", uuid);
		return false;
	}

	// set status
	ast_json_object_set(j_tmp, "status", ast_json_integer_create(status));

	// update
	ret = update_campaign(j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(ret == false) {
		ast_log(LOG_ERROR, "Could not update campaign status. camp_uuid[%s], status[%d]", uuid, status);
		return false;
	}

	return true;
}

/**
 * Get campaign for dialing.
 * @return
 */
struct ast_json* get_campaigns_by_status(E_CAMP_STATUS_T status)
{
	struct ast_json* j_res;
	struct ast_json* j_tmp;
	db_res_t* db_res;
	char* sql;

	// get "start" status campaign only.
	ast_asprintf(&sql, "select * from campaign where status = %d and in_use=%d;",
			status, E_DL_USE_OK
			);

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
 * Get campaign for dialing.
 * @return
 */
struct ast_json* get_campaign_for_dialing(void)
{
	struct ast_json* j_res;
	db_res_t* db_res;
	char* sql;

	// get "start" status campaign only.
	ast_asprintf(&sql, "select * from campaign where status = %d and in_use = %d order by %s limit 1;",
			E_CAMP_START,
			E_DL_USE_OK,
			db_translate_function(E_FUNC_RANDOM)
			);

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
 *
 * \param uuid
 * \return
 */
struct ast_json* get_campaign_stat(const char* uuid)
{
	struct ast_json* j_camp;
	struct ast_json* j_plan;
	struct ast_json* j_dlma;
	struct ast_json* j_res;

	if(uuid == NULL) {
		ast_log(LOG_WARNING, "Wrong input parameter.");
		return NULL;
	}

	// get campaign
	j_camp = get_campaign(uuid);
	if(j_camp == NULL) {
		ast_log(LOG_DEBUG, "Could not get campaign info. camp_uuid[%s]\n", uuid);
		return NULL;
	}

	// get plan
	j_plan = get_plan(ast_json_string_get(ast_json_object_get(j_camp, "plan")));
	j_dlma = get_dlma(ast_json_string_get(ast_json_object_get(j_camp, "dlma")));
	if((j_plan == NULL) || (j_dlma == NULL)) {
		ast_log(LOG_DEBUG, "Could not basic info. camp_uuid[%s], plan_uuid[%s], dlma_uuid[%s]\n",
				uuid,
				ast_json_string_get(ast_json_object_get(j_camp, "plan")),
				ast_json_string_get(ast_json_object_get(j_camp, "dlma"))
				);
		AST_JSON_UNREF(j_camp);
		return NULL;
	}

	// create
	j_res = ast_json_pack("{"
			"s:s, "
			"s:i, s:i, s:i, s:i, s:i"
			"}",

			"uuid",									ast_json_string_get(ast_json_object_get(j_camp, "uuid"))? : "",

			"dial_total_count",			get_dl_list_cnt_total(j_dlma),
			"dial_finished_count",	get_dl_list_cnt_finshed(j_dlma, j_plan),
			"dial_available_count",	get_dl_list_cnt_available(j_dlma, j_plan),
			"dial_dialing_count",		get_dl_list_cnt_dialing(j_dlma),
			"dial_called_count",		get_dl_list_cnt_tried(j_dlma)
			);

	AST_JSON_UNREF(j_camp);
	AST_JSON_UNREF(j_plan);
	AST_JSON_UNREF(j_dlma);

	return j_res;
}

struct ast_json* get_campaigns_stat_all(void)
{
	struct ast_json* j_stats;
	struct ast_json* j_stat;
	struct ast_json* j_camps;
	struct ast_json* j_camp;
	int i;
	int size;

	j_camps = get_campaigns_all();
	size = ast_json_array_size(j_camps);
	j_stats = ast_json_array_create();
	for(i = 0; i < size; i++) {
		j_camp = ast_json_array_get(j_camps, i);
		if(j_camp == NULL) {
			continue;
		}

		j_stat = get_campaign_stat(ast_json_string_get(ast_json_object_get(j_camp, "uuid")));
		if(j_stat == NULL) {
			continue;
		}

		ast_json_array_append(j_stats, j_stat);
	}

	AST_JSON_UNREF(j_camps);

	return j_stats;
}


/**
 * return the possibility of status change to start
 */
bool is_startable_campgain(struct ast_json* j_camp)
{
	if(j_camp == NULL) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return false;
	}

	return true;
}

/**
 * return the possibility of status change to stop
 */
bool is_stoppable_campgain(struct ast_json* j_camp)
{
	struct ao2_iterator iter;
	rb_dialing* dialing;
	const char* tmp_const;
	bool flg_dialing;
	int ret;

	if(j_camp == NULL) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return false;
	}
	ast_log(LOG_VERBOSE, "is_stoppable_campgain.\n");

	flg_dialing = false;
	iter = rb_dialing_iter_init();
	while(1) {
		dialing = rb_dialing_iter_next(&iter);
		if(dialing == NULL) {
			break;
		}

		tmp_const = ast_json_string_get(ast_json_object_get(dialing->j_dialing, "camp_uuid"));
		if(tmp_const == NULL) {
			continue;
		}

		ret = strcmp(tmp_const, ast_json_string_get(ast_json_object_get(j_camp, "uuid")));
		if(ret == 0) {
			ast_log(LOG_NOTICE, "Found active call. dialing-uuid[%s], dialing-name[%s]\n",
					dialing->uuid, dialing->name
					);
			flg_dialing = true;
			break;
		}
	}
	rb_dialing_iter_destroy(&iter);

	if(flg_dialing == true) {
		return false;
	}

	return true;
}

static bool is_startable_campaign_schedule(struct ast_json* j_camp)
{
	char* cur_date;
	char* cur_time;
	int cur_day;
	int ret;

	if(j_camp == NULL) {
		return false;
	}

	cur_date = get_utc_timestamp_date();
	cur_time = get_utc_timestamp_time();
	cur_day = get_utc_timestamp_day();

	// check startable date
	ret = is_startable_campaign_schedule_check(j_camp, cur_date, cur_time, cur_day);
	ast_free(cur_date);
	ast_free(cur_time);
	if(ret == true) {
		return true;
	}

	return false;
}

static bool is_startable_campaign_schedule_check(
		struct ast_json* j_camp,
		const char* cur_date,
		const char* cur_time,
		int cur_day
		)
{
	const char* date_list;
	const char* date_except;
	char* tmp;
	const char* tmp_const;
	int ret;

	if((j_camp == NULL) || (cur_date == NULL) || (cur_time == NULL)) {
		return false;
	}

	// check except date. If there's except date, return false
	date_except = ast_json_string_get(ast_json_object_get(j_camp, "sc_date_list_except"));
	if(date_except != NULL) {
		tmp = strstr(date_except, cur_date);
		if(tmp != NULL) {
			return false;
		}
	}

	// check date_list. If there's current date, return true
	date_list = ast_json_string_get(ast_json_object_get(j_camp, "sc_date_list"));
	if(date_list != NULL) {
		tmp = strstr(date_list, cur_date);
		if(tmp != NULL) {
			return true;
		}
	}

	// check start date.
	// if start date is in the future, return false
	tmp_const = ast_json_string_get(ast_json_object_get(j_camp, "sc_date_start"));
	if(tmp_const != NULL) {
		ret = strcmp(tmp_const, cur_date);
		if(ret > 0) {
			return false;
		}
	}

	// check end date.
	// if end date is in the fast, return false
	tmp_const = ast_json_string_get(ast_json_object_get(j_camp, "sc_date_end"));
	if(tmp_const != NULL) {
		ret = strcmp(tmp_const, cur_date);
		if(ret < 0) {
			return false;
		}
	}

	// check startable day
	ret = is_startable_campaign_schedule_day(j_camp, cur_day);
	if(ret == false) {
		return false;
	}

	// check end time
	// if end time is in the fast, return false
	tmp_const = ast_json_string_get(ast_json_object_get(j_camp, "sc_time_end"));
	if(tmp_const != NULL) {
		ret = strcmp(tmp_const, cur_time);
		if(ret < 0) {
			return false;
		}
	}

	// check start time
	// if start time is in the future, return false
	tmp_const = ast_json_string_get(ast_json_object_get(j_camp, "sc_time_start"));
	if(tmp_const != NULL) {
		ret = strcmp(tmp_const, cur_time);
		if(ret > 0) {
			return false;
		}
	}

	return true;
}

static bool is_startable_campaign_schedule_day(struct ast_json* j_camp, int day)
{
	const char* day_list;
	char* tmp;
	char 	day_str[2];

	// 0=Sunday, 1=Monday, ..., 6=Saturday
	if((j_camp == NULL) || (day < 0) || (day > 6)) {
		return false;
	}

	// if it doesn't set, just return true.
	day_list = ast_json_string_get(ast_json_object_get(j_camp, "sc_day_list"));
	if((day_list == NULL) || (strlen(day_list) == 0)) {
		return true;
	}

	// check date_list. If there's no current date, return false
	snprintf(day_str, sizeof(day_str), "%d", day);
	tmp = strstr(day_list, day_str);
	if(tmp == NULL) {
		return false;
	}

	return true;
}

static bool is_stopable_campaign_schedule_day_date_list(struct ast_json* j_camp)
{
	char* cur_date;
	char* cur_time;
	int cur_day;
	int ret;

	if(j_camp == NULL) {
		return false;
	}

	cur_date = get_utc_timestamp_date();
	cur_time = get_utc_timestamp_time();
	cur_day = get_utc_timestamp_day();

	// check startable date
	ret = is_stoppable_campaign_schedule_check(j_camp, cur_date, cur_time, cur_day);
	ast_free(cur_date);
	ast_free(cur_time);
	if(ret == true) {
		return true;
	}

	return false;
}

static bool is_stoppable_campaign_schedule_check(struct ast_json* j_camp, const char* cur_date, const char* cur_time, int cur_day)
{
	int ret;

	ret = is_startable_campaign_schedule_check(j_camp, cur_date, cur_time, cur_day);
	if(ret == true) {
		return false;
	}

	return true;
}
