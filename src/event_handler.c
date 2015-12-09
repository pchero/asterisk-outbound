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
#include "campaign_handler.h"
#include "dl_handler.h"
#include "plan_handler.h"

#define TEMP_FILENAME "/tmp/asterisk_outbound_tmp.txt"


struct event_base*  g_base = NULL;

static int init_outbound(void);

static void cb_campaign_start(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);
static void cb_campaign_stopping(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);
static void cb_campaign_stopping_force(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);
static void cb_check_dialing_end(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);
static void cb_check_campaign_end(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);

//static struct ast_json* get_queue_info(const char* uuid);

static void dial_desktop(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma);
static void dial_power(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma);
static void dial_predictive(struct ast_json* j_camp, struct ast_json* j_plan, struct ast_json* j_dlma);
static void dial_robo(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma);
static void dial_redirect(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma);


struct ast_json* get_queue_summary(const char* name);
static char* get_dial_number(struct ast_json* j_dlist, const int cnt);
static char* create_chan_addr_for_dial(struct ast_json* j_plan, struct ast_json* j_dl_list, int dial_num_point);

static struct ast_json* create_dl_result(rb_dialing* dialing);

// todo
static int check_dial_avaiable_predictive(struct ast_json* j_camp, struct ast_json* j_plan, struct ast_json* j_dlma);
static struct ast_json* create_dialing_info(struct ast_json* j_plan, struct ast_json* j_dl_list);

