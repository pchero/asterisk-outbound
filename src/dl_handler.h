/*
 * dl_handler.h
 *
 *  Created on: Nov 29, 2015
 *	  Author: pchero
 */

#ifndef SRC_DL_HANDLER_H_
#define SRC_DL_HANDLER_H_

#include "dialing_handler.h"

#include <stdbool.h>

/**
 * dl_list.status
 */
typedef enum _E_DL_STATUS_T
{
	E_DL_IDLE	   = 0,			//!< idle
	E_DL_DIALING	= 1,		//!< dialing
	E_DL_RESERVED   = 2,	//!< reserved for preview dialing
//	E_DL_QUEUEING   = 2,	///< not in used.
} E_DL_STATUS_T;

typedef enum _E_DL_USE
{
	E_DL_USE_NO = 0,
	E_DL_USE_OK = 1,
} E_DL_USE;

bool create_dlma(const struct ast_json* j_dlma);
bool update_dlma(const struct ast_json* j_dlma);
bool delete_dlma(const char* uuid);

char* create_dl_list(struct ast_json* j_dl);
bool update_dl_list(struct ast_json* j_dl);
bool delete_dl_list(const char* uuid);

int get_current_dialing_dl_cnt(const char* camp_uuid, const char* dl_table);
int get_dial_num_point(struct ast_json* j_dl_list, struct ast_json* j_plan);
int get_dial_try_cnt(struct ast_json* j_dl_list, int dial_num_point);

struct ast_json* get_dlmas_all(void);
struct ast_json* get_dlma(const char* uuid);
struct ast_json* get_dl_list(const char* uuid);
struct ast_json* get_dl_lists(const char* dlma_uuid, int count);
int get_dl_list_cnt_total(struct ast_json* j_dlma);
int get_dl_list_cnt_finshed(struct ast_json* j_dlma, struct ast_json* j_plan);
int get_dl_list_cnt_available(struct ast_json* j_dlma, struct ast_json* j_plan);
int get_dl_list_cnt_dialing(struct ast_json* j_dlma);
int get_dl_list_cnt_tried(struct ast_json* j_dlma);


struct ast_json* get_dl_available_predictive(struct ast_json* j_dlma, struct ast_json* j_plan);
bool is_endable_dl_list(struct ast_json* j_dlma, struct ast_json* j_plan);
void clear_dl_list_dialing(const char* uuid);

struct ast_json* create_dial_info(struct ast_json* j_plan, struct ast_json* j_dl_list, struct ast_json* j_dest);
struct ast_json* create_json_for_dl_result(rb_dialing* dialing);


#endif /* SRC_DL_HANDLER_H_ */
