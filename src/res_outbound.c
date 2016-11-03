/*
 * res_outbound.c
 *
 *  Created on: Nov 7, 2015
 *	  Author: pchero
 */

/*** MODULEINFO
	<support_level>extended</support_level>
***/

#include "asterisk.h"

#include "asterisk/module.h"
#include "asterisk/config.h"
#include "asterisk/utils.h"

#include "res_outbound.h"
#include "db_handler.h"
#include "event_handler.h"
#include "ami_handler.h"
#include "dialing_handler.h"
#include "cli_handler.h"
#include "utils.h"
#include "application_handler.h"


#include <stdbool.h>
#include <mysql/mysql.h>
#include <errno.h>
#include <signal.h>

#define MODULE_DESCRIPTION  "Outbound manager for Asterisk"

MYSQL* g_mydb;
pthread_t pth_outbound;
app* g_app;

/*!
 * \brief Load res_snmp.conf config file
 * \return 1 on load, 0 file does not exist
*/
static int load_config(void)
{
	struct ast_variable *var;
	struct ast_config *cfg;
	struct ast_json* j_tmp;
	struct ast_json* j_conf;
	struct ast_flags config_flags = { 0 };
	char *cat;

	j_conf = ast_json_object_create();

	cfg = ast_config_load("res_outbound.conf", config_flags);
	if (cfg == CONFIG_STATUS_FILEMISSING || cfg == CONFIG_STATUS_FILEINVALID) {
		ast_log(LOG_WARNING, "Could not load res_outbound.conf.\n");
		return false;
	}

	cat = ast_category_browse(cfg, NULL);
	while (cat) {

		if(ast_json_object_get(j_conf, cat) == NULL) {
			ast_json_object_set(j_conf, cat, ast_json_object_create());
		}
		j_tmp = ast_json_object_get(j_conf, cat);

		var = ast_variable_browse(cfg, cat);
		while(var) {
			ast_json_object_set(j_tmp, var->name, ast_json_string_create(var->value));
			ast_log(LOG_VERBOSE, "Loading conf. name[%s], value[%s]\n", var->name, var->value);
			var = var->next;
		}
		cat = ast_category_browse(cfg, cat);
	}
	ast_config_destroy(cfg);

	if(g_app->j_conf != NULL) {
		AST_JSON_UNREF(g_app->j_conf);
	}
	g_app->j_conf = j_conf;

	return true;
}

static int init_module(void)
{
	int ret;

	ret = db_init();
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
	AST_JSON_UNREF(g_app->j_conf);
	ast_free(g_app);

	ast_log(LOG_VERBOSE, "Released module.\n");
}

static int unload_module(void)
{
	term_ami_handle();
	term_cli_handler();
	term_application_handler();
	stop_outbound();
	usleep(10000);
	release_module();

	pthread_cancel(pth_outbound);
	pthread_kill(pth_outbound, SIGURG);
	pthread_join(pth_outbound, NULL);

	ast_log(LOG_NOTICE, "unload res_outbound.\n");
	return 0;
}

static int load_module(void)
{
	int ret;

	if(g_app != NULL) {
		AST_JSON_UNREF(g_app->j_conf);
		ast_free(g_app);
	}
	g_app = ast_malloc(sizeof(app));
	g_app->j_conf = NULL;

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
		ast_log(LOG_ERROR, "Could not initiate ami handler.\n");
		unload_module();
		return AST_MODULE_LOAD_DECLINE;
	}

	ret = init_cli_handler();
	if(ret == false) {
		ast_log(LOG_ERROR, "Could not initiate cli handler.\n");
		unload_module();
		return AST_MODULE_LOAD_DECLINE;
	}

	ret = init_application_handler();
	if(ret == false) {
		ast_log(LOG_ERROR, "Could not initiate application handler.\n");
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
	ast_log(LOG_NOTICE, "res_outbound doesn't support reload. Please do unload/load manualy.\n");
	return AST_MODULE_RELOAD_SUCCESS;
}


//AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "Outbound manager");

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_LOAD_ORDER, "Outbound manager",
		.load = load_module,
		.unload = unload_module,
		.reload = reload_module,
		.load_pri = AST_MODPRI_DEFAULT,
		.support_level = AST_MODULE_SUPPORT_CORE,
		);


