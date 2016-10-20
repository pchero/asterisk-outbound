/*
 * event_handler.c
 *
 *  Created on: Nov 8, 2015
 *	  Author: pchero
 */

#include "asterisk.h"

#include <stdbool.h>
#include <event2/event.h>
#include <event2/thread.h>
#include <errno.h>

#include "asterisk/json.h"
#include "asterisk/utils.h"
#include "asterisk/cli.h"
#include "asterisk/uuid.h"
#include "asterisk/file.h"

#include "res_outbound.h"
#include "db_handler.h"
#include "ami_handler.h"
#include "event_handler.h"
#include "dialing_handler.h"
#include "campaign_handler.h"
#include "dl_handler.h"
#include "plan_handler.h"
#include "utils.h"

#define TEMP_FILENAME "/tmp/asterisk_outbound_tmp.txt"


struct event_base*  g_base = NULL;

static int init_outbound(void);

static void cb_campaign_start(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);
static void cb_campaign_starting(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);
static void cb_campaign_stopping(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);
static void cb_campaign_stopping_force(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);
static void cb_check_dialing_end(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);
static void cb_check_dialing_error(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);
static void cb_check_campaign_end(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);

//static struct ast_json* get_queue_info(const char* uuid);

static void dial_desktop(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma);
static void dial_power(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma);
static void dial_predictive(struct ast_json* j_camp, struct ast_json* j_plan, struct ast_json* j_dlma);
static void dial_robo(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma);
static void dial_redirect(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma);

struct ast_json* get_queue_summary(const char* name);
struct ast_json* get_queue_param(const char* name);

static bool write_result_json(struct ast_json* j_res);

// todo
static int check_dial_avaiable_predictive(struct ast_json* j_camp, struct ast_json* j_plan, struct ast_json* j_dlma);

int run_outbound(void)
{
	int ret;
	struct event* ev;
//	struct timeval tm_fast = {0, 2000};	// 20 ms
	struct timeval tm_fast = {3, 0};	// 3 sec
//	struct timeval tm_slow = {0, 500000};   // 500 ms

	// init libevent
	ret = init_outbound();
	if(ret == false) {
		ast_log(LOG_ERROR, "Could not initiate outbound.\n");
		return false;
	}

	// check start.
	ev = event_new(g_base, -1, EV_TIMEOUT | EV_PERSIST, cb_campaign_start, NULL);
	event_add(ev, &tm_fast);

	// check starting
	ev = event_new(g_base, -1, EV_TIMEOUT | EV_PERSIST, cb_campaign_starting, NULL);
	event_add(ev, &tm_fast);

	// check stopping.
	ev = event_new(g_base, -1, EV_TIMEOUT | EV_PERSIST, cb_campaign_stopping, NULL);
	event_add(ev, &tm_fast);

	// check force stopping
	ev = event_new(g_base, -1, EV_TIMEOUT | EV_PERSIST, cb_campaign_stopping_force, NULL);
	event_add(ev, &tm_fast);



	ev = event_new(g_base, -1, EV_TIMEOUT | EV_PERSIST, cb_check_dialing_end, NULL);
	event_add(ev, &tm_fast);

	ev = event_new(g_base, -1, EV_TIMEOUT | EV_PERSIST, cb_check_dialing_error, NULL);
	event_add(ev, &tm_fast);

	ev = event_new(g_base, -1, EV_TIMEOUT | EV_PERSIST, cb_check_campaign_end, NULL);
	event_add(ev, &tm_fast);


	event_base_loop(g_base, 0);

	return true;
}

static int init_outbound(void)
{
	int ret;
	struct ast_json* j_res;
	db_res_t* db_res;

	ret = evthread_use_pthreads();
	if(ret == -1){
		ast_log(LOG_ERROR, "Could not initiated event thread.");
	}

	// init libevent
	if(g_base == NULL) {
		ast_log(LOG_DEBUG, "event_base_new\n");
		g_base = event_base_new();
	}

	if(g_base == NULL) {
		ast_log(LOG_ERROR, "Could not initiate libevent. err[%d:%s]\n", errno, strerror(errno));
		return false;
	}

	// check database tables.
	db_res = db_query("select 1 from campaign limit 1;");
	if(db_res == NULL) {
		ast_log(LOG_ERROR, "Could not initiate libevent. Table is not ready.\n");
		return false;
	}

	j_res = db_get_record(db_res);
	db_free(db_res);
	ast_json_unref(j_res);

	ret = init_plan();
	if(ret == false) {
		ast_log(LOG_ERROR, "Could not initiate plan.\n");
		return false;
	}

	ast_log(LOG_NOTICE, "Initiated outbound.\n");

	return true;
}

