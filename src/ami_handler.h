/*
 * ami_handler.h
 *
 *  Created on: Nov 12, 2015
 *      Author: pchero
 */

#ifndef SRC_AMI_HANDLER_H_
#define SRC_AMI_HANDLER_H_

struct ast_json* ami_cmd_handler(struct ast_json* j_cmd);
int ami_is_response_success(struct ast_json* j_ami);

struct ast_json* cmd_queue_summary(const char* name);
struct ast_json* cmd_originate_to_queue(struct ast_json* j_dial);

int ami_evt_handler(void);


#endif /* SRC_AMI_HANDLER_H_ */
