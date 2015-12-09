/*
 * dialing_handler.h
 *
 *  Created on: Nov 21, 2015
 *      Author: pchero
 */

#ifndef SRC_DIALING_HANDLER_H_
#define SRC_DIALING_HANDLER_H_

#include "asterisk/json.h"

#include <stdbool.h>

typedef enum _E_DIALING_STATUS_T
{
    E_DIALING_NONE = 0,                     ///< None state
    E_DIALING_ORIGINATE_REQUEST     = 1,
    E_DIALING_ORIGINATE_RESPONSE,
    E_DIALING_HANGUP,
} E_DIALING_STATUS_T;

typedef struct _rb_dialing{
    char* uuid;                 ///< dialing uuid(channel's unique id)
    E_DIALING_STATUS_T status;  ///< dialing status

    char* tm_create;
    char* tm_update;
    char* tm_delete;
    struct timespec timeptr_update; ///< timestamp for timeout

    struct ast_json* j_res;     ///< result info

    struct ast_json* j_chan;    ///< channel info.
    struct ast_json* j_queues;  ///< queue info.(json array)
    struct ast_json* j_agents;  ///< agents info. Who had a talk with. (json array)
} rb_dialing;

int init_rb_dialing(void);
rb_dialing* rb_dialing_create(const char* dialing_uuid, struct ast_json* j_camp, struct ast_json* j_plan, struct ast_json* j_dlma, struct ast_json* j_dl);
void rb_dialing_destory(rb_dialing* dialing);

rb_dialing* rb_dialing_find_dl_uuid(const char* chan);
rb_dialing* rb_dialing_find_chan_uuid(const char* chan);
bool rb_dialing_is_exist_uuid(const char* uuid);
struct ao2_iterator rb_dialing_iter_init(void);
struct ast_json* rb_dialing_get_all_for_cli(void);

bool rb_dialing_update_agent_append(rb_dialing* dialing, struct ast_json* j_evt);
bool rb_dialing_update_agent_update(rb_dialing* dialing, struct ast_json* j_evt);
bool rb_dialing_update_queue_append(rb_dialing* dialing, struct ast_json* j_evt);
bool rb_dialing_update_res_update(rb_dialing* dialing, struct ast_json* j_res);
bool rb_dialing_update_chan_update(rb_dialing* dialing, struct ast_json* j_evt);
bool rb_dialing_update_status(rb_dialing* dialing, E_DIALING_STATUS_T status);


#endif /* SRC_DIALING_HANDLER_H_ */
