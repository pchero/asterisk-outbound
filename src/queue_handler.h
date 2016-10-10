/*
 * queue_handler.h
 *
 *  Created on: Dec 3, 2015
 *	  Author: pchero
 */

#ifndef QUEUE_HANDLER_H_
#define QUEUE_HANDLER_H_

int create_queue(struct ast_json* j_queue);
int delete_queue(const char* uuid);
struct ast_json* get_queue(const char* uuid);
struct ast_json* get_queues_all(void);
int update_queue(struct ast_json* j_queue);


#endif /* QUEUE_HANDLER_H_ */
