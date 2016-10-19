/*
 * cli_handler.h
 *
 *  Created on: Nov 22, 2015
 *	  Author: pchero
 */

#ifndef CLI_HANDLER_H_
#define CLI_HANDLER_H_

#include "asterisk.h"
#include "asterisk/cli.h"

#include "dialing_handler.h"

int init_cli_handler(void);
void term_cli_handler(void);

void send_manager_evt_out_campaign_create(struct ast_json* j_camp);
void send_manager_evt_out_campaign_delete(const char* uuid);
void send_manager_evt_out_campaign_update(struct ast_json* j_camp);
void send_manager_evt_out_plan_create(struct ast_json* j_plan);
void send_manager_evt_out_plan_delete(const char* uuid);
void send_manager_evt_out_plan_update(struct ast_json* j_plan);
void send_manager_evt_out_dlma_create(struct ast_json* j_dlma);
void send_manager_evt_out_dlma_delete(const char* uuid);
void send_manager_evt_out_dlma_update(struct ast_json* j_dlma);
void send_manager_evt_out_queue_create(struct ast_json* j_dlma);
void send_manager_evt_out_queue_delete(const char* uuid);
void send_manager_evt_out_queue_update(struct ast_json* j_dlma);
void send_manager_evt_out_dialing_create(rb_dialing* dialing);
void send_manager_evt_out_dialing_update(rb_dialing* dialing);
void send_manager_evt_out_dialing_delete(rb_dialing* dialing);
void send_manager_evt_out_destination_create(struct ast_json* j_dest);
void send_manager_evt_out_destination_delete(const char* uuid);
void send_manager_evt_out_destination_update(struct ast_json* j_dest);


#endif /* CLI_HANDLER_H_ */
