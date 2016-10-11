/*
 * sqlite3_handler.h
 *
 *  Created on: Oct 11, 2016
 *      Author: pchero
 */

#ifndef SRC_SQLITE3_HANDLER_H_
#define SRC_SQLITE3_HANDLER_H_

#include "asterisk.h"

#include <sqlite3.h>

#include "asterisk/json.h"

typedef struct _db_res_t
{
	sqlite3_stmt* res;	 ///< result set
} db_res_t;

int 			db_connect(const char* filename);
void			db_exit(void);
db_res_t*	db_query(const char* query);
int		 		db_exec(const char* query);
void			db_free(db_res_t* ctx);
int		 		db_insert(const char* table, const struct ast_json* j_data);
char*	   	db_get_update_str(const struct ast_json* j_data);
struct ast_json*	db_get_record(db_res_t* ctx);


#endif /* SRC_SQLITE3_HANDLER_H_ */
