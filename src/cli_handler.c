/*
 * cli_handler.c
 *
 *  Created on: Nov 22, 2015
 *      Author: pchero
 */

#include "asterisk.h"
#include "asterisk/manager.h"
#include "asterisk/module.h"
#include "asterisk/json.h"
#include "asterisk/xml.h"
#include "asterisk/cli.h"

#include <stdbool.h>

#include "cli_handler.h"
#include "event_handler.h"
#include "dialing_handler.h"
#include "campaign_handler.h"
#include "dl_handler.h"
#include "plan_handler.h"
#include "queue_handler.h"

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

#define CAMP_FORMAT2 "%-36.36s %-40.40s %-6.6s %-36.36s %-36.36s %-40.40s\n"
#define CAMP_FORMAT3 "%-36.36s %-40.40s %6ld %-36.36s %-36.36s %-40.40s\n"

static char* _out_show_campaigns(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
    struct ast_json* j_res;
    struct ast_json* j_tmp;
    int size;
    int i;

    j_res = get_campaigns_all();

    if (!s) {
        /* Normal list */
        ast_cli(fd, CAMP_FORMAT2, "UUID", "Name", "Status", "Plan", "DLMA", "Detail");
    }

    size = ast_json_array_size(j_res);
    for(i = 0; i < size; i++) {
        j_tmp = ast_json_array_get(j_res, i);
        if(j_tmp == NULL) {
            continue;
        }
        ast_cli(fd, CAMP_FORMAT3,
                ast_json_string_get(ast_json_object_get(j_tmp, "uuid")) ? : "",
                ast_json_string_get(ast_json_object_get(j_tmp, "name")) ? : "",
                ast_json_integer_get(ast_json_object_get(j_tmp, "status")),
                ast_json_string_get(ast_json_object_get(j_tmp, "plan")) ? : "",
                ast_json_string_get(ast_json_object_get(j_tmp, "dlma")) ? : "",
                ast_json_string_get(ast_json_object_get(j_tmp, "detail")) ? : ""
                );
    }
    ast_json_unref(j_res);

    return CLI_SUCCESS;
}

/*! \brief CLI for show campaigns.
 */
