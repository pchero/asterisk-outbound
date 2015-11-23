/*
 * event_handler.h
 *
 *  Created on: Nov 9, 2015
 *      Author: pchero
 */

#ifndef SRC_EVENT_HANDLER_H_
#define SRC_EVENT_HANDLER_H_

int     run_outbound(void);
void    stop_outbound(void);

typedef enum _CAMP_STATUS_T
{
    // static status
    E_CAMP_STOP     = 0,
    E_CAMP_START    = 1,
    E_CAMP_PAUSE    = 2,

    // on going status
    E_CAMP_STOPPING = 10,
    E_CAMP_STARTING = 11,
    E_CAMP_PAUSING  = 12,

    // force status
    E_CAMP_STOPPING_FORCE = 30,
} E_CAMP_STATUS_T;

typedef enum _DL_STATUS_T
{
    E_DL_IDLE       = 0,
    E_DL_DIALING    = 1,

} DL_STATUS_T;

char* get_utc_timestamp(void);

struct ast_json* get_campaign_info_all(void);
struct ast_json* get_campaign_info(const char* uuid);
int update_campaign_info_status(const char* uuid, E_CAMP_STATUS_T status);

struct ast_json* get_plan_info_all(void);
struct ast_json* get_dl_master_info_all(void);

#endif /* SRC_EVENT_HANDLER_H_ */
