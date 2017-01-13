/*
 * dl_handler.c
 *
 *  Created on: Nov 29, 2015
 *	  Author: pchero
 */

#include "asterisk.h"
#include "asterisk/causes.h"
#include "asterisk/logger.h"
#include "asterisk/utils.h"
#include "asterisk/frame.h"

#include "db_handler.h"
#include "dl_handler.h"
#include "campaign_handler.h"
#include "cli_handler.h"
#include "event_handler.h"
#include "utils.h"
#include "destination_handler.h"
#include "plan_handler.h"
#include "res_outbound.h"

#include <stdbool.h>

static char* create_chan_addr_for_dial(struct ast_json* j_plan, struct ast_json* j_dl_list, int dial_num_point);
static char* get_dial_number(struct ast_json* j_dlist, const int cnt);
static char* create_view_name(const char* uuid);
static bool create_dlma_view(const char* uuid, const char* view_name);
static struct ast_json* create_dial_dl_info(struct ast_json* j_dl_list, struct ast_json* j_plan);
static bool check_more_dl_list(struct ast_json* j_dlma, struct ast_json* j_plan);
static bool is_over_retry_delay(struct ast_json* j_dlma, struct ast_json* j_dl, struct ast_json* j_plan);
static struct ast_json* get_dl_available(struct ast_json* j_dlma, struct ast_json* j_plan);

/**
 * Get dl_list for predictive dialing.
 * \param j_dlma
 * \param j_plan
 * \return
 */
struct ast_json* get_dl_available_predictive(struct ast_json* j_dlma, struct ast_json* j_plan)
{
	struct ast_json* j_dl;
	int ret;

	if((j_dlma == NULL) || (j_plan == NULL)) {
		ast_log(LOG_WARNING, "Wrong input parameters.\n");
		return NULL;
	}

	// get available dl.
	j_dl = get_dl_available(j_dlma, j_plan);
	if(j_dl == NULL) {
		return NULL;
	}

	// check retry delay
	ret = is_over_retry_delay(j_dlma, j_dl, j_plan);
	if(ret == false) {
		AST_JSON_UNREF(j_dl);
		return NULL;
	}

	return j_dl;
}

static bool check_more_dl_list(struct ast_json* j_dlma, struct ast_json* j_plan)
{
	struct ast_json* j_res;
	db_res_t* db_res;
	char* sql;

	ast_asprintf(&sql, "select * from `%s` where ("
			"(number_1 is not null and trycnt_1 < %"PRIdMAX")"
			" or (number_2 is not null and trycnt_2 < %"PRIdMAX")"
			" or (number_3 is not null and trycnt_3 < %"PRIdMAX")"
			" or (number_4 is not null and trycnt_4 < %"PRIdMAX")"
			" or (number_5 is not null and trycnt_5 < %"PRIdMAX")"
			" or (number_6 is not null and trycnt_6 < %"PRIdMAX")"
			" or (number_7 is not null and trycnt_7 < %"PRIdMAX")"
			" or (number_8 is not null and trycnt_8 < %"PRIdMAX")"
			")"
			" and res_dial != %d"
			";",
			ast_json_string_get(ast_json_object_get(j_dlma, "dl_table")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_1")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_2")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_3")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_4")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_5")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_6")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_7")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_8")),
			AST_CONTROL_ANSWER
			);

	db_res = db_query(sql);
	ast_free(sql);
	if(db_res == NULL) {
		return false;
	}

	j_res = db_get_record(db_res);
	db_free(db_res);
//	ast_log(LOG_DEBUG, "Get dial records. uuid[%s]\n", ast_json_string_get(ast_json_object_get(j_res, "uuid")));
	if(j_res == NULL) {
		return false;
	}
	AST_JSON_UNREF(j_res);

	return true;
}

/**
 *
 * @param uuid
 */
void clear_dl_list_dialing(const char* uuid)
{
	struct ast_json* j_tmp;

	if(uuid == NULL) {
		return;
	}

	j_tmp = ast_json_pack("{s:i, s:s, s:O, s:O, s:O}",
			"status",			   E_DL_IDLE,
			"uuid",				 uuid,
			"dialing_uuid",		 ast_json_null(),
			"dialing_camp_uuid",	ast_json_null(),
			"dialing_plan_uuid",	ast_json_null()
			);
	update_dl_list(j_tmp);
	AST_JSON_UNREF(j_tmp);

	return;
}

/**
 * Update dl list info.
 * @param j_dlinfo
 * @return
 */