void stop_outbound(void)
{
	struct timeval sec;

	sec.tv_sec = 0;
	sec.tv_usec = 0;

	event_base_loopexit(g_base, &sec);

	return;
}

/**
 *  @brief  Check start status campaign and trying to make a call.
 */
static void cb_campaign_start(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg)
{
	struct ast_json* j_camp;
	struct ast_json* j_plan;
	struct ast_json* j_dlma;
//	struct ast_json* j_queue;
	int dial_mode;

	ast_log(LOG_DEBUG, "cb_campagin start\n");

	j_camp = get_campaign_for_dialing();
	if(j_camp == NULL) {
		// Nothing.
		return;
	}
//	ast_log(LOG_DEBUG, "Get campaign info. camp_uuid[%s], camp_name[%s]\n",
//			ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
//			ast_json_string_get(ast_json_object_get(j_camp, "name"))
//			);

	// get plan
	j_plan = get_plan(ast_json_string_get(ast_json_object_get(j_camp, "plan")));
	if(j_plan == NULL) {
		ast_log(LOG_WARNING, "Could not get plan info. Stopping campaign camp[%s], plan[%s]\n",
				ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
				ast_json_string_get(ast_json_object_get(j_camp, "plan"))
				);
		update_campaign_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOPPING);
		ast_json_unref(j_camp);
		return;
	}

//	// get queue
//	j_queue = get_queue_info(ast_json_string_get(ast_json_object_get(j_camp, "queue")));
//	if(j_plan == NULL) {
//		ast_log(LOG_WARNING, "Could not get queue info. Stopping campaign camp[%s], queue[%s]\n",
//				ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
//				ast_json_string_get(ast_json_object_get(j_camp, "queue"))
//				);
//		update_campaign_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOPPING);
//		ast_json_unref(j_camp);
//		return;
//	}

//	ast_log(LOG_DEBUG, "Get plan info. camp_uuid[%s], camp_name[%s]\n",
//			ast_json_string_get(ast_json_object_get(j_plan, "uuid")),
//			ast_json_string_get(ast_json_object_get(j_plan, "name"))
//			);

	// get dl_master_info
	j_dlma = get_dlma(ast_json_string_get(ast_json_object_get(j_camp, "dlma")));
	if(j_dlma == NULL)
	{
		ast_log(LOG_ERROR, "Could not find dial list master info. Stopping campaign. camp[%s], dlma[%s]\n",
				ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
				ast_json_string_get(ast_json_object_get(j_camp, "dlma"))
				);
		update_campaign_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOPPING);
		ast_json_unref(j_camp);
		ast_json_unref(j_plan);
//		ast_json_unref(j_queue);
		return;
	}
	ast_log(LOG_DEBUG, "Get dlma info. dlma_uuid[%s], dlma_name[%s]\n",
			ast_json_string_get(ast_json_object_get(j_dlma, "uuid")),
			ast_json_string_get(ast_json_object_get(j_dlma, "name"))
			);

	// get dial_mode
	dial_mode = ast_json_integer_get(ast_json_object_get(j_plan, "dial_mode"));
	if(dial_mode == E_DIAL_MODE_NONE) {
		ast_log(LOG_ERROR, "Plan has no dial_mode. Stopping campaign. camp[%s], plan[%s]",
				ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
				ast_json_string_get(ast_json_object_get(j_camp, "plan"))
				);

		update_campaign_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOPPING);
		ast_json_unref(j_camp);
		ast_json_unref(j_plan);
		ast_json_unref(j_dlma);
//		ast_json_unref(j_queue);
		return;
	}

	switch(dial_mode) {
		case E_DIAL_MODE_PREDICTIVE: {
			dial_predictive(j_camp, j_plan, j_dlma);
		}
		break;

		case E_DIAL_MODE_DESKTOP: {
			dial_desktop(j_camp, j_plan, j_dlma);
		}
		break;

		case E_DIAL_MODE_POWER: {
			dial_power(j_camp, j_plan, j_dlma);
		}
		break;

		case E_DIAL_MODE_ROBO: {
			dial_robo(j_camp, j_plan, j_dlma);
		}
		break;

		case E_DIAL_MODE_REDIRECT: {
			dial_redirect(j_camp, j_plan, j_dlma);
		}
		break;

		default: {
			ast_log(LOG_ERROR, "No match dial_mode. dial_mode[%d]\n", dial_mode);
		}
		break;
	}

	// release
	ast_json_unref(j_camp);
	ast_json_unref(j_plan);
	ast_json_unref(j_dlma);
