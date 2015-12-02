/*
 * dl_handler.h
 *
 *  Created on: Nov 29, 2015
 *      Author: pchero
 */

#ifndef SRC_DL_HANDLER_H_
#define SRC_DL_HANDLER_H_

typedef enum _E_DL_STATUS_T
{
    E_DL_IDLE       = 0,
    E_DL_DIALING    = 1,
//    E_DL_QUEUEING   = 2,    ///< not in used.

} E_DL_STATUS_T;

int create_dlma(struct ast_json* j_dlma);
int update_dlma(struct ast_json* j_dlma);
int delete_dlma(const char* uuid);

int get_current_dialing_dl_cnt(const char* camp_uuid, const char* dl_table);
int get_dial_num_point(struct ast_json* j_dl_list, struct ast_json* j_plan);
int get_dial_try_cnt(struct ast_json* j_dl_list, int dial_num_point);

struct ast_json* get_dlmas_all(void);
struct ast_json* get_dlma(const char* uuid);
struct ast_json* get_dl_lists(struct ast_json* j_dlma, int count);
struct ast_json* get_dl_list(const char* uuid);
struct ast_json* get_dl_available_predictive(struct ast_json* j_dlma, struct ast_json* j_plan);
int update_dl_list(struct ast_json* j_dlinfo);
int check_more_dl_list(struct ast_json* j_dlma, struct ast_json* j_plan);
void clear_dl_list_dialing(const char* uuid);


#endif /* SRC_DL_HANDLER_H_ */
