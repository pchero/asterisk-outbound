/*
 * cli_handler.c
 *
 *  Created on: Nov 22, 2015
 *      Author: pchero
 */

#include "asterisk.h"
#include "asterisk/manager.h"
#include "asterisk/json.h"

#include <stdbool.h>

#include "cli_handler.h"
#include "event_handler.h"
#include "dialing_handler.h"


#define CAMP_FORMAT2 "%-36.36s %-40.40s %-6.6s %-36.36s %-36.36s %-40.40s\n"
#define CAMP_FORMAT3 "%-36.36s %-40.40s %6ld %-36.36s %-36.36s %-40.40s\n"

static char* _out_show_campaigns(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
    struct ast_json* j_res;
    struct ast_json* j_tmp;
    int size;
    int i;

    j_res = get_campaign_info_all();

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

/*! \brief CLI for show campaign.
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

/*! \brief CLI for show plans.
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

struct ast_cli_entry cli_out[] = {
    AST_CLI_DEFINE(out_show_campaigns, "List defined outbound campaigns"),
    AST_CLI_DEFINE(out_show_plans, "List defined outbound plans"),
    AST_CLI_DEFINE(out_show_dlmas, "List defined outbound dlmas"),
    AST_CLI_DEFINE(out_show_dialings, "List currently on serviced dialings"),
    AST_CLI_DEFINE(out_show_campaign, "Shows detail campaign info")
};

int init_cli_handler(void)
{
    ast_cli_register_multiple(cli_out, ARRAY_LEN(cli_out));
    return true;
}

void term_cli_handler(void)
{
    ast_cli_unregister_multiple(cli_out, ARRAY_LEN(cli_out));
}