//	ast_json_unref(j_queue);

	return;
}

/**
 *  @brief  Check starting status campaign and update status to stopping or start.
 */
static void cb_campaign_starting(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg)
{
	struct ast_json* j_camps;
	struct ast_json* j_camp;
	int size;
	int i;
	int ret;

	ast_log(LOG_DEBUG, "cb_campaign_starting.\n");

	j_camps = get_campaigns_by_status(E_CAMP_STARTING);
	if(j_camps == NULL) {
		// Nothing.
		return;
	}

	size = ast_json_array_size(j_camps);
	for(i = 0; i < size; i++) {
		j_camp = ast_json_array_get(j_camps, i);

		// check startable
		ret = is_startable_campgain(j_camp);
		if(ret == false) {
			continue;
		}

		// update campaign status
		ast_log(LOG_NOTICE, "Update campaign status to start. camp_uuid[%s], camp_name[%s]\n",
				ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
				ast_json_string_get(ast_json_object_get(j_camp, "name"))
				);
		ret = update_campaign_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_START);
		if(ret == false) {
			ast_log(LOG_ERROR, "Could not update campaign status to start. camp_uuid[%s], camp_name[%s]\n",
					ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
					ast_json_string_get(ast_json_object_get(j_camp, "name"))
					);
		}
	}
	ast_json_unref(j_camps);
	return;
}

/**
 * Check Stopping status campaign, and update to stop.
 * @param fd
 * @param event
 * @param arg
 */
static void cb_campaign_stopping(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg)
{
	struct ast_json* j_camps;
	struct ast_json* j_camp;
	int i;
	int size;
	int ret;

	ast_log(LOG_DEBUG, "cb_campaign_stopping.\n");

	j_camps = get_campaigns_by_status(E_CAMP_STOPPING);
	if(j_camps == NULL) {
		// Nothing.
		return;
	}

	size = ast_json_array_size(j_camps);
	for(i = 0; i < size; i++) {
		j_camp = ast_json_array_get(j_camps, i);

		// check stoppable
		ret = is_stoppable_campgain(j_camp);
		if(ret == false) {
			continue;
		}

		// update status to stop
		ast_log(LOG_NOTICE, "Update campaign status to stop. camp_uuid[%s], camp_name[%s]\n",
				ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
				ast_json_string_get(ast_json_object_get(j_camp, "name"))
				);
		ret = update_campaign_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOP);
		if(ret == false) {
			ast_log(LOG_ERROR, "Could not update campaign status to stop. camp_uuid[%s], camp_name[%s]\n",
				ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
				ast_json_string_get(ast_json_object_get(j_camp, "name"))
				);
		}
	}

	ast_json_unref(j_camps);

}

/**
 * Check Stopping status campaign, and update to stop.
 * @param fd
 * @param event
 * @param arg
 */
static void cb_campaign_stopping_force(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg)
{
	struct ast_json* j_camps;
	struct ast_json* j_camp;
	struct ao2_iterator iter;
	rb_dialing* dialing;
	const char* tmp_const;
	int i;
	int size;

	ast_log(LOG_DEBUG, "cb_campaign_stopping_force\n");

	j_camps = get_campaigns_by_status(E_CAMP_STOPPING_FORCE);
	if(j_camps == NULL) {
		// Nothing.
		return;
	}

	// find dialing info
	size = ast_json_array_size(j_camps);
	for(i = 0; i < size; i++) {
		j_camp = ast_json_array_get(j_camps, i);
		ast_log(LOG_DEBUG, "Force stop campaign info. camp_uuid[%s], camp_name[%s]\n",
				ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
				ast_json_string_get(ast_json_object_get(j_camp, "name"))
				);

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

			if(strcmp(tmp_const, ast_json_string_get(ast_json_object_get(j_camp, "uuid"))) != 0) {
				continue;
			}

			// hang up the channel
			ast_log(LOG_NOTICE, "Hangup channel. uuid[%s], channel[%s]\n",
					dialing->uuid,
					ast_json_string_get(ast_json_object_get(dialing->j_chan, "channel"))
					);
			ami_cmd_hangup(ast_json_string_get(ast_json_object_get(dialing->j_chan, "channel")), AST_CAUSE_NORMAL_CLEARING);
		}
		rb_dialing_iter_destroy(&iter);

		// update status to stop
		update_campaign_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOP);
	}
	ast_json_unref(j_camps);
}

