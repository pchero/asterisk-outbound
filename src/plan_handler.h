/*
 * plan_handler.h
 *
 *  Created on: Nov 30, 2015
 *      Author: pchero
 */

#ifndef SRC_PLAN_HANDLER_H_
#define SRC_PLAN_HANDLER_H_

int create_plan(struct ast_json* j_plan);
int delete_plan(const char* uuid);
struct ast_json* get_plan_info(const char* uuid);
struct ast_json* get_plan_info_all(void);

#endif /* SRC_PLAN_HANDLER_H_ */