int run_outbound(void)
{
    int ret;
    struct event* ev;
//    struct timeval tm_fast = {0, 2000};    // 20 ms
    struct timeval tm_fast = {3, 0};    // 3 sec
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
//    struct ast_json* j_queue;
    int dial_mode;

    ast_log(LOG_DEBUG, "cb_campagin start\n");

    j_camp = get_campaign_for_dialing();
    if(j_camp == NULL) {
        // Nothing.
        return;
    }
//    ast_log(LOG_DEBUG, "Get campaign info. camp_uuid[%s], camp_name[%s]\n",
//            ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
//            ast_json_string_get(ast_json_object_get(j_camp, "name"))
//            );

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

//    // get queue
//    j_queue = get_queue_info(ast_json_string_get(ast_json_object_get(j_camp, "queue")));
//    if(j_plan == NULL) {
//        ast_log(LOG_WARNING, "Could not get queue info. Stopping campaign camp[%s], queue[%s]\n",
//                ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
//                ast_json_string_get(ast_json_object_get(j_camp, "queue"))
//                );
//        update_campaign_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOPPING);
//        ast_json_unref(j_camp);
//        return;
//    }

//    ast_log(LOG_DEBUG, "Get plan info. camp_uuid[%s], camp_name[%s]\n",
//            ast_json_string_get(ast_json_object_get(j_plan, "uuid")),
//            ast_json_string_get(ast_json_object_get(j_plan, "name"))
//            );

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
//        ast_json_unref(j_queue);
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
//        ast_json_unref(j_queue);
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
//    ast_json_unref(j_queue);

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

    ast_log(LOG_DEBUG, "cb_campaign_stopping\n");

    j_camps = get_campaigns_by_status(E_CAMP_STOPPING);
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

            tmp_const = ast_json_string_get(ast_json_object_get(dialing->j_res, "camp_uuid"));
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
            update_campaign_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOP);
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
        while((dialing = ao2_iterator_next(&iter))) {
            ao2_ref(dialing, -1);

            tmp_const = ast_json_string_get(ast_json_object_get(dialing->j_res, "camp_uuid"));
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
    while((dialing = ao2_iterator_next(&iter))) {
        ao2_ref(dialing, -1);

        if(dialing->status != E_DIALING_HANGUP) {
            continue;
        }

        // create dl_list for update
        j_tmp = ast_json_pack("{s:s, s:i, s:O, s:O, s:O, s:O, s:O}",
                "uuid",                 ast_json_string_get(ast_json_object_get(dialing->j_res, "dl_list_uuid")),
                "status",               E_DL_IDLE,
                "dialing_uuid",         ast_json_null(),
                "dialing_camp_uuid",    ast_json_null(),
                "dialing_plan_uuid",    ast_json_null(),
                "res_hangup",           ast_json_ref(ast_json_object_get(dialing->j_res, "res_hangup")),
                "res_dial",             ast_json_ref(ast_json_object_get(dialing->j_res, "res_dial"))? : ast_json_integer_create(0)
                );

        // update dl_list
        ret = update_dl_list(j_tmp);
        ast_json_unref(j_tmp);
        if(ret == false) {
            ast_log(LOG_WARNING, "Could not update dialing result. dialing_uuid[%s], dl_list_uuid[%s]\n",
                    dialing->uuid, ast_json_string_get(ast_json_object_get(dialing->j_res, "dl_list_uuid")));
            continue;
        }

        // create result data
        j_tmp = create_dl_result(dialing);
        db_insert("dl_result", j_tmp);
        ast_json_unref(j_tmp);

        rb_dialing_destory(dialing);
        ast_log(LOG_DEBUG, "Destroied!\n");

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
//    char* sql;
//    struct ast_json* j_res;
//    db_res_t* db_res;
//
//    if(uuid == NULL) {
//        ast_log(LOG_WARNING, "Invalid input parameters.\n");
//        return NULL;
//    }
//
//    ast_asprintf(&sql, "select * from queue where uuid=\"%s\" and in_use=1;", uuid);
//
//    db_res = db_query(sql);
//    ast_free(sql);
//    if(db_res == NULL) {
//        ast_log(LOG_ERROR, "Could not get queue info. uuid[%s]\n", uuid);
//        return NULL;
//    }
//
//    j_res = db_get_record(db_res);
//    db_free(db_res);
//
//    return j_res;
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
    char* tmp;

    // validate plan
    if(ast_json_string_get(ast_json_object_get(j_plan, "trunk_name")) == NULL) {
        ast_log(LOG_WARNING, "Could not find trunk name. Stopping campaign.\n");
        update_campaign_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOPPING);
        return;
    }

    // get dl_list info to dial.
    j_dl_list = get_dl_available_predictive(j_dlma, j_plan);
    if(j_dl_list == NULL) {
        // No available list
//        ast_log(LOG_VERBOSE, "No more dialing list. stopping the campaign.\n");
//        update_campaign_info_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOPPING);
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
    dialing = rb_dialing_create(ast_json_string_get(ast_json_object_get(j_dialing, "channelid")), j_camp, j_plan, j_dlma, j_dialing);
    if(dialing == NULL) {
        ast_log(LOG_WARNING, "Could not create rbtree object.");
        ast_json_unref(j_dialing);
        return;
    }

    // dial to customer
//    j_res = ami_cmd_originate_to_queue(j_dialing);
    j_res = ami_cmd_originate_to_exten(
            j_dialing,
            PLAN_CONTEXT,
            ast_json_string_get(ast_json_object_get(dialing->j_res, "plan_uuid"))
            );
    if(j_res == NULL) {
        ast_log(LOG_WARNING, "Originating has failed.");
        clear_dl_list_dialing(ast_json_string_get(ast_json_object_get(dialing->j_res, "dl_list_uuid")));
        rb_dialing_destory(dialing);
        ast_json_unref(j_dialing);
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
    j_dl_update = ast_json_pack("{s:s, s:i, s:i, s:s, s:s, s:s, s:s}",
            "uuid",                 ast_json_string_get(ast_json_object_get(dialing->j_res, "dl_list_uuid")),
            ast_json_string_get(ast_json_object_get(j_dialing, "trycount_field")), ast_json_integer_get(ast_json_object_get(j_dialing, "dial_trycnt")) + 1,
            "status",               E_DL_DIALING,
            "dialing_uuid",         dialing->uuid,
            "dialing_camp_uuid",    ast_json_string_get(ast_json_object_get(dialing->j_res, "camp_uuid")),
            "dialing_plan_uuid",    ast_json_string_get(ast_json_object_get(dialing->j_res, "plan_uuid")),
            "tm_last_dial",         tmp
            );
    ast_json_unref(j_dialing);
    ast_free(tmp);
    ret = update_dl_list(j_dl_update);
    ast_json_unref(j_dl_update);
    if(ret == false) {
        rb_dialing_destory(dialing);
        clear_dl_list_dialing(ast_json_string_get(ast_json_object_get(dialing->j_res, "dl_list_uuid")));
        ast_log(LOG_ERROR, "Could not update dial list info.");
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
 * @param j_camp    campaign info
 * @param j_plan    plan info
 * @param j_dlma    dial list master info
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
    cnt_current_dialing = get_current_dialing_dl_cnt(
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
static struct ast_json* create_dialing_info(
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

/**
 * return utc time.
 * YYYY-MM-DDTHH:mm:ssZ
 * @return
 */
char* get_utc_timestamp_using_timespec(struct timespec timeptr)
{
    char    timestr[128];
    char*   res;
    time_t  tt;
    struct tm *t;

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

static struct ast_json* create_dl_result(rb_dialing* dialing)
{
    struct ast_json* j_res;
    char* tmp;

    j_res = ast_json_deep_copy(dialing->j_res);

    ast_log(LOG_DEBUG, "Check value. dialing_uuid[%s], camp_uuid[%s], plan_uuid[%s], dlma_uuid[%s], dl_list_uuid[%s]\n",
            ast_json_string_get(ast_json_object_get(j_res, "dialing_uuid")),
            ast_json_string_get(ast_json_object_get(j_res, "camp_uuid")),
            ast_json_string_get(ast_json_object_get(j_res, "plan_uuid")),
            ast_json_string_get(ast_json_object_get(j_res, "dlma_uuid")),
            ast_json_string_get(ast_json_object_get(j_res, "dl_list_uuid"))
            );

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