static void cb_check_dialing_end(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg)
{
	struct ao2_iterator iter;
	rb_dialing* dialing;
	struct ast_json* j_tmp;
	int ret;

	ast_log(LOG_DEBUG, "cb_check_dialing_end\n");

	iter = rb_dialing_iter_init();
	while(1) {
		dialing = rb_dialing_iter_next(&iter);
		if(dialing == NULL) {
			break;
		}

		if(dialing->status != E_DIALING_HANGUP) {
			continue;
		}

		// create dl_list for update
		j_tmp = ast_json_pack("{s:s, s:i, s:O, s:O, s:O}",
				"uuid",				 ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dl_list_uuid")),
				"status",			   E_DL_IDLE,
				"dialing_uuid",		 ast_json_null(),
				"dialing_camp_uuid",	ast_json_null(),
				"dialing_plan_uuid",	ast_json_null()
				);
		ast_json_object_set(j_tmp, "res_hangup", ast_json_ref(ast_json_object_get(dialing->j_dialing, "res_hangup")));
		ast_json_object_set(j_tmp, "res_dial", ast_json_ref(ast_json_object_get(dialing->j_dialing, "res_dial")));
		if(j_tmp == NULL) {
			ast_log(LOG_ERROR, "Could not create update dl_list json. dl_list_uuid[%s], res_hangup[%lld], res_dial[%lld]\n",
					ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dl_list_uuid")),
					ast_json_integer_get(ast_json_object_get(dialing->j_dialing, "res_hangup")),
					ast_json_integer_get(ast_json_object_get(dialing->j_dialing, "res_dial"))
					);
		}

		// update dl_list
		ret = update_dl_list(j_tmp);
		ast_json_unref(j_tmp);
		if(ret == false) {
			ast_log(LOG_WARNING, "Could not update dialing result. dialing_uuid[%s], dl_list_uuid[%s]\n",
					dialing->uuid, ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dl_list_uuid")));
			continue;
		}

		// create result data
		j_tmp = create_json_for_dl_result(dialing);
		ast_log(LOG_DEBUG, "Check result value. dial_channel[%s], dial_addr[%s], dial_index[%lld], dial_trycnt[%lld], dial_timeout[%lld], dial_type[%lld], dial_exten[%s], res_dial[%lld], res_amd[%lld], res_amd_detail[%s], res_hangup[%lld], res_hangup_detail[%s]\n",

				// dial
				ast_json_string_get(ast_json_object_get(j_tmp, "dial_channel")),
				ast_json_string_get(ast_json_object_get(j_tmp, "dial_addr")),
				ast_json_integer_get(ast_json_object_get(j_tmp, "dial_index")),
				ast_json_integer_get(ast_json_object_get(j_tmp, "dial_trycnt")),
				ast_json_integer_get(ast_json_object_get(j_tmp, "dial_timeout")),
				ast_json_integer_get(ast_json_object_get(j_tmp, "dial_type")),
				ast_json_string_get(ast_json_object_get(j_tmp, "dial_exten")),

				// result
				ast_json_integer_get(ast_json_object_get(j_tmp, "res_dial")),
				ast_json_integer_get(ast_json_object_get(j_tmp, "res_amd")),
				ast_json_string_get(ast_json_object_get(j_tmp, "res_amd_detail")),
				ast_json_integer_get(ast_json_object_get(j_tmp, "res_hangup")),
				ast_json_string_get(ast_json_object_get(j_tmp, "res_hangup_detail"))
				);

//		db_insert("dl_result", j_tmp);
		ret = write_result_json(j_tmp);
		ast_json_unref(j_tmp);
		if(ret == false) {
			ast_log(LOG_ERROR, "Could not write result correctly.\n");
			continue;
		}

		rb_dialing_destory(dialing);
		ast_log(LOG_DEBUG, "Destroyed dialing info.\n");

	}
	ao2_iterator_destroy(&iter);

	return;
}

static void cb_check_dialing_error(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg)
{
	struct ao2_iterator iter;
	rb_dialing* dialing;
	struct ast_json* j_tmp;
	int ret;

	ast_log(LOG_DEBUG, "cb_check_dialing_error.\n");

	iter = rb_dialing_iter_init();
	while(1) {
		dialing = rb_dialing_iter_next(&iter);
		if(dialing == NULL) {
			break;
		}

		if(dialing->status != E_DIALING_ERROR) {
			continue;
		}

		// create dl_list for update
		j_tmp = ast_json_pack("{s:s, s:i, s:O, s:O, s:O}",
				"uuid",				 ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dl_list_uuid")),
				"status",			   E_DL_IDLE,
				"dialing_uuid",		 ast_json_null(),
				"dialing_camp_uuid",	ast_json_null(),
				"dialing_plan_uuid",	ast_json_null()
				);
		ast_json_object_set(j_tmp, "res_hangup", ast_json_ref(ast_json_object_get(dialing->j_dialing, "res_hangup")));
		ast_json_object_set(j_tmp, "res_dial", ast_json_ref(ast_json_object_get(dialing->j_dialing, "res_dial")));
		if(j_tmp == NULL) {
			ast_log(LOG_ERROR, "Could not create update dl_list json. dl_list_uuid[%s], res_hangup[%lld], res_dial[%lld]\n",
					ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dl_list_uuid")),
					ast_json_integer_get(ast_json_object_get(dialing->j_dialing, "res_hangup")),
					ast_json_integer_get(ast_json_object_get(dialing->j_dialing, "res_dial"))
					);
		}

		// update dl_list
		ret = update_dl_list(j_tmp);
		ast_json_unref(j_tmp);
		if(ret == false) {
			ast_log(LOG_WARNING, "Could not update dialing result. dialing_uuid[%s], dl_list_uuid[%s]\n",
					dialing->uuid, ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dl_list_uuid")));
			continue;
		}

		// create result data
		j_tmp = create_json_for_dl_result(dialing);
		ast_log(LOG_DEBUG, "Check result value. dial_channel[%s], dial_addr[%s], dial_index[%lld], dial_trycnt[%lld], dial_timeout[%lld], dial_type[%lld], dial_exten[%s], res_dial[%lld], res_amd[%lld], res_amd_detail[%s], res_hangup[%lld], res_hangup_detail[%s]\n",

				// dial
				ast_json_string_get(ast_json_object_get(j_tmp, "dial_channel")),
				ast_json_string_get(ast_json_object_get(j_tmp, "dial_addr")),
				ast_json_integer_get(ast_json_object_get(j_tmp, "dial_index")),
				ast_json_integer_get(ast_json_object_get(j_tmp, "dial_trycnt")),
				ast_json_integer_get(ast_json_object_get(j_tmp, "dial_timeout")),
				ast_json_integer_get(ast_json_object_get(j_tmp, "dial_type")),
				ast_json_string_get(ast_json_object_get(j_tmp, "dial_exten")),

				// result
				ast_json_integer_get(ast_json_object_get(j_tmp, "res_dial")),
				ast_json_integer_get(ast_json_object_get(j_tmp, "res_amd")),
				ast_json_string_get(ast_json_object_get(j_tmp, "res_amd_detail")),
				ast_json_integer_get(ast_json_object_get(j_tmp, "res_hangup")),
				ast_json_string_get(ast_json_object_get(j_tmp, "res_hangup_detail"))
				);

//		db_insert("dl_result", j_tmp);
		ret = write_result_json(j_tmp);
		ast_json_unref(j_tmp);

		rb_dialing_destory(dialing);
		ast_log(LOG_DEBUG, "Destroyed!\n");

	}
	ao2_iterator_destroy(&iter);

	return;
}


static void cb_check_campaign_end(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg)
{
	struct ast_json* j_camps;
	struct ast_json* j_camp;
	struct ast_json* j_plan;
	struct ast_json* j_dlma;
	int i;
	int size;
	int ret;

	ast_log(LOG_DEBUG, "cb_check_campaign_end\n");

	j_camps = get_campaigns_by_status(E_CAMP_START);
	size = ast_json_array_size(j_camps);
	for(i = 0; i < size; i++) {
		j_camp = ast_json_array_get(j_camps, i);
		if(j_camp == NULL) {
			continue;
		}

		j_plan = get_plan(ast_json_string_get(ast_json_object_get(j_camp, "plan")));
		if(j_plan == NULL) {
			update_campaign_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOPPING);
			continue;
		}

		j_dlma = get_dlma(ast_json_string_get(ast_json_object_get(j_camp, "dlma")));
		if(j_dlma == NULL) {
			update_campaign_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOPPING);
			ast_json_unref(j_plan);
			continue;
		}

		// check_more_list(j_dlma, j_plan)
		// check_dial
		ret = check_more_dl_list(j_dlma, j_plan);
		if(ret == false) {
			ast_log(LOG_NOTICE, "No more dial list. Stopping campaign. uuid[%s], name[%s]\n",
					ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
					ast_json_string_get(ast_json_object_get(j_camp, "name"))
					);
			update_campaign_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOPPING);
		}

		ast_json_unref(j_plan);
		ast_json_unref(j_dlma);
	}
	ast_json_unref(j_camps);
	return;
}

