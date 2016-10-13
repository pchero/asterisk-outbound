/*
 * sqlite3_handler.h
 *
 *  Created on: Oct 11, 2016
 *      Author: pchero
 */

#ifndef SRC_DB_SQLITE3_HANDLER_H_
#define SRC_DB_SQLITE3_HANDLER_H_

#include "asterisk.h"

#include <sqlite3.h>
#include <stdbool.h>

#include "asterisk/json.h"

#include "db_handler.h"

bool			db_sqlite3_init(void);
void			db_sqlite3_exit(void);
db_res_t*	db_sqlite3_query(const char* query);
bool			db_sqlite3_exec(const char* query);
void			db_sqlite3_free(db_res_t* ctx);
bool			db_sqlite3_insert(const char* table, const struct ast_json* j_data);
char*	   	db_sqlite3_get_update_str(const struct ast_json* j_data);
struct ast_json*	db_sqlite3_get_record(db_res_t* ctx);


#endif /* SRC_DB_SQLITE3_HANDLER_H_ */
