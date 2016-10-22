/*
 * plan_handler.h
 *
 *  Created on: Nov 30, 2015
 *	  Author: pchero
 */

#ifndef SRC_PLAN_HANDLER_H_
#define SRC_PLAN_HANDLER_H_

typedef enum _E_DIAL_MODE {
	E_DIAL_MODE_NONE = 0,		   /// No dial mode
	E_DIAL_MODE_PREDICTIVE = 1,	 ///
	E_DIAL_MODE_DESKTOP = 2,		///
	E_DIAL_MODE_POWER = 3,		  ///
	E_DIAL_MODE_ROBO = 4,		   ///
	E_DIAL_MODE_REDIRECT = 5,	   ///
} E_DIAL_MODE;

bool init_plan(void);

bool create_plan(const struct ast_json* j_plan);
bool delete_plan(const char* uuid);
bool update_plan(const struct ast_json* j_plan);
struct ast_json* get_plan(const char* uuid);
struct ast_json* get_plans_all(void);

struct ast_json* create_dial_plan_info(struct ast_json* j_plan);

#endif /* SRC_PLAN_HANDLER_H_ */