bool update_dl_list(struct ast_json* j_dl)
{
	char* sql;
	int ret;
	char* tmp;

	if(j_dl == NULL) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return false;
	}

	tmp = db_get_update_str(j_dl);
	if(tmp == NULL) {
		ast_log(LOG_ERROR, "Could not get update sql.\n");
		return false;
	}

	ast_asprintf(&sql, "update dl_list set %s where uuid = \"%s\";\n",
			tmp, ast_json_string_get(ast_json_object_get(j_dl, "uuid"))
			);
	ast_free(tmp);

	ret = db_exec(sql);
	ast_free(sql);
	if(ret == false) {
		ast_log(LOG_ERROR, "Could not update dl_list info.");
		return false;
	}

	return true;
}

/**
 * Return the current dialing count.
 * @param camp_uuid
 * @param dl_table
 * @return
 */
int get_current_dialing_dl_cnt(const char* camp_uuid, const char* dl_table)
{
	char* sql;
	db_res_t* db_res;
	struct ast_json* j_tmp;
	int ret;

	if((camp_uuid == NULL) || (dl_table == NULL)) {
		ast_log(LOG_ERROR, "Invalid input parameters.\n");
		return -1;
	}

	ast_asprintf(&sql, "select count(*) from `%s` where dialing_camp_uuid = \"%s\" and status = \"%s\";",
			dl_table, camp_uuid, "dialing"
			);

	db_res = db_query(sql);
	ast_free(sql);
	if(db_res == NULL) {
		ast_log(LOG_ERROR, "Could not get dialing count.\n");
		return 0;
	}

	j_tmp = db_get_record(db_res);
	db_free(db_res);
	if(j_tmp == NULL) {
		// shouldn't be reach to here.
		ast_log(LOG_ERROR, "Could not get dialing count.");
		return false;
	}

	ret = ast_json_integer_get(ast_json_object_get(j_tmp, "count(*)"));
	AST_JSON_UNREF(j_tmp);

	return ret;
}

/**
 * Get dial number index
 * @param j_dl_list
 * @param j_plan
 * @return
 */
int get_dial_num_point(struct ast_json* j_dl_list, struct ast_json* j_plan)
{
	int i;
	int dial_num_point;
	int cur_trycnt;
	int max_trycnt;
	char* tmp;
	const char* tmp_const;

	// get dial number
	dial_num_point = -1;
	for(i = 1; i < 9; i++) {
		ast_asprintf(&tmp, "number_%d", i);
		tmp_const = ast_json_string_get(ast_json_object_get(j_dl_list, tmp));
		ast_free(tmp);
		if(tmp_const == NULL) {
			// No number set.
			continue;
		}

		ast_asprintf(&tmp, "trycnt_%d", i);
		cur_trycnt = ast_json_integer_get(ast_json_object_get(j_dl_list, tmp));
		ast_free(tmp);

		ast_asprintf(&tmp, "max_retry_cnt_%d", i);
		max_trycnt = ast_json_integer_get(ast_json_object_get(j_plan, tmp));
		ast_free(tmp);

		if(cur_trycnt < max_trycnt) {
			dial_num_point = i;
			break;
		}
	}

	return dial_num_point;
}

/**
 * Get dial try count for this time.
 * @param j_dl_list
 * @param dial_num_point
 * @return
 */
int get_dial_try_cnt(struct ast_json* j_dl_list, int dial_num_point)
{
	int cur_trycnt;
	char* tmp;

	ast_asprintf(&tmp, "trycnt_%d", dial_num_point);
	if(ast_json_object_get(j_dl_list, tmp) == NULL) {
		return -1;
	}

	cur_trycnt = ast_json_integer_get(ast_json_object_get(j_dl_list, tmp));
	ast_free(tmp);
	cur_trycnt++;   // for this time.

	return cur_trycnt;
}

/**
 *
 * @param uuid
 * @return
 */
struct ast_json* get_dlmas_all(void)
{
	char* sql;
	struct ast_json* j_res;
	struct ast_json* j_tmp;
	db_res_t* db_res;

	ast_asprintf(&sql, "select * from dl_list_ma where in_use=%d;", E_DL_USE_OK);

