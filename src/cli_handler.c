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

    j_res = get_campaigns_info_all();

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

    j_res = get_campaign_info(argv[3]);
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

    ret = update_campaign_info_status(uuid, status);
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

    j_res = get_plan_info_all();

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

    j_res = get_dl_master_info_all();

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

    j_dlma = get_dl_master_info(uuid);
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

static char* manager_get_campaign_str(struct ast_json* j_camp)
{
    char* tmp;

    ast_asprintf(&tmp,
            "Uuid: %s\r\n"
            "Name: %s\r\n"
            "Detail: %s\r\n"
            "Status: %ld\r\n"
            "Plan: %s\r\n"
            "Dlma: %s\r\n"
            "Queue: %s\r\n",
            ast_json_string_get(ast_json_object_get(j_camp, "uuid"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_camp, "name"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_camp, "detail"))? : "<unknown>",
            ast_json_integer_get(ast_json_object_get(j_camp, "status")),
            ast_json_string_get(ast_json_object_get(j_camp, "plan"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_camp, "dlma"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_camp, "queue"))? : "<unknown>"
            );
    return tmp;
}

static char* manager_get_plan_str(struct ast_json* j_plan)
{
    char* tmp;

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
            "MaxRetryCnt1: %ld\r\n"
            "MaxRetryCnt2: %ld\r\n"
            "MaxRetryCnt3: %ld\r\n"
            "MaxRetryCnt4: %ld\r\n"
            "MaxRetryCnt5: %ld\r\n"
            "MaxRetryCnt6: %ld\r\n"
            "MaxRetryCnt7: %ld\r\n"
            "MaxRetryCnt8: %ld\r\n",
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
            ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_1")),
            ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_2")),
            ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_3")),
            ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_4")),
            ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_5")),
            ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_6")),
            ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_7")),
            ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_8"))
            );
    return tmp;
}

/**
 * Send event notification of campaign create.
 * @param j_camp
 */
void send_manager_evt_campaign_create(struct ast_json* j_camp)
{
    char* tmp;

    if(j_camp == NULL) {
        return;
    }

    tmp = manager_get_campaign_str(j_camp);
    if(tmp == NULL) {
        return;
    }

    manager_event(EVENT_FLAG_MESSAGE, "OutCampaignCreate", "%s\r\n", tmp);
    ast_free(tmp);
}


/**
 * Send event notification of campaign delete.
 * @param j_camp
 */
void send_manager_evt_campaign_delete(const char* uuid)
{
    char* tmp;

    if(uuid == NULL) {
        // nothing to send.
        return;
    }

    ast_asprintf(&tmp,
            "Uuid: %s\r\n",
            uuid
            );
    manager_event(EVENT_FLAG_MESSAGE, "OutCampaignDelete", "%s\r\n", tmp);
    ast_free(tmp);
}

/**
 * Send CampaignUpdate event notify to AMI
 * @param j_camp
 */
void send_manager_evt_campaign_update(struct ast_json* j_camp)
{
    char* tmp;

    if(j_camp == NULL) {
        return;
    }

    tmp = manager_get_campaign_str(j_camp);
    if(tmp == NULL) {
        return;
    }

    manager_event(EVENT_FLAG_MESSAGE, "OutCampaignUpdate", "%s\r\n", tmp);
    ast_free(tmp);

    return;
}

/**
 * Send event notification of plan create.
 * @param j_camp
 */
void send_manager_evt_plan_create(struct ast_json* j_plan)
{
    char* tmp;

    if(j_plan == NULL) {
        return;
    }

    tmp = manager_get_plan_str(j_plan);
    if(tmp == NULL) {
        return;
    }

    manager_event(EVENT_FLAG_MESSAGE, "OutPlanCreate", "%s\r\n", tmp);
    ast_free(tmp);
}


/**
 * Send event notification of plan delete.
 * @param j_camp
 */
void send_manager_evt_plan_delete(const char* uuid)
{
    char* tmp;

    if(uuid == NULL) {
        // nothing to send.
        return;
    }

    ast_asprintf(&tmp,
            "Uuid: %s\r\n",
            uuid
            );
    manager_event(EVENT_FLAG_MESSAGE, "OutPlanDelete", "%s\r\n", tmp);
    ast_free(tmp);
}

/**
 * Send OutPlanUpdate event notify to AMI
 * @param j_camp
 */
