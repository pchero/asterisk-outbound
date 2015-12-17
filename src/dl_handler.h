/*
 * dl_handler.h
 *
 *  Created on: Nov 29, 2015
 *      Author: pchero
 */

#ifndef SRC_DL_HANDLER_H_
#define SRC_DL_HANDLER_H_

#include "dialing_handler.h"

#include <stdbool.h>

typedef enum _E_DL_STATUS_T
{
    E_DL_IDLE       = 0,
    E_DL_DIALING    = 1,
//    E_DL_QUEUEING   = 2,    ///< not in used.

} E_DL_STATUS_T;

typedef enum _E_DIAL_TYPE
{
    E_DIAL_EXTEN    = 0,    ///< dialing and connect to extension
    E_DIAL_APPL     = 1,    ///< dialing and connect to application
} E_DIAL_TYPE;

bool create_dlma(const struct ast_json* j_dlma);
bool update_dlma(const struct ast_json* j_dlma);
bool delete_dlma(const char* uuid);

bool create_dl_list(struct ast_json* j_dl);
bool update_dl_list(struct ast_json* j_dl);
bool delete_dl_list(const char* uuid);

int get_current_dialing_dl_cnt(const char* camp_uuid, const char* dl_table);
int get_dial_num_point(struct ast_json* j_dl_list, struct ast_json* j_plan);
int get_dial_try_cnt(struct ast_json* j_dl_list, int dial_num_point);

struct ast_json* get_dlmas_all(void);
struct ast_json* get_dlma(const char* uuid);
struct ast_json* get_dl_lists_(struct ast_json* j_dlma, int count);
struct ast_json* get_dl_list(const char* uuid);
struct ast_json* get_dl_lists(const char* dlma_uuid, int count);
struct ast_json* get_dl_available_predictive(struct ast_json* j_dlma, struct ast_json* j_plan);
int check_more_dl_list(struct ast_json* j_dlma, struct ast_json* j_plan);
void clear_dl_list_dialing(const char* uuid);

struct ast_json* create_dial_info(struct ast_json* j_plan, struct ast_json* j_dl_list, E_DIAL_TYPE dial_type, const char* data_1, const char* data_2);
struct ast_json* create_json_for_dl_result(rb_dialing* dialing);


#endif /* SRC_DL_HANDLER_H_ */