	db_res = db_query(sql);
	ast_free(sql);
	if(db_res == NULL) {
		ast_log(LOG_ERROR, "Could not get dl_list_ma info.\n");
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
 * Create dl_list_ma.
 * @param j_dlma
 * @return
 */
bool create_dlma(const struct ast_json* j_dlma)
{
	int ret;
	char* uuid;
	char* view_name;
	char* tmp;
	struct ast_json* j_tmp;

	if(j_dlma == NULL) {
		return false;
	}

	j_tmp = ast_json_deep_copy(j_dlma);

	// uuid
	uuid = gen_uuid();
	ast_json_object_set(j_tmp, "uuid", ast_json_string_create(uuid));

	// create view_table
	view_name = create_view_name(uuid);
	create_dlma_view(uuid, view_name);
	ast_json_object_set(j_tmp, "dl_table", ast_json_string_create(view_name));
	ast_free(view_name);

	// create timestamp
	tmp = get_utc_timestamp();
	ast_json_object_set(j_tmp, "tm_create", ast_json_string_create(tmp));
	ast_free(tmp);

	ast_log(LOG_NOTICE, "Create dlma. uuid[%s], name[%s]\n",
			ast_json_string_get(ast_json_object_get(j_tmp, "uuid")),
			ast_json_string_get(ast_json_object_get(j_tmp, "name"))
			);

	ret = db_insert("dl_list_ma", j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(ret == false) {
		ast_free(uuid);
		return false;
	}

	// send ami event
	j_tmp = get_dlma(uuid);
	ast_free(uuid);
	send_manager_evt_out_dlma_create(j_tmp);
	AST_JSON_UNREF(j_tmp);

	return true;
}

/**
 * Update dl_list_ma.
 * @param j_dlma
 * @return
 */
bool update_dlma(const struct ast_json* j_dlma)
{
	char* tmp;
	const char* tmp_const;
	char* sql;
	struct ast_json* j_tmp;
	char* uuid;
	int ret;

	j_tmp = ast_json_deep_copy(j_dlma);

	tmp_const = ast_json_string_get(ast_json_object_get(j_tmp, "uuid"));
	if(tmp_const == NULL) {
		ast_log(LOG_WARNING, "Could not get uuid.\n");
		AST_JSON_UNREF(j_tmp);
		return false;
	}
	uuid = ast_strdup(tmp_const);

	// update timestamp
	tmp = get_utc_timestamp();
	ast_json_object_set(j_tmp, "tm_update", ast_json_string_create(tmp));
	ast_free(tmp);

	tmp = db_get_update_str(j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(tmp == NULL) {
		ast_log(LOG_WARNING, "Could not get update str.\n");
		ast_free(uuid);
		return false;
	}

	ast_asprintf(&sql, "update dl_list_ma set %s where uuid=\"%s\";", tmp, uuid);
	ast_free(tmp);

	ret = db_exec(sql);
	ast_free(sql);
	if(ret == false) {
		ast_log(LOG_WARNING, "Could not get updated dlma. uuid[%s]\n", uuid);
		ast_free(uuid);
		return false;
	}

	j_tmp = get_dlma(uuid);
	ast_free(uuid);
	if(j_tmp == NULL) {
		ast_log(LOG_WARNING, "Could not get updated dlma info\n");
		return false;
	}
	send_manager_evt_out_dlma_update(j_tmp);
	AST_JSON_UNREF(j_tmp);

	return true;
}

bool delete_dlma(const char* uuid)
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
	ast_asprintf(&sql, "update dl_list_ma set %s where uuid=\"%s\";", tmp, uuid);
	ast_free(tmp);

	ret = db_exec(sql);
	ast_free(sql);
	if(ret == false) {
		ast_log(LOG_WARNING, "Could not delete dlma. uuid[%s]\n", uuid);
		return false;
	}

	// send notification
	send_manager_evt_out_dlma_delete(uuid);

	return true;
}


/**
 *
 * @param uuid
 * @return
 */
struct ast_json* get_dlma(const char* uuid)
{
	char* sql;
	struct ast_json* j_res;
	db_res_t* db_res;

	if(uuid == NULL) {
		ast_log(LOG_WARNING, "Invalid input parameters.\n");
		return NULL;
	}

	ast_asprintf(&sql, "select * from dl_list_ma where uuid=\"%s\" and in_use=%d;", uuid, E_DL_USE_OK);

	db_res = db_query(sql);
	ast_free(sql);
	if(db_res == NULL) {
		ast_log(LOG_ERROR, "Could not get dl_list_ma info. uuid[%s]\n", uuid);
		return NULL;
	}

	j_res = db_get_record(db_res);
	db_free(db_res);

	return j_res;
}

/**
 * Get dl_list from database.
 * @param j_dlma
 * @param j_plan
 * @return
 */
struct ast_json* get_dl_lists(const char* dlma_uuid, int count)
{
	char* sql;
	db_res_t* db_res;
	struct ast_json* j_res;
	struct ast_json* j_tmp;
	struct ast_json* j_dlma;

	if((dlma_uuid == NULL) || (count <= 0)) {
		return NULL;
	}

	j_dlma = get_dlma(dlma_uuid);
	if(j_dlma == NULL) {
		ast_log(LOG_WARNING, "Could not find dlma info. dlma_uuid[%s]\n", dlma_uuid);
		return NULL;
	}

	ast_asprintf(&sql, "select * from `%s` where in_use=%d limit %d;",
			ast_json_string_get(ast_json_object_get(j_dlma, "dl_table")),
			E_DL_USE_OK,
			count
			);
	AST_JSON_UNREF(j_dlma);

	db_res = db_query(sql);
	ast_free(sql);
	if(db_res == NULL) {
		ast_log(LOG_ERROR, "Could not get dial list info.");
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
 * Get dl_list from database.
 * @param j_dlma
 * @param j_plan
 * @return
 */
struct ast_json* get_dl_lists_by_count(int count)
{
	char* sql;
	db_res_t* db_res;
	struct ast_json* j_res;
	struct ast_json* j_tmp;

	if(count <= 0) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return NULL;
	}

	ast_asprintf(&sql, "select * from dl_list where in_use=%d limit %d;",
			E_DL_USE_OK,
			count
			);

	db_res = db_query(sql);
	ast_free(sql);
	if(db_res == NULL) {
		ast_log(LOG_ERROR, "Could not get dial list info.");
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

struct ast_json* get_dl_list(const char* uuid)
{
	char* sql;
	db_res_t* db_res;
	struct ast_json* j_res;

	if(uuid == NULL) {
		return NULL;
	}

	ast_asprintf(&sql, "select * from dl_list where in_use=%d and uuid=\"%s\"", E_DL_USE_OK, uuid);
	db_res = db_query(sql);
	ast_free(sql);

	j_res = db_get_record(db_res);
	db_free(db_res);

	return j_res;
}

int get_dl_list_cnt_total(struct ast_json* j_dlma)
{
	char* sql;
	db_res_t* db_res;
	struct ast_json* j_res;
	int ret;

	if(j_dlma == NULL) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return -1;
	}

	ast_asprintf(&sql, "select count(*) from '%s' where in_use=%d;",
			ast_json_string_get(ast_json_object_get(j_dlma, "dl_table"))? : "",
			E_DL_USE_OK
			);
	db_res = db_query(sql);
	ast_free(sql);

	j_res = db_get_record(db_res);
	db_free(db_res);

	ret = ast_json_integer_get(ast_json_object_get(j_res, "count(*)"));
	AST_JSON_UNREF(j_res);

	return ret;
}

/**
 * Get finished list count.
 * \param j_dlma
 * \param j_plan
 * \return
 */
int get_dl_list_cnt_finshed(struct ast_json* j_dlma, struct ast_json* j_plan)
{
	char* sql;
	db_res_t* db_res;
	struct ast_json* j_res;
	int ret;

	ast_asprintf(&sql, "select count(*)"
			" from `%s` where "
			"("
			" (number_1 is not null and trycnt_1 >= %"PRIdMAX")"
			" or (number_2 is not null and trycnt_2 >= %"PRIdMAX")"
			" or (number_3 is not null and trycnt_3 >= %"PRIdMAX")"
			" or (number_4 is not null and trycnt_4 >= %"PRIdMAX")"
			" or (number_5 is not null and trycnt_5 >= %"PRIdMAX")"
			" or (number_6 is not null and trycnt_6 >= %"PRIdMAX")"
			" or (number_7 is not null and trycnt_7 >= %"PRIdMAX")"
			" or (number_8 is not null and trycnt_8 >= %"PRIdMAX")"
			" or (res_dial == %d)"
			")"
			" and status = %d"
			" and in_use = %d"
			";",
			ast_json_string_get(ast_json_object_get(j_dlma, "dl_table")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_1")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_2")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_3")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_4")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_5")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_6")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_7")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_8")),
			AST_CONTROL_ANSWER,
			E_DL_IDLE,
			E_DL_USE_OK
			);

	db_res = db_query(sql);
	ast_free(sql);
	if(db_res == NULL) {
		ast_log(LOG_ERROR, "Could not get finished dial list count info.");
		return -1;
	}

	j_res = db_get_record(db_res);
	db_free(db_res);
	if(j_res == NULL) {
		return -1;
	}

	ret = ast_json_integer_get(ast_json_object_get(j_res, "count(*)"));
	return ret;
}

/**
 * Get available list count.
 * \param j_dlma
 * \param j_plan
 * \return
 */
int get_dl_list_cnt_available(struct ast_json* j_dlma, struct ast_json* j_plan)
{
	char* sql;
	db_res_t* db_res;
	struct ast_json* j_res;
	int ret;

	ast_asprintf(&sql, "select count(*)"
			" from `%s` where "
			"("
			" (number_1 is not null and trycnt_1 < %"PRIdMAX")"
			" or (number_2 is not null and trycnt_2 < %"PRIdMAX")"
			" or (number_3 is not null and trycnt_3 < %"PRIdMAX")"
			" or (number_4 is not null and trycnt_4 < %"PRIdMAX")"
			" or (number_5 is not null and trycnt_5 < %"PRIdMAX")"
			" or (number_6 is not null and trycnt_6 < %"PRIdMAX")"
			" or (number_7 is not null and trycnt_7 < %"PRIdMAX")"
			" or (number_8 is not null and trycnt_8 < %"PRIdMAX")"
			")"
			" and res_dial != %d"
			" and status = %d"
			" and in_use = %d"
			";",
			ast_json_string_get(ast_json_object_get(j_dlma, "dl_table")),

			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_1")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_2")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_3")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_4")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_5")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_6")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_7")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_8")),

			AST_CONTROL_ANSWER,
			E_DL_IDLE,
			E_DL_USE_OK
			);

	db_res = db_query(sql);
	ast_free(sql);
	if(db_res == NULL) {
		ast_log(LOG_ERROR, "Could not get finished dial list count info.");
		return -1;
	}

	j_res = db_get_record(db_res);
	db_free(db_res);
	if(j_res == NULL) {
		return -1;
	}

	ret = ast_json_integer_get(ast_json_object_get(j_res, "count(*)"));
	return ret;
}

