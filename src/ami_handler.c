/*
 * ami_handler.c
 *
 *  Created on: Nov 12, 2015
 *      Author: pchero
 */

#include "asterisk.h"

#include "asterisk/utils.h"
#include "asterisk/manager.h"
#include "asterisk/json.h"


static char* g_cmd_buf = NULL;  //!< action cmd buffer

static int ami_cmd_helper(int category, const char *event, char *content);
static struct ast_json* parse_ami_msg(char* msg);
static void trim(char * s);

/**
 *
 * @param s
 */
static void trim(char * s)
{
    char * p = s;
    int l = strlen(p);

    while(isspace(p[l - 1])) p[--l] = 0;
    while(* p && isspace(* p)) ++p, --l;

    memmove(s, p, l + 1);
}

/**
 *
 * @param msg
 * @return
 */
static struct ast_json* parse_ami_msg(char* msg)
{
    struct ast_json* j_out;
    struct ast_json* j_tmp;
    int i, j;
    int ret;
    char  tmp[2048];
    char* key;
    char* value;
    char* dump;

    ast_log(LOG_DEBUG, "Parse ami message. msg[%s]\n", msg);
    if(msg == NULL) {
        return NULL;
    }

    memset(tmp, 0x00, sizeof(tmp));
    j_out = ast_json_array_create();
    j_tmp = ast_json_object_create();
    for(i = 0, j = 0; i < strlen(msg); i++)
    {
        if((msg[i] == '\r') && (msg[i + 1] == '\n'))
        {
            // Check /r/n/r/n
            ret = strlen(tmp);
            if(ret == 0) {
                ret = ast_json_array_append(j_out, ast_json_deep_copy(j_tmp));
                ast_json_unref(j_tmp);
                j_tmp = NULL;

                j_tmp = ast_json_object_create();
                j = 0;
                i++;
                continue;
            }

            value = ast_strdup(tmp);
            dump = value;
            key = strsep(&value, ":");
            if(key == NULL) {
                ast_free(dump);
                continue;
            }

            trim(key);
            trim(value);
            ast_json_object_set(j_tmp, key, ast_json_string_create(value));

            ast_free(dump);
            memset(tmp, 0x00, sizeof(tmp));
            j = 0;
            i++;
            continue;
        }
        tmp[j] = msg[i];
        j++;
    }

    if(j_tmp != NULL) {
        ast_json_unref(j_tmp);
    }
    return j_out;
}


/**
 * zmq command msg handler
 * @param data
 */
struct ast_json* ami_cmd_handler(struct ast_json* j_cmd)
{
    int ret;
    struct ast_json* j_tmp;
    struct ast_json* j_res;
    const char*     tmp_const;
    struct manager_custom_hook* hook;
    char str_cmd[10240];
    struct ast_json_iter* j_iter;

    // just for log
    if(j_cmd == NULL) {
        return NULL;
    }

    // Get action
    j_tmp = ast_json_object_get(j_cmd, "Action");
    if(j_tmp == NULL) {
        ast_log(AST_LOG_ERROR, " not get the action.\n");
        return NULL;
    }

    memset(str_cmd, 0x00, sizeof(str_cmd));
    sprintf(str_cmd, "Action: %s\n", ast_json_string_get(j_tmp));
    for(j_iter = ast_json_object_iter(j_cmd);
            j_iter != NULL;
            j_iter = ast_json_object_iter_next(j_cmd, j_iter))
    {
        tmp_const = ast_json_object_iter_key(j_iter);
        ret = strcmp(tmp_const, "Action");
        if(ret == 0) {
            continue;
        }
        j_tmp = ast_json_object_iter_value(j_iter);
        sprintf(str_cmd, "%s%s: %s\n", str_cmd, tmp_const, ast_json_string_get(j_tmp));
    }

    ast_log(LOG_DEBUG, "action command. command[%s]\n", str_cmd);

    // Set hook
    hook = ast_calloc(1, sizeof(struct manager_custom_hook));
    hook->file      = NULL;
    hook->helper    = &ami_cmd_helper;
    if(g_cmd_buf != NULL)
    {
        ast_free(g_cmd_buf);
        g_cmd_buf = NULL;
    }

    ret = ast_hook_send_action(hook, str_cmd);
    ast_free(hook);
    if(ret != 0)
    {
        ast_log(AST_LOG_ERROR, "Could not hook. ret[%d], err[%d:%s]\n", ret, errno, strerror(errno));
        return NULL;
    }
    ast_log(AST_LOG_DEBUG, "End hook. ret[%d]\n", ret);

    j_res = parse_ami_msg(g_cmd_buf);
    if(j_res == NULL) {
        ast_log(LOG_ERROR, "Could not parse response message.");
        return NULL;
    }

    return j_res;
}

/**
 *
 * @param category
 * @param event
 * @param content
 * @return
 */
static int ami_cmd_helper(int category, const char *event, char *content)
{
    char* tmp;
    int   ret;

    ast_log(LOG_DEBUG, "Fired ami_cmd_helper. len[%lu], category[%d], event[%s], content[%s]\n", strlen(content), category, event, content);
    if(g_cmd_buf == NULL) {
        ast_asprintf(&tmp, "%s", content);
    }
    else {
        ast_asprintf(&tmp, "%s%s", g_cmd_buf, content);
    }

    if(g_cmd_buf != NULL) {
        ast_free(g_cmd_buf);
    }

    ret = ast_asprintf(&g_cmd_buf, "%s", tmp);
    if(ret == -1) {
        ast_log(LOG_ERROR, "Could not allocate string. err[%d:%s]\n", errno, strerror(errno));
        return 0;
    }
    ast_free(tmp);

    return 1;
}