//static struct ast_json* get_queue_info(const char* uuid)
//{
//	char* sql;
//	struct ast_json* j_res;
//	db_res_t* db_res;
//
//	if(uuid == NULL) {
//		ast_log(LOG_WARNING, "Invalid input parameters.\n");
//		return NULL;
//	}
//
//	ast_asprintf(&sql, "select * from queue where uuid=\"%s\" and in_use=1;", uuid);
//
//	db_res = db_query(sql);
//	ast_free(sql);
//	if(db_res == NULL) {
//		ast_log(LOG_ERROR, "Could not get queue info. uuid[%s]\n", uuid);
//		return NULL;
//	}
//
//	j_res = db_get_record(db_res);
//	db_free(db_res);
//
//	return j_res;
//}

/**
 *
 * @param j_camp
 * @param j_plan
 */
static void dial_desktop(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma)
{
	return;
}

/**
 *
 * @param j_camp
 * @param j_plan
 */
static void dial_power(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma)
{
	return;
}

/**
 *  Make a call by predictive algorithms.
 *  Currently, just consider ready agent only.
 * @param j_camp	campaign info
 * @param j_plan	plan info
 * @param j_dlma	dial list master info
 */
static void dial_predictive(struct ast_json* j_camp, struct ast_json* j_plan, struct ast_json* j_dlma)
{
	int ret;
	struct ast_json* j_dl_list;
	struct ast_json* j_dial;
	struct ast_json* j_dl_update;
	struct ast_json* j_res;
	rb_dialing* dialing;
	char* tmp;
	const char* tmp_const;
	char* try_count_field;
	E_DIAL_TYPE dial_type;

	// get dl_list info to dial.
	j_dl_list = get_dl_available_predictive(j_dlma, j_plan);
	if(j_dl_list == NULL) {
		// No available list
//		ast_log(LOG_VERBOSE, "No more dialing list. stopping the campaign.\n");
//		update_campaign_info_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOPPING);
		return;
	}

	// check available outgoing call.
	ret = check_dial_avaiable_predictive(j_camp, j_plan, j_dlma);
	if(ret == -1) {
		// something was wrong. stop the campaign.
		update_campaign_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOPPING);
		ast_json_unref(j_dl_list);
		return;
	}
	else if(ret == 0) {
		// Too much calls already outgoing.
		ast_json_unref(j_dl_list);
		return;
	}

	// creating dialing info
	tmp_const = ast_json_string_get(ast_json_object_get(j_plan, "uuid"));
	j_dial = create_dial_info(j_plan, j_dl_list, E_DIAL_EXTEN, tmp_const, PLAN_CONTEXT);
	ast_json_unref(j_dl_list);
	if(j_dial == NULL) {
		ast_log(LOG_DEBUG, "Could not create dialing info.");
		return;
	}
	ast_log(LOG_NOTICE, "Originating. camp_uuid[%s], camp_name[%s], channel[%s], chan_id[%s], timeout[%s], dial_index[%lld], dial_trycnt[%lld], dial_type[%lld]\n",
			ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
			ast_json_string_get(ast_json_object_get(j_camp, "name")),
			ast_json_string_get(ast_json_object_get(j_dial, "dial_channel")),
			ast_json_string_get(ast_json_object_get(j_dial, "channelid")),
			ast_json_string_get(ast_json_object_get(j_dial, "timeout")),
			ast_json_integer_get(ast_json_object_get(j_dial, "dial_index")),
			ast_json_integer_get(ast_json_object_get(j_dial, "dial_trycnt")),
			ast_json_integer_get(ast_json_object_get(j_dial, "dial_type"))
			);

	// create rbtree
	dialing = rb_dialing_create(ast_json_string_get(ast_json_object_get(j_dial, "channelid")), j_camp, j_plan, j_dlma, j_dial);
	if(dialing == NULL) {
		ast_log(LOG_WARNING, "Could not create rbtree object.");
		ast_json_unref(j_dial);
		return;
	}

	// dial to customer
	dial_type = ast_json_integer_get(ast_json_object_get(j_dial, "dial_type"));
	switch(dial_type) {
		case E_DIAL_EXTEN: {
			j_res = ami_cmd_originate_to_exten(j_dial);
			ast_json_unref(j_dial);
		}
		break;

		case E_DIAL_APPL: {
			j_res = ami_cmd_originate_to_exten(j_dial);
			ast_json_unref(j_dial);
		}
		break;

		default: {
			ast_json_unref(j_dial);
			ast_log(LOG_ERROR, "Unsupported dialing type.");
			clear_dl_list_dialing(ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dl_list_uuid")));
			rb_dialing_destory(dialing);
			return;
		}
		break;
	}

	if(j_res == NULL) {
		ast_log(LOG_WARNING, "Originating has failed.");
		clear_dl_list_dialing(ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dl_list_uuid")));
		rb_dialing_destory(dialing);
		return;
	}

	tmp = ast_json_dump_string_format(j_res, 0);
	ast_log(LOG_DEBUG, "Check value. tmp[%s]\n", tmp);
	ast_json_free(tmp);
	ast_json_unref(j_res);

	// update dialing status
	rb_dialing_update_status(dialing, E_DIALING_ORIGINATE_REQUEST);

	// create update dl_list
	tmp = get_utc_timestamp();
	ast_asprintf(&try_count_field, "trycnt_%lld", ast_json_integer_get(ast_json_object_get(dialing->j_dialing, "dial_index")));

	j_dl_update = ast_json_pack("{s:s, s:I, s:i, s:s, s:s, s:s, s:s}",
			"uuid",				 ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dl_list_uuid")),
			try_count_field,		ast_json_integer_get(ast_json_object_get(dialing->j_dialing, "dial_trycnt")),
			"status",			   E_DL_DIALING,
			"dialing_uuid",		 dialing->uuid,
			"dialing_camp_uuid",	ast_json_string_get(ast_json_object_get(dialing->j_dialing, "camp_uuid")),
			"dialing_plan_uuid",	ast_json_string_get(ast_json_object_get(dialing->j_dialing, "plan_uuid")),
			"tm_last_dial",		 tmp
			);
	ast_free(tmp);
	ast_free(try_count_field);

	// dl update
	ret = update_dl_list(j_dl_update);
	ast_json_unref(j_dl_update);
	if(ret == false) {
		rb_dialing_destory(dialing);
		clear_dl_list_dialing(ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dl_list_uuid")));
		ast_log(LOG_ERROR, "Could not update dial list info.\n");
		return;
	}

	return;
}

/**
 *
 * @param j_camp
 * @param j_plan
 */
static void dial_robo(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma)
{
	return;
}

/**
 *  Redirect call to other dialplan.
 * @param j_camp	campaign info
 * @param j_plan	plan info
 * @param j_dlma	dial list master info
 */
static void dial_redirect(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma)
{
	return;
}




/**
 * Return dialing availability.
 * todo: need something more here.. currently, just compare dial numbers..
 * If can dialing returns true, if not returns false.
 * @param j_camp
 * @param j_plan
 * @return 1:OK, 0:NO, -1:ERROR
 */
static int check_dial_avaiable_predictive(
		struct ast_json* j_camp,
		struct ast_json* j_plan,
		struct ast_json* j_dlma
		)
{
	struct ast_json* j_tmp;
	int cnt_avail_chan;
	int cnt_current_dialing;
	double service_perf;
	int plan_service_level;
	int available;

	// get queue param info
	j_tmp = get_queue_param(ast_json_string_get(ast_json_object_get(j_plan, "queue_name")));
	if(j_tmp == NULL) {
		ast_log(LOG_ERROR, "Could not get queue_param info. plan_uuid[%s], plan_name[%s], queue_name[%s]\n",
				ast_json_string_get(ast_json_object_get(j_plan, "uuid")),
				ast_json_string_get(ast_json_object_get(j_plan, "name")),
				ast_json_string_get(ast_json_object_get(j_plan, "queue_name")));
		return -1;
	}
	service_perf = atof(ast_json_string_get(ast_json_object_get(j_tmp, "ServicelevelPerf")));
	if(service_perf == 0) {
		service_perf = 100;
	}
	ast_json_unref(j_tmp);

	// get queue summary info.
	j_tmp = get_queue_summary(ast_json_string_get(ast_json_object_get(j_plan, "queue_name")));
	if(j_tmp == NULL) {
		ast_log(LOG_ERROR, "Could not get queue_summary info. plan_uuid[%s], plan_name[%s], queue_name[%s]\n",
				ast_json_string_get(ast_json_object_get(j_plan, "uuid")),
				ast_json_string_get(ast_json_object_get(j_plan, "name")),
				ast_json_string_get(ast_json_object_get(j_plan, "queue_name")));
		return -1;
	}
	// log
	ast_log(LOG_DEBUG, "Queue summary status. queue[%s], loggedin[%s], available[%s], callers[%s], holdtime[%s], talktime[%s], longestholdtime[%s]\n",
			ast_json_string_get(ast_json_object_get(j_tmp, "Queue")),
			ast_json_string_get(ast_json_object_get(j_tmp, "LoggedIn")),
			ast_json_string_get(ast_json_object_get(j_tmp, "Available")),
			ast_json_string_get(ast_json_object_get(j_tmp, "Callers")),
			ast_json_string_get(ast_json_object_get(j_tmp, "HoldTime")),
			ast_json_string_get(ast_json_object_get(j_tmp, "TalkTime")),
			ast_json_string_get(ast_json_object_get(j_tmp, "LongestHoldTime"))
			);
	cnt_avail_chan = atoi(ast_json_string_get(ast_json_object_get(j_tmp, "Available")));
	ast_json_unref(j_tmp);

	// get current dialing count;
	cnt_current_dialing = rb_dialing_get_count_by_camp_uuid(ast_json_string_get(ast_json_object_get(j_camp, "uuid")));
	if(cnt_current_dialing == -1) {
		ast_log(LOG_ERROR, "Could not get current dialing count info. camp_uuid[%s]\n",
				ast_json_string_get(ast_json_object_get(j_camp, "uuid"))
				);
		return -1;
	}
//	cnt_current_dialing = get_current_dialing_dl_cnt(
//			ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
//			ast_json_string_get(ast_json_object_get(j_dlma, "dl_table"))
//			);
//	if(cnt_current_dialing == -1) {
//		ast_log(LOG_ERROR, "Could not get current dialing count info. camp_uuid[%s], dl_table[%s]\n",
//				ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
//				ast_json_string_get(ast_json_object_get(j_dlma, "dl_table"))
//				);
//		return -1;
//	}

	// get service level
	plan_service_level = ast_json_integer_get(ast_json_object_get(j_plan, "service_level"));
	if(plan_service_level < 0) {
		plan_service_level = 0;
	}

	// calculate call
	// wait_agents * ((queue.performance + plan.performance) / 100) > dialing_calls
	// (service_perf + plan_service_level) shouldn't be 0.
	available = cnt_avail_chan * ((service_perf + plan_service_level) / 100);

	// compare
	if(available <= cnt_current_dialing) {
		return 0;
	}
	return 1;
}

struct ast_json* get_queue_summary(const char* name)
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
	ast_json_unref(j_ami_res);
	if(j_queue == NULL) {
		ast_log(LOG_NOTICE, "Could not get queue summary. name[%s]\n", name);
		return NULL;
	}

	return j_queue;
}

/**
 * Get Queue param only.
 * @param name
 * @return
 */
struct ast_json* get_queue_param(const char* name)
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
	ast_json_unref(j_ami_res);
	if(j_param == NULL) {
		ast_log(LOG_NOTICE, "Could not get queue param. name[%s]\n", name);
		return NULL;
	}

	return j_param;
}

