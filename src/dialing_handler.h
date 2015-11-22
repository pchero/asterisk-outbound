/*
 * dialing_handler.h
 *
 *  Created on: Nov 21, 2015
 *      Author: pchero
 */

#ifndef SRC_DIALING_HANDLER_H_
#define SRC_DIALING_HANDLER_H_

#include "asterisk/json.h"

typedef struct _rb_dialing{
    char* uuid;                 ///< dialing uuid(channel's unique id)

    struct ast_json* j_camp;    ///< campaign
    struct ast_json* j_plan;    ///< plan
    struct ast_json* j_dlma;    ///< dlma
    struct ast_json* j_dl;      ///< dl(dialing) info.

    struct ast_json* j_chan;    ///< channel info.
    struct ast_json* j_queues;  ///< queue info.(json array)
    struct ast_json* j_agents;  ///< agents info. Who had a talk with. (json array)
} rb_dialing;

int init_rb_dialing(void);
rb_dialing* rb_dialing_create(struct ast_json* j_camp, struct ast_json* j_plan, struct ast_json* j_dlma, struct ast_json* j_dl);
void rb_dialing_destory(rb_dialing* dialing);

rb_dialing* rb_dialing_find_uuid_dl(const char* chan);
rb_dialing* rb_dialing_find_uuid_chan(const char* chan);
struct ao2_iterator rb_dialing_iter_init(void);
struct ast_json* rb_dialing_get_all_for_cli(void);

#endif /* SRC_DIALING_HANDLER_H_ */