/**
 * Get dl list tryed count.
 * \param j_dlma
 * \param j_plan
 * \return
 */
int get_dl_list_cnt_dialing(struct ast_json* j_dlma)
{
	char* sql;
	db_res_t* db_res;
	struct ast_json* j_res;
	int ret;

	if(j_dlma == NULL) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return -1;
	}

	ast_asprintf(&sql, "select count(*) from '%s' where status != %d and in_use = %d;",
			ast_json_string_get(ast_json_object_get(j_dlma, "dl_table"))? : "",
			E_DL_IDLE,
			E_DL_USE_OK
			);
	db_res = db_query(sql);
	ast_free(sql);

	j_res = db_get_record(db_res);
	db_free(db_res);

	ret = ast_json_integer_get(ast_json_object_get(j_res, "count(*)"));
	AST_JSON_UNREF(j_res);

	return ret;
}

/**
 * Get dialing list count.
 * \param j_dlma
 * \param j_plan
 * \return
 */
int get_dl_list_cnt_tried(struct ast_json* j_dlma)
{
	char* sql;
	db_res_t* db_res;
	struct ast_json* j_res;
	int ret;

	ast_asprintf(&sql, "select "
			" sum(trycnt_1 + trycnt_2 + trycnt_3 + trycnt_4 + trycnt_5 + trycnt_6 + trycnt_7 + trycnt_8) as trycnt"
			" from `%s` where in_use = %d"
			";",
			ast_json_string_get(ast_json_object_get(j_dlma, "dl_table")),
			E_DL_USE_OK
			);

	db_res = db_query(sql);
	ast_free(sql);
	if(db_res == NULL) {
		ast_log(LOG_ERROR, "Could not get dial list info.");
		return -1;
	}

	j_res = db_get_record(db_res);
	db_free(db_res);
	if(j_res == NULL) {
		return -1;
	}

	ret = ast_json_integer_get(ast_json_object_get(j_res, "trycnt"));
	AST_JSON_UNREF(j_res);

	return ret;
}


