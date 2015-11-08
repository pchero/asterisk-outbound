/*
 * res_outbound.c
 *
 *  Created on: Nov 7, 2015
 *      Author: pchero
 */

/*** MODULEINFO
    <depend>mysqlclient</depend>
    <support_level>extended</support_level>
***/

#include "asterisk.h"

ASTERISK_FILE_VERSION(__FILE__, "$Revision: $")

#include "asterisk/module.h"

#include "outbound/res_outbound.h"

#include <stdbool.h>

#define MODULE_DESCRIPTION  "Outbound manager for Asterisk"


static int unload_module(void)
{
    return 0;
}

static int load_module(void)
{
    int ret;
    ret = db_connect("localhost", 3306, "root", "passwd", "test");
    if(ret == false) {
        ast_log(LOG_ERROR, "Could not connect to db.\n");
        return AST_MODULE_LOAD_DECLINE;
    }

    return AST_MODULE_LOAD_SUCCESS;
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "Outbound manager");
