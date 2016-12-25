/*
 * cli_handler.c
 *
 *  Created on: Nov 22, 2015
 *	  Author: pchero
 */

#include "asterisk.h"
#include "asterisk/manager.h"
#include "asterisk/module.h"
#include "asterisk/json.h"
#include "asterisk/xml.h"
#include "asterisk/cli.h"
#include "asterisk/config.h"

#include <stdbool.h>

#include "cli_handler.h"
#include "event_handler.h"
#include "dialing_handler.h"
#include "campaign_handler.h"
#include "dl_handler.h"
#include "plan_handler.h"
#include "queue_handler.h"
#include "destination_handler.h"
#include "utils.h"

/*** DOCUMENTATION
	<manager name="OutCampaignCreate" language="en_US">
		<synopsis>
			Create outbound campaign.
		</synopsis>
		<syntax>
			<xi:include xpointer="xpointer(/docs/manager[@name='Login']/syntax/parameter[@name='ActionID'])" />
			<parameter name="Name" required="true">
				<para>The name of campaign.</para>
			</parameter>
		</syntax>
		<description>
			<para>Changes the dtmfmode for a SIP call.</para>
		</description>
	</manager>
	<managerEvent language="en_US" name="OutCampaignCreate">
		<managerEventInstance class="EVENT_FLAG_MESSAGE">
			<synopsis>Raised when SIPQualifyPeer has finished qualifying the specified peer.</synopsis>
			<syntax>
				<parameter name="Name">
					<para>The name of the peer.</para>
				</parameter>
			</syntax>
			<see-also>
			</see-also>
		</managerEventInstance>
	</managerEvent>
***/


static char* get_variables(const struct message *m);


#define CAMPS_FORMAT2 "%-36.36s %-10.10s %-20.20s %-10.10s %-10.10s %-10.10s %-10.10s\n"
#define CAMPS_FORMAT3 "%-36.36s %-10.10s %-20.20s %10"PRIdMAX" %-10.10s %-10.10s %-10.10s\n"

static char* _out_show_campaigns(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
	struct ast_json* j_res;
	struct ast_json* j_tmp;
	int size;
	int i;

	j_res = get_campaigns_all();

	if (!s) {
		/* Normal list */
		ast_cli(fd, CAMPS_FORMAT2, "Uuid", "Name", "Detail", "Status", "Plan", "Dlma", "Dest");
	}

	size = ast_json_array_size(j_res);
	for(i = 0; i < size; i++) {
		j_tmp = ast_json_array_get(j_res, i);
		if(j_tmp == NULL) {
			continue;
		}
		ast_cli(fd, CAMPS_FORMAT3,
				ast_json_string_get(ast_json_object_get(j_tmp, "uuid")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "name")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "detail")) ? : "",
				ast_json_integer_get(ast_json_object_get(j_tmp, "status")),
				ast_json_string_get(ast_json_object_get(j_tmp, "plan")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "dlma")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "dest")) ? : ""
				);
	}
	AST_JSON_UNREF(j_res);

	return CLI_SUCCESS;
}

/*! \brief CLI for show campaigns.
 */
static char *out_show_campaigns(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
//	struct __show_chan_arg arg = { .fd = a->fd, .numchans = 0 };

	if (cmd == CLI_INIT) {
		e->command = "out show campaigns";
		e->usage =
			"Usage: out show campaigns\n"
			"	   Lists all currently registered campaigns.\n";
		return NULL;
	} else if (cmd == CLI_GENERATE) {
		return NULL;
	}
	return _out_show_campaigns(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

static char* _out_show_campaign(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
	struct ast_json* j_res;
	char* tmp;

	j_res = get_campaign(argv[3]);
	if(j_res == NULL) {
		ast_cli(fd, "Campaign %s not found.\n", argv[3]);
		return CLI_FAILURE;
	}

	if(!s) {
		ast_cli(fd, "Campaign detail info. camp-uuid[%s]\n\n", argv[3]);
	}

	tmp = ast_json_dump_string_format(j_res, AST_JSON_PRETTY);
	ast_cli(fd, "%s\n", tmp);
	ast_json_free(tmp);
	AST_JSON_UNREF(j_res);

	return CLI_SUCCESS;
}

static char* _out_set_campaign(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
	// out set campaign <uuid> <key> <value>
	// out set status {start|starting|stop|stopping|pause|pausing} on <uuid-campaign>
	int ret;
	const char* uuid;
	const char* value;
	E_CAMP_STATUS_T status;

	value = argv[3];
	uuid = argv[5];

	if((uuid == NULL) || (value == NULL)) {
		return NULL;
	}

	if(strcmp(value, "stop") == 0) {
		status = E_CAMP_STOPPING;
	}
	else if(strcmp(value, "stopping") == 0) {
		status = E_CAMP_STOPPING;
	}
	else if(strcmp(value, "start") == 0) {
		status = E_CAMP_STARTING;
	}
	else if(strcmp(value, "starting") == 0) {
		status = E_CAMP_STARTING;
	}
	else if(strcmp(value, "pause") == 0) {
		status = E_CAMP_PAUSING;
	}
	else if(strcmp(value, "pausing") == 0) {
		status = E_CAMP_PAUSING;
	}
	else {
		// wrong status value.
		ast_cli(fd, "Wrong status value. value[%s]\n", value);
		ast_cli(fd, "\n");
		return CLI_SUCCESS;
	}

	ret = update_campaign_status(uuid, status);
	if(ret == false) {
		ast_cli(fd, "Could not set campaign status. uuid[%s]\n", uuid);
		return CLI_FAILURE;
	}
	return CLI_SUCCESS;
}

/*! \brief CLI for set campaigns.
 */
static char *out_set_campaign(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	if (cmd == CLI_INIT) {
		e->command = "out set status {start|starting|stop|stopping|pause|pausing} on";
		e->usage =
			"Usage: out set status {start|starting|stop|stopping|pause|pausing} on <uuid-campaign>\n"
			"	   Set campaign's status.\n";
		return NULL;
	} else if (cmd == CLI_GENERATE) {
		return NULL;
	}
	return _out_set_campaign(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

/*!
 * \brief CLI for show campaign.
 */
static char *out_show_campaign(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	if (cmd == CLI_INIT) {
		e->command = "out show campaign";
		e->usage =
			"Usage: out show campaign <camp-uuid>\n"
			"	   Show detail given campaign info.\n";
		return NULL;
	} else if (cmd == CLI_GENERATE) {
		return NULL;
	}
	return _out_show_campaign(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

#define PLANS_FORMAT2 "%-36.36s %-20.20s %-20.20s %-8.8s %-11.11s %-10.10s %-10.10s\n"
#define PLANS_FORMAT3 "%-36.36s %-20.20s %-20.20s %8"PRIdMAX" %11"PRIdMAX" %-10.10s %-10.10s\n"

static char* _out_show_plans(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
	struct ast_json* j_res;
	struct ast_json* j_tmp;
	int size;
	int i;

	j_res = get_plans_all();

	if (!s) {
		/* Normal list */
		ast_cli(fd, PLANS_FORMAT2, "Uuid", "Name", "Detail", "DialMode", "DialTimeout", "TrunkName", "TechName");
	}

	size = ast_json_array_size(j_res);
	for(i = 0; i < size; i++) {
		j_tmp = ast_json_array_get(j_res, i);
		if(j_tmp == NULL) {
			continue;
		}
		ast_cli(fd, PLANS_FORMAT3,
				ast_json_string_get(ast_json_object_get(j_tmp, "uuid")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "name")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "detail")) ? : "",
				ast_json_integer_get(ast_json_object_get(j_tmp, "dial_mode")),
				ast_json_integer_get(ast_json_object_get(j_tmp, "dial_timeout")),
				ast_json_string_get(ast_json_object_get(j_tmp, "trunk_name")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "tech_name")) ? : ""
				);
	}
	AST_JSON_UNREF(j_res);

	return CLI_SUCCESS;
}

/*! \brief CLI for show plans.
 */
static char *out_show_plans(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{

	if (cmd == CLI_INIT) {
		e->command = "out show plans";
		e->usage =
			"Usage: out show plans\n"
			"	   Lists all currently registered plans.\n";
		return NULL;
	} else if (cmd == CLI_GENERATE) {
		return NULL;
	}
	return _out_show_plans(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

static char* _out_show_plan(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
	struct ast_json* j_res;
	char* tmp;

	// get plan info
	j_res = get_plan(argv[3]);
	if(j_res == NULL) {
		ast_cli(fd, "Plan %s not found.\n", argv[3]);
		return CLI_FAILURE;
	}

	if(!s) {
		ast_cli(fd, "Plan detail info. plan-uuid[%s]\n\n", argv[3]);
	}

	tmp = ast_json_dump_string_format(j_res, AST_JSON_PRETTY);
	ast_cli(fd, "%s\n", tmp);
	ast_json_free(tmp);
	AST_JSON_UNREF(j_res);

	return CLI_SUCCESS;
}

/*! \brief CLI for show plans.
 */
static char *out_show_plan(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{

	if (cmd == CLI_INIT) {
		e->command = "out show plan";
		e->usage =
			"Usage: out show plan <plan-uuid>\n"
			"	   Show detail given plan info.\n";
		return NULL;
	} else if (cmd == CLI_GENERATE) {
		return NULL;
	}
	return _out_show_plan(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

#define DLMAS_FORMAT2 "%-36.36s %-20.20s %-20.20s %-30.30s\n"
#define DLMAS_FORMAT3 "%-36.36s %-20.20s %-20.20s %-30.30s\n"

static char* _out_show_dlmas(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
	struct ast_json* j_res;
	struct ast_json* j_tmp;
	int size;
	int i;

	j_res = get_dlmas_all();

	if (!s) {
		/* Normal list */
		ast_cli(fd, DLMAS_FORMAT2, "Uuid", "Name", "Detail", "Table");
	}

	size = ast_json_array_size(j_res);
	for(i = 0; i < size; i++) {
		j_tmp = ast_json_array_get(j_res, i);
		if(j_tmp == NULL) {
			continue;
		}
		ast_cli(fd, DLMAS_FORMAT3,
				ast_json_string_get(ast_json_object_get(j_tmp, "uuid")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "name")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "detail")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "dl_table")) ? : ""
				);
	}
	AST_JSON_UNREF(j_res);

	return CLI_SUCCESS;
}

/*!
 * \brief CLI for show DLMAS.
 */
static char *out_show_dlmas(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{

	if (cmd == CLI_INIT) {
		e->command = "out show dlmas";
		e->usage =
			"Usage: out show dlmas\n"
			"	   Lists all currently registered dlmas.\n";
		return NULL;
	} else if (cmd == CLI_GENERATE) {
		return NULL;
	}
	return _out_show_dlmas(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

static char* _out_show_dlma(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
	struct ast_json* j_res;
	char* tmp;

	j_res = get_dlma(argv[3]);
	if(j_res == NULL) {
		ast_cli(fd, "Dlma %s not found.\n", argv[3]);
		return CLI_FAILURE;
	}

	if(!s) {
		ast_cli(fd, "Dlma detail info. dlma-uuid[%s]\n\n", argv[3]);
	}

	tmp = ast_json_dump_string_format(j_res, AST_JSON_PRETTY);
	ast_cli(fd, "%s\n", tmp);
	ast_json_free(tmp);
	AST_JSON_UNREF(j_res);

	return CLI_SUCCESS;
}

/*!
 * \brief CLI for show DLMAS.
 */
static char *out_show_dlma(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{

	if (cmd == CLI_INIT) {
		e->command = "out show dlma";
		e->usage =
			"Usage: out show dlma <dlma-uuid>\n"
			"	   Show detail given dlma info.\n";
		return NULL;
	} else if (cmd == CLI_GENERATE) {
		return NULL;
	}
	return _out_show_dlma(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}


#define DIALINGS_FORMAT2 "%-36.36s %-6.6s %-20.20s %-20.20s %-10.10s %-10.10s %-10.10s %-10.10s %-10.10s\n"
#define DIALINGS_FORMAT3 "%-36.36s %6"PRIdMAX" %-20.20s %-20.20s %-10.10s %-10.10s %-10.10s %-10.10s %-10.10s\n"

static char* _out_show_dialings(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
	struct ast_json* j_res;
	struct ast_json* j_tmp;
	int size;
	int i;

	j_res = rb_dialing_get_all_for_cli();

	if (!s) {
		/* Normal list */
		ast_cli(fd, DIALINGS_FORMAT2, "Uuid", "Status", "Channel", "Addr", "Camp", "Plan", "Dlma", "Dest", "Dl");
	}

	size = ast_json_array_size(j_res);
	for(i = 0; i < size; i++) {
		j_tmp = ast_json_array_get(j_res, i);
		if(j_tmp == NULL) {
			continue;
		}
		ast_cli(fd, DIALINGS_FORMAT3,
				ast_json_string_get(ast_json_object_get(j_tmp, "dialing_uuid")) ? : "",
				ast_json_integer_get(ast_json_object_get(j_tmp, "status")),
				ast_json_string_get(ast_json_object_get(j_tmp, "dial_channel")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "dial_addr")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "camp_uuid")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "plan_uuid")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "dlma_uuid")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "dest_uuid")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "dl_list_uuid")) ? : ""
				);
	}
	AST_JSON_UNREF(j_res);

	return CLI_SUCCESS;
}

/*! \brief CLI for show plans.
 */
static char *out_show_dialings(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{

	if (cmd == CLI_INIT) {
		e->command = "out show dialings";
		e->usage =
			"Usage: out show dialings\n"
			"	   Lists all currently on service dialings.\n";
		return NULL;
	} else if (cmd == CLI_GENERATE) {
		return NULL;
	}
	return _out_show_dialings(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

static char* _out_show_dialing(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
	struct ast_json* j_res;
	char* tmp;

	if(argc != 4) {
		return NULL;
	}

	j_res = rb_dialing_get_info_for_cli(argv[3]);
	if(j_res == NULL) {
		ast_cli(fd, "Dialing %s not found.\n", argv[3]);
		return CLI_FAILURE;
	}

	if(!s) {
		ast_cli(fd, "Dialing detail info. dialing-uuid[%s]\n\n", argv[3]);
	}

	tmp = ast_json_dump_string_format(j_res, AST_JSON_PRETTY);
	ast_cli(fd, "%s\n", tmp);
	ast_json_free(tmp);
	AST_JSON_UNREF(j_res);

	return CLI_SUCCESS;
}

/*! \brief CLI for show plans.
 */
static char *out_show_dialing(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{

	if (cmd == CLI_INIT) {
		e->command = "out show dialing";
		e->usage =
			"Usage: out show dialing <dialing-uuid>\n"
				"	   Show detail given dialing info.\n";
		return NULL;
	} else if (cmd == CLI_GENERATE) {
		return NULL;
	}
	return _out_show_dialing(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

#define DL_LIST_FORMAT2 "%-36.36s %-10.10s %-20.20s %-20.20s %-20.20s %-20.20s %-20.20s %-20.20s %-20.20s %-20.20s %-20.20s\n"
#define DL_LIST_FORMAT3 "%-36.36s %-10.10s %-20.20s %-20.20s %-20.20s %-20.20s %-20.20s %-20.20s %-20.20s %-20.20s %-20.20s\n"

static char* _out_show_dlma_list(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
	// out show dlma list <dlma-uuid> <count=100>
	const char* uuid;
	int count;
	struct ast_json* j_dls;
	struct ast_json* j_tmp;
	int i;
	int size;

	uuid = argv[4];
	if(uuid == NULL) {
		return NULL;
	}

	count = 0;
	if(argv[5] != NULL) {
		count = atoi(argv[5]);
	}
	if(count <= 0) {
		count = 100;
	}

	j_dls = get_dl_lists(uuid, count);
	if(j_dls == NULL) {
		ast_cli(fd, "Dlma lists %s not found.\n", argv[3]);
		return CLI_FAILURE;
	}

	if(!s) {
		/* Normal list */
	  ast_cli(fd, DL_LIST_FORMAT2, "Uuid", "Name", "Detail", "Num1", "Num2", "Num3", "Num4", "Num5", "Num6", "Num7", "Num8");
	}

	size = ast_json_array_size(j_dls);
	for(i = 0; i < size; i++) {
		j_tmp = ast_json_array_get(j_dls, i);
		if(j_tmp == NULL) {
			continue;
		}

		ast_cli(fd, DL_LIST_FORMAT3,
				ast_json_string_get(ast_json_object_get(j_tmp, "uuid"))? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "name"))? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "detail"))? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "number_1"))? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "number_2"))? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "number_3"))? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "number_4"))? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "number_5"))? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "number_6"))? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "number_7"))? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "number_8"))? : ""
				);
	}

	AST_JSON_UNREF(j_dls);

	return CLI_SUCCESS;
}

