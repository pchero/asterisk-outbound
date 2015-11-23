/*
 * event_handler.c
 *
 *  Created on: Nov 8, 2015
 *      Author: pchero
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

#include "db_handler.h"
#include "ami_handler.h"
#include "event_handler.h"
#include "dialing_handler.h"

#define TEMP_FILENAME "/tmp/asterisk_outbound_tmp.txt"


struct event_base*  g_base = NULL;

static int init_outbound(void);

static void cb_campaign_start(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);
static void cb_campaign_stopping(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);
static void cb_campaign_stopping_force(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);
static void cb_check_dialing_end(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);

static struct ast_json* get_campaign_info_for_dialing(void);
static struct ast_json* get_plan_info(const char* uuid);
static struct ast_json* get_dl_master_info(const char* uuid);
static int update_campaign_info_status(const char* uuid, CAMP_STATUS_T status);

static void dial_desktop(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma);
static void dial_power(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma);
static void dial_predictive(struct ast_json* j_camp, struct ast_json* j_plan, struct ast_json* j_dlma);
static void dial_robo(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma);
static void dial_redirect(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma);

//static struct ast_json* get_dl_dial_available(const struct ast_json* j_dlma, const char* dial_mode);
static struct ast_json* get_dl_available_predictive(struct ast_json* j_dlma, struct ast_json* j_plan);


struct ast_json* get_queue_summary(const char* name);
int get_current_dialing_cnt(const char* camp_uuid, const char* dl_table);
static int get_dial_num_point(struct ast_json* j_dl_list, struct ast_json* j_plan);
static char* get_dial_number(struct ast_json* j_dlist, const int cnt);
static char* create_chan_addr_for_dial(struct ast_json* j_plan, struct ast_json* j_dl_list, int dial_num_point);

static char* gen_uuid(void);
static int get_dial_try_cnt(struct ast_json* j_dl_list, int dial_num_point);
static int update_dl_list(const char* table, struct ast_json* j_dlinfo);

static struct ast_json* create_campaign_result(rb_dialing* dialing);
static struct ast_json* get_campaigns_info_by_status(CAMP_STATUS_T status);

// todo
static int check_dial_avaiable_predictive(struct ast_json* j_camp, struct ast_json* j_plan, struct ast_json* j_dlma);
struct ast_json* create_dialing_info(struct ast_json* j_plan, struct ast_json* j_dl_list);

int run_outbound(void)
{
    int ret;
    struct event* ev;
    struct timeval tm_fast = {0, 2000};    // 20 ms
//    struct timeval tm_fast = {3, 20000};    // 2 sec
//    struct timeval tm_slow = {0, 500000};   // 500 ms

    // init libevent
    ret = init_outbound();
    if(ret == false) {
        ast_log(LOG_ERROR, "Could not initiate outbound.\n");
        return false;
    }

    // check start.
    ev = event_new(g_base, -1, EV_TIMEOUT | EV_PERSIST, cb_campaign_start, NULL);
    event_add(ev, &tm_fast);

    // check stopping.
    ev = event_new(g_base, -1, EV_TIMEOUT | EV_PERSIST, cb_campaign_stopping, NULL);
    event_add(ev, &tm_fast);

    // check force stopping
    ev = event_new(g_base, -1, EV_TIMEOUT | EV_PERSIST, cb_campaign_stopping_force, NULL);
    event_add(ev, &tm_fast);



    ev = event_new(g_base, -1, EV_TIMEOUT | EV_PERSIST, cb_check_dialing_end, NULL);
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
    const char* dial_mode;

//    ast_log(LOG_DEBUG, "cb_campagin start\n");

    j_camp = get_campaign_info_for_dialing();
    if(j_camp == NULL) {
        // Nothing.
        return;
    }
    ast_log(LOG_DEBUG, "Get campaign info. camp_uuid[%s], camp_name[%s]\n",
            ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
            ast_json_string_get(ast_json_object_get(j_camp, "name"))
            );

    // get plan
    j_plan = get_plan_info(ast_json_string_get(ast_json_object_get(j_camp, "plan")));
    if(j_plan == NULL) {
        ast_log(LOG_WARNING, "Could not get plan info. Stopping campaign camp[%s], plan[%s]\n",
                ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
                ast_json_string_get(ast_json_object_get(j_camp, "plan"))
                );
        update_campaign_info_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOPPING);
        ast_json_unref(j_camp);
        return;
    }
    ast_log(LOG_DEBUG, "Get plan info. camp_uuid[%s], camp_name[%s]\n",
            ast_json_string_get(ast_json_object_get(j_plan, "uuid")),
            ast_json_string_get(ast_json_object_get(j_plan, "name"))
            );

    // get dl_master_info
    j_dlma = get_dl_master_info(ast_json_string_get(ast_json_object_get(j_camp, "dlma")));
    if(j_dlma == NULL)
    {
        ast_log(LOG_ERROR, "Could not find dial list master info. Stopping campaign. camp[%s], dlma[%s]\n",
                ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
                ast_json_string_get(ast_json_object_get(j_camp, "dlma"))
                );
        update_campaign_info_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOPPING);
        ast_json_unref(j_camp);
        ast_json_unref(j_plan);
        return;
    }
    ast_log(LOG_DEBUG, "Get dlma info. dlma_uuid[%s], dlma_name[%s]\n",
            ast_json_string_get(ast_json_object_get(j_dlma, "uuid")),
            ast_json_string_get(ast_json_object_get(j_dlma, "name"))
            );

    // get dial_mode
    dial_mode = ast_json_string_get(ast_json_object_get(j_plan, "dial_mode"));
    if(dial_mode == NULL) {
        ast_log(LOG_ERROR, "Plan has no dial_mode. Stopping campaign. camp[%s], plan[%s]",
                ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
                ast_json_string_get(ast_json_object_get(j_camp, "plan"))
                );

        update_campaign_info_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOPPING);
        ast_json_unref(j_camp);
        ast_json_unref(j_plan);
        ast_json_unref(j_dlma);
        return;
    }

    if(strcmp(dial_mode, "desktop") == 0) {
        dial_desktop(j_camp, j_plan, j_dlma);
    }
    else if(strcmp(dial_mode, "power") == 0) {
        dial_power(j_camp, j_plan, j_dlma);
    }
    else if(strcmp(dial_mode, "predictive") == 0) {
        dial_predictive(j_camp, j_plan, j_dlma);
    }
    else if(strcmp(dial_mode, "robo") == 0) {
        dial_robo(j_camp, j_plan, j_dlma);
    }
    else if(strcmp(dial_mode, "redirect") == 0) {
        dial_redirect(j_camp, j_plan, j_dlma);
    }
    else {
        ast_log(LOG_ERROR, "No match dial_mode. dial_mode[%s]\n", dial_mode);
    }

    // release
    ast_json_unref(j_camp);
    ast_json_unref(j_plan);
    ast_json_unref(j_dlma);

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
    struct ao2_iterator iter;
    rb_dialing* dialing;
    const char* tmp_const;
    int i;
    int size;
    int flg_dialing;

    j_camps = get_campaigns_info_by_status(E_CAMP_STOPPING);
    if(j_camps == NULL) {
        // Nothing.
        return;
    }

    size = ast_json_array_size(j_camps);
    for(i = 0; i < size; i++) {
        j_camp = ast_json_array_get(j_camps, i);

        flg_dialing = false;
        iter = rb_dialing_iter_init();
        while((dialing = ao2_iterator_next(&iter))) {
            ao2_ref(dialing, -1);

            tmp_const = ast_json_string_get(ast_json_object_get(dialing->j_camp, "uuid"));
            if(tmp_const == NULL) {
                continue;
            }

            if(strcmp(tmp_const, ast_json_string_get(ast_json_object_get(j_camp, "uuid"))) == 0) {
                flg_dialing = true;
                continue;
            }
        }
        ao2_iterator_destroy(&iter);

        if(flg_dialing == false) {
            // update status to stop
            ast_log(LOG_DEBUG, "Stop campaign info. camp_uuid[%s], camp_name[%s]\n",
                    ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
                    ast_json_string_get(ast_json_object_get(j_camp, "name"))
                    );
            update_campaign_info_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOP);
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

    j_camps = get_campaigns_info_by_status(E_CAMP_STOPPING_FORCE);
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
        while((dialing = ao2_iterator_next(&iter))) {
            ao2_ref(dialing, -1);

            tmp_const = ast_json_string_get(ast_json_object_get(dialing->j_camp, "uuid"));
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
        ao2_iterator_destroy(&iter);

        // update status to stop
        update_campaign_info_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOP);
    }
    ast_json_unref(j_camps);
}

static void cb_check_dialing_end(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg)
{
    struct ao2_iterator iter;
    rb_dialing* dialing;
    const char* tmp_const;
    char* tmp;
    struct ast_json* j_res;

    iter = rb_dialing_iter_init();
    while((dialing = ao2_iterator_next(&iter))) {
        ao2_ref(dialing, -1);

        tmp_const = ast_json_string_get(ast_json_object_get(dialing->j_chan, "tm_hangup"));
        if(tmp_const == NULL) {
            continue;
        }

        // create result data
        j_res = create_campaign_result(dialing);
        db_insert("campaign_result", j_res);
        ast_json_unref(j_res);

        tmp = ast_json_dump_string_format(dialing->j_dl, 0);
        ast_log(LOG_DEBUG, "Result dl. result[%s]\n", tmp);
        ast_json_free(tmp);

        tmp = ast_json_dump_string_format(dialing->j_chan, 0);
        ast_log(LOG_DEBUG, "Result chan. result[%s]\n", tmp);
        ast_json_free(tmp);

        tmp = ast_json_dump_string_format(dialing->j_queues, 0);
        ast_log(LOG_DEBUG, "Result queues. result[%s]\n", tmp);
        ast_json_free(tmp);

        tmp = ast_json_dump_string_format(dialing->j_agents, 0);
        ast_log(LOG_DEBUG, "Result agents. result[%s]\n", tmp);
        ast_json_free(tmp);

        rb_dialing_destory(dialing);
        ast_log(LOG_DEBUG, "Destroied!\n");

    }
    ao2_iterator_destroy(&iter);

    return;
}


/**
 * Get campaign for dialing.
 * @return
 */
