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


static char *out_show_campaigns(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a);
static char* _out_show_campaigns(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[]);

struct ast_cli_entry cli_out[] = {
    AST_CLI_DEFINE(out_show_campaigns, "List defined outbound campaigns")
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


/*! \brief CLI for show channels or subscriptions.
 * This is a new-style CLI handler so a single function contains
 * the prototype for the function, the 'generator' to produce multiple
 * entries in case it is required, and the actual handler for the command.
 */
static char *out_show_campaigns(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
//    struct __show_chan_arg arg = { .fd = a->fd, .numchans = 0 };

    if (cmd == CLI_INIT) {
        e->command = "out show campaigns";
        e->usage =
            "Usage: out show campaigns\n"
            "       Lists all currently active campaigns.\n";
        return NULL;
    } else if (cmd == CLI_GENERATE) {
        return NULL;
    }
    return _out_show_campaigns(a->fd, NULL, NULL, NULL, a->argc, (const char**)a->argv);
}

static char* _out_show_campaigns(int fd, int *total, struct mansession *s, const struct message *m, int argc, const char *argv[])
{
    struct ast_json* j_res;
    struct ast_json* j_tmp;
    int size;
    int i;

    j_res = get_campaign_info_all();

    size = ast_json_array_size(j_res);
    for(i = 0; i < size; i++) {
        j_tmp = ast_json_array_get(j_res, i);
        if(j_tmp == NULL) {
            continue;
        }
        ast_cli(fd, "%s %s %ld %s %s\n",
                ast_json_string_get(ast_json_object_get(j_tmp, "uuid")),
                ast_json_string_get(ast_json_object_get(j_tmp, "name")),
                ast_json_integer_get(ast_json_object_get(j_tmp, "status")),
                ast_json_string_get(ast_json_object_get(j_tmp, "plan")),
                ast_json_string_get(ast_json_object_get(j_tmp, "dlma"))
                );
    }
    ast_json_unref(j_res);

    return CLI_SUCCESS;
}


