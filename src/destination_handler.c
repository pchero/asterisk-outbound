/*
 * destination_handler.c
 *
 *  Created on: Oct 19, 2016
 *      Author: pchero
 */


#include "asterisk.h"
#include "asterisk/json.h"
#include "asterisk/logger.h"
#include "asterisk/utils.h"
#include "asterisk/uuid.h"

#include "db_handler.h"
#include "cli_handler.h"
#include "event_handler.h"
#include "utils.h"
#include "ami_handler.h"

#include "destination_handler.h"

static int get_avail_cnt_exten(struct ast_json* j_dest);
static int get_avail_cnt_app(struct ast_json* j_dest);
static int get_avail_cnt_app_park(void);
static int get_avail_cnt_app_queue(const char* name);
static int get_avail_cnt_app_queue_service_perf(const char* name);

static struct ast_json* get_app_queue_param(const char* name);
struct ast_json* get_app_queue_summary(const char* name);

/**
 * Create destination.
 * @param j_camp
 * @return
 */
bool create_destination(const struct ast_json* j_dest)
{
	int ret;
	char* uuid;
	char* tmp;
	struct ast_json* j_tmp;

	if(j_dest == NULL) {
		ast_log(LOG_ERROR, "Wrong input parameter.\n");
		return false;
	}
	j_tmp = ast_json_deep_copy(j_dest);

	uuid = gen_uuid();
	ast_json_object_set(j_tmp, "uuid", ast_json_string_create(uuid));

	tmp = get_utc_timestamp();
	ast_json_object_set(j_tmp, "tm_create", ast_json_string_create(tmp));
	ast_free(tmp);

	ast_log(LOG_NOTICE, "Create destination. uuid[%s], name[%s]\n",
			ast_json_string_get(ast_json_object_get(j_tmp, "uuid")),
			ast_json_string_get(ast_json_object_get(j_tmp, "name"))? : ""
			);
	ret = db_insert("destination", j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(ret == false) {
		ast_free(uuid);
		return false;
	}

	// send ami event
	j_tmp = get_destination(uuid);
	ast_free(uuid);
	send_manager_evt_out_destination_create(j_tmp);
	AST_JSON_UNREF(j_tmp);

	return true;
}


/**
 * Delete destination.
 * @param uuid
 * @return
 */
bool delete_destination(const char* uuid)
{
	struct ast_json* j_tmp;
	char* tmp;
	char* sql;
	int ret;

	if(uuid == NULL) {
		// invalid parameter.
		ast_log(LOG_ERROR, "Wrong input parameter.\n");
		return false;
	}

	j_tmp = ast_json_object_create();
	tmp = get_utc_timestamp();
	ast_json_object_set(j_tmp, "tm_delete", ast_json_string_create(tmp));
	ast_json_object_set(j_tmp, "in_use", ast_json_integer_create(0));
	ast_free(tmp);

	tmp = db_get_update_str(j_tmp);
	AST_JSON_UNREF(j_tmp);
	ast_asprintf(&sql, "update campaign set %s where uuid=\"%s\";", tmp, uuid);
	ast_free(tmp);

	ret = db_exec(sql);
	ast_free(sql);
	if(ret == false) {
		ast_log(LOG_WARNING, "Could not delete destination. uuid[%s]\n", uuid);
		return false;
	}

	// send notification
	send_manager_evt_out_destination_delete(uuid);

	return true;
}

/**
 * Get specified destination
 * @return
 */
struct ast_json* get_destination(const char* uuid)
{
	struct ast_json* j_res;
	db_res_t* db_res;
	char* sql;

	if(uuid == NULL) {
		ast_log(LOG_ERROR, "Wrong input parameter.\n");
		return NULL;
	}
	ast_log(LOG_DEBUG, "Get destination info. uuid[%s]\n", uuid);

	// get specified destination
	ast_asprintf(&sql, "select * from destination where uuid=\"%s\" and in_use=1;", uuid);

	db_res = db_query(sql);
	ast_free(sql);
	if(db_res == NULL) {
		ast_log(LOG_WARNING, "Could not get destination info.\n");
		return NULL;
	}

	j_res = db_get_record(db_res);
	db_free(db_res);

	return j_res;
}


/**
 * Get all destinations
 * @return
 */
struct ast_json* get_destinations_all(void)
{
	struct ast_json* j_res;
	struct ast_json* j_tmp;
	db_res_t* db_res;
	char* sql;

	// get all campaigns
	ast_asprintf(&sql, "%s", "select * from destination where in_use=1");

	db_res = db_query(sql);
	ast_free(sql);
	if(db_res == NULL) {
		ast_log(LOG_WARNING, "Could not get destinations info.\n");
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
 * Update destination
 * @param j_dest
 * @return
 */
bool update_destination(const struct ast_json* j_dest)
{
	char* tmp;
	const char* tmp_const;
	char* sql;
	struct ast_json* j_tmp;
	char* uuid;

	if(j_dest == NULL) {
		ast_log(LOG_ERROR, "Wrong input parameter.\n");
		return false;
	}

	j_tmp = ast_json_deep_copy(j_dest);
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

	ast_asprintf(&sql, "update destination set %s where in_use=1 and uuid=\"%s\";", tmp, uuid);
	ast_free(tmp);

	db_exec(sql);
	ast_free(sql);

	j_tmp = get_destination(uuid);
	ast_free(uuid);
	if(j_tmp == NULL) {
		ast_log(LOG_WARNING, "Could not get updated destination info.\n");
		return false;
	}
	send_manager_evt_out_destination_update(j_tmp);
	AST_JSON_UNREF(j_tmp);

	return true;
}

/**
 * Return the available resource count of given destination
 * \param j_dest
 * \return
 */
int get_destination_available_count(struct ast_json* j_dest)
{
	int type;
	int ret;

	type = ast_json_integer_get(ast_json_object_get(j_dest, "type"));
	switch(type) {
		case DESTINATION_EXTEN:
		{
			ret = get_avail_cnt_exten(j_dest);
		}
		break;

		case DESTINATION_APPLICATION:
		{
			ret = get_avail_cnt_app(j_dest);
		}
		break;

		default:
		{
			ast_log(LOG_ERROR, "No support destination type. type[%d]\n", type);
			ret = 0;
		}
		break;
	}

	return ret;
}

/**
 * \param j_dest
 * \return
 */
static int get_avail_cnt_exten(struct ast_json* j_dest)
{
	return DEF_DESTINATION_AVAIL_CNT_UNLIMITED;
}

/**
 * TODO:
 * \param j_dest
 * \return
 */
static int get_avail_cnt_app(struct ast_json* j_dest)
{
	const char* application;
	const char* data;
	int ret;

	if(j_dest == NULL) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return 0;
	}

	application = ast_json_string_get(ast_json_object_get(j_dest, "application"));
	if(application == NULL) {
		ast_log(LOG_WARNING, "Could not get correct application name. uuid[%s]\n",
				ast_json_string_get(ast_json_object_get(j_dest, "uuid"))
				);
		return 0;
	}

	data = ast_json_string_get(ast_json_object_get(j_dest, "data"));

	if(strcasecmp(application, "queue") == 0) {
		ret = get_avail_cnt_app_queue(data);
	}
	else if(strcasecmp(application, "park") == 0) {
		ret = get_avail_cnt_app_park();
	}
	else {
		ast_log(LOG_WARNING, "Unsupported application. Consider unlimited. application[%s]\n", application);
		ret = DEF_DESTINATION_AVAIL_CNT_UNLIMITED;
	}

	ast_log(LOG_DEBUG, "Available application count. cnt[%d]\n", ret);

	return ret;
}

static int get_avail_cnt_app_queue_service_perf(const char* name)
{
	struct ast_json* j_tmp;
	double service_perf;

	if(name == NULL) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return 0;
	}

	// get queue param
	j_tmp = get_app_queue_param(name);
	if(j_tmp == NULL) {
		ast_log(LOG_ERROR, "Could not get queue_param info. queue_name[%s]\n", name);
		return 0;
	}

	service_perf = atof(ast_json_string_get(ast_json_object_get(j_tmp, "ServicelevelPerf")));
	if(service_perf == 0) {
		service_perf = 100;
	}
	AST_JSON_UNREF(j_tmp);

	return service_perf;
}

static int get_avail_cnt_app_queue(const char* name)
{
	struct ast_json* j_tmp;
	const char* tmp_const;
	int ret;
	int perf;
	int avail;
	int loggedin;

	if(name == NULL) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return 0;
	}

	// get queue summary info.
	j_tmp = get_app_queue_summary(name);
	if(j_tmp == NULL) {
		ast_log(LOG_ERROR, "Could not get queue_summary info. queue_name[%s]\n", name);
		return 0;
	}

	// get available, loggedin
	tmp_const = ast_json_string_get(ast_json_object_get(j_tmp, "Available"));
	avail = atoi(tmp_const);

	tmp_const = ast_json_string_get(ast_json_object_get(j_tmp, "LoggedIn"));
	loggedin = atoi(tmp_const);
	AST_JSON_UNREF(j_tmp);

	if(loggedin < 10) {
		ast_log(LOG_DEBUG, "Not many people logged in. Ignore perf calculate. loggenin[%d], avail[%d]\n", loggedin, avail);
		return avail;
	}

	// get perf
	perf = get_avail_cnt_app_queue_service_perf(name);

	ret = avail * perf / 100;
	ast_log(LOG_DEBUG, "Application queue available count. name[%s], available[%d], performance[%d]\n", name, avail, perf);
	return ret;
}