/**
 * \brief CLI for show plans.
 */
static char *out_show_dlma_list(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{

	if (cmd == CLI_INIT) {
		e->command = "out show dlma list";
		e->usage =
			"Usage: out show dlma list <dlma-uuid> <count=100>\n"
			"	   Lists count of dial list.\n";
		return NULL;
	} else if (cmd == CLI_GENERATE) {
		return NULL;
	}
	return _out_show_dlma_list(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

#define DL_FORMAT2 "%-36.36s %-36.36s %-10.10s %-10.10s %-10.10s %-6.6s %-36.36s %-36.36s %-36.36s %-30.30s %-12.12s %-12.12s %-12.12s %-12.12s %-12.12s %-12.12s %-12.12s %-12.12s %-6.6s %-6.6s %-6.6s %-6.6s %-6.6s %-6.6s %-6.6s %-6.6s %-6.6s %-6.6s\n"
#define DL_FORMAT3 "%-36.36s %-36.36s %-10.10s %-10.10s %-10.10s %6ld %-36.36s %-36.36s %-36.36s %-30.30s %-12.12s %-12.12s %-12.12s %-12.12s %-12.12s %-12.12s %-12.12s %-12.12s %6ld %6ld %6ld %6ld %6ld %6ld %6ld %6ld %6ld %6ld\n"

static char* _out_show_dl(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
	struct ast_json* j_res;
	char* tmp;

	if(argc != 4) {
		return NULL;
	}

	j_res = get_dl_list(argv[3]);
	if(j_res == NULL) {
		ast_cli(fd, "Dl %s not found.\n", argv[3]);
		return CLI_FAILURE;
	}

	if(!s) {
		ast_cli(fd, "Dl detail info. dl-uuid[%s]\n\n", argv[3]);
	}

	tmp = ast_json_dump_string_format(j_res, AST_JSON_PRETTY);
	ast_cli(fd, "%s\n", tmp);
	ast_json_free(tmp);
	AST_JSON_UNREF(j_res);

	return CLI_SUCCESS;
}

/**
 *
 * @param e
 * @param cmd
 * @param a
 * @return
 */
static char *out_show_dls(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{

	if (cmd == CLI_INIT) {
		e->command = "out show dls";
		e->usage =
			"Usage: out show dls <dlma-uuid>\n"
			"	   synonym of \"out show dlma list <dlma-uuid> <count>\".\n";
		return NULL;
	} else if (cmd == CLI_GENERATE) {
		return NULL;
	}
	return _out_show_dlma_list(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

/**
 *
 * @param e
 * @param cmd
 * @param a
 * @return
 */
static char *out_show_dl(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{

	if (cmd == CLI_INIT) {
		e->command = "out show dl";
		e->usage =
			"Usage: out show dl <dl-uuid>\n"
			"	   Show detail given dl info.\n";
		return NULL;
	} else if (cmd == CLI_GENERATE) {
		return NULL;
	}
	return _out_show_dl(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

static char* _out_show_destination(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
	struct ast_json* j_res;
	char* tmp;

	if(argc != 4) {
		return NULL;
	}

	j_res = get_destination(argv[3]);
	if(j_res == NULL) {
		ast_cli(fd, "Destination %s not found.\n", argv[3]);
		return CLI_FAILURE;
	}

	if(!s) {
		ast_cli(fd, "Destination detail info. dest-uuid[%s]\n\n", argv[3]);
	}

	tmp = ast_json_dump_string_format(j_res, AST_JSON_PRETTY);
	ast_cli(fd, "%s\n", tmp);
	ast_json_free(tmp);
	AST_JSON_UNREF(j_res);

	return CLI_SUCCESS;
}

/**
 *
 * @param e
 * @param cmd
 * @param a
 * @return
 */
static char *out_show_destination(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{

	if (cmd == CLI_INIT) {
		e->command = "out show destination";
		e->usage =
			"Usage: out show destination <dest-uuid>\n"
			"	   Show detail given destination info.\n";
		return NULL;
	} else if (cmd == CLI_GENERATE) {
		return NULL;
	}
	return _out_show_destination(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

#define DESTS_FORMAT2 "%-36.36s %-10.10s %-20.20s %-5.5s %-10.10s %-10.10s %-10.10s %-10.10s\n"
#define DESTS_FORMAT3 "%-36.36s %-10.10s %-20.20s %5"PRIdMAX" %-10.10s %-10.10s %-10.10s %-10.10s\n"

static char* _out_show_destinations(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
	struct ast_json* j_res;
	struct ast_json* j_tmp;
	int size;
	int i;

	if(!s) {
		/* Normal list */
		ast_cli(fd, DESTS_FORMAT2, "Uuid", "Name", "Detail", "Type", "Exten", "Context", "Application", "Data");
	}

	j_res = get_destinations_all();

	size = ast_json_array_size(j_res);
	for(i = 0; i < size; i++) {
		j_tmp = ast_json_array_get(j_res, i);
		if(j_tmp == NULL) {
			continue;
		}
		ast_cli(fd, DESTS_FORMAT3,
				ast_json_string_get(ast_json_object_get(j_tmp, "uuid")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "name")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "detail")) ? : "",
				ast_json_integer_get(ast_json_object_get(j_tmp, "type")),
				ast_json_string_get(ast_json_object_get(j_tmp, "exten")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "context")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "application")) ? : "",
				ast_json_string_get(ast_json_object_get(j_tmp, "data")) ? : ""
				);
	}
	AST_JSON_UNREF(j_res);

	return CLI_SUCCESS;
}

/**
 *
 * @param e
 * @param cmd
 * @param a
 * @return
 */
static char *out_show_destinations(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{

	if (cmd == CLI_INIT) {
		e->command = "out show destinations";
		e->usage =
			"Usage: out show destinations\n"
			"	   Lists all registered destinations.\n";
		return NULL;
	} else if (cmd == CLI_GENERATE) {
		return NULL;
	}
	return _out_show_destinations(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}


static char* _out_create_campaign(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
//	out create campaign <name> <plan-uuid> <dlma-uuid> <queue-uuid> [detail]
	struct ast_json* j_camp;
	int ret;

	j_camp = ast_json_object_create();
	if(argc >= 7) { ast_json_object_set(j_camp, "detail", ast_json_string_create(argv[7])); }
	if(argc >= 6) { ast_json_object_set(j_camp, "dest", ast_json_string_create(argv[6])); }
	if(argc >= 5) { ast_json_object_set(j_camp, "dlma", ast_json_string_create(argv[5])); }
	if(argc >= 4) { ast_json_object_set(j_camp, "plan", ast_json_string_create(argv[4])); }
	if(argc >= 3) { ast_json_object_set(j_camp, "name", ast_json_string_create(argv[3])); }

	ret = create_campaign(j_camp);
	AST_JSON_UNREF(j_camp);
	if(ret == false) {
		return CLI_FAILURE;
	}

	return CLI_SUCCESS;
}

/*! \brief CLI for create campaigns.
 */
static char *out_create_campaign(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	if (cmd == CLI_INIT) {
		e->command = "out create campaign";
		e->usage =
			"Usage: out create campaign <name> <plan-uuid> <dlma-uuid> <queue-uuid> [detail] \n"
			"	   Create new campaign.\n";
		return NULL;
	} else if (cmd == CLI_GENERATE) {
		return NULL;
	}
	return _out_create_campaign(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

static char* _out_delete_campaign(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
//	out delete campaign <camp-uuid>
	int ret;

	if(argc != 4) {
		return CLI_SHOWUSAGE;
	}

	ret = delete_campaign(argv[3]);
	if(ret == false) {
		return CLI_FAILURE;
	}
	return CLI_SUCCESS;
}

static char *out_delete_campaign(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	if (cmd == CLI_INIT) {
		e->command = "out delete campaign";
		e->usage =
			"Usage: out delete campaign <camp-uuid>\n"
			"	   Delete the campaign.\n";
		return NULL;
	} else if (cmd == CLI_GENERATE) {
		return NULL;
	}
	return _out_delete_campaign(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

/**
 * Create AMI string for campaign
 * @param j_camp
 * @return
 */
static char* get_campaign_str(struct ast_json* j_camp)
{
	char* tmp;

	if(j_camp == NULL) {
		return NULL;
	}

	ast_asprintf(&tmp,
			"Uuid: %s\r\n"
			"Name: %s\r\n"
			"Detail: %s\r\n"
			"Status: %"PRIdMAX"\r\n"
			"Plan: %s\r\n"
			"Dlma: %s\r\n"
			"Dest: %s\r\n"

			"ScMode: %"PRIdMAX"\r\n"
			"ScTimeStart: %s\r\n"
			"ScTimeEnd: %s\r\n"
			"ScDateStart: %s\r\n"
			"ScDateEnd: %s\r\n"
			"ScDateList: %s\r\n"
			"ScDateListExcept: %s\r\n"
			"ScDayList: %s\r\n"

			"TmCreate: %s\r\n"
			"TmDelete: %s\r\n"
			"TmUpdate: %s\r\n",

			ast_json_string_get(ast_json_object_get(j_camp, "uuid"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_camp, "name"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_camp, "detail"))? : "<unknown>",
			ast_json_integer_get(ast_json_object_get(j_camp, "status")),
			ast_json_string_get(ast_json_object_get(j_camp, "plan"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_camp, "dlma"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_camp, "dest"))? : "<unknown>",

			ast_json_integer_get(ast_json_object_get(j_camp, "sc_mode")),
			ast_json_string_get(ast_json_object_get(j_camp, "sc_time_start"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_camp, "sc_time_end"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_camp, "sc_date_start"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_camp, "sc_date_end"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_camp, "sc_date_list"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_camp, "sc_date_list_except"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_camp, "sc_day_list"))? : "<unknown>",

			ast_json_string_get(ast_json_object_get(j_camp, "tm_create"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_camp, "tm_delete"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_camp, "tm_update"))? : "<unknown>"
			);
	return tmp;
}

/**
 * Create AMI string for campaign
 * @param j_camp
 * @return
 */
static char* get_campaign_stat_str(struct ast_json* j_stat)
{
	char* tmp;

	if(j_stat == NULL) {
		return NULL;
	}

	ast_asprintf(&tmp,
			"Uuid: %s\r\n"

			"DlTotalCount: %"PRIdMAX"\r\n"
			"DlFinishedCount: %"PRIdMAX"\r\n"
			"DlAvailableCount: %"PRIdMAX"\r\n"
			"DlDialingCount: %"PRIdMAX"\r\n"
			"DlCalledCount: %"PRIdMAX"\r\n",

			ast_json_string_get(ast_json_object_get(j_stat, "uuid"))? : "<unknown>",

			ast_json_integer_get(ast_json_object_get(j_stat, "dial_total_count")),
			ast_json_integer_get(ast_json_object_get(j_stat, "dial_finished_count")),
			ast_json_integer_get(ast_json_object_get(j_stat, "dial_available_count")),
			ast_json_integer_get(ast_json_object_get(j_stat, "dial_dialing_count")),
			ast_json_integer_get(ast_json_object_get(j_stat, "dial_called_count"))
			);

	ast_log(LOG_VERBOSE, "Value check. created campaign stat string. str[%s]\n", tmp);
	return tmp;
}

/**
 * Create AMI string for plan
 * @param j_plan
 * @return
 */
static char* get_plan_str(struct ast_json* j_plan)
{
	char* tmp;
	char* variables;

	if(j_plan == NULL) {
		return NULL;
	}

	// get variables
	variables = get_variables_info_ami_str(j_plan, "variables");

	ast_asprintf(&tmp,
			"Uuid: %s\r\n"
			"Name: %s\r\n"
			"Detail: %s\r\n"
			"DialMode: %"PRIdMAX"\r\n"
			"DialTimeout: %"PRIdMAX"\r\n"

			"CallerId: %s\r\n"
			"DlEndHandle: %"PRIdMAX"\r\n"
			"RetryDelay: %"PRIdMAX"\r\n"
			"TrunkName: %s\r\n"
			"TechName: %s\r\n"
			"%s"// Variables

			"MaxRetryCnt1: %"PRIdMAX"\r\n"
			"MaxRetryCnt2: %"PRIdMAX"\r\n"
			"MaxRetryCnt3: %"PRIdMAX"\r\n"
			"MaxRetryCnt4: %"PRIdMAX"\r\n"
			"MaxRetryCnt5: %"PRIdMAX"\r\n"
			"MaxRetryCnt6: %"PRIdMAX"\r\n"
			"MaxRetryCnt7: %"PRIdMAX"\r\n"
			"MaxRetryCnt8: %"PRIdMAX"\r\n"

			"TmCreate: %s\r\n"
			"TmDelete: %s\r\n"
			"TmUpdate: %s\r\n",
			ast_json_string_get(ast_json_object_get(j_plan, "uuid"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_plan, "name"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_plan, "detail"))? : "<unknown>",
			ast_json_integer_get(ast_json_object_get(j_plan, "dial_mode")),
			ast_json_integer_get(ast_json_object_get(j_plan, "dial_timeout")),

			ast_json_string_get(ast_json_object_get(j_plan, "caller_id"))? : "<unknown>",
			ast_json_integer_get(ast_json_object_get(j_plan, "dl_end_handle")),
			ast_json_integer_get(ast_json_object_get(j_plan, "retry_delay")),
			ast_json_string_get(ast_json_object_get(j_plan, "trunk_name"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_plan, "tech_name"))? : "<unknown>",
			variables? : "Variable: <unknown>\r\n",

			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_1")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_2")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_3")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_4")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_5")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_6")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_7")),
			ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_8")),

			ast_json_string_get(ast_json_object_get(j_plan, "tm_create"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_plan, "tm_delete"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_plan, "tm_update"))? : "<unknown>"
			);
	ast_free(variables);

	ast_log(LOG_VERBOSE, "Value check. created plan string. str[%s]\n", tmp);
	return tmp;
}

/**
 * Create AMI string for dlma
 * @param j_dlma
 * @return
 */
static char* get_dlma_str(struct ast_json* j_dlma)
{
	char* tmp;
	char* variables;

	if(j_dlma == NULL) {
		return NULL;
	}

	// get variables
	variables = get_variables_info_ami_str(j_dlma, "variables");

	ast_asprintf(&tmp,
			"Uuid: %s\r\n"
			"Name: %s\r\n"
			"Detail: %s\r\n"
			"DlTable: %s\r\n"
			"%s"// Variables

			"TmCreate: %s\r\n"
			"TmDelete: %s\r\n"
			"TmUpdate: %s\r\n",

			ast_json_string_get(ast_json_object_get(j_dlma, "uuid"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dlma, "name"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dlma, "detail"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dlma, "dl_table"))? : "<unknown>",
			variables? : "Variable: <unknown>\r\n",

			ast_json_string_get(ast_json_object_get(j_dlma, "tm_create"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dlma, "tm_delete"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dlma, "tm_update"))? : "<unknown>"
			);
	ast_free(variables);

	return tmp;
}

static char* get_queue_str(struct ast_json* j_queue)
{
	char* tmp;

	if(j_queue == NULL) {
		return NULL;
	}

	ast_asprintf(&tmp,
			"Uuid: %s\r\n"
			"Name: %s\r\n"
			"Detail: %s\r\n",
			ast_json_string_get(ast_json_object_get(j_queue, "uuid")),
			ast_json_string_get(ast_json_object_get(j_queue, "name")),
			ast_json_string_get(ast_json_object_get(j_queue, "detail"))
			);
	return tmp;
}

/**
 * Get string for dialing
 * @param dialing
 * @return
 */
static char* get_dialing_str(const rb_dialing* dialing)
{
	char* tmp;
	char* variables;

	if(dialing == NULL) {
		return NULL;
	}

	variables = get_variables_info_ami_str(dialing->j_dialing, "variables");

	ast_asprintf(&tmp,
			// identity
			"Uuid: %s\r\n"
			"Status: %d\r\n"

			// uuid info
			"CampUuid: %s\r\n"
			"PlanUuid: %s\r\n"
			"DlmaUuid: %s\r\n"
			"DestUuid: %s\r\n"
			"DlListUuid: %s\r\n"	// todo: need to do more...

			// dial info
			"DialIndex: %"PRIdMAX"\r\n"
			"DialAddr: %s\r\n"
			"DialChannel: %s\r\n"
			"DialTryCnt: %"PRIdMAX"\r\n"
			"DialTimeout: %"PRIdMAX"\r\n"
			"DialType: %"PRIdMAX"\r\n"
			"DialExten: %s\r\n"
			"DialContext: %s\r\n"
			"DialApplication: %s\r\n"
			"DialData: %s\r\n"
			"%s"// Variables

			// channel info
			"ChannelName: %s\r\n"

			// dial result
			"ResDial: %"PRIdMAX"\r\n"
			"ResHangup: %"PRIdMAX"\r\n"
			"ResHangupDetail: %s\r\n"

			// tm info
			"TmCreate: %s\r\n"
			"TmUpdate: %s\r\n"
			"TmDelete: %s\r\n",

			// identity
			dialing->uuid? : "<unknown>",
			dialing->status,

			// uuid info
			ast_json_string_get(ast_json_object_get(dialing->j_dialing, "camp_uuid"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(dialing->j_dialing, "plan_uuid"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dlma_uuid"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dest_uuid"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dl_list_uuid"))? : "<unknown>",

			// dial info
			ast_json_integer_get(ast_json_object_get(dialing->j_dialing, "dial_index")),
			ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dial_addr"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dial_channel"))? : "<unknown>",
			ast_json_integer_get(ast_json_object_get(dialing->j_dialing, "dial_trycnt")),
			ast_json_integer_get(ast_json_object_get(dialing->j_dialing, "dial_timeout")),
			ast_json_integer_get(ast_json_object_get(dialing->j_dialing, "dial_type")),
			ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dial_exten"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dial_context"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dial_application"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dial_data"))? : "<unknown>",
			variables? : "Variable: <unknown>\r\n",

			// channel info
			ast_json_string_get(ast_json_object_get(dialing->j_dialing, "channel_name"))? : "<unknown>",

			// result info
			ast_json_integer_get(ast_json_object_get(dialing->j_dialing, "res_dial")),
			ast_json_integer_get(ast_json_object_get(dialing->j_dialing, "res_hangup")),
			ast_json_string_get(ast_json_object_get(dialing->j_dialing, "res_hangup_detail"))? : "<unknown>",

			// tm info
			dialing->tm_create? : "<unknown>",
			dialing->tm_update? : "<unknown>",
			dialing->tm_delete? : "<unknown>"
			);
	ast_free(variables);

	return tmp;
}

/**
 * Get string for dialing summary
 * @param dialing
 * @return
 */
static char* get_dialing_summary_str(void)
{
	char* tmp;

	ast_asprintf(&tmp,
			"Count: %d\r\n",
			rb_dialing_get_count()
			);
	return tmp;
}

static char* get_dl_list_str(struct ast_json* j_dl)
{
	char* tmp;
	char* variables;

	if(j_dl == NULL) {
		return NULL;
	}

	// get variables
	variables = get_variables_info_ami_str(j_dl, "variables");

	ast_asprintf(&tmp,
			"Uuid: %s\r\n"
			"DlmaUuid: %s\r\n"
			"Name: %s\r\n"
			"Detail: %s\r\n"
			"Status: %"PRIdMAX"\r\n"

			"UKey: %s\r\n"
			"%s"// Variables

			"DialingUuid: %s\r\n"
			"DialingCampUuid: %s\r\n"
			"DialingPlanUuid: %s\r\n"

			"Number1: %s\r\n"
			"Number2: %s\r\n"
			"Number3: %s\r\n"
			"Number4: %s\r\n"
			"Number5: %s\r\n"
			"Number6: %s\r\n"
			"Number7: %s\r\n"
			"Number8: %s\r\n"

			"Email: %s\r\n"

			"TryCnt1: %"PRIdMAX"\r\n"
			"TryCnt2: %"PRIdMAX"\r\n"
			"TryCnt3: %"PRIdMAX"\r\n"
			"TryCnt4: %"PRIdMAX"\r\n"
			"TryCnt5: %"PRIdMAX"\r\n"
			"TryCnt6: %"PRIdMAX"\r\n"
			"TryCnt7: %"PRIdMAX"\r\n"
			"TryCnt8: %"PRIdMAX"\r\n"

			"ResDial: %"PRIdMAX"\r\n"
			"ResDialDetail: %s\r\n"
			"ResHangup: %"PRIdMAX"\r\n"
			"ResHangupDetail: %s\r\n"

			"TmCreate: %s\r\n"
			"TmDelete: %s\r\n"
			"TmUpdate: %s\r\n",
			ast_json_string_get(ast_json_object_get(j_dl, "uuid"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dl, "dlma_uuid"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dl, "name"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dl, "detail"))? : "<unknown>",
			ast_json_integer_get(ast_json_object_get(j_dl, "status")),

			ast_json_string_get(ast_json_object_get(j_dl, "ukey"))? : "<unknown>",
			variables? : "Variable: <unknown>\r\n",

			ast_json_string_get(ast_json_object_get(j_dl, "dialing_uuid"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dl, "dialing_camp_uuid"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dl, "dialing_plan_uuid"))? : "<unknown>",

			ast_json_string_get(ast_json_object_get(j_dl, "number_1"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dl, "number_2"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dl, "number_3"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dl, "number_4"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dl, "number_5"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dl, "number_6"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dl, "number_7"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dl, "number_8"))? : "<unknown>",

			ast_json_string_get(ast_json_object_get(j_dl, "email"))? : "<unknown>",

			ast_json_integer_get(ast_json_object_get(j_dl, "trycnt_1")),
			ast_json_integer_get(ast_json_object_get(j_dl, "trycnt_2")),
			ast_json_integer_get(ast_json_object_get(j_dl, "trycnt_3")),
			ast_json_integer_get(ast_json_object_get(j_dl, "trycnt_4")),
			ast_json_integer_get(ast_json_object_get(j_dl, "trycnt_5")),
			ast_json_integer_get(ast_json_object_get(j_dl, "trycnt_6")),
			ast_json_integer_get(ast_json_object_get(j_dl, "trycnt_7")),
			ast_json_integer_get(ast_json_object_get(j_dl, "trycnt_8")),

			ast_json_integer_get(ast_json_object_get(j_dl, "res_dial")),
			ast_json_string_get(ast_json_object_get(j_dl, "res_dial_detail"))? : "<unknown>",
			ast_json_integer_get(ast_json_object_get(j_dl, "res_hangup")),
			ast_json_string_get(ast_json_object_get(j_dl, "res_hangup_detail"))? : "<unknown>",

			ast_json_string_get(ast_json_object_get(j_dl, "tm_create"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dl, "tm_delete"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dl, "tm_update"))? : "<unknown>"
			);
	ast_free(variables);

	return tmp;
}

/**
 * Create AMI string for plan
 * @param j_plan
 * @return
 */
static char* get_destination_str(struct ast_json* j_dest)
{
	char* tmp;
	char* variables;


	if(j_dest == NULL) {
		return NULL;
	}

	// get variables
	variables = get_variables_info_ami_str(j_dest, "variables");

	ast_asprintf(&tmp,
			"Uuid: %s\r\n"
			"Name: %s\r\n"
			"Detail: %s\r\n"

			"Type: %"PRIdMAX"\r\n"

			"Exten: %s\r\n"
			"Context: %s\r\n"
			"Priority: %s\r\n"
			"%s"// Variables

			"Application: %s\r\n"
			"Data: %s\r\n"

			"TmCreate: %s\r\n"
			"TmDelete: %s\r\n"
			"TmUpdate: %s\r\n",

			ast_json_string_get(ast_json_object_get(j_dest, "uuid"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dest, "name"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dest, "detail"))? : "<unknown>",

			ast_json_integer_get(ast_json_object_get(j_dest, "type")),

			ast_json_string_get(ast_json_object_get(j_dest, "exten"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dest, "context"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dest, "priority"))? : "<unknown>",
			variables? : "Variable: <unknown>\r\n",

			ast_json_string_get(ast_json_object_get(j_dest, "application"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dest, "data"))? : "<unknown>",

			ast_json_string_get(ast_json_object_get(j_dest, "tm_create"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dest, "tm_delete"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_dest, "tm_update"))? : "<unknown>"
			);
	ast_free(variables);

	ast_log(LOG_VERBOSE, "Value check. created plan string. str[%s]\n", tmp);
	return tmp;
}

static struct ast_json* create_json_plan(const struct message* m)
{
	const char* tmp_const;
	struct ast_json* j_tmp;
	char* tmp;

	if(m == NULL) {
		return NULL;
	}

	j_tmp = ast_json_object_create();

	tmp_const = message_get_header(m, "Name");
	if(tmp_const != NULL) {
		ast_json_object_set(j_tmp, "name", ast_json_string_create(tmp_const));
	}

	tmp_const = message_get_header(m, "Detail");
	if(tmp_const != NULL) {
		ast_json_object_set(j_tmp, "detail", ast_json_string_create(tmp_const));
	}

	tmp_const = message_get_header(m, "DialMode");
	if(tmp_const != NULL) {
		ast_json_object_set(j_tmp, "dial_mode", ast_json_integer_create(atoi(tmp_const)));
	}

	tmp_const = message_get_header(m, "DialTimeout");
	if(tmp_const != NULL) {
		ast_json_object_set(j_tmp, "dial_timeout", ast_json_integer_create(atoi(tmp_const)));
	}

	tmp_const = message_get_header(m, "CallerId");
	if(tmp_const != NULL) {
		ast_json_object_set(j_tmp, "caller_id", ast_json_string_create(tmp_const));
	}

	tmp_const = message_get_header(m, "DlEndHandle");
	if(tmp_const != NULL) {
		ast_json_object_set(j_tmp, "dl_end_handle", ast_json_integer_create(atoi(tmp_const)));
	}

	tmp_const = message_get_header(m, "RetryDelay");
	if(tmp_const != NULL) {
		ast_json_object_set(j_tmp, "retry_delay", ast_json_integer_create(atoi(tmp_const)));
	}

	tmp_const = message_get_header(m, "TrunkName");
	if(tmp_const != NULL) {
		ast_json_object_set(j_tmp, "trunk_name", ast_json_string_create(tmp_const));
	}

	tmp_const = message_get_header(m, "TechName");
	if(tmp_const != NULL) {
		ast_json_object_set(j_tmp, "tech_name", ast_json_string_create(tmp_const));
	}

	tmp_const = message_get_header(m, "ServiceLevel");
	if(tmp_const != NULL) {
		ast_json_object_set(j_tmp, "service_level", ast_json_integer_create(atoi(tmp_const)));
	}

	tmp_const = message_get_header(m, "MaxRetry1");
	if(tmp_const != NULL) {
		ast_json_object_set(j_tmp, "max_retry_cnt_1", ast_json_integer_create(atoi(tmp_const)));
	}

	tmp_const = message_get_header(m, "MaxRetry2");
	if(tmp_const != NULL) {
		ast_json_object_set(j_tmp, "max_retry_cnt_2", ast_json_integer_create(atoi(tmp_const)));
	}

	tmp_const = message_get_header(m, "MaxRetry3");
	if(tmp_const != NULL) {
		ast_json_object_set(j_tmp, "max_retry_cnt_3", ast_json_integer_create(atoi(tmp_const)));
	}

	tmp_const = message_get_header(m, "MaxRetry4");
	if(tmp_const != NULL) {
		ast_json_object_set(j_tmp, "max_retry_cnt_4", ast_json_integer_create(atoi(tmp_const)));
	}

	tmp_const = message_get_header(m, "MaxRetry5");
	if(tmp_const != NULL) {
		ast_json_object_set(j_tmp, "max_retry_cnt_5", ast_json_integer_create(atoi(tmp_const)));
	}

	tmp_const = message_get_header(m, "MaxRetry6");
	if(tmp_const != NULL) {
		ast_json_object_set(j_tmp, "max_retry_cnt_6", ast_json_integer_create(atoi(tmp_const)));
	}

	tmp_const = message_get_header(m, "MaxRetry7");
	if(tmp_const != NULL) {
		ast_json_object_set(j_tmp, "max_retry_cnt_7", ast_json_integer_create(atoi(tmp_const)));
	}

	tmp_const = message_get_header(m, "MaxRetry8");
	if(tmp_const != NULL) {
		ast_json_object_set(j_tmp, "max_retry_cnt_8", ast_json_integer_create(atoi(tmp_const)));
	}

	// Variable
	tmp = get_variables(m);
	if((tmp != NULL) && (strlen(tmp) > 0)) {ast_json_object_set(j_tmp, "variables", ast_json_string_create(tmp));}
	ast_free(tmp);

	return j_tmp;
}

static struct ast_json* create_json_dlma(const struct message* m)
{
	const char* tmp_const;
	struct ast_json* j_tmp;
	char* tmp;

	if(m == NULL) {
		return NULL;
	}

	j_tmp = ast_json_object_create();

	tmp_const = message_get_header(m, "Name");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "name", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Detail");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "detail", ast_json_string_create(tmp_const));}

	// Variable
	tmp = get_variables(m);
	if((tmp != NULL) && (strlen(tmp) > 0)) {ast_json_object_set(j_tmp, "variables", ast_json_string_create(tmp));}
	ast_free(tmp);

	return j_tmp;
}

static struct ast_json* create_json_destination(const struct message* m)
{
	struct ast_json* j_tmp;
	const char* tmp_const;
	char* tmp;

	if(m == NULL) {
		return NULL;
	}

	j_tmp = ast_json_object_create();

	tmp_const = message_get_header(m, "Name");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "name", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Detail");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "detail", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Type");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "type", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Exten");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "exten", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Context");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "context", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Priority");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "priority", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Application");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "application", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Data");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "data", ast_json_string_create(tmp_const));}

	// Variable
	tmp = get_variables(m);
	if((tmp != NULL) && (strlen(tmp) > 0)) {ast_json_object_set(j_tmp, "variables", ast_json_string_create(tmp));}
	ast_free(tmp);

	return j_tmp;
}


static struct ast_json* create_json_campaign(const struct message* m)
{
	struct ast_json* j_tmp;
	const char* tmp_const;
	char* tmp;

	if(m == NULL) {
		return NULL;
	}

	j_tmp = ast_json_object_create();

	tmp_const = message_get_header(m, "Name");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "name", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Detail");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "detail", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Plan");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "plan", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Dlma");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "dlma", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Dest");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "dest", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "ScMode");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "sc_mode", ast_json_integer_create(atoi(tmp_const)));}

	tmp_const = message_get_header(m, "ScTimeStart");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "sc_time_start", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "ScTimeEnd");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "sc_time_end", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "ScDateStart");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "sc_date_start", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "ScDateEnd");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "sc_date_end", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "ScDateList");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "sc_date_list", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "ScDateListExcept");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "sc_date_list_except", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "ScDayList");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "sc_day_list", ast_json_string_create(tmp_const));}

	// Variable
	tmp = get_variables(m);
	if((tmp != NULL) && (strlen(tmp) > 0)) {ast_json_object_set(j_tmp, "variables", ast_json_string_create(tmp));}
	ast_free(tmp);

	return j_tmp;
}


