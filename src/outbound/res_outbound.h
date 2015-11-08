/*
 * res_outbound.h
 *
 *  Created on: Nov 7, 2015
 *      Author: pchero
 */

#ifndef SRC_OUTBOUND_RES_OUTBOUND_H_
#define SRC_OUTBOUND_RES_OUTBOUND_H_

#include "asterisk/json.h"

typedef struct _db_res_t
{
    void*   result;     ///< result set
} db_res_t;


// db_handler.c
int     db_connect(const char* host, int port, const char* user, const char* pass, const char* dbname);
void    db_exit(void);
db_res_t*   db_query(char* query);
int         db_exec(char* query);
void        db_free(db_res_t* ctx);
struct ast_json*    db_get_record(db_res_t* ctx);
int         db_insert(const char* table, const struct ast_json* j_data);
char*       db_get_update_str(const struct ast_json* j_data);


#endif /* SRC_OUTBOUND_RES_OUTBOUND_H_ */