static struct ast_json* get_campaign_info_for_dialing(void)
{
    struct ast_json* j_res;
    db_res_t* db_res;
    char* sql;

    // get "start" status campaign only.
    ast_asprintf(&sql, "select * from campaign where status = %d order by rand() limit 1;",
            E_CAMP_RUN
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
 * Get campaign for dialing.
 * @return
 */
static struct ast_json* get_campaigns_info_by_status(CAMP_STATUS_T status)
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
 * Get plan record info.
 * @param uuid
 * @return
 */
static struct ast_json* get_plan_info(const char* uuid)
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

/**
 *
 * @param uuid
 * @return
 */
struct ast_json* get_dl_master_info(const char* uuid)
{
    char* sql;
    struct ast_json* j_res;
    db_res_t* db_res;

    if(uuid == NULL) {
        ast_log(LOG_WARNING, "Invalid input paramters.\n");
        return NULL;
    }

    ast_asprintf(&sql, "select * from dial_list_ma where uuid = \"%s\";", uuid);

    db_res = db_query(sql);
    ast_free(sql);
    if(db_res == NULL) {
        ast_log(LOG_ERROR, "Could not get dial_list_ma info. uuid[%s]\n", uuid);
        return NULL;
    }

    j_res = db_get_record(db_res);
    db_free(db_res);

    return j_res;
}

/**
 *
 * @param uuid
 * @return
 */
struct ast_json* get_dl_master_info_all(void)
{
    char* sql;
    struct ast_json* j_res;
    struct ast_json* j_tmp;
    db_res_t* db_res;

    ast_asprintf(&sql, "%s", "select * from dial_list_ma;");

    db_res = db_query(sql);
    ast_free(sql);
    if(db_res == NULL) {
        ast_log(LOG_ERROR, "Could not get dial_list_ma info.\n");
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
static int update_campaign_info_status(const char* uuid, CAMP_STATUS_T status)
{
    char* sql;
    int ret;
    char* tmp_status;

    if(uuid == NULL) {
        ast_log(LOG_WARNING, "Invalid input parameters.\n");
        return false;
    }
    ast_log(LOG_NOTICE, "Update campaign status. uuid[%s], status[%d]\n", uuid, status);

    if(status == E_CAMP_RUN) tmp_status = "run";
    else if(status == E_CAMP_STOP) tmp_status = "stop";
    else if(status == E_CAMP_PAUSE) tmp_status = "pause";
    else if(status == E_CAMP_RUNNING) tmp_status = "running";
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
 * @param j_camp    campaign info
 * @param j_plan    plan info
 * @param j_dlma    dial list master info
 */
static void dial_predictive(struct ast_json* j_camp, struct ast_json* j_plan, struct ast_json* j_dlma)
{
    int ret;
    struct ast_json* j_dl_list;
    struct ast_json* j_dialing;
    struct ast_json* j_dl_update;
    struct ast_json* j_res;
    rb_dialing* dialing;

    // validate plan
    if(ast_json_string_get(ast_json_object_get(j_plan, "trunk_name")) == NULL) {
        ast_log(LOG_WARNING, "Could not find trunk name. Stopping campaign.\n");
        update_campaign_info_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOPPING);
        return;
    }

    // get dl_list info to dial.
    j_dl_list = get_dl_available_predictive(j_dlma, j_plan);
    if(j_dl_list == NULL) {
        // No available list
        ast_log(LOG_VERBOSE, "No more dialing list. stopping the campaign.\n");
        update_campaign_info_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOPPING);
        return;
    }

    // check available outgoing call.
    ret = check_dial_avaiable_predictive(j_camp, j_plan, j_dlma);
    if(ret == -1) {
        // something was wrong. stop the campaign.
        update_campaign_info_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOPPING);
        ast_json_unref(j_dl_list);
        return;
    }
    else if(ret == 0) {
        // Too much calls already outgoing.
        ast_json_unref(j_dl_list);
        return;
    }

    // creating dialing info
    j_dialing = create_dialing_info(j_plan, j_dl_list);
    ast_json_unref(j_dl_list);
    if(j_dialing == NULL) {
        ast_log(LOG_DEBUG, "Could not create dialing info.");
        return;
    }
    ast_log(LOG_NOTICE, "Originating. camp_uuid[%s], camp_name[%s], channel[%s], chan_id[%s], timeout[%s]\n",
            ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
            ast_json_string_get(ast_json_object_get(j_camp, "name")),
            ast_json_string_get(ast_json_object_get(j_dialing, "dial_addr")),
            ast_json_string_get(ast_json_object_get(j_dialing, "channelid")),
            ast_json_string_get(ast_json_object_get(j_dialing, "timeout"))
            );

    // create rbtree
    dialing = rb_dialing_create(j_camp, j_plan, j_dlma, j_dialing);
    ast_json_unref(j_dialing);
    if(dialing == NULL) {
        ast_log(LOG_WARNING, "Could not create rbtree object.");
        return;
    }

//    if(dialing != NULL) {
//        ast_log(LOG_DEBUG, "Created dialing. uuid[%s]\n", dialing->uuid);
//        ao2_find(g_rb_dialings, "021a26ec-1201-4a36-a03a-3795773b236c", OBJ_KEY);
//        dialing = rb_dialing_find_uuid("021a26ec-1201-4a36-a03a-3795773b236c");
//        if(dialing != NULL) ast_log(LOG_DEBUG, "RB find. uuid[%s]\n", dialing->uuid);
//        ao2_ref(dialing, -1);
//    }

//    struct ast_json* j_tmp = rb_channel_search_chan(ast_json_string_get(ast_json_object_get(j_dialing, "channelid")));
//    ast_log(LOG_DEBUG, "Found info. dl_uuid[%s]\n", ast_json_string_get(ast_json_object_get(j_tmp, "uuid_dl")));

    // dial to customer
//    j_res = ami_cmd_originate_to_queue(j_dialing);
    j_res = ami_cmd_originate_to_queue(dialing->j_dl);
    if(j_res == NULL) {
        ast_log(LOG_WARNING, "Originating has failed.");
//        ast_json_unref(j_dialing);
        ao2_ref(dialing, -1);
        return;
    }
    char* tmp = ast_json_dump_string_format(j_res, 0);
    ast_log(LOG_DEBUG, "Check value. tmp[%s]\n", tmp);
    ast_json_free(tmp);
    ast_json_unref(j_res);

    // create update dl_list
//    j_dl_update = ast_json_pack("{s:i, s:s, s:i, s:s, s:s, s:s}",
//            "status",                   E_DL_DIALING,
//            "uuid",                     ast_json_string_get(ast_json_object_get(j_dialing, "uuid_dl")),
//            ast_json_string_get(ast_json_object_get(j_dialing, "trycount_field")), ast_json_integer_get(ast_json_object_get(j_dialing, "dial_trycnt")) + 1,
//            "dialing_camp_uuid",        ast_json_string_get(ast_json_object_get(j_dialing, "uuid_camp")),
//            "dialing_chan_unique_id",   ast_json_string_get(ast_json_object_get(j_dialing, "channelid")),
//            "tm_last_dial",             ast_json_string_get(ast_json_object_get(j_dialing, "tm_dial"))
//            );
//    if(j_dl_update == NULL) {
//        ast_log(LOG_ERROR, "Could not create dl update info json.");
//        ast_json_unref(j_dialing);
//        return;
//    }
    j_dl_update = ast_json_pack("{s:i, s:s, s:i, s:s, s:s, s:s}",
            "status",                   E_DL_DIALING,
            "uuid",                     ast_json_string_get(ast_json_object_get(dialing->j_dl, "uuid")),
            ast_json_string_get(ast_json_object_get(dialing->j_dl, "trycount_field")), ast_json_integer_get(ast_json_object_get(dialing->j_dl, "dial_trycnt")) + 1,
            "dialing_camp_uuid",        ast_json_string_get(ast_json_object_get(dialing->j_camp, "uuid")),
            "dialing_chan_unique_id",   ast_json_string_get(ast_json_object_get(dialing->j_dl, "channelid")),
            "tm_last_dial",             ast_json_string_get(ast_json_object_get(dialing->j_dl, "tm_originate_request"))
            );
    if(j_dl_update == NULL) {
        ast_log(LOG_ERROR, "Could not create dl update info json.\n");
        rb_dialing_destory(dialing);
        return;
    }

    // update dl_list
    ret = update_dl_list(ast_json_string_get(ast_json_object_get(j_dlma, "dl_table")), j_dl_update);
    ast_json_unref(j_dl_update);
    if(ret == false) {
//        ast_json_unref(j_dialing);
        ao2_ref(dialing, -1);
        ast_log(LOG_ERROR, "Could not update dial list info.");
        return;
    }

    // add bsd tree
//
//    // insert dialing
//    ast_log(LOG_NOTICE, "Insert new dialing. chan_unique_id[%s]", ast_json_string_get(ast_json_object_get(j_dialing, "chan_unique_id")));
//    ret = memdb_insert("dialing", j_dialing);
//    ast_json_unref(j_dialing);
//    if(ret == false)
//    {
//        ast_log(LOG_ERROR, "Could not insert dialing info.");
//        return;
//    }

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
 * @param j_camp    campaign info
 * @param j_plan    plan info
 * @param j_dlma    dial list master info
 */
static void dial_redirect(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma)
{
    return;
}

/**
 * Get dl_list from database.
 * @param j_dlma
 * @param j_plan
 * @return
 */
static struct ast_json* get_dl_available_predictive(struct ast_json* j_dlma, struct ast_json* j_plan)
{
    char* sql;
    db_res_t* db_res;
    struct ast_json* j_res;

    ast_asprintf(&sql, "select "
            "*, "
            "trycnt_1 + trycnt_2 + trycnt_3 + trycnt_4 + trycnt_5 + trycnt_6 + trycnt_7 + trycnt_8 as trycnt, "
            "case when number_1 is null then 0 when trycnt_1 < %d then 1 else 0 end as num_1, "
            "case when number_2 is null then 0 when trycnt_2 < %d then 1 else 0 end as num_2, "
            "case when number_3 is null then 0 when trycnt_3 < %d then 1 else 0 end as num_3, "
            "case when number_4 is null then 0 when trycnt_4 < %d then 1 else 0 end as num_4, "
            "case when number_5 is null then 0 when trycnt_5 < %d then 1 else 0 end as num_5, "
            "case when number_6 is null then 0 when trycnt_6 < %d then 1 else 0 end as num_6, "
            "case when number_7 is null then 0 when trycnt_7 < %d then 1 else 0 end as num_7, "
            "case when number_8 is null then 0 when trycnt_8 < %d then 1 else 0 end as num_8 "
            "from %s "
            "having "
            "status = %d "
            "and num_1 + num_2 + num_3 + num_4 + num_5 + num_6 + num_7 + num_8 > 0 "
            "order by trycnt asc "
            "limit 1"
            ";",
            (int)ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_1")),
            (int)ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_2")),
            (int)ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_3")),
            (int)ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_4")),
            (int)ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_5")),
            (int)ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_6")),
            (int)ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_7")),
            (int)ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_8")),
            ast_json_string_get(ast_json_object_get(j_dlma, "dl_table")),
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

/**
 * Return dialing availability.
 * todo: need something more here.. currently, just compare dial numbers..
 * If can dialing returns true, if not returns false.
 * @param j_camp
 * @param j_plan
 * @return 1:OK, 0:NO, -1:ERROR
 */
static int check_dial_avaiable_predictive(struct ast_json* j_camp, struct ast_json* j_plan, struct ast_json* j_dlma)
{
    char* sql;
    struct ast_json* j_queue;
    int cnt_avail_chan;
    int cnt_current_dialing;

    // get queue summary info.
    j_queue = get_queue_summary(ast_json_string_get(ast_json_object_get(j_plan, "queue_name")));
    if(j_queue == NULL) {
        ast_log(LOG_ERROR, "Could not get queue_summary info. plan_uuid[%s], plan_name[%s], queue_name[%s]\n",
                ast_json_string_get(ast_json_object_get(j_plan, "uuid")),
                ast_json_string_get(ast_json_object_get(j_plan, "name")),
                ast_json_string_get(ast_json_object_get(j_plan, "queue_name")));
        return -1;
    }
    // log
    ast_log(LOG_DEBUG, "Queue summary status. queue[%s], loggedin[%s], available[%s], callers[%s], holdtime[%s], talktime[%s], longestholdtime[%s]\n",
            ast_json_string_get(ast_json_object_get(j_queue, "Queue")),
            ast_json_string_get(ast_json_object_get(j_queue, "LoggedIn")),
            ast_json_string_get(ast_json_object_get(j_queue, "Available")),
            ast_json_string_get(ast_json_object_get(j_queue, "Callers")),
            ast_json_string_get(ast_json_object_get(j_queue, "HoldTime")),
            ast_json_string_get(ast_json_object_get(j_queue, "TalkTime")),
            ast_json_string_get(ast_json_object_get(j_queue, "LongestHoldTime"))
            );
    cnt_avail_chan = atoi(ast_json_string_get(ast_json_object_get(j_queue, "Available")));
    ast_json_unref(j_queue);

    // get current dialing count;
    cnt_current_dialing = get_current_dialing_cnt(
            ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
            ast_json_string_get(ast_json_object_get(j_dlma, "dl_table"))
            );
    if(cnt_current_dialing == -1) {
        ast_log(LOG_ERROR, "Could not get current dialing count info. camp_uuid[%s], dl_table[%s]\n",
                ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
                ast_json_string_get(ast_json_object_get(j_dlma, "dl_table"))
                );
        return -1;
    }

    // get count of currently dailings.
    ast_asprintf(&sql, "select count(*) from %s where dialing_camp_uuid = \"%s\" and status = \"%s\";",
            ast_json_string_get(ast_json_object_get(j_dlma, "dl_table")),
            ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
            "dialing"
            );

    // compare
    if(cnt_avail_chan <= cnt_current_dialing) {
        return 0;
    }
    return 1;
}

/**
 * Create dialing json object
 * @param j_camp
 * @param j_plan
 * @param j_dlma
 * @param j_dl_list
 * @return
 */
struct ast_json* create_dialing_info(
        struct ast_json* j_plan,
        struct ast_json* j_dl_list
        )
{
    struct ast_json* j_dial;
    char* dial_addr;
    int dial_num_point;
    char tmp_timeout[10];
    char tmp_dial_num_point[10];
    char* channel_id;
    char* other_channel_id;
    char* dial_try_count;
    int dial_count;

    // get dial number point
    dial_num_point = get_dial_num_point(j_dl_list, j_plan);
    if(dial_num_point < 0) {
        ast_log(LOG_ERROR, "Could not find correct number count.\n");
        return NULL;
    }
    sprintf(tmp_dial_num_point, "%d", dial_num_point);

    // get dial count
    dial_count = get_dial_try_cnt(j_dl_list, dial_num_point);
    if(dial_count == -1) {
        ast_log(LOG_ERROR, "Could not get correct dial count number.\n");
        return NULL;
    }

    // create destination channel address.
    dial_addr = create_chan_addr_for_dial(j_plan, j_dl_list, dial_num_point);
    if(dial_addr == NULL) {
        ast_log(LOG_ERROR, "Could not get correct channel address.\n");
        return NULL;
    }

    ast_asprintf(&dial_try_count, "trycnt_%d", dial_num_point);
    sprintf(tmp_timeout, "%ld", ast_json_integer_get(ast_json_object_get(j_plan, "dial_timeout")));
    channel_id = gen_uuid();
    other_channel_id = gen_uuid();

    j_dial = ast_json_pack("{"
            "s:s, "
            "s:s, s:s, s:s, s:s, s:s, "
            "s:s, s:i, s:s"
            "}",
            // identity info
            "uuid",      ast_json_string_get(ast_json_object_get(j_dl_list, "uuid")),

            // channel set
            "dial_addr",    dial_addr,      ///< Destination
            "data",         ast_json_string_get(ast_json_object_get(j_plan, "queue_name")),        ///< Queue name
            "timeout",      tmp_timeout,
            "channelid",        channel_id, ///< Channel unique ID
            "otherchannelid",   other_channel_id,    ///< Other channel unique id.
            //            "Variable",     ///< todo: More set dial info
            //            "Account",      ///< ????.

            // other info
            "dial_num_point",   tmp_dial_num_point,
            "dial_trycnt",      dial_count,
            "trycount_field",   dial_try_count
            );
    // caller id
    if(ast_json_string_get(ast_json_object_get(j_plan, "caller_id")) != NULL)   ast_json_object_set(j_dial, "callerid", ast_json_ref(ast_json_object_get(j_plan, "caller_id")));


    ast_free(dial_addr);
    ast_free(channel_id);
    ast_free(other_channel_id);
    ast_free(dial_try_count);

    return j_dial;
}


/**
 * return utc time.
 * YYYY-MM-DDTHH:mm:ssZ
 * @return
 */
char* get_utc_timestamp(void)
{
    char    timestr[128];
    char*   res;
    struct  timespec timeptr;
    time_t  tt;
    struct tm *t;

    clock_gettime(CLOCK_REALTIME, &timeptr);
    tt = (time_t)timeptr.tv_sec;
    t = gmtime(&tt);

    strftime(timestr, sizeof(timestr), "%Y-%m-%dT%H:%M:%S", t);
    ast_asprintf(&res, "%s.%ldZ", timestr, timeptr.tv_nsec);

    return res;
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
 * Return dialing count currently.
 * @param camp_uuid
 * @param dl_table
 * @return
 */
int get_current_dialing_cnt(const char* camp_uuid, const char* dl_table)
{
    char* sql;
    db_res_t* db_res;
    struct ast_json* j_tmp;
    int ret;

    if((camp_uuid == NULL) || (dl_table == NULL)) {
        ast_log(LOG_ERROR, "Invalid input parameters.\n");
        return -1;
    }

    ast_asprintf(&sql, "select count(*) from %s where dialing_camp_uuid = \"%s\" and status = \"%s\";",
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
    ast_json_unref(j_tmp);

    return ret;
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

    if(dial_num_point < 0) {
        ast_log(LOG_WARNING, "Wrong dial number point.\n");
        return NULL;
    }

    if(ast_json_string_get(ast_json_object_get(j_plan, "trunk_name")) == NULL) {
        ast_log(LOG_WARNING, "Could not get trunk_name info.\n");
        return NULL;
    }

    // get dial number
    dest_addr = get_dial_number(j_dl_list, dial_num_point);
    if(dest_addr == NULL) {
        ast_log(LOG_WARNING, "Could not get destination address.\n");
        return NULL;
    }

    // create dial addr
    ast_asprintf(&chan_addr, "SIP/%s@%s", dest_addr, ast_json_string_get(ast_json_object_get(j_plan, "trunk_name")));
    ast_log(LOG_DEBUG, "Created dialing channel address. chan_addr[%s].\n", chan_addr);
    ast_free(dest_addr);

    return chan_addr;
}

/**
 * Get dial number index
 * @param j_dl_list
 * @param j_plan
 * @return
 */
static int get_dial_num_point(struct ast_json* j_dl_list, struct ast_json* j_plan)
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

static int get_dial_try_cnt(struct ast_json* j_dl_list, int dial_num_point)
{
    int cur_trycnt;
    char* tmp;

    ast_asprintf(&tmp, "trycnt_%d", dial_num_point);
    if(ast_json_object_get(j_dl_list, tmp) == NULL) {
        return -1;
    }

    cur_trycnt = ast_json_integer_get(ast_json_object_get(j_dl_list, tmp));
    ast_free(tmp);

    return cur_trycnt;
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
 * Generate uuid.
 * Return value should be free after used.
 * @param prefix
 * @return
 */
static char* gen_uuid(void)
{
    char tmp[AST_UUID_STR_LEN];
    char* res;

    ast_uuid_generate_str(tmp, sizeof(tmp));
    res = ast_strdup(tmp);

    return res;
}

/**
 * Update dl list info.
 * @param j_dlinfo
 * @return
 */
static int update_dl_list(const char* table, struct ast_json* j_dlinfo)
{
    char* sql;
    int ret;
    char* tmp;

    tmp = db_get_update_str(j_dlinfo);
    if(tmp == NULL) {
        ast_log(LOG_ERROR, "Could not get update sql.\n");
        return false;
    }

    ast_asprintf(&sql, "update %s set %s where uuid = \"%s\";\n",
            table, tmp, ast_json_string_get(ast_json_object_get(j_dlinfo, "uuid"))
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

static struct ast_json* create_campaign_result(rb_dialing* dialing)
{
    struct ast_json* j_res;
    char* tmp;

    ast_log(LOG_DEBUG, "Check value. dialing_uuid[%s], camp_uuid[%s], plan_uuid[%s], dlma_uuid[%s], dl_uuid[%s]\n",
            dialing->uuid,
            ast_json_string_get(ast_json_object_get(dialing->j_camp, "uuid")),
            ast_json_string_get(ast_json_object_get(dialing->j_plan, "uuid")),
            ast_json_string_get(ast_json_object_get(dialing->j_dlma, "uuid")),
            ast_json_string_get(ast_json_object_get(dialing->j_dl, "uuid"))
            );
    j_res = ast_json_pack("{s:s, s:s, s:s, s:s, s:s}",
            "dialing_uuid", dialing->uuid,
            "camp_uuid",    ast_json_string_get(ast_json_object_get(dialing->j_camp, "uuid")),
            "plan_uuid",    ast_json_string_get(ast_json_object_get(dialing->j_plan, "uuid")),
            "dlma_uuid",    ast_json_string_get(ast_json_object_get(dialing->j_dlma, "uuid")),
            "dl_uuid",      ast_json_string_get(ast_json_object_get(dialing->j_dl, "uuid"))
            );

    // info_camp
    tmp = ast_json_dump_string_format(dialing->j_camp, 0);
    ast_json_object_set(j_res, "info_camp", ast_json_string_create(tmp));
    ast_json_free(tmp);

    // info_plan
    tmp = ast_json_dump_string_format(dialing->j_plan, 0);
    ast_json_object_set(j_res, "info_plan", ast_json_string_create(tmp));
    ast_json_free(tmp);

    // info_plan
    tmp = ast_json_dump_string_format(dialing->j_dlma, 0);
    ast_json_object_set(j_res, "info_dlma", ast_json_string_create(tmp));
    ast_json_free(tmp);

    // info_dl
    tmp = ast_json_dump_string_format(dialing->j_dl, 0);
    ast_json_object_set(j_res, "info_dl", ast_json_string_create(tmp));
    ast_json_free(tmp);

    // info_chan
    tmp = ast_json_dump_string_format(dialing->j_chan, 0);
    ast_json_object_set(j_res, "info_chan", ast_json_string_create(tmp));
    ast_json_free(tmp);

    // info_queues
    tmp = ast_json_dump_string_format(dialing->j_queues, 0);
    ast_json_object_set(j_res, "info_queues", ast_json_string_create(tmp));
    ast_json_free(tmp);

    // info_agents
    tmp = ast_json_dump_string_format(dialing->j_agents, 0);
    ast_json_object_set(j_res, "info_agents", ast_json_string_create(tmp));
    ast_json_free(tmp);


    return j_res;
}

