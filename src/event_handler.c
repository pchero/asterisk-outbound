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

#define TEMP_FILENAME "/tmp/asterisk_outbound_tmp.txt"


struct event_base*  g_base = NULL;

static int init_outbound(void);

static void cb_campaign_start(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);

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

char* get_utc_timestamp(void);

struct ast_json* get_queue_summary(const char* name);
int get_current_dialing_cnt(const char* camp_uuid, const char* dl_table);
static int get_dial_num_point(struct ast_json* j_dl_list, struct ast_json* j_plan);
static char* get_dial_number(struct ast_json* j_dlist, const int cnt);
static char* create_chan_addr_for_dial(struct ast_json* j_plan, struct ast_json* j_dl_list, int dial_num_point);

static char* gen_uuid(void);
static int get_dial_try_cnt(struct ast_json* j_dl_list, int dial_num_point);
static int update_dl_list(const char* table, struct ast_json* j_dlinfo);

// todo
static int check_dial_avaiable_predictive(struct ast_json* j_camp, struct ast_json* j_plan, struct ast_json* j_dlma);
struct ast_json* create_dialing_info(struct ast_json* j_camp, struct ast_json* j_plan, struct ast_json* j_dlma, struct ast_json* j_dl_list);
struct ast_json* create_dial_info(const struct ast_json* j_camp);
int cmd_originate(const struct ast_json* j_dial);
int memdb_insert(const char* table, const struct ast_json* j_data);