void send_manager_evt_plan_update(struct ast_json* j_plan)
{
    char* tmp;

    if(j_plan == NULL) {
        return;
    }

    tmp = manager_get_plan_str(j_plan);
    if(tmp == NULL) {
        return;
    }

    manager_event(EVENT_FLAG_MESSAGE, "OutPlanUpdate", "%s\r\n", tmp);
    ast_free(tmp);

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
void manager_out_plan_entry(struct mansession *s, const struct message *m, struct ast_json* j_tmp)
{
    char* tmp;

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
            "MaxRetryCnt1: %ld\r\n"
            "MaxRetryCnt2: %ld\r\n"
            "MaxRetryCnt3: %ld\r\n"
            "MaxRetryCnt4: %ld\r\n"
            "MaxRetryCnt5: %ld\r\n"
            "MaxRetryCnt6: %ld\r\n"
            "MaxRetryCnt7: %ld\r\n"
            "MaxRetryCnt8: %ld\r\n",

            ast_json_string_get(ast_json_object_get(j_tmp, "uuid"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_tmp, "name"))? : "<unknown>",
            ast_json_string_get(ast_json_object_get(j_tmp, "detail"))? : "<unknown>",
            ast_json_integer_get(ast_json_object_get(j_tmp, "dial_mode")),
            ast_json_integer_get(ast_json_object_get(j_tmp, "dial_timeout")),
            ast_json_string_get(ast_json_object_get(j_tmp, "caller_id"))? : "<unknown>",
            ast_json_integer_get(ast_json_object_get(j_tmp, "answer_handle")),
            ast_json_integer_get(ast_json_object_get(j_tmp, "dl_end_handle")),
            ast_json_integer_get(ast_json_object_get(j_tmp, "retry_delay")),
            ast_json_string_get(ast_json_object_get(j_tmp, "trunk_name"))? : "<unknown>",
            ast_json_integer_get(ast_json_object_get(j_tmp, "max_retry_cnt_1")),
            ast_json_integer_get(ast_json_object_get(j_tmp, "max_retry_cnt_2")),
            ast_json_integer_get(ast_json_object_get(j_tmp, "max_retry_cnt_3")),
            ast_json_integer_get(ast_json_object_get(j_tmp, "max_retry_cnt_4")),
            ast_json_integer_get(ast_json_object_get(j_tmp, "max_retry_cnt_5")),
            ast_json_integer_get(ast_json_object_get(j_tmp, "max_retry_cnt_6")),
            ast_json_integer_get(ast_json_object_get(j_tmp, "max_retry_cnt_7")),
            ast_json_integer_get(ast_json_object_get(j_tmp, "max_retry_cnt_8"))
            );

    if(s != NULL) {
        astman_append(s, "Event: OutPlanEntry\r\n%s\r\n", tmp);
    }
    else {
        manager_event(EVENT_FLAG_MESSAGE, "OutPlanEntry", "%s\r\n", tmp);
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


static int manager_out_campaign_create(struct mansession *s, const struct message *m)
{
    struct ast_json* j_camp;
    const char* tmp_const;
    int ret;

    j_camp = ast_json_object_create();
    if((tmp_const = astman_get_header(m, "Name")) != NULL) {ast_json_object_set(j_camp, "name", ast_json_string_create(tmp_const));}
    if((tmp_const = astman_get_header(m, "Detail")) != NULL) {ast_json_object_set(j_camp, "detail", ast_json_string_create(tmp_const));}
    if((tmp_const = astman_get_header(m, "Plan")) != NULL) {ast_json_object_set(j_camp, "plan", ast_json_string_create(tmp_const));}
    if((tmp_const = astman_get_header(m, "Dlma")) != NULL) {ast_json_object_set(j_camp, "dlma", ast_json_string_create(tmp_const));}
    if((tmp_const = astman_get_header(m, "Queue")) != NULL) {ast_json_object_set(j_camp, "queue", ast_json_string_create(tmp_const));}

    ret = create_campaign(j_camp);
    ast_json_unref(j_camp);
    if(ret == false) {
        astman_send_error(s, m, "Error encountered while creating campaign");
        return 0;
    }
    astman_send_ack(s, m, "Campaign created successfully");

    return 0;
}

/**
 * CampaignDelete AMI message handle.
 * @param s
 * @param m
 * @return
 */
static int manager_out_campaign_delete(struct mansession *s, const struct message *m)
{
    const char* tmp_const;
    int ret;

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") == 0) {
        astman_send_error(s, m, "Error encountered while deleting campaign");
        return 0;
    }

    ret = delete_cmapaign(tmp_const);
    if(ret == false) {
        astman_send_error(s, m, "Error encountered while creating campaign");
        return 0;
    }
    astman_send_ack(s, m, "Campaign deleted successfully");

    return 0;
}

/**
 * OutPlanCreate AMI message handle.
 * @param s
 * @param m
 * @return
 */
static int manager_out_plan_create(struct mansession *s, const struct message *m)
{
    const char* tmp_const;
    int ret;
    struct ast_json* j_tmp;

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

    ret = create_plan(j_tmp);
    if(ret == false) {
        astman_send_error(s, m, "Error encountered while creating plan");
        return 0;
    }
    astman_send_ack(s, m, "Plan created successfully");

    return 0;
}

/**
 * OutPlanDelete AMI message handle.
 * @param s
 * @param m
 * @return
 */
static int manager_out_plan_delete(struct mansession *s, const struct message *m)
{
    const char* tmp_const;
    int ret;

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") == 0) {
        astman_send_error(s, m, "Error encountered while deleting plan");
        return 0;
    }

    ret = delete_plan(tmp_const);
    if(ret == false) {
        astman_send_error(s, m, "Error encountered while deleting plan");
        return 0;
    }
    astman_send_ack(s, m, "plan deleted successfully");

    return 0;
}

