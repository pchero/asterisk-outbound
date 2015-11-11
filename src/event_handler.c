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
#include "db_handler.h"


struct event_base*  g_base = NULL;

static int init_outbound(void);

static void cb_campaign_start(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);

static struct ast_json* get_campaign_info_for_dialing(void);
static struct ast_json* get_plan_info(const char* uuid);
static struct ast_json* get_dl_master_info(const char* uuid);
static int update_campaign_info_status(const char* uuid, const char* status);

static void dial_desktop(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma);
static void dial_power(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma);
static void dial_predictive(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma);
static void dial_robo(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma);
static void dial_redirect(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma);

static struct ast_json* get_dl_dial_available(const struct ast_json* j_dlma, const char* dial_mode);
static struct ast_json* get_dl_available_predictive(const struct ast_json* j_dlma, const struct ast_json* j_plan);


int run_outbound(void)
{
    int ret;
    struct event* ev;
    struct timeval tm_fast = {0, 20000};    // 20 ms
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
    const char* dial_mode;

    ast_log(LOG_DEBUG, "cb_campagin start\n");

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
        update_campaign_info_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), "stopping");
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
        update_campaign_info_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), "stopping");
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

        update_campaign_info_status(ast_json_string_get(ast_json_object_get(j_camp, "uuid")), "stopping");
        ast_json_unref(j_camp);
        ast_json_unref(j_plan);
        ast_json_unref(j_dlma);
        return;
    }

    if(strcmp(dial_mode, "desktop") == 0)
    {
        dial_desktop(j_camp, j_plan, j_dlma);
    }
    else if(strcmp(dial_mode, "power") == 0)
    {
        dial_power(j_camp, j_plan, j_dlma);
    }
    else if(strcmp(dial_mode, "predictive") == 0)
    {
        dial_predictive(j_camp, j_plan, j_dlma);
    }
    else if(strcmp(dial_mode, "robo") == 0)
    {
        dial_robo(j_camp, j_plan, j_dlma);
    }
    else if(strcmp(dial_mode, "redirect") == 0)
    {
        dial_redirect(j_camp, j_plan, j_dlma);
    }
    else
    {
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
static int update_campaign_info_status(const char* uuid, const char* status)
{
    char* sql;
    int ret;

    if((uuid == NULL) || (status == NULL)) {
        ast_log(LOG_WARNING, "Invalid input parameters.\n");
        return false;
    }

    ast_asprintf(&sql, "update campaign set status = \"%s\" where uuid = \"%s\";", status, uuid );
    ret = db_exec(sql);
    ast_free(sql);
    if(ret == false) {
        ast_log(LOG_ERROR, "Could not update campaign status info. uuid[%s], status[%s]\n", uuid, status);
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
static void dial_predictive(const struct ast_json* j_camp, const struct ast_json* j_plan, const struct ast_json* j_dlma)
{
    int ret;
    struct ast_json* j_dl_list;
    struct ast_json* j_dial;
    struct ast_json* j_dialing;
    struct ast_json* j_dl_update;
    char    try_cnt[128];   // string buffer for "trycnt_1"...
    char* tmp;

    // get dl_list info to dial.
    j_dl_list = get_dl_available_predictive(j_dlma, j_plan);
//    j_dl_list = get_dl_dial_available(j_dlma, "predictive");
    if(j_dl_list == NULL)
    {
        // No available list
        return;
    }

    // check available outgoing call.
    ret = check_dial_avaiable(j_camp, j_plan, j_dlma);
    if(ret == false)
    {
        // No available outgoing call.
        json_decref(j_dl_list);
        return;
    }

    j_dialing = create_dialing_info(j_camp, j_plan, j_dlma, j_dl_list);
    json_decref(j_dl_list);
    if(j_dialing == NULL)
    {
        slog(LOG_DEBUG, "Could not create dialing info.");
        return;
    }

    // create dial
    j_dial = create_dial_info(j_dialing);
    if(j_dial == NULL)
    {
        slog(LOG_ERR, "Could not create dial info.");
        json_decref(j_dialing);

        return;
    }
    slog(LOG_INFO, "Originating. camp_uuid[%s], camp_name[%s], channel[%s], chan_unique_id[%s], timeout[%s]",
            json_string_value(json_object_get(j_camp, "uuid")),
            json_string_value(json_object_get(j_camp, "name")),
            json_string_value(json_object_get(j_dial, "Channel")),
            json_string_value(json_object_get(j_dial, "ChannelId")),
            json_string_value(json_object_get(j_dial, "Timeout"))
            );

    // dial to customer
    ret = cmd_originate(j_dial);
    json_decref(j_dial);
    if(ret == false)
    {
        slog(LOG_ERR, "Could not originate.");
        json_decref(j_dialing);

        return;
    }

    // set utc timestamp
    tmp = get_utc_timestamp();
    json_object_set_new(j_dialing, "tm_dial", json_string(tmp));
    free(tmp);

    // create update dl_list
    sprintf(try_cnt, "trycnt_%d", (int)json_integer_value(json_object_get(j_dialing, "dial_index")));
    j_dl_update = json_pack("{s:s, s:i, s:s, s:s, s:s, s:s}",
            "status",                   "dialing",
            try_cnt,                    json_integer_value(json_object_get(j_dialing, "dial_trycnt")) + 1,
            "dialing_camp_uuid",        json_string_value(json_object_get(j_dialing, "camp_uuid")),
            "dialing_chan_unique_id",   json_string_value(json_object_get(j_dialing, "chan_unique_id")),
            "tm_last_dial",             json_string_value(json_object_get(j_dialing, "tm_dial")),
            "uuid",                     json_string_value(json_object_get(j_dialing, "dl_uuid"))
            );
    if(j_dl_update == NULL)
    {
        slog(LOG_ERR, "Could not create dl update info json.");
        json_decref(j_dialing);
        return;
    }

    // update dl_list
    ret = update_dl_list(json_string_value(json_object_get(j_dlma, "dl_table")), j_dl_update);
    json_decref(j_dl_update);
    if(ret == false)
    {
        json_decref(j_dialing);
        slog(LOG_ERR, "Could not update dial list info.");
        return;
    }

    // insert dialing
    slog(LOG_INFO, "Insert new dialing. chan_unique_id[%s]", json_string_value(json_object_get(j_dialing, "chan_unique_id")));
    ret = memdb_insert("dialing", j_dialing);
    json_decref(j_dialing);
    if(ret == false)
    {
        slog(LOG_ERR, "Could not insert dialing info.");
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
    int ret;
    char*   sql;
    db_ctx_t* db_res;
    json_t* j_avail_agent;
    json_t* j_dlist;
    json_t* j_trunk;
    json_t* j_dial;
    char*   channel_id;
    char*   tmp;
    char    try_cnt[128];   // string buffer for "trycnt_1"...
    uuid_t uuid;
    int i;
    int cur_trycnt;
    int max_trycnt;
    char*   dial_addr;
    memdb_res* mem_res;
    int dial_num_point;

    // Need some module for compare currently dialing calls and currently ready agent.

    // get available agent(just figure out how many calls are can go at this moment)
    ret = asprintf(&sql, "select * from agent where "
            "id = (select agent_uuid from agent_group where group_uuid=\"%s\") "
            "and status=\"%s\" "
            "limit 1;",

            json_string_value(json_object_get(j_camp, "agent_group")),
            "ready"
            );

    db_res = db_query(sql);
    free(sql);
    if(db_res == NULL)
    {
        slog(LOG_DEBUG, "Could not get available agent.");
        return;
    }

    j_avail_agent = db_get_record(db_res);
    db_free(db_res);
    if(j_avail_agent == NULL)
    {
        // No available agent
        // Don't set any log here. Too much log..
        return;
    }
    json_decref(j_avail_agent);

    // get dial list
    ret = asprintf(&sql, "select "
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
            (int)json_integer_value(json_object_get(j_plan, "max_retry_cnt_1")),
            (int)json_integer_value(json_object_get(j_plan, "max_retry_cnt_2")),
            (int)json_integer_value(json_object_get(j_plan, "max_retry_cnt_3")),
            (int)json_integer_value(json_object_get(j_plan, "max_retry_cnt_4")),
            (int)json_integer_value(json_object_get(j_plan, "max_retry_cnt_5")),
            (int)json_integer_value(json_object_get(j_plan, "max_retry_cnt_6")),
            (int)json_integer_value(json_object_get(j_plan, "max_retry_cnt_7")),
            (int)json_integer_value(json_object_get(j_plan, "max_retry_cnt_8")),
            json_string_value(json_object_get(j_dlma, "dl_table"))
            );

    db_res = db_query(sql);
    free(sql);
    if(db_res == NULL)
    {
        slog(LOG_ERR, "Could not get dial list info.");
        return;
    }
    j_dlist = db_get_record(db_res);
    db_free(db_res);
    if(j_dlist == NULL)
    {
        return;
    }

    // get dial number
    dial_num_point = -1;
    for(i = 1; i < 9; i++)
    {
        ret = asprintf(&tmp, "number_%d", i);
        ret = strlen(json_string_value(json_object_get(j_dlist, tmp)));
        free(tmp);
        if(ret == 0)
        {
            // No number set.
            continue;
        }

        ret = asprintf(&tmp, "trycnt_%d", i);
        cur_trycnt = json_integer_value(json_object_get(j_dlist, tmp));
        free(tmp);

        ret = asprintf(&tmp, "max_retry_cnt_%d", i);
        max_trycnt = json_integer_value(json_object_get(j_plan, tmp));
        free(tmp);

        if(cur_trycnt < max_trycnt)
        {
            dial_num_point = i;
            break;
        }
    }
    if(dial_num_point < 0)
    {
        slog(LOG_ERR, "Could not find correct number count.");
        json_decref(j_dlist);
        return;
    }

    // create dial address
    // get trunk
    ret = asprintf(&sql, "select * from peer where status like \"OK%%\" "
            "and name = (select trunk_name from trunk_group where group_uuid = \"%s\" order by random()) "
            "limit 1;",
            json_string_value(json_object_get(j_plan, "trunk_group"))
            );
    mem_res = memdb_query(sql);
    free(sql);
    j_trunk = memdb_get_result(mem_res);
    memdb_free(mem_res);
    if(j_trunk == NULL)
    {
        slog(LOG_INFO, "No available trunk.");
        json_decref(j_dlist);

        return;
    }

    // create uniq id
    tmp = NULL;
    tmp = calloc(100, sizeof(char));
    uuid_generate(uuid);
    uuid_unparse_lower(uuid, tmp);
    ret = asprintf(&channel_id, "channel-%s", tmp);
    slog(LOG_INFO, "Create channel id. channel[%s]", channel_id);
    free(tmp);

    // dial to
    ret = asprintf(&tmp, "number_%d", dial_num_point);
    ret = asprintf(&dial_addr, "sip/%s@%s",
            json_string_value(json_object_get(j_dlist, tmp)),
            json_string_value(json_object_get(j_trunk, "name"))
            );
    free(tmp);
    slog(LOG_INFO, "Dialing info. dial_addr[%s]", dial_addr);
    json_decref(j_trunk);

    ret = asprintf(&tmp, "%d", (int)json_integer_value(json_object_get(j_plan, "dial_timeout")));
    slog(LOG_DEBUG, "Check info. dial_addr[%s], channel_id[%s], timeout[%s], timeout_org[%d]",
            dial_addr, channel_id, tmp, (int)json_integer_value(json_object_get(j_plan, "dial_timeout"))
            );
    j_dial = json_pack("{s:s, s:s, s:s, s:s, s:s, s:s}",
            "Channel", dial_addr,
            "ChannelId", channel_id,
            "Exten", "s",
            "Context", "olive_outbound_amd_default",
            "Priority", "1",
            "Timeout", tmp
            );
    free(tmp);

    slog(LOG_INFO, "Dialing. Campaign info. uuid[%s], name[%s], status[%s], dial_mode[%s], dial_num[%s], channel[%s]",
            json_string_value(json_object_get(j_camp, "uuid")),
            json_string_value(json_object_get(j_camp, "name")),
            json_string_value(json_object_get(j_camp, "status")),
            json_string_value(json_object_get(j_plan, "dial_mode")),
            dial_addr,
            channel_id
            );
    free(channel_id);
    free(dial_addr);

    ret = cmd_originate(j_dial);
    if(ret == false)
    {
        slog(LOG_ERR, "Could not originate.");
        json_decref(j_dial);
        json_decref(j_dlist);
        return;
    }

    // insert into dialing
    ret = asprintf(&sql, "insert into dialing("
            "dl_uuid, chan_unique_id, camp_uuid, status, tm_dial, "
            "dial_index, dial_addr, dial_trycnt"
            ") values ("
            "\"%s\", \"%s\", \"%s\", \"%s\", %s, "
            "%d, \"%s\", %d"
            ");",

            json_string_value(json_object_get(j_dlist, "uuid")),
            json_string_value(json_object_get(j_dial, "ChannelId")),
            json_string_value(json_object_get(j_camp, "uuid")),
            "dialing",
            "strftime('%Y-%m-%d %H:%m:%f', 'now')",

            dial_num_point,
            json_string_value(json_object_get(j_dial, "Channel")),
            cur_trycnt

            );

    ret = memdb_exec(sql);
    free(sql);
    if(ret == false)
    {
        slog(LOG_ERR, "Could not insert channel info into memdb.");
        // Just going.
    }

    sprintf(try_cnt, "trycnt_%d", dial_num_point);

    // update dial list status
    ret = asprintf(&sql, "update %s set status = \"%s\", %s = %s + 1, chan_unique_id = \"%s\" where uuid =\"%s\"",
            json_string_value(json_object_get(j_dlma, "dl_table")),
            "dialing",
            try_cnt, try_cnt,
            json_string_value(json_object_get(j_dial, "ChannelId")),
            json_string_value(json_object_get(j_dlist, "uuid"))
            );

    ret = db_exec(sql);
    free(sql);
    if(ret == false)
    {
        slog(LOG_ERR, "Could not insert channel info into db.");
    }
    json_decref(j_dial);
    json_decref(j_dlist);

    return;

}

/**
 * Get outgoing available dl.
 * @param j_dlma
 * @param j_plan
 * @return
 */
static struct ast_json* get_dl_dial_available(const struct ast_json* j_dlma, const char* dial_mode)
{
    struct ast_json* j_res;
    const char* dial_mode;

    // get dial_mode
    dial_mode = ast_json_string_get(ast_json_object_get(j_plan, "dial_mode"));
    if(dial_mode == NULL)
    {
        ast_log(LOG_ERROR, "Could not get dial_mode. plan[%s]\n", ast_json_string_get(ast_json_object_get(j_plan, "uuid")));
        return NULL;
    }

    if(strcmp(dial_mode, "predictive") == 0)
    {
        j_res = get_dl_available_predictive(j_dlma, j_plan);
    }
    else
    {
        // Not supported yet.
        ast_log(LOG_ERR, "Not supported dial_mode.");
        return NULL;
    }

    return j_res;
}

/**
 * Get dl_list from database.
 * @param j_dlma
 * @param j_plan
 * @return
 */
static struct ast_json* get_dl_available_predictive(const struct ast_json* j_dlma, const struct ast_json* j_plan)
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