/**
 * Create dialing json object
 * @param j_camp
 * @param j_plan
 * @param j_dlma
 * @param j_dl_list
 * @return
 */
struct ast_json* create_dial_info(
		struct ast_json* j_plan,
		struct ast_json* j_dl_list,
		struct ast_json* j_dest
		)
{
	struct ast_json* j_dial;
	struct ast_json* j_dial_dest;
	struct ast_json* j_dial_dl;
	struct ast_json* j_dial_plan;
	struct ast_json* j_variables;
	struct ast_json* j_tmp;
	const char* tmp_const;
	char* tmp;


	if((j_plan == NULL) || (j_dl_list == NULL) || (j_dest == NULL)) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return NULL;
	}

	// get dial destination
	j_dial_dest = create_dial_destination_info(j_dest);
	if(j_dial_dest == NULL) {
		ast_log(LOG_ERROR, "Could not create correct dial destination.\n");
		return NULL;
	}

	// get dial dl
	j_dial_dl = create_dial_dl_info(j_dl_list, j_plan);
	if(j_dial_dl == NULL) {
		ast_log(LOG_ERROR, "Could not create correct dial dl info.\n");
		AST_JSON_UNREF(j_dial_dest);
		return NULL;
	}

	j_dial_plan = create_dial_plan_info(j_plan);
	if(j_dial_plan == NULL) {
		ast_log(LOG_ERROR, "Could not create correct dial plan info.\n");
		AST_JSON_UNREF(j_dial_dest);
		AST_JSON_UNREF(j_dial_dl);
		return NULL;
	}

	// create dial
	j_dial = ast_json_object_create();
	ast_json_object_update(j_dial, j_dial_dest);
	ast_json_object_update(j_dial, j_dial_dl);
	ast_json_object_update(j_dial, j_dial_plan);
	AST_JSON_UNREF(j_dial_dest);
	AST_JSON_UNREF(j_dial_dl);
	AST_JSON_UNREF(j_dial_plan);

	// update variables
	j_variables = ast_json_object_create();

	tmp_const = ast_json_string_get(ast_json_object_get(j_dial, "plan_variables"));
	if((tmp_const != NULL) || (strlen(tmp_const) != 0)) {
		j_tmp = ast_json_load_string(tmp_const, NULL);
		if(j_tmp != NULL) {
			ast_json_object_update(j_variables, j_tmp);
		}
		AST_JSON_UNREF(j_tmp);
	}

	tmp_const = ast_json_string_get(ast_json_object_get(j_dial, "dest_variables"));
	if((tmp_const != NULL) || (strlen(tmp_const) != 0)) {
		j_tmp = ast_json_load_string(tmp_const, NULL);
		if(j_tmp != NULL) {
			ast_json_object_update(j_variables, j_tmp);
		}
		AST_JSON_UNREF(j_tmp);
	}

	tmp_const = ast_json_string_get(ast_json_object_get(j_dial, "dl_variables"));
	if((tmp_const != NULL) || (strlen(tmp_const) != 0)) {
		j_tmp = ast_json_load_string(tmp_const, NULL);
		if(j_tmp != NULL) {
			ast_json_object_update(j_variables, j_tmp);
		}
		AST_JSON_UNREF(j_tmp);
	}

	tmp = ast_json_dump_string(j_variables);
	ast_json_object_set(j_dial, "variables", ast_json_string_create(tmp?:""));
	ast_json_free(tmp);
	AST_JSON_UNREF(j_variables);

	return j_dial;
}