int run_outbound(void)
{
    int ret;
    struct event* ev;
    struct timeval tm_fast = {3, 20000};    // 20 ms
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

    sec.tv_sec = 1;
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
    struct ast_json* j_queue;
    struct ast_json* j_ami_res;
    struct ast_json* j_tmp;
    const char* dial_mode;
    const char* tmp_const;
    int i;
    size_t size;

    ast_log(LOG_DEBUG, "cb_campagin start\n");

    j_ami_res = cmd_queue_summary("sales");
    if(j_ami_res == NULL) {
        ast_log(LOG_NOTICE, "Could not get queue summary. name[%s]\n", "sales");
        return;
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
        if(strcmp(tmp_const, "sales") == 0) {
            j_queue = ast_json_deep_copy(j_tmp);
            break;
        }
    }
    ast_json_unref(j_ami_res);

    ast_log(LOG_DEBUG, "Queue simple status. queue[%s], loggedin[%s], available[%s], callers[%s], holdtime[%s], talktime[%s], longestholdtime[%s]\n",
            ast_json_string_get(ast_json_object_get(j_queue, "Queue")),
            ast_json_string_get(ast_json_object_get(j_queue, "LoggedIn")),
            ast_json_string_get(ast_json_object_get(j_queue, "Available")),
            ast_json_string_get(ast_json_object_get(j_queue, "Callers")),
            ast_json_string_get(ast_json_object_get(j_queue, "HoldTime")),
            ast_json_string_get(ast_json_object_get(j_queue, "TalkTime")),
            ast_json_string_get(ast_json_object_get(j_queue, "LongestHoldTime"))
            );

    j_camp = get_campaign_info_for_dialing();
    if(j_camp == NULL) {
        // Nothing.
        return;
    }
    ast_log(LOG_DEBUG, "Get campaign info. camp[%s]\n", ast_json_string_get(ast_json_object_get(j_camp, "uuid")));

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
 * Get campaign for dialing.
 * @return
 */
static struct ast_json* get_campaign_info_for_dialing(void)
{
    struct ast_json* j_res;
    db_res_t* db_res;
    char* sql;

    // get "start" status campaign only.
    ast_asprintf(&sql, "select * from campaign where status = \"%s\" order by rand() limit 1;",
            "start"
            );

    db_res = db_query(sql);
    ast_free(sql);
    if(db_res == NULL) {
        ast_log(LOG_WARNING, "Could not get campaign info.");
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

    ast_asprintf(&sql, "update campaign set status = \"%s\" where uuid = \"%s\";", tmp_status, uuid );
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

    // get dl_list info to dial.
    j_dl_list = get_dl_available_predictive(j_dlma, j_plan);
    if(j_dl_list == NULL) {
        // No available list
        return;
    }

    // check available outgoing call.
    ret = check_dial_avaiable_predictive(j_camp, j_plan, j_dlma);
    if(ret != 1) {
        if(ret == -1) {
            // something was wrong. stop the campaign.
            update_campaign_info_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), E_CAMP_STOPPING);
        }
        ast_json_unref(j_dl_list);
        return;
    }

    // creating dialing info
    j_dialing = create_dialing_info(j_camp, j_plan, j_dlma, j_dl_list);
    ast_json_unref(j_dl_list);
    if(j_dialing == NULL) {
        ast_log(LOG_DEBUG, "Could not create dialing info.");
        return;
    }
    ast_log(LOG_NOTICE, "Originating. camp_uuid[%s], camp_name[%s], channel[%s], chan_unique_id[%s], timeout[%s]",
            ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
            ast_json_string_get(ast_json_object_get(j_camp, "name")),
            ast_json_string_get(ast_json_object_get(j_dialing, "Channel")),
            ast_json_string_get(ast_json_object_get(j_dialing, "ChannelId")),
            ast_json_string_get(ast_json_object_get(j_dialing, "Timeout"))
            );

    // dial to customer
    j_res = cmd_originate_to_queue(j_dialing);
    if(j_res == NULL) {
        ast_log(LOG_WARNING, "Originating has failed.");
        ast_json_unref(j_dialing);
        return;
    }

    // create update dl_list
    j_dl_update = ast_json_pack("{s:s, s:i, s:s, s:s, s:s, s:s}",
            "status",                   "dialing",
            ast_json_string_get(ast_json_object_get(j_dialing, "trycount_field")), ast_json_integer_get(ast_json_object_get(j_dialing, "dial_trycnt")) + 1,
            "dialing_camp_uuid",        ast_json_string_get(ast_json_object_get(j_dialing, "camp_uuid")),
            "dialing_chan_unique_id",   ast_json_string_get(ast_json_object_get(j_dialing, "chan_unique_id")),
            "tm_last_dial",             ast_json_string_get(ast_json_object_get(j_dialing, "tm_dial")),
            "uuid",                     ast_json_string_get(ast_json_object_get(j_dialing, "dl_uuid"))
            );
    if(j_dl_update == NULL)
    {
        ast_log(LOG_ERROR, "Could not create dl update info json.");
        ast_json_unref(j_dialing);
        return;
    }

    // update dl_list
    ret = update_dl_list(ast_json_string_get(ast_json_object_get(j_dlma, "dl_table")), j_dl_update);
    ast_json_unref(j_dl_update);
    if(ret == false)
    {
        ast_json_unref(j_dialing);
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
            "status = \"idle\" "
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
            ast_json_string_get(ast_json_object_get(j_dlma, "dl_table"))
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
        ast_json_unref(j_queue);
        return -1;
    }


    // get count of currently dailings.
    ast_asprintf(&sql, "select count(*) from %s where dialing_camp_uuid = \"%s\" and status = \"%s\";",
            ast_json_string_get(ast_json_object_get(j_dlma, "dl_table")),
            ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
            "dialing"
            );

    // compare
    cnt_avail_chan = atoi(ast_json_string_get(ast_json_object_get(j_queue, "Available")));

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
        struct ast_json* j_camp,
        struct ast_json* j_plan,
        struct ast_json* j_dlma,
        struct ast_json* j_dl_list
        )
{
    struct ast_json* j_dial;
    char* chan_addr;
    int dial_num_point;
    char tmp_timeout[10];
    char tmp_dial_num_point[10];
    char* channel_id;
    char* other_channel_id;
    char* tm_dial;
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
    chan_addr = create_chan_addr_for_dial(j_plan, j_dl_list, dial_num_point);
    if(chan_addr == NULL) {
        ast_log(LOG_ERROR, "Could not get correct channel address.\n");
        return NULL;
    }

    ast_asprintf(&dial_try_count, "trycnt_%d", dial_num_point);
    sprintf(tmp_timeout, "%ld", ast_json_integer_get(ast_json_object_get(j_plan, "dial_timeout")));
    channel_id = gen_uuid();
    other_channel_id = gen_uuid();
    tm_dial = get_utc_timestamp();

    j_dial = ast_json_pack("{"
            "s:s, s:s, s:s, s:s, "
            "s:s, s:s, s:s, s:s, s:s, s:s, "
            "s:s, s:s, s:i, s:s"
            "}",
            // identity info
            "uuid_camp",    ast_json_string_get(ast_json_object_get(j_camp, "uuid")),
            "uuid_plan",    ast_json_string_get(ast_json_object_get(j_plan, "uuid")),
            "uuid_dlma",    ast_json_string_get(ast_json_object_get(j_dlma, "uuid")),
            "uuid_dl",      ast_json_string_get(ast_json_object_get(j_dl_list, "uuid")),

            // channel set
            "channel",      chan_addr,      ///< Destination
            "data",         ast_json_string_get(ast_json_object_get(j_plan, "queue_name")),        ///< Queue name
            "timeout",      tmp_timeout,
            "callerid",     ast_json_string_get(ast_json_object_get(j_plan, "caller_id")), ///< Caller id
            "channelid",        channel_id, ///< Channel ID
            "otherchannelid",   other_channel_id,    ///< Other channel id.
            //            "Variable",     ///< todo: More set dial info
            //            "Account",      ///< ????.

            // other info
            "dial_num_point",   tmp_dial_num_point,
            "tm_dial",          tm_dial,
            "dial_trycnt",      dial_count,
            "trycount_field",   dial_try_count
            );
    ast_free(chan_addr);
    ast_free(channel_id);
    ast_free(other_channel_id);
    ast_free(tm_dial);
    ast_free(dial_try_count);

    return j_dial;
}

struct ast_json* create_dial_info(const struct ast_json* j_camp)
{
    return NULL;
}

int cmd_originate(const struct ast_json* j_dial)
{
    return true;
}

int memdb_insert(const char* table, const struct ast_json* j_data)
{
    return true;
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

    j_ami_res = cmd_queue_summary(name);
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
 *
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
    if(j_tmp == NULL) {
        // shouldn't be reach to here.
        ast_log(LOG_ERROR, "Could not get dialing count.");
        return false;
    }
    db_free(db_res);

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

    if(ast_json_object_get(j_plan, "trunk_name") == NULL) {
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
    res = strdup(tmp);

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
