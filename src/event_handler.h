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
char* get_utc_timestamp_using_timespec(struct timespec timeptr);

#endif /* SRC_EVENT_HANDLER_H_ */