static struct ast_json* create_json_dl_list(const struct message* m)
{
	struct ast_json* j_tmp;
	const char* tmp_const;
	char* tmp;

	j_tmp = ast_json_object_create();

	tmp_const = message_get_header(m, "DlmaUuid");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "dlma_uuid", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Name");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "name", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Detail");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "detail", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "UKey");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "ukey", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Number1");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "number_1", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Number2");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "number_2", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Number3");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "number_3", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Number4");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "number_4", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Number5");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "number_5", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Number6");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "number_6", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Number7");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "number_7", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Number8");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "number_8", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Email");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "email", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "UKey");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "ukey", ast_json_string_create(tmp_const));}

	// Variables
	tmp = get_variables(m);
	if((tmp != NULL) && (strlen(tmp) > 0)) {ast_json_object_set(j_tmp, "variables", ast_json_string_create(tmp));}
	ast_free(tmp);

	return j_tmp;
}

/**
 * AMI Event handler
 * Event: OutCampaignCreate
 * @param j_camp
 */
void send_manager_evt_out_campaign_create(struct ast_json* j_camp)
{
	char* tmp;

	ast_log(LOG_VERBOSE, "AMI event. OutCampaignCreate.\n");

	if(j_camp == NULL) {
		ast_log(LOG_WARNING, "AMI event. OutCampaignCreate. Failed.\n");
		return;
	}

	tmp = get_campaign_str(j_camp);
	if(tmp == NULL) {
		return;
	}

	manager_event(EVENT_FLAG_MESSAGE, "OutCampaignCreate", "%s\r\n", tmp);
	ast_free(tmp);
	ast_log(LOG_VERBOSE, "AMI event. OutCampaignCreate. Succeed.\n");

	return;
}


