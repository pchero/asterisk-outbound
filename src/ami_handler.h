/*
 * ami_handler.h
 *
 *  Created on: Nov 12, 2015
 *      Author: pchero
 */

#ifndef SRC_AMI_HANDLER_H_
#define SRC_AMI_HANDLER_H_

#include "asterisk/causes.h"

#include <stdbool.h>

int init_ami_handle(void);
void term_ami_handle(void);

struct ast_json* ami_cmd_handler(struct ast_json* j_cmd);
bool ami_is_response_success(struct ast_json* j_ami);

struct ast_json* ami_cmd_queue_summary(const char* name);
struct ast_json* ami_cmd_originate_to_application(struct ast_json* j_dial);
struct ast_json* ami_cmd_originate_to_exten(struct ast_json* j_dial);
struct ast_json* ami_cmd_hangup(const char* channel, int cause);
struct ast_json* ami_cmd_dialplan_extension_add(struct ast_json* j_dialplan);
struct ast_json* ami_cmd_dialplan_extension_remove(const char* context, const char* extension, const int priority);

#endif /* SRC_AMI_HANDLER_H_ */
