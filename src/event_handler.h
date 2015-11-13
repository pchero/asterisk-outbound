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
    E_CAMP_RUN = 1,
    E_CAMP_STOP = 2,
    E_CAMP_PAUSE = 3,

    // on going status
    E_CAMP_RUNNING = 11,
    E_CAMP_STOPPING = 12,
    E_CAMP_PAUSING = 13,
} CAMP_STATUS_T;

#endif /* SRC_EVENT_HANDLER_H_ */