/**
 * AMI Event handler
 * Event: OutCampaignDelete
 * @param j_camp
 */
void send_manager_evt_out_campaign_delete(struct ast_json* j_camp)
{
	char* tmp;

	ast_log(LOG_VERBOSE, "AMI event. OutCampaignDelete.\n");
	if(j_camp == NULL) {
		// nothing to send.
		ast_log(LOG_WARNING, "AMI event. OutCampaignDelete. Failed.\n");
		return;
	}

	tmp = get_campaign_str(j_camp);
	if(tmp == NULL) {
		return;
	}
	manager_event(EVENT_FLAG_MESSAGE, "OutCampaignDelete", "%s\r\n", tmp);
	ast_free(tmp);
	ast_log(LOG_VERBOSE, "AMI event. OutCampaignDelete. Succeed.\n");

	return;
}

/**
 * AMI Event handler
 * Event: OutCampaignUpdate
 * @param j_camp
 */
void send_manager_evt_out_campaign_update(struct ast_json* j_camp)
{
	char* tmp;

	ast_log(LOG_VERBOSE, "AMI event. OutCampaignUpdate.\n");

	if(j_camp == NULL) {
		// nothing to send.
		ast_log(LOG_WARNING, "AMI event. OutCampaignUpdate. Failed.\n");
		return;
	}

	tmp = get_campaign_str(j_camp);
	if(tmp == NULL) {
		return;
	}

	manager_event(EVENT_FLAG_MESSAGE, "OutCampaignUpdate", "%s\r\n", tmp);
	ast_free(tmp);
	ast_log(LOG_VERBOSE, "AMI event. OutCampaignUpdate. Succeed.\n");

	return;
}

/**
 * AMI Event handler
 * Event: OutPlanCreate
 * @param j_camp
 */
void send_manager_evt_out_plan_create(struct ast_json* j_plan)
{
	char* tmp;

	ast_log(LOG_VERBOSE, "AMI event. OutPlanCreate.\n");

	if(j_plan == NULL) {
		// nothing to send.
		ast_log(LOG_WARNING, "AMI event. OutPlanCreate. Failed.\n");
		return;
	}

	tmp = get_plan_str(j_plan);
	if(tmp == NULL) {
		return;
	}

	manager_event(EVENT_FLAG_MESSAGE, "OutPlanCreate", "%s\r\n", tmp);
	ast_free(tmp);
	ast_log(LOG_VERBOSE, "AMI event. OutPlanCreate. Succeed.\n");

	return;
}


/**
 * AMI Event handler
 * Event: OutPlanDelete
 * @param j_camp
 */
void send_manager_evt_out_plan_delete(struct ast_json* j_plan)
{
	char* tmp;

	ast_log(LOG_VERBOSE, "AMI event. OutPlanDelete.\n");

	if(j_plan == NULL) {
		// nothing to send.
		ast_log(LOG_WARNING, "AMI event. OutPlanDelete. Failed.\n");
		return;
	}

	tmp = get_plan_str(j_plan);
	manager_event(EVENT_FLAG_MESSAGE, "OutPlanDelete", "%s\r\n", tmp);
	ast_free(tmp);
	ast_log(LOG_VERBOSE, "AMI event. OutPlanDelete. Succeed.\n");

	return;
}

/**
 * AMI Event handler
 * Event: OutPlanDelete
 * @param j_camp
 */
void send_manager_evt_out_plan_update(struct ast_json* j_plan)
{
	char* tmp;

	ast_log(LOG_VERBOSE, "AMI event. OutPlanUpdate.\n");

	if(j_plan == NULL) {
		// nothing to send.
		ast_log(LOG_WARNING, "AMI event. OutPlanUpdate. Failed.\n");
		return;
	}

	tmp = get_plan_str(j_plan);
	if(tmp == NULL) {
		return;
	}

	manager_event(EVENT_FLAG_MESSAGE, "OutPlanUpdate", "%s\r\n", tmp);
	ast_free(tmp);
	ast_log(LOG_VERBOSE, "AMI event. OutPlanUpdate. Succeed.\n");

	return;
}

/**
 * Send OutQueueCreate event notify to AMI.
 * @param j_camp
 */
void send_manager_evt_out_queue_create(struct ast_json* j_queue)
{
	char* tmp;

	if(j_queue == NULL) {
		return;
	}

	tmp = get_queue_str(j_queue);
	if(tmp == NULL) {
		return;
	}

	manager_event(EVENT_FLAG_MESSAGE, "OutQueueCreate", "%s\r\n", tmp);
	ast_free(tmp);
}

/**
 * Send OutQueueUpdate event notify to AMI
 * @param j_camp
 */
void send_manager_evt_out_queue_update(struct ast_json* j_queue)
{
	char* tmp;

	if(j_queue == NULL) {
		return;
	}

	tmp = get_queue_str(j_queue);
	if(tmp == NULL) {
		return;
	}

	manager_event(EVENT_FLAG_MESSAGE, "OutQueueUpdate", "%s\r\n", tmp);
	ast_free(tmp);

	return;
}

/**
 * Send event notification of queue delete.
 * OutQueueDelete
 * @param j_camp
 */
void send_manager_evt_out_queue_delete(const char* uuid)
{
	char* tmp;

	if(uuid == NULL) {
		ast_log(LOG_WARNING, "AMI event. OutQueueDelete. Failed.\n");
		return;
	}

	ast_asprintf(&tmp,
			"Uuid: %s\r\n",
			uuid
			);
	manager_event(EVENT_FLAG_MESSAGE, "OutQueueDelete", "%s\r\n", tmp);
	ast_free(tmp);
	ast_log(LOG_VERBOSE, "AMI event. OutQueueDelete. Succeed.\n");

	return;
}


