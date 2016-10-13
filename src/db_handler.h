/*
 * db_handler.h
 *
 *  Created on: Nov 9, 2015
 *	  Author: pchero
 */

#ifndef SRC_DB_HANDLER_H_
#define SRC_DB_HANDLER_H_

#include "asterisk.h"

#include "asterisk/json.h"

#include <stdbool.h>

typedef struct _db_res_t
{
	void* res;		///< result set
} db_res_t;


bool			db_init(void);
void			db_exit(void);
db_res_t* db_query(const char* query);
bool			db_exec(const char* query);
void			db_free(db_res_t* ctx);
bool	 		db_insert(const char* table, const struct ast_json* j_data);
char*	   	db_get_update_str(const struct ast_json* j_data);
struct ast_json*	db_get_record(db_res_t* ctx);


#endif /* SRC_DB_HANDLER_H_ */