static char *out_show_campaigns(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
//    struct __show_chan_arg arg = { .fd = a->fd, .numchans = 0 };

    if (cmd == CLI_INIT) {
        e->command = "out show campaigns";
        e->usage =
            "Usage: out show campaigns\n"
            "       Lists all currently registered campaigns.\n";
        return NULL;
    } else if (cmd == CLI_GENERATE) {
        return NULL;
    }
    return _out_show_campaigns(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

static char* _out_show_campaign(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
    struct ast_json* j_res;

    j_res = get_campaign(argv[3]);
    if(j_res == NULL) {
        ast_cli(fd, "Could not find campaign info. uuid[%s]\n", argv[3]);
        ast_cli(fd, "\n");
        return CLI_SUCCESS;
    }

    ast_cli(fd, "  Uuid   : %s\n", ast_json_string_get(ast_json_object_get(j_res, "uuid")));
    ast_cli(fd, "  Name   : %s\n", ast_json_string_get(ast_json_object_get(j_res, "name")));
    ast_cli(fd, "  Detail : %s\n", ast_json_string_get(ast_json_object_get(j_res, "detail")));
    ast_cli(fd, "  Status : %ld\n", ast_json_integer_get(ast_json_object_get(j_res, "status")));
    ast_cli(fd, "  Plan : %s\n", ast_json_string_get(ast_json_object_get(j_res, "plan")));
    ast_cli(fd, "  DLMA : %s\n", ast_json_string_get(ast_json_object_get(j_res, "dlma")));
    ast_cli(fd, "\n");

    ast_json_unref(j_res);

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
            "       Set campaign's status.\n";
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
            "Usage: out show campaign <uuid>\n"
            "       Shows all details on one campaign and the current status.\n";
        return NULL;
    } else if (cmd == CLI_GENERATE) {
        return NULL;
    }
    return _out_show_campaign(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

#define PLAN_FORMAT2 "%-36.36s %-40.40s %-40.40s %-8.8s %-11.11s %-5.5s %-5.5s\n"
#define PLAN_FORMAT3 "%-36.36s %-40.40s %-40.40s %-8.8s %11ld %-5.5s %-5.5s\n"

static char* _out_show_plans(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
    struct ast_json* j_res;
    struct ast_json* j_tmp;
    int size;
    int i;

    j_res = get_plans_all();

    if (!s) {
        /* Normal list */
        ast_cli(fd, PLAN_FORMAT2, "UUID", "Name", "Detail", "DialMode", "DialTimeout", "Queue", "Trunk");
    }

    size = ast_json_array_size(j_res);
    for(i = 0; i < size; i++) {
        j_tmp = ast_json_array_get(j_res, i);
        if(j_tmp == NULL) {
            continue;
        }
        ast_cli(fd, PLAN_FORMAT3,
                ast_json_string_get(ast_json_object_get(j_tmp, "uuid")) ? : "",
                ast_json_string_get(ast_json_object_get(j_tmp, "name")) ? : "",
                ast_json_string_get(ast_json_object_get(j_tmp, "detail")) ? : "",
                ast_json_string_get(ast_json_object_get(j_tmp, "dial_mode")) ? : "",
                ast_json_integer_get(ast_json_object_get(j_tmp, "dial_timeout")) ? : -1,
                ast_json_string_get(ast_json_object_get(j_tmp, "queue_name")) ? : "",
                ast_json_string_get(ast_json_object_get(j_tmp, "trunk_name")) ? : ""
                );
    }
    ast_json_unref(j_res);

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
            "       Lists all currently registered plans.\n";
        return NULL;
    } else if (cmd == CLI_GENERATE) {
        return NULL;
    }
    return _out_show_plans(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

#define DLMA_FORMAT2 "%-36.36s %-40.40s %-40.40s %-30.30s\n"
#define DLMA_FORMAT3 "%-36.36s %-40.40s %-40.40s %-30.30s\n"

static char* _out_show_dlmas(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
    struct ast_json* j_res;
    struct ast_json* j_tmp;
    int size;
    int i;

    j_res = get_dlmas_all();

    if (!s) {
        /* Normal list */
        ast_cli(fd, DLMA_FORMAT2, "UUID", "Name", "Detail", "Table");
    }

    size = ast_json_array_size(j_res);
    for(i = 0; i < size; i++) {
        j_tmp = ast_json_array_get(j_res, i);
        if(j_tmp == NULL) {
            continue;
        }
        ast_cli(fd, DLMA_FORMAT3,
                ast_json_string_get(ast_json_object_get(j_tmp, "uuid")) ? : "",
                ast_json_string_get(ast_json_object_get(j_tmp, "name")) ? : "",
                ast_json_string_get(ast_json_object_get(j_tmp, "detail")) ? : "",
                ast_json_string_get(ast_json_object_get(j_tmp, "dl_table")) ? : ""
                );
    }
    ast_json_unref(j_res);

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
            "       Lists all currently registered dlmas.\n";
        return NULL;
    } else if (cmd == CLI_GENERATE) {
        return NULL;
    }
    return _out_show_dlmas(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}


#define DIALING_FORMAT2 "%-36.36s %-5.5s %-20.20s %-10.10s %-10.10s %-10.10s\n"
#define DIALING_FORMAT3 "%-36.36s %-5.5s %-20.20s %-10.10s %-10.10s %-10.10s\n"

static char* _out_show_dialings(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
    struct ast_json* j_res;
    struct ast_json* j_tmp;
    int size;
    int i;

    j_res = rb_dialing_get_all_for_cli();

    if (!s) {
        /* Normal list */
        ast_cli(fd, DIALING_FORMAT2, "UUID", "State", "Channel", "Queue", "MemberName", "TM_Hangup");
    }

    size = ast_json_array_size(j_res);
    for(i = 0; i < size; i++) {
        j_tmp = ast_json_array_get(j_res, i);
        if(j_tmp == NULL) {
            continue;
        }
        ast_cli(fd, DIALING_FORMAT3,
                ast_json_string_get(ast_json_object_get(j_tmp, "uuid")) ? : "",
                ast_json_string_get(ast_json_object_get(j_tmp, "channelstate")) ? : "",
                ast_json_string_get(ast_json_object_get(j_tmp, "channel")) ? : "",
                ast_json_string_get(ast_json_object_get(j_tmp, "queue")) ? : "",
                ast_json_string_get(ast_json_object_get(j_tmp, "membername")) ? : "",
                ast_json_string_get(ast_json_object_get(j_tmp, "tm_hangup")) ? : ""
                );
    }
    ast_json_unref(j_res);

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
            "       Lists all currently on service dialings.\n";
        return NULL;
    } else if (cmd == CLI_GENERATE) {
        return NULL;
    }
    return _out_show_dialings(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

#define DL_LIST_FORMAT2 "%-36.36s %-10.10s %-20.20s\n"
#define DL_LIST_FORMAT3 "%-36.36s %-10.10s %-20.20s\n"


static char* _out_show_dlma_list(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
    // out show dlma list <dlma-uuid> <count=100>
    const char* uuid;
    int count;
    struct ast_json* j_dlma;
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

    j_dlma = get_dlma(uuid);
    if(j_dlma == NULL) {
        return NULL;
    }

    j_dls = get_dl_lists(j_dlma, count);
    ast_json_unref(j_dlma);

    size = ast_json_array_size(j_dls);
    for(i = 0; i < size; i++) {
        j_tmp = ast_json_array_get(j_dls, i);
        if(j_tmp == NULL) {
            continue;
        }

        ast_cli(fd, DL_LIST_FORMAT2,
                ast_json_string_get(ast_json_object_get(j_tmp, "uuid")),
                ast_json_string_get(ast_json_object_get(j_tmp, "name")),
                ast_json_string_get(ast_json_object_get(j_tmp, "detail"))
                );
    }

    ast_json_unref(j_dls);

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
            "       Lists count of dial list.\n";
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
    // out show dl <dl-uuid>
    struct ast_json* j_tmp;
    const char* uuid;

    if(argc != 4) {
        return NULL;
    }

    uuid = argv[3];
    j_tmp = get_dl_list(uuid);
    if(j_tmp == NULL) {
        return NULL;
    }

    ast_cli(fd, DL_FORMAT2, "UUID", "DLMA_UUID",
            "Name", "Detail", "UUI",
            "Status",
            "Dialing_UUID", "Dialing_CAMP_UUID", "Dialing_PLAN_UUID",
            "Email",
            "Number 1", "Number 2", "Number 3", "Number 4", "Number 5", "Number 6", "Number 7", "Number 8",
            "TryCnt 1", "TryCnt 2", "TryCnt 3", "TryCnt 4", "TryCnt 5", "TryCnt 6", "TryCnt 7", "TryCnt 8",
            "ResDial", "ResHangup"
            );

    ast_cli(fd, DL_FORMAT3,
            ast_json_string_get(ast_json_object_get(j_tmp, "uuid")),
            ast_json_string_get(ast_json_object_get(j_tmp, "dlma_uuid")),

            ast_json_string_get(ast_json_object_get(j_tmp, "name")),
            ast_json_string_get(ast_json_object_get(j_tmp, "detail")),
            ast_json_string_get(ast_json_object_get(j_tmp, "uui")),

            ast_json_integer_get(ast_json_object_get(j_tmp, "status")),

            ast_json_string_get(ast_json_object_get(j_tmp, "dialing_uuid")),
            ast_json_string_get(ast_json_object_get(j_tmp, "dialing_camp_uuid")),
            ast_json_string_get(ast_json_object_get(j_tmp, "dialing_plan_uuid")),

            ast_json_string_get(ast_json_object_get(j_tmp, "email")),

            ast_json_string_get(ast_json_object_get(j_tmp, "number_1")),
            ast_json_string_get(ast_json_object_get(j_tmp, "number_2")),
            ast_json_string_get(ast_json_object_get(j_tmp, "number_3")),
            ast_json_string_get(ast_json_object_get(j_tmp, "number_4")),
            ast_json_string_get(ast_json_object_get(j_tmp, "number_5")),
            ast_json_string_get(ast_json_object_get(j_tmp, "number_6")),
            ast_json_string_get(ast_json_object_get(j_tmp, "number_7")),
            ast_json_string_get(ast_json_object_get(j_tmp, "number_8")),

            ast_json_integer_get(ast_json_object_get(j_tmp, "trycnt_1")),
            ast_json_integer_get(ast_json_object_get(j_tmp, "trycnt_2")),
            ast_json_integer_get(ast_json_object_get(j_tmp, "trycnt_3")),
            ast_json_integer_get(ast_json_object_get(j_tmp, "trycnt_4")),
            ast_json_integer_get(ast_json_object_get(j_tmp, "trycnt_5")),
            ast_json_integer_get(ast_json_object_get(j_tmp, "trycnt_6")),
            ast_json_integer_get(ast_json_object_get(j_tmp, "trycnt_7")),
            ast_json_integer_get(ast_json_object_get(j_tmp, "trycnt_8")),

            ast_json_integer_get(ast_json_object_get(j_tmp, "res_dial")),
            ast_json_integer_get(ast_json_object_get(j_tmp, "res_hangup"))
            );

    ast_json_unref(j_tmp);

    return CLI_SUCCESS;

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
            "       Show detail of dl.\n";
        return NULL;
    } else if (cmd == CLI_GENERATE) {
        return NULL;
    }
    return _out_show_dl(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

static char* _out_create_campaign(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
//    out create campaign <name> <plan-uuid> <dlma-uuid> <queue-uuid> [detail]
    struct ast_json* j_camp;
    int ret;

    j_camp = ast_json_object_create();
    if(argc >= 7) { ast_json_object_set(j_camp, "detail", ast_json_string_create(argv[7])); }
    if(argc >= 6) { ast_json_object_set(j_camp, "queue", ast_json_string_create(argv[6])); }
    if(argc >= 5) { ast_json_object_set(j_camp, "dlma", ast_json_string_create(argv[5])); }
    if(argc >= 4) { ast_json_object_set(j_camp, "plan", ast_json_string_create(argv[4])); }
    if(argc >= 3) { ast_json_object_set(j_camp, "name", ast_json_string_create(argv[3])); }

    ret = create_campaign(j_camp);
    ast_json_unref(j_camp);
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
            "       Create new campaign.\n";
        return NULL;
    } else if (cmd == CLI_GENERATE) {
        return NULL;
    }
    return _out_create_campaign(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

static char* _out_delete_campaign(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
//    out delete campaign <camp-uuid>
    int ret;

    if(argc != 4) {
        return CLI_SHOWUSAGE;
    }

    ret = delete_cmapaign(argv[3]);
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
            "       Delete the campaign.\n";
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
static char* manager_get_campaign_str(struct ast_json* j_camp)
{
    char* tmp;

    if(j_camp == NULL) {
        return NULL;
    }

    ast_asprintf(&tmp,
            "Uuid: %s\r\n"
            "Name: %s\r\n"
            "Detail: %s\r\n"
            "Status: %ld\r\n"
            "Plan: %s\r\n"
            "Dlma: %s\r\n"
            "TmCreate: %s\r\n"
            "TmDelete: %s\r\n"
            "TmUpdate: %s\r\n",
            ast_json_string_get(ast_json_object_get(j_camp, "uuid"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_camp, "name"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_camp, "detail"))? : "<unknown>",
            ast_json_integer_get(ast_json_object_get(j_camp, "status")),
            ast_json_string_get(ast_json_object_get(j_camp, "plan"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_camp, "dlma"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_camp, "tm_create"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_camp, "tm_delete"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_camp, "tm_update"))? : "<unknown>"
            );
    return tmp;
}

/**
 * Create AMI string for plan
 * @param j_plan
 * @return
 */
static char* manager_get_plan_str(struct ast_json* j_plan)
{
    char* tmp;

    if(j_plan == NULL) {
        return NULL;
    }

    ast_asprintf(&tmp,
            "Uuid: %s\r\n"
            "Name: %s\r\n"
            "Detail: %s\r\n"
            "DialMode: %ld\r\n"
            "DialTimeout: %ld\r\n"
            "CallerId: %s\r\n"
            "AnswerHandle: %ld\r\n"
            "DlEndHandle: %ld\r\n"
            "RetryDelay: %ld\r\n"
            "TrunkName: %s\r\n"
            "QueueName: %s\r\n"
            "AmdMode: %ld\r\n"
            "MaxRetryCnt1: %ld\r\n"
            "MaxRetryCnt2: %ld\r\n"
            "MaxRetryCnt3: %ld\r\n"
            "MaxRetryCnt4: %ld\r\n"
            "MaxRetryCnt5: %ld\r\n"
            "MaxRetryCnt6: %ld\r\n"
            "MaxRetryCnt7: %ld\r\n"
            "MaxRetryCnt8: %ld\r\n"
            "TmCreate: %s\r\n"
            "TmDelete: %s\r\n"
            "TmUpdate: %s\r\n",
            ast_json_string_get(ast_json_object_get(j_plan, "uuid"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_plan, "name"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_plan, "detail"))? : "<unknown>",
            ast_json_integer_get(ast_json_object_get(j_plan, "dial_mode")),
            ast_json_integer_get(ast_json_object_get(j_plan, "dial_timeout")),
            ast_json_string_get(ast_json_object_get(j_plan, "caller_id"))? : "<unknown>",
            ast_json_integer_get(ast_json_object_get(j_plan, "answer_handle")),
            ast_json_integer_get(ast_json_object_get(j_plan, "dl_end_handle")),
            ast_json_integer_get(ast_json_object_get(j_plan, "retry_delay")),
            ast_json_string_get(ast_json_object_get(j_plan, "trunk_name"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_plan, "queue_name"))? : "<unknown>",
            ast_json_integer_get(ast_json_object_get(j_plan, "amd_mode")),
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
    return tmp;
}

/**
 * Create AMI string for dlma
 * @param j_dlma
 * @return
 */
static char* manager_get_dlma_str(struct ast_json* j_dlma)
{
    char* tmp;

    if(j_dlma == NULL) {
        return NULL;
    }

    ast_asprintf(&tmp,
            "Uuid: %s\r\n"
            "Name: %s\r\n"
            "Detail: %s\r\n"
            "DlTable: %s\r\n"
            "TmCreate: %s\r\n"
            "TmDelete: %s\r\n"
            "TmUpdate: %s\r\n",
            ast_json_string_get(ast_json_object_get(j_dlma, "uuid"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dlma, "name"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dlma, "detail"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dlma, "dl_table"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dlma, "tm_create"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dlma, "tm_delete"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dlma, "tm_update"))? : "<unknown>"
            );
    return tmp;
}

static char* manager_get_queue_str(struct ast_json* j_queue)
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
static char* manager_get_dialing_str(const rb_dialing* dialing)
{
    char* tmp;

    if(dialing == NULL) {
        return NULL;
    }

    ast_asprintf(&tmp,
            // identity
            "Uuid: %s\r\n"
            "Status: %d\r\n"

            // uuid info
            "CampUuid: %s\r\n"
            "PlanUuid: %s\r\n"
            "DlmaUuid: %s\r\n"
            "DlListUuid: %s\r\n"    // todo: need to do more...

            // current info
            "CurrentQueue: %s\r\n"
            "CurrentAgent: %s\r\n"

            // dial info
            "DialIndex: %ld\r\n"
            "DialAddr: %s\r\n"
            "DialChannel: %s\r\n"
            "DialTryCnt: %ld\r\n"
            "DialTimeout: %ld\r\n"
            "DialType: %ld\r\n"
            "DialExten: %s\r\n"
            "DialContext: %s\r\n"
            "DialApplication: %s\r\n"
            "DialData: %s\r\n"

            // channel info
            "ChannelName: %s\r\n"

            // dial result
            "ResDial: %ld\r\n"
            "ResAmd: %s\r\n"
            "ResAmdDetail: %s\r\n"
            "ResHangup: %ld\r\n"
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
            ast_json_string_get(ast_json_object_get(dialing->j_dialing, "dl_list_uuid"))? : "<unknown>",

            // current info
            ast_json_string_get(ast_json_object_get(dialing->j_dialing, "current_queue"))? : "<null>",
            ast_json_string_get(ast_json_object_get(dialing->j_dialing, "current_agent"))? : "<null>",

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

            // channel info
            ast_json_string_get(ast_json_object_get(dialing->j_dialing, "channel_name"))? : "<unknown>",

            // result info
            ast_json_integer_get(ast_json_object_get(dialing->j_dialing, "res_dial")),
            ast_json_string_get(ast_json_object_get(dialing->j_dialing, "res_amd"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(dialing->j_dialing, "res_amd_detail"))? : "<unknown>",
            ast_json_integer_get(ast_json_object_get(dialing->j_dialing, "res_hangup")),
            ast_json_string_get(ast_json_object_get(dialing->j_dialing, "res_hangup_detail"))? : "<unknown>",

            // tm info
            dialing->tm_create? : "<unknown>",
            dialing->tm_update? : "<unknown>",
            dialing->tm_delete? : "<unknown>"
            );
    return tmp;
}

/**
 * Get string for dialing summary
 * @param dialing
 * @return
 */
static char* manager_get_dialing_summary_str(void)
{
    char* tmp;

    ast_asprintf(&tmp,
            "Count: %d\r\n",
            rb_dialing_get_count()
            );
    return tmp;
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

    tmp = manager_get_campaign_str(j_camp);
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
void send_manager_evt_out_campaign_delete(const char* uuid)
{
    char* tmp;

    ast_log(LOG_VERBOSE, "AMI event. OutCampaignDelete.\n");
    if(uuid == NULL) {
        // nothing to send.
        ast_log(LOG_WARNING, "AMI event. OutCampaignDelete. Failed.\n");
        return;
    }

    ast_asprintf(&tmp,
            "Uuid: %s\r\n",
            uuid
            );
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

    tmp = manager_get_campaign_str(j_camp);
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

    tmp = manager_get_plan_str(j_plan);
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
void send_manager_evt_out_plan_delete(const char* uuid)
{
    char* tmp;

    ast_log(LOG_VERBOSE, "AMI event. OutPlanDelete.\n");

    if(uuid == NULL) {
        // nothing to send.
        ast_log(LOG_WARNING, "AMI event. OutPlanDelete. Failed.\n");
        return;
    }

    ast_asprintf(&tmp,
            "Uuid: %s\r\n",
            uuid
            );
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

    tmp = manager_get_plan_str(j_plan);
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

    tmp = manager_get_queue_str(j_queue);
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

    tmp = manager_get_queue_str(j_queue);
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

    tmp = manager_get_dlma_str(j_tmp);
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

    tmp = manager_get_dlma_str(j_tmp);
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

    tmp = manager_get_dialing_str(dialing);
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

    tmp = manager_get_dialing_str(dialing);
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
 * Event: OutDialingEntry
 * @param s
 * @param m
 * @param j_tmp
 * @param action_id
 */
void manager_evt_out_dialing_entry(struct mansession *s, const struct message *m, const rb_dialing* dialing, char* action_id)
{
    char* tmp;

    tmp = manager_get_dialing_str(dialing);

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

    tmp = manager_get_dialing_summary_str();

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

    ast_asprintf(&tmp,
            "Uuid: %s\r\n",
            dialing->uuid
            );
    manager_event(EVENT_FLAG_MESSAGE, "OutDialingDelete", "%s\r\n", tmp);
    ast_free(tmp);
    ast_log(LOG_VERBOSE, "AMI event. OutDialingDelete. Succeed.\n");

    return;
}

/**
 * OutDlList
 * @param s
 * @param m
 * @param j_dl
 */
void manager_out_dl_entry(struct mansession *s, const struct message *m, struct ast_json* j_dl)
{
    char* tmp;

    ast_asprintf(&tmp,
            "Uuid: %s\r\n"
            "DlmaUUID: %s\r\n"
            "Name: %s\r\n"
            "Detail: %s\r\n"
            "UUI: %s\r\n"
            "Status: %ld\r\n"
            "DialingUUID: %s\r\n"
            "DialingCampUUID: %s\r\n"
            "DialingPlanUUID: %s\r\n"
            "Email: %s\r\n"
            "Number1: %s\r\n"
            "Number2: %s\r\n"
            "Number3: %s\r\n"
            "Number4: %s\r\n"
            "Number5: %s\r\n"
            "Number6: %s\r\n"
            "Number7: %s\r\n"
            "Number8: %s\r\n"
            "Trycnt1: %ld\r\n"
            "Trycnt2: %ld\r\n"
            "Trycnt3: %ld\r\n"
            "Trycnt4: %ld\r\n"
            "Trycnt5: %ld\r\n"
            "Trycnt6: %ld\r\n"
            "Trycnt7: %ld\r\n"
            "Trycnt8: %ld\r\n"
            "ResDial: %ld\r\n"
            "ResHangup: %ld\r\n",

            ast_json_string_get(ast_json_object_get(j_dl, "uuid"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dl, "dlma_uuid"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dl, "name"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dl, "detail"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dl, "uui"))? : "<unknown>",
            ast_json_integer_get(ast_json_object_get(j_dl, "status")),
            ast_json_string_get(ast_json_object_get(j_dl, "dialing_uuid"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dl, "dialing_camp_uuid"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dl, "dialing_plan_uuid"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dl, "email"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dl, "number_1"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dl, "number_2"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dl, "number_3"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dl, "number_4"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dl, "number_5"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dl, "number_6"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dl, "number_7"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_dl, "number_8"))? : "<unknown>",
            ast_json_integer_get(ast_json_object_get(j_dl, "trycnt_1")),
            ast_json_integer_get(ast_json_object_get(j_dl, "trycnt_2")),
            ast_json_integer_get(ast_json_object_get(j_dl, "trycnt_3")),
            ast_json_integer_get(ast_json_object_get(j_dl, "trycnt_4")),
            ast_json_integer_get(ast_json_object_get(j_dl, "trycnt_5")),
            ast_json_integer_get(ast_json_object_get(j_dl, "trycnt_6")),
            ast_json_integer_get(ast_json_object_get(j_dl, "trycnt_7")),
            ast_json_integer_get(ast_json_object_get(j_dl, "trycnt_8")),
            ast_json_integer_get(ast_json_object_get(j_dl, "res_dial")),
            ast_json_integer_get(ast_json_object_get(j_dl, "res_hangup"))
            );

    if(s != NULL) {
        astman_append(s, "Event: OutDlListEntry\r\n%s\r\n", tmp);
    }
    else {
        manager_event(EVENT_FLAG_MESSAGE, "OutDlListEntry", "%s\r\n", tmp);
    }
    ast_free(tmp);
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

    tmp = manager_get_plan_str(j_tmp);

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

    tmp = manager_get_dlma_str(j_tmp);

    if(s != NULL) {
        astman_append(s, "Event: OutDlmaEntry\r\n%s%s\r\n", action_id, tmp);
    }
    else {
        manager_event(EVENT_FLAG_MESSAGE, "OutDlmaEntry", "%s\r\n", tmp);
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

static int manager_out_dl_show(struct mansession *s, const struct message *m)
{
    struct ast_json* j_tmp;
    const char* tmp_const;

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") == 0) {
        astman_send_error(s, m, "Error encountered while creating campaign");
        return 0;
    }

    j_tmp = get_dl_list(tmp_const);
    if(j_tmp == NULL) {
        astman_send_error(s, m, "Error encountered while creating campaign");
        return 0;
    }

    astman_send_listack(s, m, "Dl List will follow", "start");

    manager_out_dl_entry(s, m, j_tmp);

    astman_send_list_complete_start(s, m, "OutDlListComplete", 1);
    astman_send_list_complete_end(s);
    return 0;
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

    tmp = manager_get_campaign_str(j_tmp);

    if(s != NULL) {
        astman_append(s, "Event: OutCampaignEntry\r\n%s%s\r\n", action_id, tmp);
    }
    else {
        manager_event(EVENT_FLAG_MESSAGE, "OutCampaignEntry", "%s\r\n", tmp);
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
    const char* tmp_const;
    int ret;

    ast_log(LOG_VERBOSE, "AMI request. OutCampaignCreate.\n");

    j_tmp = ast_json_object_create();

    tmp_const = astman_get_header(m, "Name");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "name", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "Detail");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "detail", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "Plan");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "plan", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "Dlma");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "dlma", ast_json_string_create(tmp_const));}

    ret = create_campaign(j_tmp);
    ast_json_unref(j_tmp);
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

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") == 0) {
        astman_send_error(s, m, "Error encountered while deleting campaign");
        ast_log(LOG_WARNING, "OutCampaignDelete failed.\n");
        return 0;
    }

    ret = delete_cmapaign(tmp_const);
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
    const char* tmp_const;
    int ret;

    ast_log(LOG_VERBOSE, "AMI request. OutCampaignUpdate.\n");

    j_tmp = ast_json_object_create();

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "uuid", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "Name");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "name", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "Detail");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "detail", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "Plan");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "plan", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "Dlma");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "dlma", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "Status");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "status", ast_json_integer_create(atoi(tmp_const)));}

    ret = update_campaign(j_tmp);
    ast_json_unref(j_tmp);
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

    tmp_const = astman_get_header(m, "ActionID");
    if(strlen(tmp_const) != 0) {
        ast_asprintf(&action_id, "ActionID: %s\r\n", tmp_const);
    }
    else {
        ast_asprintf(&action_id, "%s", "");
    }

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") != 0) {
        j_tmp = get_campaign(tmp_const);
        if(j_tmp == NULL) {
            astman_send_error(s, m, "Error encountered while show campaign");
            ast_log(LOG_WARNING, "OutCampaignShow failed.\n");
            ast_free(action_id);
            return 0;
        }

        astman_send_listack(s, m, "Campaign List will follow", "start");

        manager_out_campaign_entry(s, m, j_tmp, action_id);
        ast_json_unref(j_tmp);

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
        ast_json_unref(j_arr);
    }

    ast_log(LOG_NOTICE, "OutCampaignShow succeed.\n");
    ast_free(action_id);
    return 0;
}

static struct ast_json* parse_ami_message_plan(const struct message* m)
{
    const char* tmp_const;
    struct ast_json* j_tmp;

    if(m == NULL) {
        return NULL;
    }

    j_tmp = ast_json_object_create();

    tmp_const = astman_get_header(m, "Name");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "name", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "Detail");
    if(strcmp(tmp_const, "") != 0)
    {ast_json_object_set(j_tmp, "detail", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "DialMode");
    if(strcmp(tmp_const, "") != 0)
    {ast_json_object_set(j_tmp, "dial_mode", ast_json_integer_create(atoi(tmp_const)));}

    tmp_const = astman_get_header(m, "CallerId");
    if(strcmp(tmp_const, "") != 0)
    {ast_json_object_set(j_tmp, "caller_id", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "AnswerHandle");
    if(strcmp(tmp_const, "") != 0)
    {ast_json_object_set(j_tmp, "answer_handle", ast_json_integer_create(atoi(tmp_const)));}

    tmp_const = astman_get_header(m, "DlEndHandle");
    if(strcmp(tmp_const, "") != 0)
    {ast_json_object_set(j_tmp, "dl_end_handle", ast_json_integer_create(atoi(tmp_const)));}

    tmp_const = astman_get_header(m, "RetryDelay");
    if(strcmp(tmp_const, "") != 0)
    {ast_json_object_set(j_tmp, "retry_delay", ast_json_integer_create(atoi(tmp_const)));}

    tmp_const = astman_get_header(m, "TrunkName");
    if(strcmp(tmp_const, "") != 0)
    {ast_json_object_set(j_tmp, "trunk_name", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "QueueName");
    if(strcmp(tmp_const, "") != 0)
    {ast_json_object_set(j_tmp, "queue_name", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "AmdMode");
    if(strcmp(tmp_const, "") != 0)
    {ast_json_object_set(j_tmp, "amd_mode", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "MaxRetry1");
    if(strcmp(tmp_const, "") != 0)
    {ast_json_object_set(j_tmp, "max_retry_cnt_1", ast_json_integer_create(atoi(tmp_const)));}

    tmp_const = astman_get_header(m, "MaxRetry2");
    if(strcmp(tmp_const, "") != 0)
    {ast_json_object_set(j_tmp, "max_retry_cnt_2", ast_json_integer_create(atoi(tmp_const)));}

    tmp_const = astman_get_header(m, "MaxRetry3");
    if(strcmp(tmp_const, "") != 0)
    {ast_json_object_set(j_tmp, "max_retry_cnt_3", ast_json_integer_create(atoi(tmp_const)));}

    tmp_const = astman_get_header(m, "MaxRetry4");
    if(strcmp(tmp_const, "") != 0)
    {ast_json_object_set(j_tmp, "max_retry_cnt_4", ast_json_integer_create(atoi(tmp_const)));}

    tmp_const = astman_get_header(m, "MaxRetry5");
    if(strcmp(tmp_const, "") != 0)
    {ast_json_object_set(j_tmp, "max_retry_cnt_5", ast_json_integer_create(atoi(tmp_const)));}

    tmp_const = astman_get_header(m, "MaxRetry6");
    if(strcmp(tmp_const, "") != 0)
    {ast_json_object_set(j_tmp, "max_retry_cnt_6", ast_json_integer_create(atoi(tmp_const)));}

    tmp_const = astman_get_header(m, "MaxRetry7");
    if(strcmp(tmp_const, "") != 0)
    {ast_json_object_set(j_tmp, "max_retry_cnt_7", ast_json_integer_create(atoi(tmp_const)));}

    tmp_const = astman_get_header(m, "MaxRetry8");
    if(strcmp(tmp_const, "") != 0)
    {ast_json_object_set(j_tmp, "max_retry_cnt_8", ast_json_integer_create(atoi(tmp_const)));}

    return j_tmp;
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

    j_tmp = parse_ami_message_plan(m);
    ret = create_plan(j_tmp);
    ast_json_unref(j_tmp);
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

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") == 0) {
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

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") == 0) {
        astman_send_error(s, m, "Error encountered while updating plan");
        ast_log(LOG_WARNING, "OutPlanUpdate failed.\n");
        return 0;
    }

    j_tmp = parse_ami_message_plan(m);

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "uuid", ast_json_string_create(tmp_const));}

    ret = update_plan(j_tmp);
    ast_json_unref(j_tmp);
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

    tmp_const = astman_get_header(m, "ActionID");
    if(strlen(tmp_const) != 0) {
        ast_asprintf(&action_id, "ActionID: %s\r\n", tmp_const);
    }
    else {
        ast_asprintf(&action_id, "%s", "");
    }

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") != 0) {
        j_tmp = get_plan(tmp_const);
        if(j_tmp == NULL) {
            astman_send_error(s, m, "Error encountered while show plan");
            ast_log(LOG_WARNING, "OutPlanShow failed.\n");
            ast_free(action_id);
            return 0;
        }

        astman_send_listack(s, m, "Plan List will follow", "start");

        manager_out_plan_entry(s, m, j_tmp, action_id);
        ast_json_unref(j_tmp);

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
        ast_json_unref(j_arr);
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
    const char* tmp_const;
    int ret;

    ast_log(LOG_VERBOSE, "AMI request. OutDlmaCreate.\n");

    j_tmp = ast_json_object_create();

    tmp_const = astman_get_header(m, "Name");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "name", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "Detail");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "detail", ast_json_string_create(tmp_const));}

    ret = create_dlma(j_tmp);
    ast_json_unref(j_tmp);
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

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") == 0) {
        astman_send_error(s, m, "Error encountered while updating dlma");
        return 0;
    }

    j_tmp = ast_json_object_create();

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "uuid", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "Name");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "name", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "Detail");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "detail", ast_json_string_create(tmp_const));}

    ret = update_dlma(j_tmp);
    ast_json_unref(j_tmp);
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

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") == 0) {
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
 * OutPlanShow AMI message handle.
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

    tmp_const = astman_get_header(m, "ActionID");
    if(strlen(tmp_const) != 0) {
        ast_asprintf(&action_id, "ActionID: %s\r\n", tmp_const);
    }
    else {
        ast_asprintf(&action_id, "%s", "");
    }

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") != 0) {
        j_tmp = get_dlma(tmp_const);
        if(j_tmp == NULL) {
            astman_send_error(s, m, "Error encountered while show dlma");
            ast_free(action_id);
            return 0;
        }

        astman_send_listack(s, m, "Dlma List will follow", "start");

        manager_out_dlma_entry(s, m, j_tmp, action_id);
        ast_json_unref(j_tmp);

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
        ast_json_unref(j_arr);
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

    tmp_const = astman_get_header(m, "Name");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "name", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "Detail");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "detail", ast_json_string_create(tmp_const));}

    ret = create_queue(j_tmp);
    ast_json_unref(j_tmp);
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

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") == 0) {
        astman_send_error(s, m, "Error encountered while updating queue");
        return 0;
    }

    j_tmp = ast_json_object_create();

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "uuid", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "Name");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "name", ast_json_string_create(tmp_const));}

    tmp_const = astman_get_header(m, "Detail");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "detail", ast_json_string_create(tmp_const));}

    ret = update_queue(j_tmp);
    ast_json_unref(j_tmp);
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

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") == 0) {
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

    tmp_const = astman_get_header(m, "ActionID");
    if(strlen(tmp_const) != 0) {
        ast_asprintf(&action_id, "ActionID: %s\r\n", tmp_const);
    }
    else {
        ast_asprintf(&action_id, "%s", "");
    }

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") != 0) {
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
        ast_json_unref(j_arr);
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

    tmp_const = astman_get_header(m, "ActionID");
    if(strlen(tmp_const) != 0) {
        ast_asprintf(&action_id, "ActionID: %s\r\n", tmp_const);
    }
    else {
        ast_asprintf(&action_id, "%s", "");
    }

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") != 0) {
        dialing = rb_dialing_find_chan_uuid(tmp_const);
        if(dialing == NULL) {
            astman_send_error(s, m, "Error encountered while show dialing");
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

   tmp_const = astman_get_header(m, "ActionID");
   if(strlen(tmp_const) != 0) {
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

struct ast_cli_entry cli_out[] = {
    AST_CLI_DEFINE(out_show_campaigns,      "List defined outbound campaigns"),
    AST_CLI_DEFINE(out_show_plans,          "List defined outbound plans"),
    AST_CLI_DEFINE(out_show_dlmas,          "List defined outbound dlmas"),
    AST_CLI_DEFINE(out_show_dlma_list,      "Show list of dlma dial list"),
    AST_CLI_DEFINE(out_show_dialings,       "List currently on serviced dialings"),
    AST_CLI_DEFINE(out_show_campaign,       "Shows detail campaign info"),
    AST_CLI_DEFINE(out_show_dl,             "Show detail dl info"),
    AST_CLI_DEFINE(out_set_campaign,        "Set campaign parameters"),
    AST_CLI_DEFINE(out_create_campaign,     "Create new campaign"),
    AST_CLI_DEFINE(out_delete_campaign,     "Delete campaign")
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
    err |= ast_manager_register2("OutDlListShow", EVENT_FLAG_COMMAND, manager_out_dl_show, NULL, NULL, NULL);
    err |= ast_manager_register2("OutPlanCreate", EVENT_FLAG_COMMAND, manager_out_plan_create, NULL, NULL, NULL);
    err |= ast_manager_register2("OutPlanDelete", EVENT_FLAG_COMMAND, manager_out_plan_delete, NULL, NULL, NULL);
    err |= ast_manager_register2("OutPlanUpdate", EVENT_FLAG_COMMAND, manager_out_plan_update, NULL, NULL, NULL);
    err |= ast_manager_register2("OutPlanShow", EVENT_FLAG_COMMAND, manager_out_plan_show, NULL, NULL, NULL);
    err |= ast_manager_register2("OutDlmaCreate", EVENT_FLAG_COMMAND, manager_out_dlma_create, NULL, NULL, NULL);
    err |= ast_manager_register2("OutDlmaUpdate", EVENT_FLAG_COMMAND, manager_out_dlma_update, NULL, NULL, NULL);
    err |= ast_manager_register2("OutDlmaDelete", EVENT_FLAG_COMMAND, manager_out_dlma_delete, NULL, NULL, NULL);
    err |= ast_manager_register2("OutDlmaShow", EVENT_FLAG_COMMAND, manager_out_dlma_show, NULL, NULL, NULL);
    err |= ast_manager_register2("OutQueueCreate", EVENT_FLAG_COMMAND, manager_out_queue_create, NULL, NULL, NULL);
    err |= ast_manager_register2("OutQueueUpdate", EVENT_FLAG_COMMAND, manager_out_queue_update, NULL, NULL, NULL);
    err |= ast_manager_register2("OutQueueDelete", EVENT_FLAG_COMMAND, manager_out_queue_delete, NULL, NULL, NULL);
    err |= ast_manager_register2("OutQueueShow", EVENT_FLAG_COMMAND, manager_out_queue_show, NULL, NULL, NULL);
    err |= ast_manager_register2("OutDialingShow", EVENT_FLAG_COMMAND, manager_out_dialing_show, NULL, NULL, NULL);
    err |= ast_manager_register2("OutDialingSummary", EVENT_FLAG_COMMAND, manager_out_dialing_summary, NULL, NULL, NULL);


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
    ast_manager_unregister("OutDlListShow");
    ast_manager_unregister("OutPlanCreate");
    ast_manager_unregister("OutPlanDelete");
    ast_manager_unregister("OutPlanUpdate");
    ast_manager_unregister("OutPlanShow");
    ast_manager_unregister("OutDlmaCreate");
    ast_manager_unregister("OutDlmaUpdate");
    ast_manager_unregister("OutDlmaDelete");
    ast_manager_unregister("OutDlmaShow");
    ast_manager_unregister("OutQueueCreate");
    ast_manager_unregister("OutQueueUpdate");
    ast_manager_unregister("OutQueueDelete");
    ast_manager_unregister("OutQueueShow");
    ast_manager_unregister("OutDialingShow");
    ast_manager_unregister("OutDialingSummary");


    return;
}