/**
 * AMI Event handler
 * Event: OutDlmaCreate
 * @param j_camp
 */
void send_manager_evt_out_dlma_create(struct ast_json* j_tmp)
{
	char* tmp;

	ast_log(LOG_VERBOSE, "AMI event. OutDlmaCreate.\n");

	if(j_tmp == NULL) {
		// nothing to send.
		ast_log(LOG_WARNING, "AMI event. OutDlmaCreate. Failed.\n");
		return;
	}

	tmp = get_dlma_str(j_tmp);
	if(tmp == NULL) {
		return;
	}

	manager_event(EVENT_FLAG_MESSAGE, "OutDlmaCreate", "%s\r\n", tmp);
	ast_free(tmp);
	ast_log(LOG_VERBOSE, "AMI event. OutDlmaCreate. Succeed.\n");

	return;
}


/**
 * AMI Event handler
 * Event: OutDlmaDelete
 * @param j_camp
 */
void send_manager_evt_out_dlma_delete(const char* uuid)
{
	char* tmp;

	ast_log(LOG_VERBOSE, "AMI event. OutDlmaDelete.\n");

	if(uuid == NULL) {
		// nothing to send.
		ast_log(LOG_WARNING, "AMI event. OutDlmaDelete. Failed.\n");
		return;
	}

	ast_asprintf(&tmp,
			"Uuid: %s\r\n",
			uuid
			);
	manager_event(EVENT_FLAG_MESSAGE, "OutDlmaDelete", "%s\r\n", tmp);
	ast_free(tmp);
	ast_log(LOG_VERBOSE, "AMI event. OutDlmaDelete. Succeed.\n");

	return;
}

/**
 * AMI Event handler
 * Event: OutDlmaUpdate
 * @param j_camp
 */
void send_manager_evt_out_dlma_update(struct ast_json* j_tmp)
{
	char* tmp;

	ast_log(LOG_VERBOSE, "AMI event. OutDlmaUpdate.\n");

	if(j_tmp == NULL) {
		// nothing to send.
		ast_log(LOG_WARNING, "Nothing to send.\n");
		return;
	}

	tmp = get_dlma_str(j_tmp);
	if(tmp == NULL) {
		return;
	}

	manager_event(EVENT_FLAG_MESSAGE, "OutDlmaUpdate", "%s\r\n", tmp);
	ast_free(tmp);
	ast_log(LOG_VERBOSE, "AMI event. OutDlmaUpdate. Succeed.\n");

	return;
}

/**
 * AMI Event handler
 * Event: OutDialingCreate
 * @param j_camp
 */
void send_manager_evt_out_dialing_create(rb_dialing* dialing)
{
	char* tmp;

	ast_log(LOG_VERBOSE, "AMI event. OutDialingCreate.\n");

	if(dialing == NULL) {
		// nothing to send.
		ast_log(LOG_WARNING, "OutDialingCreate. Nothing to send.\n");
		return;
	}

	tmp = get_dialing_str(dialing);
	if(tmp == NULL) {
		ast_log(LOG_WARNING, "AMI event. OutDialingCreate. Failed.\n");
		return;
	}

	manager_event(EVENT_FLAG_MESSAGE, "OutDialingCreate", "%s\r\n", tmp);
	ast_free(tmp);
	ast_log(LOG_VERBOSE, "AMI event. OutDialingCreate. Succeed.\n");

	return;
}

/**
 * AMI Event handler
 * Event: OutDialingUpdate
 * @param j_camp
 */
void send_manager_evt_out_dialing_update(rb_dialing* dialing)
{
	char* tmp;

	ast_log(LOG_VERBOSE, "AMI event. OutDialingUpdate.\n");

	if(dialing == NULL) {
		// nothing to send.
		ast_log(LOG_WARNING, "AMI event. OutDialingUpdate. Failed.\n");
		return;
	}

	tmp = get_dialing_str(dialing);
	if(tmp == NULL) {
		return;
	}

	manager_event(EVENT_FLAG_MESSAGE, "OutDialingUpdate", "%s\r\n", tmp);
	ast_free(tmp);
	ast_log(LOG_VERBOSE, "AMI event. OutDialingUpdate. Succeed.\n");

	return;
}

/**
 * AMI Event handler
 * Event: OutDestinationCreate
 * @param j_obj
 */
void send_manager_evt_out_destination_create(struct ast_json* j_dest)
{
	char* tmp;

	ast_log(LOG_VERBOSE, "AMI event. OutDestinationCreate.\n");

	if(j_dest == NULL) {
		// nothing to send.
		ast_log(LOG_WARNING, "Nothing to send.\n");
		return;
	}

	tmp = get_destination_str(j_dest);
	if(tmp == NULL) {
		return;
	}

	manager_event(EVENT_FLAG_MESSAGE, "OutDestinationCreate", "%s\r\n", tmp);
	ast_free(tmp);
	ast_log(LOG_VERBOSE, "AMI event. OutDestinationCreate. Succeed.\n");

	return;
}

/**
 * AMI Event handler
 * Event: OutDestinationUpdate
 * @param j_obj
 */
void send_manager_evt_out_destination_update(struct ast_json* j_dest)
{
	char* tmp;

	ast_log(LOG_VERBOSE, "AMI event. OutDestinationUpdate.\n");

	if(j_dest == NULL) {
		// nothing to send.
		ast_log(LOG_WARNING, "Nothing to send.\n");
		return;
	}

	tmp = get_destination_str(j_dest);
	if(tmp == NULL) {
		return;
	}

	manager_event(EVENT_FLAG_MESSAGE, "OutDestinationUpdate", "%s\r\n", tmp);
	ast_free(tmp);
	ast_log(LOG_VERBOSE, "AMI event. OutDestinationUpdate. Succeed.\n");

	return;
}

/**
 * AMI Event handler
 * Event: OutDestinationDelete
 * @param j_camp
 */
void send_manager_evt_out_destination_delete(struct ast_json* j_dest)
{
	char* tmp;

	ast_log(LOG_VERBOSE, "AMI event. OutDestinationDelete.\n");

	if(j_dest == NULL) {
		// nothing to send.
		ast_log(LOG_WARNING, "AMI event. OutDestinationDelete. Failed.\n");
		return;
	}

	tmp = get_destination_str(j_dest);
	manager_event(EVENT_FLAG_MESSAGE, "OutDestinationDelete", "%s\r\n", tmp);
	ast_free(tmp);
	ast_log(LOG_VERBOSE, "AMI event. OutDestinationDelete. Succeed.\n");

	return;
}

/**
 * AMI Event handler
 * Event: OutDialingEntry
 * @param s
 * @param m
 * @param j_tmp
 * @param action_id
 */
void manager_evt_out_dialing_entry(struct mansession *s, const struct message *m, const rb_dialing* dialing, char* action_id)
{
	char* tmp;

	tmp = get_dialing_str(dialing);

	if(s != NULL) {
		astman_append(s, "Event: OutDialingEntry\r\n%s%s\r\n", action_id, tmp);
	}
	else {
		manager_event(EVENT_FLAG_MESSAGE, "OutDialingEntry", "%s\r\n", tmp);
	}
	ast_free(tmp);
}

/**
 * AMI Event handler
 * Event: OutDialingSummary
 * @param s
 * @param m
 * @param j_tmp
 * @param action_id
 */
void manager_evt_out_dialing_summary(struct mansession *s, const struct message *m, char* action_id)
{
	char* tmp;

	tmp = get_dialing_summary_str();

	if(s != NULL) {
		astman_append(s, "Event: OutDialingSummary\r\n%s%s\r\n", action_id, tmp);
	}
	else {
		manager_event(EVENT_FLAG_MESSAGE, "OutDialingSummary", "%s\r\n", tmp);
	}
	ast_free(tmp);
}

/**
 * AMI Event handler
 * Event: OutDialingDelete
 * @param j_camp
 */
void send_manager_evt_out_dialing_delete(rb_dialing* dialing)
{
	char* tmp;

	ast_log(LOG_VERBOSE, "AMI event. OutDialingDelete.\n");

	if(dialing == NULL) {
		// nothing to send.
		ast_log(LOG_WARNING, "AMI event. OutDialingDelete. Failed.\n");
		return;
	}

	tmp = get_dialing_str(dialing);
	manager_event(EVENT_FLAG_MESSAGE, "OutDialingDelete", "%s\r\n", tmp);
	ast_free(tmp);
	ast_log(LOG_VERBOSE, "AMI event. OutDialingDelete. Succeed.\n");

	return;
}

/**
 * AMI Action handler
 * Action: OutDlListCreate
 * @param s
 * @param m
 * @return
 */