/**
 * Create dl_list's dial address.
 * @param j_camp
 * @param j_plan
 * @param j_dl_list
 * @return
 */
static char* create_chan_addr_for_dial(struct ast_json* j_plan, struct ast_json* j_dl_list, int dial_num_point)
{
	char* dest_addr;
	char* chan_addr;
	const char* trunk_name;
	const char* tech_name;

	if(dial_num_point < 0) {
		ast_log(LOG_WARNING, "Wrong dial number point.\n");
		return NULL;
	}

	trunk_name = ast_json_string_get(ast_json_object_get(j_plan, "trunk_name"));
	if(trunk_name == NULL) {
		trunk_name = "";
	}

	tech_name = ast_json_string_get(ast_json_object_get(j_plan, "tech_name"));
	if(tech_name == NULL) {
		tech_name = "";
	}

	// get dial number
	dest_addr = get_dial_number(j_dl_list, dial_num_point);
	if(dest_addr == NULL) {
		ast_log(LOG_WARNING, "Could not get destination address.\n");
		return NULL;
	}

	// create dial addr
	if(strlen(trunk_name) > 0) {
		ast_asprintf(&chan_addr, "%s%s@%s", tech_name, dest_addr, trunk_name);
	}
	else {
		ast_asprintf(&chan_addr, "%s%s", tech_name, dest_addr);
	}
	ast_log(LOG_DEBUG, "Created dialing channel address. chan_addr[%s].\n", chan_addr);
	ast_free(dest_addr);

	return chan_addr;
}

/**
 * Return dial number of j_dlist.
 * @param j_dlist
 * @param cnt
 * @return
 */
static char* get_dial_number(struct ast_json* j_dlist, const int cnt)
{
	char* res;
	char* tmp;

	ast_asprintf(&tmp, "number_%d", cnt);

	ast_asprintf(&res, "%s", ast_json_string_get(ast_json_object_get(j_dlist, tmp)));
	ast_free(tmp);

	return res;
}

/**
 * Create dl_result
 * @param dialing
 * @return
 */
struct ast_json* create_json_for_dl_result(rb_dialing* dialing)
{
	struct ast_json* j_res;
	const char* tmp_const;


	j_res = ast_json_deep_copy(dialing->j_dialing);

	ast_log(LOG_DEBUG, "Check value. dialing_uuid[%s], camp_uuid[%s], plan_uuid[%s], dlma_uuid[%s], dl_list_uuid[%s]\n",
			ast_json_string_get(ast_json_object_get(j_res, "dialing_uuid")),
			ast_json_string_get(ast_json_object_get(j_res, "camp_uuid")),
			ast_json_string_get(ast_json_object_get(j_res, "plan_uuid")),
			ast_json_string_get(ast_json_object_get(j_res, "dlma_uuid")),
			ast_json_string_get(ast_json_object_get(j_res, "dl_list_uuid"))
			);

	// result_info_enable
	tmp_const = ast_json_string_get(ast_json_object_get(ast_json_object_get(g_app->j_conf, "general"), "result_info_enable"));
	if((tmp_const != NULL) && (atoi(tmp_const) == 1)) {
		// write info
		// already copied it.
	}
	else {
		// otherwise, remove
		ast_json_object_del(j_res, "info_camp");
		ast_json_object_del(j_res, "info_dial");
		ast_json_object_del(j_res, "info_plan");
		ast_json_object_del(j_res, "info_dest");
		ast_json_object_del(j_res, "info_dlma");
		ast_json_object_del(j_res, "info_dl_list");
		ast_json_object_del(j_res, "info_events");
	}