static bool write_result_json(struct ast_json* j_res)
{
	FILE* fp;
	struct ast_json* j_general;
	const char* filename;
	char* tmp;

	if(j_res == NULL) {
		ast_log(LOG_ERROR, "Wrong input parameter.\n");
		return false;
	}

	// open json file
	j_general = ast_json_object_get(g_app->j_conf, "general");
	filename = ast_json_string_get(ast_json_object_get(j_general, "result_filename"));
	if(filename == NULL) {
		ast_log(LOG_ERROR, "Could not get option value. option[%s]\n", "result_filename");
		return false;
	}

	// open file
	fp = fopen(filename, "a");
	if(fp == NULL) {
		ast_log(LOG_ERROR, "Could not open result file. filename[%s], err[%s]\n",
				filename, strerror(errno)
				);
		return false;
	}

	tmp = ast_json_dump_string_format(j_res, AST_JSON_COMPACT);
	if(tmp == NULL) {
		ast_log(LOG_ERROR, "Could not get result string to the file. filename[%s], err[%s]\n",
				filename, strerror(errno)
				);
		fclose(fp);
		return false;
	}

	fprintf(fp, "%s\n", tmp);
	ast_json_free(tmp);
	fclose(fp);

	ast_log(LOG_VERBOSE, "Result write succeed.\n");

	return true;
}