static int manager_out_dl_list_create(struct mansession *s, const struct message *m)
{
	struct ast_json* j_tmp;
	char* dl_uuid;

	ast_log(LOG_VERBOSE, "AMI request. OutDlListCreate.\n");

	j_tmp = create_json_dl_list(m);
	if(j_tmp == NULL) {
		astman_send_error(s, m, "Error encountered while creating dl list.");
		ast_log(LOG_NOTICE, "OutDlListCreate failed.\n");
		return 0;
	}

	dl_uuid = create_dl_list(j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(dl_uuid == NULL) {
		astman_send_error(s, m, "Error encountered while creating dl list.");
		ast_log(LOG_NOTICE, "OutDlListCreate failed.\n");
		return 0;
	}
	ast_free(dl_uuid);
	astman_send_ack(s, m, "Dl list created successfully");
	ast_log(LOG_NOTICE, "OutDlListCreate succeed.\n");

	return 0;
}

/**
 * AMI Action handler
 * Action: OutDlListUpdate
 * @param s
 * @param m
 * @return
 */
static int manager_out_dl_list_update(struct mansession *s, const struct message *m)
{
	struct ast_json* j_tmp;
	const char* uuid;
	int ret;

	ast_log(LOG_VERBOSE, "AMI request. OutDlListUpdate.\n");

	uuid = message_get_header(m, "Uuid");
	if(strcmp(uuid, "") == 0) {
		astman_send_error(s, m, "Error encountered while deleting dl_list");
		return 0;
	}

	j_tmp = create_json_dl_list(m);
	if(j_tmp == NULL) {
		astman_send_error(s, m, "Error encountered while updating dl list.");
		ast_log(LOG_NOTICE, "OutDlListUpdate failed.\n");
		return 0;
	}

	ast_json_object_set(j_tmp, "uuid", ast_json_string_create(uuid));

	ret = update_dl_list(j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(ret == false) {
		astman_send_error(s, m, "Error encountered while updating dl list.");
		ast_log(LOG_NOTICE, "OutDlListUpdate failed.\n");
		return 0;
	}
	astman_send_ack(s, m, "Dl list updated successfully");
	ast_log(LOG_NOTICE, "OutDlListUpdate succeed.\n");

	return 0;
}

/**
 * AMI Action handler
 * Action: OutDlListDelete
 * @param s
 * @param m
 * @return
 */
static int manager_out_dl_list_delete(struct mansession *s, const struct message *m)
{
	const char* tmp_const;
	int ret;

	ast_log(LOG_VERBOSE, "AMI request. OutDlListDelete.\n");

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const == NULL) {
		astman_send_error(s, m, "Error encountered while deleting dl_list");
		return 0;
	}

	ret = delete_dl_list(tmp_const);
	if(ret == false) {
		astman_send_error(s, m, "Error encountered while deleting dl_list");
		return 0;
	}
	astman_send_ack(s, m, "Dl list deleted successfully");

	return 0;
}

/**
 * OutDlListEntry
 * @param s
 * @param m
 * @param j_dl
 */
void manager_out_dl_list_entry(struct mansession *s, const struct message *m, struct ast_json* j_dl, const char* action_id)
{
	char* tmp;

	if(j_dl == NULL) {
		return;
	}

	tmp = get_dl_list_str(j_dl);
	if(tmp == NULL) {
		ast_log(LOG_WARNING, "Could not get dl_list str.\n");
		return;
	}
	ast_log(LOG_DEBUG, "Check value. tmp[%s]\n", tmp);

	if(s != NULL) {
		astman_append(s, "Event: OutDlListEntry\r\n%s%s\r\n", action_id, tmp);
	}
	else {
		manager_event(EVENT_FLAG_MESSAGE, "OutDlListEntry", "%s\r\n", tmp);
	}
	ast_free(tmp);
}

/**
 * AMI Action handler
 * Action: OutDlListShow
 * @param s
 * @param m
 * @return
 */
static int manager_out_dl_list_show(struct mansession *s, const struct message *m)
{
	struct ast_json* j_tmp;
	struct ast_json* j_arr;
	const char* tmp_const;
	const char* tmp_count;
	const char* uuid;
	const char* dlma_uuid;
	char* action_id;
	int count;
	int size;
	int i;

	ast_log(LOG_VERBOSE, "AMI request. OutDlListShow.\n");

	tmp_const = message_get_header(m, "ActionID");
	if((tmp_const != NULL) && (strlen(tmp_const)) != 0) {
		ast_asprintf(&action_id, "ActionID: %s\r\n", tmp_const);
	}
	else {
		ast_asprintf(&action_id, "%s", "");
	}

	uuid = message_get_header(m, "Uuid");
	dlma_uuid = message_get_header(m, "DlmaUuid");
	if(uuid != NULL) {
		ast_log(LOG_DEBUG, "Finding dl_list. uuid[%s]\n", tmp_const);

		j_tmp = get_dl_list(tmp_const);
		if(j_tmp == NULL) {
			astman_send_error(s, m, "No such dl_list");
			ast_free(action_id);
			return 0;
		}

		astman_send_listack(s, m, "Dl List will follow", "start");

		manager_out_dl_list_entry(s, m, j_tmp, action_id);
		AST_JSON_UNREF(j_tmp);

		astman_send_list_complete_start(s, m, "OutDlListComplete", 1);
		astman_send_list_complete_end(s);
	}
	else if(dlma_uuid != NULL) {
		tmp_count = message_get_header(m, "Count");
		count = 100;	// default
		if(tmp_count != NULL) {
			count = atoi(tmp_count);
		}

		ast_log(LOG_DEBUG, "Finding dl_list. dlam_uuid[%s], count[%d]\n", tmp_const, count);

		j_arr = get_dl_lists(tmp_const, count);
		astman_send_listack(s, m, "Dl List will follow", "start");
		size = ast_json_array_size(j_arr);
		for(i = 0; i < size; i++) {
			j_tmp = ast_json_array_get(j_arr, i);
			if(j_tmp == NULL) {
				ast_log(LOG_WARNING, "Could not get correct object. idx[%i]\n", i);
				continue;
			}
			manager_out_dl_list_entry(s, m, j_tmp, action_id);
		}
		AST_JSON_UNREF(j_arr);
		astman_send_list_complete_start(s, m, "OutDlListComplete", size);
		astman_send_list_complete_end(s);
	}

	ast_free(action_id);

	return 0;
}

/**
 * OutPlanList
 * @param s
 * @param m
 * @param j_dl
 */
void manager_out_plan_entry(struct mansession *s, const struct message *m, struct ast_json* j_tmp, char* action_id)
{
	char* tmp;

	tmp = get_plan_str(j_tmp);

	if(s != NULL) {
		astman_append(s, "Event: OutPlanEntry\r\n%s%s\r\n", action_id, tmp);
	}
	else {
		manager_event(EVENT_FLAG_MESSAGE, "OutPlanEntry", "%s\r\n", tmp);
	}
	ast_free(tmp);
}

/**
 * OutDlmaEntry
 * @param s
 * @param m
 * @param j_dl
 */
void manager_out_dlma_entry(struct mansession *s, const struct message *m, struct ast_json* j_tmp, char* action_id)
{
	char* tmp;

	tmp = get_dlma_str(j_tmp);

	if(s != NULL) {
		astman_append(s, "Event: OutDlmaEntry\r\n%s%s\r\n", action_id, tmp);
	}
	else {
		manager_event(EVENT_FLAG_MESSAGE, "OutDlmaEntry", "%s\r\n", tmp);
	}
	ast_free(tmp);
}

/**
 * OutDestinationEntry
 * @param s
 * @param m
 * @param j_dl
 */
void manager_out_destination_entry(struct mansession *s, const struct message *m, struct ast_json* j_tmp, char* action_id)
{
	char* tmp;

	tmp = get_destination_str(j_tmp);

	if(s != NULL) {
		astman_append(s, "Event: OutDestinationEntry\r\n%s%s\r\n", action_id, tmp);
	}
	else {
		manager_event(EVENT_FLAG_MESSAGE, "OutDestinationEntry", "%s\r\n", tmp);
	}
	ast_free(tmp);
}


/**
 * OutQueueEntry
 * @param s
 * @param m
 * @param j_dl
 */
void manager_out_queue_entry(struct mansession *s, const struct message *m, struct ast_json* j_tmp, char* action_id)
{
	char* tmp;

	ast_asprintf(&tmp,
			"Uuid: %s\r\n"
			"Name: %s\r\n"
			"Detail: %s\r\n",
			ast_json_string_get(ast_json_object_get(j_tmp, "uuid"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_tmp, "name"))? : "<unknown>",
			ast_json_string_get(ast_json_object_get(j_tmp, "detail"))? : "<unknown>"
			);

	if(s != NULL) {
		astman_append(s, "Event: OutQueueEntry\r\n%s%s\r\n", action_id, tmp);
	}
	else {
		manager_event(EVENT_FLAG_MESSAGE, "OutQueueEntry", "%s\r\n", tmp);
	}
	ast_free(tmp);
}

/**
 * AMI Event handler
 * Event: OutCampaignEntry
 * @param s
 * @param m
 * @param j_tmp
 * @param action_id
 */
void manager_out_campaign_entry(struct mansession *s, const struct message *m, struct ast_json* j_tmp, char* action_id)
{
	char* tmp;

	tmp = get_campaign_str(j_tmp);

	if(s != NULL) {
		astman_append(s, "Event: OutCampaignEntry\r\n%s%s\r\n", action_id, tmp);
	}
	else {
		manager_event(EVENT_FLAG_MESSAGE, "OutCampaignEntry", "%s\r\n", tmp);
	}
	ast_free(tmp);
}

/**
 * AMI Event handler
 * Event: OutCampaignStatEntry
 * @param s
 * @param m
 * @param j_tmp
 * @param action_id
 */
void manager_out_campaign_stat_entry(struct mansession *s, const struct message *m, struct ast_json* j_tmp, char* action_id)
{
	char* tmp;

	tmp = get_campaign_stat_str(j_tmp);

	if(s != NULL) {
		astman_append(s, "Event: OutCampaignStatEntry\r\n%s%s\r\n", action_id, tmp);
	}
	else {
		manager_event(EVENT_FLAG_MESSAGE, "OutCampaignStatEntry", "%s\r\n", tmp);
	}
	ast_free(tmp);
}


/**
 * AMI Action handler
 * Action: OutCampaignCreate
 * @param s
 * @param m
 * @return
 */
static int manager_out_campaign_create(struct mansession *s, const struct message *m)
{
	struct ast_json* j_tmp;
	int ret;

	ast_log(LOG_VERBOSE, "AMI request. OutCampaignCreate.\n");

	j_tmp = create_json_campaign(m);
	if(j_tmp == NULL) {
		astman_send_error(s, m, "Error encountered while creating campaign");
		ast_log(LOG_NOTICE, "OutCampaignCreate failed.\n");
		return 0;
	}

	ret = create_campaign(j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(ret == false) {
		astman_send_error(s, m, "Error encountered while creating campaign");
		ast_log(LOG_NOTICE, "OutCampaignCreate failed.\n");
		return 0;
	}

	astman_send_ack(s, m, "Campaign created successfully");
	ast_log(LOG_NOTICE, "OutCampaignCreate succeed.\n");

	return 0;
}

/**
 * AMI Action handler
 * Action: OutCampaignDelete
 * @param s
 * @param m
 * @return
 */
static int manager_out_campaign_delete(struct mansession *s, const struct message *m)
{
	const char* tmp_const;
	int ret;

	ast_log(LOG_VERBOSE, "AMI request. OutCampaignDelete.\n");

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const == NULL) {
		astman_send_error(s, m, "Error encountered while deleting campaign");
		ast_log(LOG_WARNING, "OutCampaignDelete failed.\n");
		return 0;
	}

	ret = delete_campaign(tmp_const);
	if(ret == false) {
		astman_send_error(s, m, "Error encountered while deleting campaign");
		ast_log(LOG_WARNING, "OutCampaignDelete failed.\n");
		return 0;
	}
	astman_send_ack(s, m, "Campaign deleted successfully");
	ast_log(LOG_NOTICE, "OutCampaignDelete succeed.\n");

	return 0;
}

/**
 * AMI Action handler
 * Action: OutCampaignUpdate
 * @param s
 * @param m
 * @return
 */
static int manager_out_campaign_update(struct mansession *s, const struct message *m)
{
	struct ast_json* j_tmp;
	const char* uuid;
	const char* tmp_const;
	int status;
	int ret;

	ast_log(LOG_VERBOSE, "AMI request. OutCampaignUpdate.\n");

	uuid = message_get_header(m, "Uuid");
	if(uuid == NULL) {
		astman_send_error(s, m, "Error encountered while updating campaign");
		ast_log(LOG_WARNING, "OutCampaignUpdate failed.\n");
		return 0;
	}

	// create update info
	j_tmp = create_json_campaign(m);

	// set uuid
	ast_json_object_set(j_tmp, "uuid", ast_json_string_create(uuid));

	// set status
	tmp_const = message_get_header(m, "Status");
	if(tmp_const != NULL) {
		status = atoi(tmp_const);

		if((status == E_CAMP_START) || (status == E_CAMP_STARTING)) {
			status = E_CAMP_STARTING;
		}
		else if((status == E_CAMP_STOP) || (status == E_CAMP_STOPPING)) {
			status = E_CAMP_STOPPING;
		}
		else if((status == E_CAMP_PAUSE) || (status == E_CAMP_PAUSING)) {
			status = E_CAMP_PAUSING;
		}
		else {
			astman_send_error(s, m, "Error encountered while updating campaign");
			ast_log(LOG_WARNING, "OutCampaignUpdate failed. Wrong status parameter. status[%d]\n", status);
			AST_JSON_UNREF(j_tmp);
			return 0;
		}

		// set status object
		ast_json_object_set(j_tmp, "status", ast_json_integer_create(status));
	}

	// update campaign
	ret = update_campaign(j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(ret == false) {
		astman_send_error(s, m, "Error encountered while updating campaign");
		ast_log(LOG_WARNING, "OutCampaignUpdate failed.\n");
		return 0;
	}
	astman_send_ack(s, m, "Campaign updated successfully");
	ast_log(LOG_NOTICE, "OutCampaignUpdate succeed.\n");

	return 0;
}

/**
 * AMI Action handler
 * Action: OutCampaignShow
 * @param s
 * @param m
 * @return
 */
static int manager_out_campaign_show(struct mansession *s, const struct message *m)
{
	const char* tmp_const;
	struct ast_json* j_tmp;
	struct ast_json* j_arr;
	int i;
	int size;
	char* action_id;

	ast_log(LOG_VERBOSE, "AMI request. OutCampaignShow.\n");

	tmp_const = message_get_header(m, "ActionID");
	if((tmp_const != NULL) && (strlen(tmp_const) != 0)) {
		ast_asprintf(&action_id, "ActionID: %s\r\n", tmp_const);
	}
	else {
		ast_asprintf(&action_id, "%s", "");
	}

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const != NULL) {
		j_tmp = get_campaign(tmp_const);
		if(j_tmp == NULL) {
			astman_send_error(s, m, "No such campaign");
			ast_free(action_id);
			return 0;
		}

		astman_send_listack(s, m, "Campaign List will follow", "start");

		manager_out_campaign_entry(s, m, j_tmp, action_id);
		AST_JSON_UNREF(j_tmp);

		astman_send_list_complete_start(s, m, "OutCampaignListComplete", 1);
		astman_send_list_complete_end(s);
	}
	else {
		j_arr = get_campaigns_all();
		size = ast_json_array_size(j_arr);

		astman_send_listack(s, m, "Campaign List will follow", "start");
		for(i = 0; i < size; i++) {
			j_tmp = ast_json_array_get(j_arr, i);
			if(j_tmp == NULL) {
				continue;
			}
			manager_out_campaign_entry(s, m, j_tmp, action_id);
		}
		astman_send_list_complete_start(s, m, "OutCampaignListComplete", size);
		astman_send_list_complete_end(s);
		AST_JSON_UNREF(j_arr);
	}

	ast_log(LOG_NOTICE, "OutCampaignShow succeed.\n");
	ast_free(action_id);
	return 0;
}

/**
 * AMI Action handler
 * Action: OutCampaignStatShow
 * @param s
 * @param m
 * @return
 */
static int manager_out_campaign_stat_show(struct mansession *s, const struct message *m)
{
	const char* tmp_const;
	struct ast_json* j_tmp;
	struct ast_json* j_arr;
	int i;
	int size;
	char* action_id;

	ast_log(LOG_VERBOSE, "AMI request. OutCampaignStatShow.\n");

	tmp_const = message_get_header(m, "ActionID");
	if((tmp_const != NULL) && (strlen(tmp_const) != 0)) {
		ast_asprintf(&action_id, "ActionID: %s\r\n", tmp_const);
	}
	else {
		ast_asprintf(&action_id, "%s", "");
	}

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const != NULL) {
		j_tmp = get_campaign_stat(tmp_const);
		if(j_tmp == NULL) {
			astman_send_error(s, m, "No such campaign");
			ast_free(action_id);
			return 0;
		}

		astman_send_listack(s, m, "Campaign Stat List will follow", "start");

		manager_out_campaign_stat_entry(s, m, j_tmp, action_id);
		AST_JSON_UNREF(j_tmp);

		astman_send_list_complete_start(s, m, "OutCampaignStatListComplete", 1);
		astman_send_list_complete_end(s);
	}
	else {
		j_arr = get_campaigns_stat_all();
		size = ast_json_array_size(j_arr);

		astman_send_listack(s, m, "Campaign Stat List will follow", "start");
		for(i = 0; i < size; i++) {
			j_tmp = ast_json_array_get(j_arr, i);
			if(j_tmp == NULL) {
				continue;
			}
			manager_out_campaign_stat_entry(s, m, j_tmp, action_id);
		}
		astman_send_list_complete_start(s, m, "OutCampaignStatListComplete", size);
		astman_send_list_complete_end(s);
		AST_JSON_UNREF(j_arr);
	}

	ast_log(LOG_NOTICE, "OutCampaignStatShow succeed.\n");
	ast_free(action_id);
	return 0;
}


/**
 * AMI Action handler
 * Action: OutPlanCreate
 * @param s
 * @param m
 * @return
 */
