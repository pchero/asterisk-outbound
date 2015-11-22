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
#include "asterisk/config.h"
#include "asterisk/utils.h"

#include "res_outbound.h"
#include "db_handler.h"
#include "event_handler.h"
#include "ami_handler.h"
#include "dialing_handler.h"
#include "cli_handler.h"


#include <stdbool.h>
#include <mysql/mysql.h>
#include <errno.h>
#include <signal.h>

#define MODULE_DESCRIPTION  "Outbound manager for Asterisk"

struct ast_json* g_cfg = NULL;
MYSQL* g_mydb;
pthread_t pth_outbound;

/*!
 * \brief Load res_snmp.conf config file
 * \return 1 on load, 0 file does not exist
*/
static int load_config(void)
{
    struct ast_variable *var;
    struct ast_config *cfg;
    struct ast_json* j_tmp;
    struct ast_flags config_flags = { 0 };
    char *cat;

    if(g_cfg != NULL) {
        ast_json_unref(g_cfg);
    }
    g_cfg = ast_json_object_create();

    cfg = ast_config_load("res_outbound.conf", config_flags);
    if (cfg == CONFIG_STATUS_FILEMISSING || cfg == CONFIG_STATUS_FILEINVALID) {
        ast_log(LOG_WARNING, "Could not load res_snmp.conf\n");
        return false;
    }

    cat = ast_category_browse(cfg, NULL);
    while (cat) {
        var = ast_variable_browse(cfg, cat);

        if (strcasecmp(cat, "general") == 0) {
            if(ast_json_object_get(g_cfg, "general") == NULL) {
                ast_json_object_set(g_cfg, "general", ast_json_object_create());
            }
            j_tmp = ast_json_object_get(g_cfg, "general");

            while (var) {
                if (strcasecmp(var->name, "db_host") == 0) {
                    ast_json_object_set(j_tmp, "db_host", ast_json_string_create(var->value));
                }
                else if (strcasecmp(var->name, "db_port") == 0) {
                    ast_json_object_set(j_tmp, "db_port", ast_json_string_create(var->value));
                }
                else if (strcasecmp(var->name, "db_user") == 0) {
                    ast_json_object_set(j_tmp, "db_user", ast_json_string_create(var->value));
                }
                else if (strcasecmp(var->name, "db_pass") == 0) {
                    ast_json_object_set(j_tmp, "db_pass", ast_json_string_create(var->value));
                }
                else if (strcasecmp(var->name, "db_name") == 0) {
                    ast_json_object_set(j_tmp, "db_name", ast_json_string_create(var->value));
                }
                else {
                    ast_log(LOG_ERROR, "Unrecognized variable. category[%s], name[%s], value[%s]\n",
                            cat, var->name, var->value);
                    ast_config_destroy(cfg);
                    return 1;
                }
                var = var->next;
            }
        } else {
            ast_log(LOG_ERROR, "Unrecognized category. category[%s]\n", cat);
            ast_config_destroy(cfg);
            return false;
        }
        cat = ast_category_browse(cfg, cat);
    }
    ast_config_destroy(cfg);
    return true;
}

static int init_module(void)
{
    int ret;
    int port;
    struct ast_json* j_general;

    // connect to db
    j_general = ast_json_object_get(g_cfg, "general");

    port = 5036;
    if(ast_json_string_get(ast_json_object_get(j_general, "db_port")) != NULL) {
        port = atoi(ast_json_string_get(ast_json_object_get(j_general, "db_port")));
    }

    ret = db_connect(
            ast_json_string_get(ast_json_object_get(j_general, "db_host")),
            port,
            ast_json_string_get(ast_json_object_get(j_general, "db_user")),
            ast_json_string_get(ast_json_object_get(j_general, "db_pass")),
            ast_json_string_get(ast_json_object_get(j_general, "db_name"))
            );
    if(ret == false) {
        ast_log(LOG_ERROR, "Could not connect to db.\n");
        return false;
    }
    ast_log(LOG_VERBOSE, "Initiated module.\n");

    return true;
}

static void release_module(void)
{
    db_exit();
    ast_json_unref(g_cfg);

    ast_log(LOG_VERBOSE, "Released module.\n");
}

static int unload_module(void)
{
    release_module();
    stop_outbound();
    term_ami_handle();
    term_cli_handler();

    pthread_cancel(pth_outbound);
    pthread_kill(pth_outbound, SIGURG);
    pthread_join(pth_outbound, NULL);

    ast_log(LOG_NOTICE, "Released res_outbound.\n");
    return 0;
}

static int load_module(void)
{
    int ret;

    ret = load_config();
    if(ret == false) {
        ast_log(LOG_ERROR, "Could not load config file.");
        unload_module();
        return AST_MODULE_LOAD_DECLINE;
    }

    ret = init_module();
    if(ret == false) {
        ast_log(LOG_ERROR, "Could not connect to db.\n");
        unload_module();
        return AST_MODULE_LOAD_DECLINE;
    }

    ret = init_rb_dialing();
    if(ret == false) {
        ast_log(LOG_ERROR, "Could not initiate dialing handler.\n");
        unload_module();
        return AST_MODULE_LOAD_DECLINE;
    }

    ret = init_ami_handle();
    if(ret == false) {
        ast_log(LOG_ERROR, "Could not initiate AMI handler.\n");
        unload_module();
        return AST_MODULE_LOAD_DECLINE;
    }

    ret = init_cli_handler();
    if(ret == false) {
        ast_log(LOG_ERROR, "Could not initiate AMI handler.\n");
        unload_module();
        return AST_MODULE_LOAD_DECLINE;
    }

    ret = ast_pthread_create_background(&pth_outbound, NULL, (void*)&run_outbound, NULL);
    if(ret > 0)
    {
        ast_log(LOG_ERROR, "Unable to launch thread for outbound. err[%d:%s]\n", errno, strerror(errno));
        unload_module();
        return AST_MODULE_LOAD_FAILURE;
    }

    return AST_MODULE_LOAD_SUCCESS;
}

static int reload_module(void)
{
    int ret;

    ret = unload_module();
    if(ret == false) {
        return AST_MODULE_LOAD_DECLINE;
    }

    ret = load_module();
    if(ret == false) {
        return AST_MODULE_LOAD_DECLINE;
    }

    return AST_MODULE_LOAD_SUCCESS;
}


//AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "Outbound manager");

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_LOAD_ORDER, "Outbound manager",
        .load = load_module,
        .unload = unload_module,
        .reload = reload_module,
        .load_pri = AST_MODPRI_DEFAULT,
        .support_level = AST_MODULE_SUPPORT_CORE,
        );


