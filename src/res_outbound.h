/*
 * res_outbound.h
 *
 *  Created on: Nov 7, 2015
 *	  Author: pchero
 */

#ifndef SRC_RES_OUTBOUND_H_
#define SRC_RES_OUTBOUND_H_

#include "asterisk.h"
#include "asterisk/json.h"

typedef struct _app {
	struct ast_json*   j_conf;	///< config
} app;

extern app* g_app;	// global application info

#endif /* SRC_RES_OUTBOUND_H_ */
