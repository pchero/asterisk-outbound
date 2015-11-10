/*
 * event_handler.c
 *
 *  Created on: Nov 8, 2015
 *      Author: pchero
 */

#include "asterisk.h"

#include <stdbool.h>
#include <event2/event.h>
#include <errno.h>

#include "asterisk/json.h"
#include "asterisk/utils.h"
#include "db_handler.h"


struct event_base*  g_base = NULL;

static int init_outbound(void);

static void cb_campaign_start(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);

static struct ast_json* get_campaign_for_dial(void);

int run_outbound(void)
{
    int ret;
    struct event* ev;
//    struct timeval tm_fast = {0, 20000};    // 20 ms
    struct timeval tm_fast = {0, 200000};    // 20 ms
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

    ast_log(LOG_DEBUG, "cb_campagin start\n");

    j_camp = get_campaign_for_dial();
    if(j_camp == NULL) {
        return;
    }
    ast_log(LOG_DEBUG, "Get campaign info. uuid[%s]\n",
            ast_json_string_get(ast_json_object_get(j_camp, "uuid"))
            );


    // get plan

    // release
    ast_json_unref(j_camp);

    return;
}

/**
 * Get campaign for dialing.
 * @return
 */
static struct ast_json* get_campaign_for_dial(void)
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