	// check history options.
	tmp_const = ast_json_string_get(ast_json_object_get(ast_json_object_get(g_app->j_conf, "general"), "result_history_events_enable"));
	if((tmp_const != NULL) && (atoi(tmp_const) == 1)) {
		// write info
		ast_json_object_set(j_res, "history_events", ast_json_ref(dialing->j_events));
	}

	return j_res;
}

/**
 * Create view name using uuid.
 * @param uuid
 * @return
 */
static char* create_view_name(const char* uuid)
{
	char* tmp;
	int len;
	int i;
	int j;

	if(uuid == NULL) {
		return NULL;
	}

	len = strlen(uuid);
	tmp = ast_calloc(len + 1, sizeof(char));
	j = 0;
	for(i = 0; i < len; i++) {
		if(uuid[i] == '-') {
			tmp[j] = '_';
		}
		else {
			tmp[j] = uuid[i];
		}
		j++;
	}
	tmp[j] = '\0';
	return tmp;
}

/**
 * Create dl_list
 * @param j_dl
 * @return
 */
char* create_dl_list(struct ast_json* j_dl)
{
	int ret;
	char* uuid;
	char* tmp;
	struct ast_json* j_tmp;

	if(j_dl == NULL) {
		return NULL;
	}

	j_tmp = ast_json_deep_copy(j_dl);

	// uuid
	uuid = gen_uuid();
	ast_json_object_set(j_tmp, "uuid", ast_json_string_create(uuid));

	// create timestamp
	tmp = get_utc_timestamp();
	ast_json_object_set(j_tmp, "tm_create", ast_json_string_create(tmp));
	ast_free(tmp);

	ast_log(LOG_NOTICE, "Create dl_list. dl_uuid[%s], dlma_uuid[%s], name[%s]\n",
			ast_json_string_get(ast_json_object_get(j_tmp, "uuid")),
			ast_json_string_get(ast_json_object_get(j_tmp, "dlma_uuid")),
			ast_json_string_get(ast_json_object_get(j_tmp, "name"))
			);

	ret = db_insert("dl_list", j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(ret == false) {
		ast_free(uuid);
		return NULL;
	}

	// do not send any create event for dl_list.
	return uuid;
}

/**
 * delete dl_list
 * @param uuid
 * @return
 */
bool delete_dl_list(const char* uuid)
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
	ast_asprintf(&sql, "update dl_list set %s where uuid=\"%s\";", tmp, uuid);
	ast_free(tmp);

	ret = db_exec(sql);
	ast_free(sql);
	if(ret == false) {
		ast_log(LOG_WARNING, "Could not delete dl_list. uuid[%s]\n", uuid);
		return false;
	}

	// No notifications for dl list.

	return true;
}

static bool create_dlma_view(const char* uuid, const char* view_name)
{
	char* sql;
	int ret;

	if((uuid == NULL) || (view_name == NULL)) {
		return false;
	}

	ast_asprintf(&sql, "create view `%s` as select * from dl_list where dlma_uuid=\"%s\";", view_name, uuid);

	ret = db_exec(sql);
	ast_free(sql);
	if(ret == false) {
		ast_log(LOG_WARNING, "Could not create view. uuid[%s], view_name[%s]\n", uuid, view_name);
		return false;
	}

	return true;
}

static struct ast_json* create_dial_dl_info(struct ast_json* j_dl_list, struct ast_json* j_plan)
{
	int index;
	int count;
	char* addr;
	char* channel;
	struct ast_json* j_res;
	char* channel_id;
	char* other_channel_id;

	if((j_dl_list == NULL) || (j_plan == NULL)) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return NULL;
	}

	// get dial number point(index)
	index = get_dial_num_point(j_dl_list, j_plan);
	if(index < 0) {
		ast_log(LOG_ERROR, "Could not find correct number count.\n");
		return NULL;
	}

	// get dial count
	count = get_dial_try_cnt(j_dl_list, index);
	if(count == -1) {
		ast_log(LOG_ERROR, "Could not get correct dial count number.\n");
		return NULL;
	}

	// get dial address
	addr = get_dial_number(j_dl_list, index);
	if(addr == NULL) {
		ast_log(LOG_ERROR, "Could not get correct dial address.\n");
		return NULL;
	}

	// create destination channel address.
	channel = create_chan_addr_for_dial(j_plan, j_dl_list, index);
	if(channel == NULL) {
		ast_log(LOG_ERROR, "Could not get correct channel address.\n");
		ast_free(addr);
		return NULL;
	}

	channel_id = gen_uuid();
	other_channel_id = gen_uuid();

	j_res = ast_json_pack(
			"{"
			"s:s, "
			"s:s, s:s, s:i, s:i, s:s, "
			"s:s, s:s"
			"}",

			"uuid",	  ast_json_string_get(ast_json_object_get(j_dl_list, "uuid")),

			"dial_channel", 	channel,
			"dial_addr",			addr,
			"dial_index",			index,
			"dial_trycnt",		count,
			"dl_variables",	  ast_json_string_get(ast_json_object_get(j_dl_list, "variables"))? : "",

			"channelid",			channel_id,
			"otherchannelid",	other_channel_id
			);
	ast_free(channel);
	ast_free(addr);
	ast_free(channel_id);
	ast_free(other_channel_id);

	return j_res;
}