static int manager_out_plan_create(struct mansession *s, const struct message *m)
{
	int ret;
	struct ast_json* j_tmp;

	ast_log(LOG_VERBOSE, "AMI request. OutPlanCreate.\n");

	j_tmp = create_json_plan(m);
	if(j_tmp == NULL) {
		astman_send_error(s, m, "Error encountered while creating plan");
		ast_log(LOG_WARNING, "Could not create plan json.\n");
		return 0;
	}

	ret = create_plan(j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(ret == false) {
		astman_send_error(s, m, "Error encountered while creating plan");
		ast_log(LOG_WARNING, "OutPlanCreate failed.\n");
		return 0;
	}

	astman_send_ack(s, m, "Plan created successfully");
	ast_log(LOG_NOTICE, "OutPlanCreate succeed.\n");

	return 0;
}

/**
 * AMI Action handler
 * Action: OutPlanDelete
 * @param s
 * @param m
 * @return
 */
static int manager_out_plan_delete(struct mansession *s, const struct message *m)
{
	const char* tmp_const;
	int ret;

	ast_log(LOG_VERBOSE, "AMI request. OutPlanDelete.\n");

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const == NULL) {
		astman_send_error(s, m, "Error encountered while deleting plan");
		return 0;
	}

	ret = delete_plan(tmp_const);
	if(ret == false) {
		astman_send_error(s, m, "Error encountered while deleting plan");
		ast_log(LOG_WARNING, "OutPlanDelete failed.\n");
		return 0;
	}
	astman_send_ack(s, m, "Plan deleted successfully");
	ast_log(LOG_NOTICE, "OutPlanDelete succeed.\n");

	return 0;
}

/**
 * AMI Action handler
 * Action: OutPlanUpdate
 * @param s
 * @param m
 * @return
 */
static int manager_out_plan_update(struct mansession *s, const struct message *m)
{
	const char* tmp_const;
	int ret;
	struct ast_json* j_tmp;

	ast_log(LOG_VERBOSE, "AMI request. OutPlanUpdate.\n");

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const == NULL) {
		astman_send_error(s, m, "Error encountered while updating plan");
		ast_log(LOG_WARNING, "OutPlanUpdate failed.\n");
		return 0;
	}

	j_tmp = create_json_plan(m);

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "uuid", ast_json_string_create(tmp_const));}

	ret = update_plan(j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(ret == false) {
		astman_send_error(s, m, "Error encountered while updating plan");
		ast_log(LOG_WARNING, "OutPlanUpdate failed.\n");
		return 0;
	}

	astman_send_ack(s, m, "Plan updated successfully");
	ast_log(LOG_NOTICE, "OutPlanUpdate succeed.\n");

	return 0;
}

/**
 * AMI Action handler
 * Action: OutPlanShow
 * @param s
 * @param m
 * @return
 */
static int manager_out_plan_show(struct mansession *s, const struct message *m)
{
	const char* tmp_const;
	struct ast_json* j_tmp;
	struct ast_json* j_arr;
	int i;
	int size;
	char* action_id;

	ast_log(LOG_VERBOSE, "AMI request. OutPlanShow.\n");

	tmp_const = message_get_header(m, "ActionID");
	if((tmp_const != NULL) && (strlen(tmp_const) != 0)) {
		ast_asprintf(&action_id, "ActionID: %s\r\n", tmp_const);
	}
	else {
		ast_asprintf(&action_id, "%s", "");
	}

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const != NULL) {
		j_tmp = get_plan(tmp_const);
		if(j_tmp == NULL) {
			astman_send_error(s, m, "No such plan");
			ast_free(action_id);
			return 0;
		}

		astman_send_listack(s, m, "Plan List will follow", "start");

		manager_out_plan_entry(s, m, j_tmp, action_id);
		AST_JSON_UNREF(j_tmp);

		astman_send_list_complete_start(s, m, "OutPlanListComplete", 1);
		astman_send_list_complete_end(s);
	}
	else {
		j_arr = get_plans_all();
		size = ast_json_array_size(j_arr);

		astman_send_listack(s, m, "Plan List will follow", "start");
		for(i = 0; i < size; i++) {
			j_tmp = ast_json_array_get(j_arr, i);
			if(j_tmp == NULL) {
				continue;
			}
			manager_out_plan_entry(s, m, j_tmp, action_id);
		}
		astman_send_list_complete_start(s, m, "OutPlanListComplete", size);
		astman_send_list_complete_end(s);
		AST_JSON_UNREF(j_arr);
	}

	ast_free(action_id);
	ast_log(LOG_NOTICE, "OutPlanShow succeed.\n");

	return 0;
}

/**
 * AMI Action handler
 * Action: OutDlmaCreate
 * @param s
 * @param m
 * @return
 */
static int manager_out_dlma_create(struct mansession *s, const struct message *m)
{
	struct ast_json* j_tmp;
	int ret;

	ast_log(LOG_VERBOSE, "AMI request. OutDlmaCreate.\n");

	j_tmp = create_json_dlma(m);
	if(j_tmp == NULL) {
		astman_send_error(s, m, "Error encountered while creating dlma");
		ast_log(LOG_WARNING, "OutDlmaCreate failed.\n");
		return 0;
	}

	ret = create_dlma(j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(ret == false) {
		astman_send_error(s, m, "Error encountered while creating dlma");
		ast_log(LOG_WARNING, "OutDlmaCreate failed.\n");
		return 0;
	}
	astman_send_ack(s, m, "Dlma created successfully");
	ast_log(LOG_NOTICE, "OutDlmaCreate succeed.\n");

	return 0;
}

/**
 * OutDlmaUpdate AMI message handler.
 * @param s
 * @param m
 * @return
 */
static int manager_out_dlma_update(struct mansession *s, const struct message *m)
{
	struct ast_json* j_tmp;
	const char* tmp_const;
	int ret;

	// validate
	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const == NULL) {
		astman_send_error(s, m, "Error encountered while updating dlma");
		return 0;
	}

	j_tmp = create_json_dlma(m);
	if(j_tmp == NULL) {
		astman_send_error(s, m, "Error encountered while updating dlma");
		return 0;
	}

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "uuid", ast_json_string_create(tmp_const));}

	ret = update_dlma(j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(ret == false) {
		astman_send_error(s, m, "Error encountered while updating dlma");
		return 0;
	}
	astman_send_ack(s, m, "Dlma updated successfully");

	return 0;
}
/**
 * OutDlmaDelete AMI message handler.
 * @param s
 * @param m
 * @return
 */
static int manager_out_dlma_delete(struct mansession *s, const struct message *m)
{
	const char* tmp_const;
	int ret;

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const == NULL) {
		astman_send_error(s, m, "Error encountered while deleting dlma");
		return 0;
	}

	ret = delete_dlma(tmp_const);
	if(ret == false) {
		astman_send_error(s, m, "Error encountered while deleting dlma");
		return 0;
	}
	astman_send_ack(s, m, "Dlma deleted successfully");

	return 0;
}

/**
 * OutDlmaShow AMI message handle.
 * @param s
 * @param m
 * @return
 */
static int manager_out_dlma_show(struct mansession *s, const struct message *m)
{
	const char* tmp_const;
	struct ast_json* j_tmp;
	struct ast_json* j_arr;
	int i;
	int size;
	char* action_id;

	tmp_const = message_get_header(m, "ActionID");
	if((tmp_const != NULL) && (strlen(tmp_const) != 0)) {
		ast_asprintf(&action_id, "ActionID: %s\r\n", tmp_const);
	}
	else {
		ast_asprintf(&action_id, "%s", "");
	}

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const != NULL) {
		j_tmp = get_dlma(tmp_const);
		if(j_tmp == NULL) {
			astman_send_error(s, m, "No such dlma");
			ast_free(action_id);
			return 0;
		}

		astman_send_listack(s, m, "Dlma List will follow", "start");

		manager_out_dlma_entry(s, m, j_tmp, action_id);
		AST_JSON_UNREF(j_tmp);

		astman_send_list_complete_start(s, m, "OutDlmaListComplete", 1);
		astman_send_list_complete_end(s);
	}
	else {
		j_arr = get_dlmas_all();
		size = ast_json_array_size(j_arr);

		astman_send_listack(s, m, "Dlma List will follow", "start");
		for(i = 0; i < size; i++) {
			j_tmp = ast_json_array_get(j_arr, i);
			if(j_tmp == NULL) {
				continue;
			}
			manager_out_dlma_entry(s, m, j_tmp, action_id);
		}
		astman_send_list_complete_start(s, m, "OutDlmaListComplete", size);
		astman_send_list_complete_end(s);
		AST_JSON_UNREF(j_arr);
	}
	ast_free(action_id);
	return 0;
}

/**
 * OutQueueCreate AMI message handler.
 * @param s
 * @param m
 * @return
 */
static int manager_out_queue_create(struct mansession *s, const struct message *m)
{
	struct ast_json* j_tmp;
	const char* tmp_const;
	int ret;

	j_tmp = ast_json_object_create();

	tmp_const = message_get_header(m, "Name");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "name", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Detail");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "detail", ast_json_string_create(tmp_const));}

	ret = create_queue(j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(ret == false) {
		astman_send_error(s, m, "Error encountered while creating queue");
		return 0;
	}
	astman_send_ack(s, m, "Queue created successfully");

	return 0;
}

/**
 * OutQueueUpdate AMI message handler.
 * @param s
 * @param m
 * @return
 */
static int manager_out_queue_update(struct mansession *s, const struct message *m)
{
	struct ast_json* j_tmp;
	const char* tmp_const;
	int ret;

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const == NULL) {
		astman_send_error(s, m, "Error encountered while updating queue");
		return 0;
	}

	j_tmp = ast_json_object_create();

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "uuid", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Name");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "name", ast_json_string_create(tmp_const));}

	tmp_const = message_get_header(m, "Detail");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "detail", ast_json_string_create(tmp_const));}

	ret = update_queue(j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(ret == false) {
		astman_send_error(s, m, "Error encountered while updating queue");
		return 0;
	}
	astman_send_ack(s, m, "Queue updated successfully");

	return 0;
}
/**
 * OutQueueDelete AMI message handler.
 * @param s
 * @param m
 * @return
 */
static int manager_out_queue_delete(struct mansession *s, const struct message *m)
{
	const char* tmp_const;
	int ret;

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const == NULL) {
		astman_send_error(s, m, "Error encountered while deleting queue");
		return 0;
	}

	ret = delete_queue(tmp_const);
	if(ret == false) {
		astman_send_error(s, m, "Error encountered while deleting queue");
		return 0;
	}
	astman_send_ack(s, m, "Queue deleted successfully");

	return 0;
}

/**
 * OutQueueShow AMI message handle.
 * @param s
 * @param m
 * @return
 */
static int manager_out_queue_show(struct mansession *s, const struct message *m)
{
	const char* tmp_const;
	struct ast_json* j_tmp;
	struct ast_json* j_arr;
	int i;
	int size;
	char* action_id;

	tmp_const = message_get_header(m, "ActionID");
	if((tmp_const != NULL) && (strlen(tmp_const) != 0)) {
		ast_asprintf(&action_id, "ActionID: %s\r\n", tmp_const);
	}
	else {
		ast_asprintf(&action_id, "%s", "");
	}

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const != NULL) {
		j_tmp = get_queue(tmp_const);
		if(j_tmp == NULL) {
			ast_log(LOG_WARNING, "Could not find queue. uuid[%s]\n", tmp_const);
			astman_send_error(s, m, "Error encountered while show queue");
			ast_free(action_id);
			return 0;
		}

		astman_send_listack(s, m, "Queue List will follow", "start");

		manager_out_queue_entry(s, m, j_tmp, action_id);

		astman_send_list_complete_start(s, m, "OutQueueListComplete", 1);
		astman_send_list_complete_end(s);
	}
	else {
		j_arr = get_queues_all();
		size = ast_json_array_size(j_arr);

		astman_send_listack(s, m, "Queue List will follow", "start");
		for(i = 0; i < size; i++) {
			j_tmp = ast_json_array_get(j_arr, i);
			if(j_tmp == NULL) {
				continue;
			}
			manager_out_queue_entry(s, m, j_tmp, action_id);
		}
		astman_send_list_complete_start(s, m, "OutQueueListComplete", size);
		astman_send_list_complete_end(s);
		AST_JSON_UNREF(j_arr);
	}
	ast_free(action_id);
	return 0;
}

/**
 * AMI Action handler
 * Action: OutDialingShow
 * @param s
 * @param m
 * @return
 */
static int manager_out_dialing_show(struct mansession *s, const struct message *m)
{
	const char* tmp_const;
	rb_dialing* dialing;
	struct ao2_iterator iter;
	char* action_id;

	ast_log(LOG_VERBOSE, "AMI request. OutDialingShow.\n");

	tmp_const = message_get_header(m, "ActionID");
	if((tmp_const != NULL) && (strlen(tmp_const) != 0)) {
		ast_asprintf(&action_id, "ActionID: %s\r\n", tmp_const);
	}
	else {
		ast_asprintf(&action_id, "%s", "");
	}

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const != NULL) {
		dialing = rb_dialing_find_chan_uuid(tmp_const);
		if(dialing == NULL) {
			astman_send_error(s, m, "No such dialing");
			ast_log(LOG_WARNING, "OutDialingShow failed.\n");
			ast_free(action_id);
			return 0;
		}

		manager_evt_out_dialing_entry(s, m, dialing, action_id);
		astman_send_list_complete_start(s, m, "OutDialingListComplete", 1);
		astman_send_list_complete_end(s);
	}
	else {
		astman_send_listack(s, m, "Dialing List will follow", "start");
		iter = rb_dialing_iter_init();
		while(1) {
			dialing = rb_dialing_iter_next(&iter);
			if(dialing == NULL) {
				break;
			}
			manager_evt_out_dialing_entry(s, m, dialing, action_id);
		}
		astman_send_list_complete_start(s, m, "OutDialingListComplete", 1);
		astman_send_list_complete_end(s);
		rb_dialing_iter_destroy(&iter);
	}

	ast_log(LOG_NOTICE, "OutDialingShow succeed.\n");
	ast_free(action_id);
	return 0;
}

