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

#include "asterisk/utils.h"

struct event_base*  g_base = NULL;

void cb_campaign_start(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg);

int init_event(void)
{
    struct event* ev;
    struct timeval tm_fast = {0, 20000};    // 20 ms
//    struct timeval tm_slow = {0, 500000};   // 500 ms

    // init libevent
    if(g_base == NULL) {
        g_base = event_base_new();
    }

    if(g_base == NULL) {
        ast_log(LOG_ERROR, "Could not initiate libevent. err[%d:%s]\n", errno, strerror(errno));
        return false;
    }

    // check start.
    ev = event_new(g_base, -1, EV_TIMEOUT | EV_PERSIST, cb_campaign_start, NULL);
    event_add(ev, &tm_fast);

    return true;
}

/**
 *  @brief  Check start status campaign and trying to make a call.
 */
void cb_campaign_start(__attribute__((unused)) int fd, __attribute__((unused)) short event, __attribute__((unused)) void *arg)
{

    return;
}
