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

typedef enum _E_CAMP_STATUS_T
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

typedef enum _E_DL_STATUS_T
{
    E_DL_IDLE       = 0,
//    E_DL_QUEUEING   = 1,    ///< not in used.
    E_DL_DIALING    = 1,

} E_DL_STATUS_T;

char* get_utc_timestamp(void);

// campaign
struct ast_json* get_campaign_info_all(void);
struct ast_json* get_campaign_info(const char* uuid);
int update_campaign_info_status(const char* uuid, E_CAMP_STATUS_T status);

// plan
struct ast_json* get_plan_info_all(void);

// dl_list/dl_master
struct ast_json* get_dl_master_info_all(void);
struct ast_json* get_dl_master_info(const char* uuid);
struct ast_json* get_dl_list(struct ast_json* j_dlma, int count);

#endif /* SRC_EVENT_HANDLER_H_ */