/**
 * OutPlanUpdate AMI message handle.
 * @param s
 * @param m
 * @return
 */
static int manager_out_plan_update(struct mansession *s, const struct message *m)
{
    const char* tmp_const;
    int ret;
    struct ast_json* j_tmp;

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") == 0) {
        astman_send_error(s, m, "Error encountered while creating plan");
        return 0;
    }

    j_tmp = ast_json_object_create();

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") != 0) {ast_json_object_set(j_tmp, "uuid", ast_json_string_create(tmp_const));}

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

    ret = create_plan(j_tmp);
    if(ret == false) {
        astman_send_error(s, m, "Error encountered while creating plan");
        return 0;
    }
    astman_send_ack(s, m, "Plan created successfully");

    return 0;
}

/**
 * OutPlanShow AMI message handle.
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

    tmp_const = astman_get_header(m, "Uuid");
    if(strcmp(tmp_const, "") != 0) {
        j_tmp = get_plan_info(tmp_const);
        if(j_tmp == false) {
            astman_send_error(s, m, "Error encountered while show plan");
            return 0;
        }

        astman_send_listack(s, m, "Plan List will follow", "start");

        manager_out_plan_entry(s, m, j_tmp);

        astman_send_list_complete_start(s, m, "OutPlanListComplete", 1);
        astman_send_list_complete_end(s);
    }
    else {
        j_arr = get_plan_info_all();
        size = ast_json_array_size(j_arr);

        astman_send_listack(s, m, "Plan List will follow", "start");
        for(i = 0; i < size; i++) {
            j_tmp = ast_json_array_get(j_arr, i);
            if(j_tmp == NULL) {
                continue;
            }
            manager_out_plan_entry(s, m, j_tmp);
        }
        astman_send_list_complete_start(s, m, "OutPlanListComplete", size);
        ast_json_unref(j_arr);
    }

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
    err |= ast_manager_register2("OutDlListShow", EVENT_FLAG_COMMAND, manager_out_dl_show, NULL, NULL, NULL);
    err |= ast_manager_register2("OutPlanCreate", EVENT_FLAG_COMMAND, manager_out_plan_create, NULL, NULL, NULL);
    err |= ast_manager_register2("OutPlanDelete", EVENT_FLAG_COMMAND, manager_out_plan_delete, NULL, NULL, NULL);
    err |= ast_manager_register2("OutPlanUpdate", EVENT_FLAG_COMMAND, manager_out_plan_update, NULL, NULL, NULL);
    err |= ast_manager_register2("OutPlanShow", EVENT_FLAG_COMMAND, manager_out_plan_show, NULL, NULL, NULL);


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
    ast_manager_unregister("OutDlListShow");
    ast_manager_unregister("OutPlanCreate");
    ast_manager_unregister("OutPlanDelete");
    ast_manager_unregister("OutPlanUpdate");
    ast_manager_unregister("OutPlanShow");

    return;
}
