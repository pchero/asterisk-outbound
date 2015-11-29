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

char* get_utc_timestamp(void);

// campaign

// plan
struct ast_json* get_plan_info_all(void);

// dl_list/dl_master


#endif /* SRC_EVENT_HANDLER_H_ */
