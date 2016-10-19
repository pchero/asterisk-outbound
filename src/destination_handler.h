/*
 * destination_handler.h
 *
 *  Created on: Oct 19, 2016
 *      Author: pchero
 */

#ifndef SRC_DESTINATION_HANDLER_H_
#define SRC_DESTINATION_HANDLER_H_

#include "asterisk.h"

#include <stdbool.h>

#include "asterisk/json.h"

bool create_destination(const struct ast_json* j_dest);
bool delete_destination(const char* uuid);
struct ast_json* get_destination(const char* uuid);
struct ast_json* get_destinations_all(void);
bool update_destination(const struct ast_json* j_dest);

#endif /* SRC_DESTINATION_HANDLER_H_ */