/**
 *
 * \return -1:unlimited
 */
static int get_avail_cnt_app_park(void)
{
	return DEF_DESTINATION_AVAIL_CNT_UNLIMITED;
}


/**
 * Get Queue param only.
 * @param name
 * @return
 */
static struct ast_json* get_app_queue_param(const char* name)
{
	struct ast_json* j_ami_res;
	struct ast_json* j_param;
	struct ast_json* j_tmp;
	size_t size;
	int i;
	const char* tmp_const;

	if(name == NULL) {
		return NULL;
	}
	ast_log(LOG_DEBUG, "Getting queue param info. queue_name[%s]\n", name);

	j_ami_res = ami_cmd_queue_status(name);
	if(j_ami_res == NULL) {
		ast_log(LOG_NOTICE, "Could not get queue status. name[%s]\n", name);
		return NULL;
	}

	// get result.
	i = 0;
	j_param = NULL;
	size = ast_json_array_size(j_ami_res);
	for(i = 0; i < size; i++) {
		j_tmp = ast_json_array_get(j_ami_res, i);

		// Event check
		tmp_const = ast_json_string_get(ast_json_object_get(j_tmp, "Event"));
		if(tmp_const == NULL) {
			continue;
		}
		if(strcmp(tmp_const, "QueueParams") != 0) {
			continue;
		}

		// compare the queue name
		tmp_const = ast_json_string_get(ast_json_object_get(j_tmp, "Queue"));
		if(tmp_const == NULL) {
			continue;
		}
		if(strcmp(tmp_const, name) == 0) {
			j_param = ast_json_deep_copy(j_tmp);
			break;
		}
	}
	AST_JSON_UNREF(j_ami_res);
	if(j_param == NULL) {
		ast_log(LOG_NOTICE, "Could not get queue param. name[%s]\n", name);
		return NULL;
	}

