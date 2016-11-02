/*
 * application_handler.h
 *
 *  Created on: Nov 2, 2016
 *      Author: pchero
 */

#ifndef SRC_APPLICATION_HANDLER_H_
#define SRC_APPLICATION_HANDLER_H_

#include "asterisk.h"
#include "asterisk/app.h"


int init_application_handler(void);
bool term_application_handler(void);

#endif /* SRC_APPLICATION_HANDLER_H_ */