/**
* AMI Action handler
* Action: OutDialingSummary
* @param s
* @param m
* @return
*/
static int manager_out_dialing_summary(struct mansession *s, const struct message *m)
{
   const char* tmp_const;
   char* action_id;

   ast_log(LOG_VERBOSE, "AMI request. OutDialingSummary.\n");

   tmp_const = message_get_header(m, "ActionID");
   if((tmp_const != NULL) && (strlen(tmp_const) != 0)) {
	   ast_asprintf(&action_id, "ActionID: %s\r\n", tmp_const);
   }
   else {
	   ast_asprintf(&action_id, "%s", "");
   }

   astman_send_listack(s, m, "Summary List will follow", "start");

   manager_evt_out_dialing_summary(s, m, action_id);

   astman_send_list_complete_start(s, m, "OutSummaryListComplete", 1);
   astman_send_list_complete_end(s);

   ast_log(LOG_NOTICE, "OutDialingSummary succeed.\n");
   ast_free(action_id);
   return 0;
}

/**
 * AMI Action handler
 * Action: OutDestinationCreate
 * @param s
 * @param m
 * @return
 */
static int manager_out_destination_create(struct mansession *s, const struct message *m)
{
	struct ast_json* j_tmp;
	int ret;

	ast_log(LOG_VERBOSE, "AMI request. OutDestinationCreate.\n");

	j_tmp = create_json_destination(m);
	if(j_tmp == NULL) {
		astman_send_error(s, m, "Error encountered while creating destination");
		ast_log(LOG_NOTICE, "OutDestinationCreate failed.\n");
		return 0;
	}

	ret = create_destination(j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(ret == false) {
		astman_send_error(s, m, "Error encountered while creating destination");
		ast_log(LOG_NOTICE, "OutDestinationCreate failed.\n");
		return 0;
	}
	astman_send_ack(s, m, "Destination created successfully");
	ast_log(LOG_NOTICE, "OutDestinationCreate succeed.\n");

	return 0;
}

/**
 * AMI Action handler
 * Action: OutDestinationUpdate
 * @param s
 * @param m
 * @return
 */
static int manager_out_destination_update(struct mansession *s, const struct message *m)
{
	struct ast_json* j_tmp;
	const char* tmp_const;
	int ret;

	ast_log(LOG_VERBOSE, "AMI request. OutDestinationCreate.\n");

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const == NULL) {
		astman_send_error(s, m, "Error encountered while updating destination");
		ast_log(LOG_WARNING, "OutDestinationUpdate failed.\n");
		return 0;
	}

	j_tmp = create_json_destination(m);

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const != NULL) {ast_json_object_set(j_tmp, "uuid", ast_json_string_create(tmp_const));}

	ret = update_destination(j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(ret == false) {
		astman_send_error(s, m, "Error encountered while updating destination");
		ast_log(LOG_NOTICE, "OutDestinationUpdate failed.\n");
		return 0;
	}
	astman_send_ack(s, m, "Destination updated successfully");
	ast_log(LOG_NOTICE, "OutDestinationUpdate succeed.\n");

	return 0;
}

/**
 * AMI Action handler
 * Action: OutDestinationDelete
 * @param s
 * @param m
 * @return
 */
static int manager_out_destination_delete(struct mansession *s, const struct message *m)
{
	const char* tmp_const;
	int ret;

	ast_log(LOG_VERBOSE, "AMI request. OutDestinationDelete.\n");

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const == NULL) {
		astman_send_error(s, m, "Error encountered while deleting destination");
		ast_log(LOG_WARNING, "OutDestinationDelete failed.\n");
		return 0;
	}

	ret = delete_destination(tmp_const);
	if(ret == false) {
		astman_send_error(s, m, "Error encountered while deleting destination");
		ast_log(LOG_WARNING, "OutDestinationDelete failed.\n");
		return 0;
	}
	astman_send_ack(s, m, "Destination deleted successfully");
	ast_log(LOG_NOTICE, "OutDestinationDelete succeed.\n");

	return 0;
}

/**
 * OutDestinationShow AMI message handle.
 * @param s
 * @param m
 * @return
 */
static int manager_out_destination_show(struct mansession *s, const struct message *m)
{
	const char* tmp_const;
	struct ast_json* j_tmp;
	struct ast_json* j_arr;
	int i;
	int size;
	char* action_id;

	tmp_const = message_get_header(m, "ActionID");
	if((tmp_const != NULL) && (strlen(tmp_const) != 0)) {
		ast_asprintf(&action_id, "ActionID: %s\r\n", tmp_const);
	}
	else {
		ast_asprintf(&action_id, "%s", "");
	}

	tmp_const = message_get_header(m, "Uuid");
	if(tmp_const != NULL) {
		j_tmp = get_destination(tmp_const);
		if(j_tmp == NULL) {
			astman_send_error(s, m, "No such destination");
			ast_free(action_id);
			return 0;
		}

		astman_send_listack(s, m, "Destination List will follow", "start");

		manager_out_destination_entry(s, m, j_tmp, action_id);
		AST_JSON_UNREF(j_tmp);

		astman_send_list_complete_start(s, m, "OutDestinationListComplete", 1);
		astman_send_list_complete_end(s);
	}
	else {
		j_arr = get_destinations_all();
		size = ast_json_array_size(j_arr);

		astman_send_listack(s, m, "Destination List will follow", "start");
		for(i = 0; i < size; i++) {
			j_tmp = ast_json_array_get(j_arr, i);
			if(j_tmp == NULL) {
				continue;
			}
			manager_out_destination_entry(s, m, j_tmp, action_id);
		}
		astman_send_list_complete_start(s, m, "OutDestinationListComplete", size);
		astman_send_list_complete_end(s);
		AST_JSON_UNREF(j_arr);
	}
	ast_free(action_id);
	return 0;
}

/**
 * Parsing the AMI message
 * Return the json string
 *
 * Variable: var1=val1
 * Variable: var2=val2
 *
 * "{\"var1\":\"val1\", \"var2\":\"val2\"}"
 * \param m
 * \return
 */
static char* get_variables(const struct message *m)
{
	struct ast_variable* var_org;
	struct ast_variable* var;
	struct ast_json* j_res;
	char* res;
	char* tmp;

	var_org = astman_get_variables_order(m, ORDER_NATURAL);
	if(var_org == NULL) {
		return NULL;
	}

	// get every each variables.
	var = var_org;
	j_res = ast_json_object_create();
	while(var != NULL) {
		ast_json_object_set(j_res, var->name, ast_json_string_create(var->value));
		var = var->next;
	}
	ast_variables_destroy(var_org);

	tmp = ast_json_dump_string_format(j_res, AST_JSON_COMPACT);
	res = ast_strdup(tmp);

	ast_json_free(tmp);
	AST_JSON_UNREF(j_res);

	return res;
}


struct ast_cli_entry cli_out[] = {
	AST_CLI_DEFINE(out_show_campaigns,		"List all defined outbound campaigns"),
	AST_CLI_DEFINE(out_show_campaign,			"Shows detail campaign info"),

	AST_CLI_DEFINE(out_show_plans,				"List all defined outbound plans"),
	AST_CLI_DEFINE(out_show_plan, 				"Show detail given plan info"),

	AST_CLI_DEFINE(out_show_dlmas,				"List all defined outbound dlmas"),
	AST_CLI_DEFINE(out_show_dlma,					"Show detail given dlma info"),
	AST_CLI_DEFINE(out_show_dlma_list,		"Show list of dlma dial list"),

	AST_CLI_DEFINE(out_show_dl,						"Show detail given dl info"),
	AST_CLI_DEFINE(out_show_dls, 					"Show list of dlma dial list"),

	AST_CLI_DEFINE(out_show_destinations, "List all defined outbound destinations"),
	AST_CLI_DEFINE(out_show_destination, 	"Show detail given destination info"),

	AST_CLI_DEFINE(out_show_dialings,			"List currently on serviced dialings"),
	AST_CLI_DEFINE(out_show_dialing,			"Show detail given dialing info"),

	AST_CLI_DEFINE(out_set_campaign,			"Set campaign parameters"),
	AST_CLI_DEFINE(out_create_campaign,		"Create new campaign"),
	AST_CLI_DEFINE(out_delete_campaign,		"Delete campaign")
};

int init_cli_handler(void)
{
	int err;

	err = 0;

	err |= ast_cli_register_multiple(cli_out, ARRAY_LEN(cli_out));
	err |= ast_manager_register2("OutCampaignCreate", EVENT_FLAG_COMMAND, manager_out_campaign_create, NULL, NULL, NULL);
	err |= ast_manager_register2("OutCampaignDelete", EVENT_FLAG_COMMAND, manager_out_campaign_delete, NULL, NULL, NULL);
	err |= ast_manager_register2("OutCampaignUpdate", EVENT_FLAG_COMMAND, manager_out_campaign_update, NULL, NULL, NULL);
	err |= ast_manager_register2("OutCampaignShow", EVENT_FLAG_COMMAND, manager_out_campaign_show, NULL, NULL, NULL);
	err |= ast_manager_register2("OutCampaignStatShow", EVENT_FLAG_COMMAND, manager_out_campaign_stat_show, NULL, NULL, NULL);
	err |= ast_manager_register2("OutPlanCreate", EVENT_FLAG_COMMAND, manager_out_plan_create, NULL, NULL, NULL);
	err |= ast_manager_register2("OutPlanDelete", EVENT_FLAG_COMMAND, manager_out_plan_delete, NULL, NULL, NULL);
	err |= ast_manager_register2("OutPlanUpdate", EVENT_FLAG_COMMAND, manager_out_plan_update, NULL, NULL, NULL);
	err |= ast_manager_register2("OutPlanShow", EVENT_FLAG_COMMAND, manager_out_plan_show, NULL, NULL, NULL);
	err |= ast_manager_register2("OutDlmaCreate", EVENT_FLAG_COMMAND, manager_out_dlma_create, NULL, NULL, NULL);
	err |= ast_manager_register2("OutDlmaUpdate", EVENT_FLAG_COMMAND, manager_out_dlma_update, NULL, NULL, NULL);
	err |= ast_manager_register2("OutDlmaDelete", EVENT_FLAG_COMMAND, manager_out_dlma_delete, NULL, NULL, NULL);
	err |= ast_manager_register2("OutDlmaShow", EVENT_FLAG_COMMAND, manager_out_dlma_show, NULL, NULL, NULL);
	err |= ast_manager_register2("OutDlListCreate", EVENT_FLAG_COMMAND, manager_out_dl_list_create, NULL, NULL, NULL);
	err |= ast_manager_register2("OutDlListUpdate", EVENT_FLAG_COMMAND, manager_out_dl_list_update, NULL, NULL, NULL);
	err |= ast_manager_register2("OutDlListDelete", EVENT_FLAG_COMMAND, manager_out_dl_list_delete, NULL, NULL, NULL);
	err |= ast_manager_register2("OutDlListShow", EVENT_FLAG_COMMAND, manager_out_dl_list_show, NULL, NULL, NULL);
	err |= ast_manager_register2("OutQueueCreate", EVENT_FLAG_COMMAND, manager_out_queue_create, NULL, NULL, NULL);
	err |= ast_manager_register2("OutQueueUpdate", EVENT_FLAG_COMMAND, manager_out_queue_update, NULL, NULL, NULL);
	err |= ast_manager_register2("OutQueueDelete", EVENT_FLAG_COMMAND, manager_out_queue_delete, NULL, NULL, NULL);
	err |= ast_manager_register2("OutQueueShow", EVENT_FLAG_COMMAND, manager_out_queue_show, NULL, NULL, NULL);
	err |= ast_manager_register2("OutDialingShow", EVENT_FLAG_COMMAND, manager_out_dialing_show, NULL, NULL, NULL);
	err |= ast_manager_register2("OutDialingSummary", EVENT_FLAG_COMMAND, manager_out_dialing_summary, NULL, NULL, NULL);
	err |= ast_manager_register2("OutDestinationCreate", EVENT_FLAG_COMMAND, manager_out_destination_create, NULL, NULL, NULL);
	err |= ast_manager_register2("OutDestinationDelete", EVENT_FLAG_COMMAND, manager_out_destination_delete, NULL, NULL, NULL);
	err |= ast_manager_register2("OutDestinationUpdate", EVENT_FLAG_COMMAND, manager_out_destination_update, NULL, NULL, NULL);
	err |= ast_manager_register2("OutDestinationShow", EVENT_FLAG_COMMAND, manager_out_destination_show, NULL, NULL, NULL);


	if(err != 0) {
		term_cli_handler();
		return false;
	}

	return true;
}

void term_cli_handler(void)
{
	ast_cli_unregister_multiple(cli_out, ARRAY_LEN(cli_out));
	ast_manager_unregister("OutCampaignCreate");
	ast_manager_unregister("OutCampaignDelete");
	ast_manager_unregister("OutCampaignUpdate");
	ast_manager_unregister("OutCampaignShow");
	ast_manager_unregister("OutCampaignStatShow");
	ast_manager_unregister("OutDlListShow");
	ast_manager_unregister("OutPlanCreate");
	ast_manager_unregister("OutPlanDelete");
	ast_manager_unregister("OutPlanUpdate");
	ast_manager_unregister("OutPlanShow");
	ast_manager_unregister("OutDlmaCreate");
	ast_manager_unregister("OutDlmaUpdate");
	ast_manager_unregister("OutDlmaDelete");
	ast_manager_unregister("OutDlmaShow");
	ast_manager_unregister("OutDlListCreate");
	ast_manager_unregister("OutDlListUpdate");
	ast_manager_unregister("OutDlListDelete");
	ast_manager_unregister("OutDlListShow");
	ast_manager_unregister("OutQueueCreate");
	ast_manager_unregister("OutQueueUpdate");
	ast_manager_unregister("OutQueueDelete");
	ast_manager_unregister("OutQueueShow");
	ast_manager_unregister("OutDialingShow");
	ast_manager_unregister("OutDialingSummary");
	ast_manager_unregister("OutDestinationCreate");
	ast_manager_unregister("OutDestinationUpdate");
	ast_manager_unregister("OutDestinationDelete");
	ast_manager_unregister("OutDestinationShow");


	return;
}