	return j_param;
}

struct ast_json* get_app_queue_summary(const char* name)
{
	struct ast_json* j_ami_res;
	struct ast_json* j_queue;
	struct ast_json* j_tmp;
	size_t size;
	int i;
	const char* tmp_const;

	if(name == NULL) {
		return NULL;
	}

	j_ami_res = ami_cmd_queue_summary(name);
	if(j_ami_res == NULL) {
		ast_log(LOG_NOTICE, "Could not get queue summary. name[%s]\n", name);
		return NULL;
	}

	// get result.
	i = 0;
	j_queue = NULL;
	size = ast_json_array_size(j_ami_res);
	for(i = 0; i < size; i++) {
		j_tmp = ast_json_array_get(j_ami_res, i);
		tmp_const = ast_json_string_get(ast_json_object_get(j_tmp, "Queue"));
		if(tmp_const == NULL) {
			continue;
		}
		if(strcmp(tmp_const, name) == 0) {
			j_queue = ast_json_deep_copy(j_tmp);
			break;
		}
	}
	AST_JSON_UNREF(j_ami_res);
	if(j_queue == NULL) {
		ast_log(LOG_NOTICE, "Could not get queue summary. name[%s]\n", name);
		return NULL;
	}

	return j_queue;
}

static struct ast_json* create_destination_exten(struct ast_json* j_dest)
{
	struct ast_json* j_res;

	if(j_dest == NULL) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return NULL;
	}

	j_res = ast_json_pack("{s:s, s:s, s:s, s:s}",
			"dial_exten",			ast_json_string_get(ast_json_object_get(j_dest, "exten"))? : "",
			"dial_context",		ast_json_string_get(ast_json_object_get(j_dest, "context"))? : "",
			"dial_priority",	ast_json_string_get(ast_json_object_get(j_dest, "priority"))? : "",
			"dest_variables",	ast_json_string_get(ast_json_object_get(j_dest, "variables"))? : ""
			);

	return j_res;
}

static struct ast_json* create_dial_destination_application(struct ast_json* j_dest)
{
	struct ast_json* j_res;

	if(j_dest == NULL) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return NULL;
	}

	j_res = ast_json_pack("{s:s, s:s, s:s}",
			"dial_application",		ast_json_string_get(ast_json_object_get(j_dest, "application"))? : "",
			"dial_data",					ast_json_string_get(ast_json_object_get(j_dest, "data"))? : "",
			"dest_variables",			ast_json_string_get(ast_json_object_get(j_dest, "variables"))? : ""
			);

	return j_res;
}

struct ast_json* create_dial_destination_info(struct ast_json* j_dest)
{
	int type;
	struct ast_json* j_res;

	if(j_dest == NULL) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
		return NULL;
	}

	type = ast_json_integer_get(ast_json_object_get(j_dest, "type"));
	switch(type) {
		case DESTINATION_EXTEN:
		{
			j_res = create_destination_exten(j_dest);
		}
		break;

		case DESTINATION_APPLICATION:
		{
			j_res = create_dial_destination_application(j_dest);
		}
		break;

		default:
		{
			ast_log(LOG_ERROR, "No support destination type. type[%d]\n", type);
			j_res = NULL;
		}
		break;
	}

	if(j_res == NULL) {
		ast_log(LOG_ERROR, "Could not get correct dial destination info.\n");
		return NULL;
	}

	// set dial_type
	ast_json_object_set(j_res, "dial_type", ast_json_integer_create(type));

	return j_res;
}