/**
 * Return is this endable dl list.
 * \param j_dlma
 * \param j_plan
 * \return
 */
bool is_endable_dl_list(struct ast_json* j_dlma, struct ast_json* j_plan)
{
	int ret;

	// check is there dial-able dl list.
	ret = check_more_dl_list(j_dlma, j_plan);
	if(ret == true) {
		return false;
	}

	return true;
}

/**
 * Check is the given dl is older than given plan's retry delay time.
 * @param j_dlma
 * @param j_dl
 * @param j_plan
 * @return
 */
static bool is_over_retry_delay(struct ast_json* j_dlma, struct ast_json* j_dl, struct ast_json* j_plan)
{
	struct ast_json* j_tmp;
	const char* tm_last_hangup;
	int retry_delay;
	char* sql;
	db_res_t* db_res;

	if((j_dlma == NULL) || (j_dl == NULL) || (j_plan == NULL)) {
		ast_log(LOG_WARNING, "Wrong input parameters.\n");
		return false;
	}

	// check last hangup timestamp.
	// if there's no tm_last_hangup, then it's first try.
	tm_last_hangup = ast_json_string_get(ast_json_object_get(j_dl, "tm_last_hangup"));
	if((tm_last_hangup == NULL) || (strlen(tm_last_hangup) == 0)) {
		return true;
	}

	retry_delay = ast_json_integer_get(ast_json_object_get(j_plan, "retry_delay"));

	ast_asprintf(&sql, "select * from '%s' where uuid = '%s' and ((strftime('%%s', 'now') - strftime('%%s', tm_last_hangup)) > %d);",
			ast_json_string_get(ast_json_object_get(j_dlma, "dl_table")),
			ast_json_string_get(ast_json_object_get(j_dl, "uuid")),
			retry_delay
			);

	db_res = db_query(sql);
	ast_free(sql);

	if(db_res == NULL) {
		return false;
	}

	j_tmp = db_get_record(db_res);
	db_free(db_res);
	if(j_tmp == NULL) {
		return false;
	}
	AST_JSON_UNREF(j_tmp);

	return true;
}

/**
 * Get available dl_list from database.
 * @param j_dlma
 * @param j_plan
 * @return
 */
static struct ast_json* get_dl_available(struct ast_json* j_dlma, struct ast_json* j_plan)
{
	char* sql;
	db_res_t* db_res;
	struct ast_json* j_res;

	ast_asprintf(&sql, "select *, "
			"(trycnt_1 + trycnt_2 + trycnt_3 + trycnt_4 + trycnt_5 + trycnt_6 + trycnt_7 + trycnt_8) as trycnt"
			" from `%s` where ("
			"(number_1 is not null and trycnt_1 < %"PRIdMAX")"
			" or (number_2 is not null and trycnt_2 < %"PRIdMAX")"
			" or (number_3 is not null and trycnt_3 < %"PRIdMAX")"
			" or (number_4 is not null and trycnt_4 < %"PRIdMAX")"
			" or (number_5 is not null and trycnt_5 < %"PRIdMAX")"
			" or (number_6 is not null and trycnt_6 < %"PRIdMAX")"
			" or (number_7 is not null and trycnt_7 < %"PRIdMAX")"
			" or (number_8 is not null and trycnt_8 < %"PRIdMAX")"
			")"
			" and res_dial != %d"
			" and status = %d"
			" order by trycnt asc"
			" limit 1"
			";",
			ast_json_string_get(ast_json_object_get(j_dlma, "dl_table")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_1")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_2")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_3")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_4")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_5")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_6")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_7")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_8")),
			AST_CONTROL_ANSWER,
			E_DL_IDLE
			);

	db_res = db_query(sql);
	ast_free(sql);
	if(db_res == NULL) {
		ast_log(LOG_ERROR, "Could not get dial list info.");
		return NULL;
	}

	j_res = db_get_record(db_res);
	db_free(db_res);
	if(j_res == NULL) {
		return NULL;
	}

	return j_res;
}
