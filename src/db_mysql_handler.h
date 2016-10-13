/*
 * mysql_handler.h
 *
 *  Created on: Oct 12, 2016
 *      Author: pchero
 */

#ifndef SRC_DB_MYSQL_HANDLER_H_
#define SRC_DB_MYSQL_HANDLER_H_

#include "asterisk.h"

#include <stdbool.h>

#include "asterisk/json.h"

#include "db_handler.h"

bool 			db_mysql_init(void);
void			db_mysql_exit(void);
db_res_t* db_mysql_query(const char* query);
bool			db_mysql_exec(const char* query);
void			db_mysql_free(db_res_t* ctx);
bool			db_mysql_insert(const char* table, const struct ast_json* j_data);
char*	   	db_mysql_get_update_str(const struct ast_json* j_data);
struct ast_json*	db_mysql_get_record(db_res_t* ctx);


#endif /* SRC_DB_MYSQL_HANDLER_H_ */
