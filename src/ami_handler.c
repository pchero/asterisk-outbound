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

#include <stdbool.h>

#include "dialing_handler.h"
#include "event_handler.h"


static char* g_cmd_buf = NULL;  //!< action cmd buffer
struct manager_custom_hook* g_hook_evt;

static int ami_evt_handler(void);
static int ami_cmd_helper(int category, const char *event, char *content);
static int ami_evt_helper(int category, const char *event, char *content);
static struct ast_json* parse_ami_msg(char* msg);
static void trim(char * s);
static void lower_string(char s[]);

void ami_evt_process(struct ast_json* j_evt);
static void ami_evt_Newchannel(struct ast_json* j_evt);
static void ami_evt_Newexten(struct ast_json* j_evt);
static void ami_evt_Newstate(struct ast_json* j_evt);
static void ami_evt_QueueCallerJoin(struct ast_json* j_evt);
static void ami_evt_QueueCallerLeave(struct ast_json* j_evt);
static void ami_evt_OriginateResponse(struct ast_json* j_evt);
static void ami_evt_AgentCalled(struct ast_json* j_evt);
static void ami_evt_AgentConnect(struct ast_json* j_evt);
static void ami_evt_DialBegin(struct ast_json* j_evt);
static void ami_evt_DialEnd(struct ast_json* j_evt);
static void ami_evt_Hangup(struct ast_json* j_evt);


int init_ami_handle(void)
{
    int ret;

    ret = ami_evt_handler();
    if(ret == false) {
        return false;
    }
    ast_log(LOG_NOTICE, "Initiated AMI handler.\n");

    return true;
}

void term_ami_handle(void)
{
    ast_manager_unregister_hook(g_hook_evt);
}

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

static void lower_string(char s[])
{
   int c = 0;

   while (s[c] != '\0') {
      if (s[c] >= 'A' && s[c] <= 'Z') {
         s[c] = s[c] + 32;
      }
      c++;
   }
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

//    ast_log(LOG_DEBUG, "Parse ami message. msg[%s]\n", msg);
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
    ast_log(LOG_DEBUG, "AMI Action command. action[%s]\n", ast_json_string_get(j_tmp));
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
//    ast_log(LOG_DEBUG, "action command. command[%s]\n", str_cmd);

    // Set hook
    hook = ast_calloc(1, sizeof(struct manager_custom_hook));
    hook->file      = NULL;
    hook->helper    = &ami_cmd_helper;
    if(g_cmd_buf != NULL) {
        ast_free(g_cmd_buf);
        g_cmd_buf = NULL;
    }

    ret = ast_hook_send_action(hook, str_cmd);
    ast_free(hook);
    if(ret != 0) {
        ast_log(AST_LOG_ERROR, "Could not hook. ret[%d], err[%d:%s]\n", ret, errno, strerror(errno));
        return NULL;
    }
//    ast_log(AST_LOG_DEBUG, "End hook. ret[%d]\n", ret);

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

//    ast_log(LOG_DEBUG, "Fired ami_cmd_helper. len[%lu], category[%d], event[%s], content[%s]\n", strlen(content), category, event, content);
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

static int ami_evt_handler(void)
{
//    struct manager_custom_hook* hook;

    g_hook_evt = ast_calloc(1, sizeof(struct manager_custom_hook));
    g_hook_evt->file = __FILE__;
    g_hook_evt->helper = &ami_evt_helper;
    ast_manager_register_hook(g_hook_evt);
    ast_log(LOG_NOTICE, "Start zmq_evt hook.\n");

    return true;
}

static int ami_evt_helper(int category, const char *event, char *content)
{
    struct ast_json* j_out;
    struct ast_json* j_tmp;
    int i;
    int j;
    int ret;
    char*   key;
    char*   key_tmp;
    char*   value;
    char    tmp_line[4096];
    char*   tmp_org;

    i = j = 0;
    memset(tmp_line, 0x00, sizeof(tmp_line));

    j_out = ast_json_object_create();
    for(i = 0; i < strlen(content); i++)
    {
        if((content[i] == '\r') && (content[i + 1] == '\n')) {
            ret = strlen(tmp_line);
            if(ret == 0) {
                break;
            }

//            ast_log(LOG_DEBUG, "Check value. tmp_line[%s]\n", tmp_line);
            value = ast_strdup(tmp_line);
            tmp_org = value;

            key = strsep(&value, ":");

            trim(key);
            key_tmp = ast_strdup(key);
            lower_string(key_tmp);
            trim(value);
            j_tmp = ast_json_string_create(value);
            ret = ast_json_object_set(j_out, key_tmp, j_tmp);

            ast_free(tmp_org);
            ast_free(key_tmp);
            memset(tmp_line, 0x00, sizeof(tmp_line));

            j = 0;
            i++;
            continue;
        }
        tmp_line[j] = content[i];
        j++;
    }

    ami_evt_process(j_out);
    ast_json_unref(j_out);

    return 0;
}

/**
 * Check that the ami response is Success or not.
 * @param j_ami
 * @return
 */
int ami_is_response_success(struct ast_json* j_ami)
{
    int ret;
    int i;
    size_t size;
    struct ast_json* j_tmp;
    const char* tmp_const;

    if((j_ami == NULL) || (ast_json_typeof(j_ami) != AST_JSON_ARRAY)) {
        return false;
    }

    size = ast_json_array_size(j_ami);
    for(i = 0; i < size; i++) {
        j_tmp = ast_json_array_get(j_ami, i);
        tmp_const = ast_json_string_get(ast_json_object_get(j_tmp, "Response"));
        if(tmp_const == NULL) {
            continue;
        }

        ret = strcmp(tmp_const, "Success");
        if(ret == 0) {
            return true;
        }
    }

    return false;
}

/**
 * Send/Get QueueSummary AMI.
 * @param name
 * @return
 */
struct ast_json* ami_cmd_queue_summary(const char* name)
{
    struct ast_json* j_cmd;
    struct ast_json* j_res;

    if(name == NULL) {
        return NULL;
    }

    j_cmd = ast_json_pack("{s:s, s:s}",
            "Action",   "QueueSummary",
            "Queue",    name
            );
    if(j_cmd == NULL) {
        ast_log(LOG_ERROR, "Could not create ami json. name[%s]\n", name);
        return NULL;
    }

    j_res = ami_cmd_handler(j_cmd);
    ast_json_unref(j_cmd);

    return j_res;
}

/**
 * Send/Get QueueSummary AMI.
 * @param name
 * @return
 */
struct ast_json* ami_cmd_originate_to_queue(struct ast_json* j_dl)
{
    struct ast_json* j_cmd;
    struct ast_json* j_res;
    int ret;
    char* tmp;

    if(j_dl == NULL) {
        return NULL;
    }

    //    Action: Originate
    //    ActionID: <value>
    //    Channel: <value>
    //    Exten: <value>
    //    Context: <value>
    //    Priority: <value>
    //    Application: <value>
    //    Data: <value>
    //    Timeout: <value>
    //    CallerID: <value>
    //    Variable: <value>
    //    Account: <value>
    //    EarlyMedia: <value>
    //    Async: <value>
    //    Codecs: <value>
    //    ChannelId: <value>
    //    OtherChannelId: <value>
    j_cmd = ast_json_pack("{s:s, s:s, s:s, s:s, s:s}",
            "Action",       "Originate",
            "Channel",      ast_json_string_get(ast_json_object_get(j_dl, "dial_addr")),
            "Async",        "true",
            "Application",  "Queue",
            "Data",         ast_json_string_get(ast_json_object_get(j_dl, "data"))
            );
    if(ast_json_object_get(j_dl, "timeout") != NULL)        ast_json_object_set(j_cmd, "Timeout", ast_json_ref(ast_json_object_get(j_dl, "timeout")));
    if(ast_json_object_get(j_dl, "callerid") != NULL)       ast_json_object_set(j_cmd, "CallerID", ast_json_ref(ast_json_object_get(j_dl, "callerid")));
    if(ast_json_object_get(j_dl, "variable") != NULL)       ast_json_object_set(j_cmd, "Variable", ast_json_ref(ast_json_object_get(j_dl, "variable")));
    if(ast_json_object_get(j_dl, "account") != NULL)        ast_json_object_set(j_cmd, "Account", ast_json_ref(ast_json_object_get(j_dl, "account")));
    if(ast_json_object_get(j_dl, "earlymedia") != NULL)     ast_json_object_set(j_cmd, "EarlyMedia", ast_json_ref(ast_json_object_get(j_dl, "earlymedia")));
    if(ast_json_object_get(j_dl, "codecs") != NULL)         ast_json_object_set(j_cmd, "Codecs", ast_json_ref(ast_json_object_get(j_dl, "codecs")));
    if(ast_json_object_get(j_dl, "channelid") != NULL)      ast_json_object_set(j_cmd, "ChannelId", ast_json_ref(ast_json_object_get(j_dl, "channelid")));
    if(ast_json_object_get(j_dl, "otherchannelid") != NULL) ast_json_object_set(j_cmd, "OtherChannelId", ast_json_ref(ast_json_object_get(j_dl, "otherchannelid")));

    if(j_cmd == NULL) {
        ast_log(LOG_ERROR, "Could not create ami json.\n");
        return NULL;
    }
    tmp = ast_json_dump_string_format(j_cmd, 0);
    ast_log(LOG_DEBUG, "Dialing. tmp[%s]\n", tmp);
    ast_json_free(tmp);

    j_res = ami_cmd_handler(j_cmd);
    ast_json_unref(j_cmd);

    ret = ami_is_response_success(j_res);
    if(ret == false) {
        ast_json_unref(j_res);
        return NULL;
    }

    return j_res;
}

struct ast_json* ami_cmd_hangup(const char* channel, int cause)
{
//    Action: Hangup
//    ActionID: <value>
//    Channel: <value>
//    Cause: <value>

    struct ast_json* j_cmd;
    struct ast_json* j_res;
    int ret;
    char* tmp;

    if(channel == NULL) {
        ast_log(LOG_WARNING, "Wrong input parameters.\n");
        return NULL;
    }

    ast_asprintf(&tmp, "%d", cause);
    j_cmd = ast_json_pack("{s:s, s:s, s:s}",
            "Action",   "Hangup",
            "Channel",  channel,
            "Cause",    tmp
            );
    ast_free(tmp);

    tmp = ast_json_dump_string_format(j_cmd, 0);
    ast_log(LOG_DEBUG, "Hangup. tmp[%s]\n", tmp);
    ast_json_free(tmp);

    j_res = ami_cmd_handler(j_cmd);
    ast_json_unref(j_cmd);

    ret = ami_is_response_success(j_res);
    if(ret == false) {
        ast_json_unref(j_res);
        return NULL;
    }

    return j_res;
}



struct ast_json* ami_cmd_SIPshowregistry(void)
{
    struct ast_json* j_cmd;
    struct ast_json* j_res;
    int ret;

    j_cmd = ast_json_pack("{s:s}",
            "Action",       "SIPshowregistry"
            );

    j_res = ami_cmd_handler(j_cmd);
    ast_json_unref(j_cmd);

    // check response
    ret = ami_is_response_success(j_res);
    if(ret == false) {
        ast_json_unref(j_res);
        return NULL;
    }

    return j_res;
}

void ami_evt_process(struct ast_json* j_evt)
{
    char* tmp;
    const char* event;

    event = ast_json_string_get(ast_json_object_get(j_evt, "event"));
    if(event == NULL) {
        ast_log(LOG_WARNING, "Could not get event.\n");
        return;
    }

    // log
    tmp = ast_json_dump_string_format(j_evt, 0);
    ast_log(LOG_DEBUG, "Received event. tmp[%s]\n", tmp);
    ast_json_free(tmp);


    if(strcmp(event, "Newchannel") == 0) {
        ami_evt_Newchannel(j_evt);
    }
    else if(strcmp(event, "Newexten") == 0) {
        ami_evt_Newexten(j_evt);
    }
    else if(strcmp(event, "Newstate") == 0) {
        ami_evt_Newstate(j_evt);
    }
    else if(strcmp(event, "QueueCallerJoin") == 0) {
        ami_evt_QueueCallerJoin(j_evt);
    }
    else if(strcmp(event, "QueueCallerLeave") == 0) {
        ami_evt_QueueCallerLeave(j_evt);
    }
    else if(strcmp(event, "OriginateResponse") == 0) {
        ami_evt_OriginateResponse(j_evt);
    }
    else if(strcmp(event, "AgentCalled") == 0) {
        ami_evt_AgentCalled(j_evt);
    }
    else if(strcmp(event, "AgentConnect") == 0) {
        ami_evt_AgentConnect(j_evt);
    }
    else if(strcmp(event, "DialBegin") == 0) {
        ami_evt_DialBegin(j_evt);
    }
    else if(strcmp(event, "DialEnd") == 0) {
        ami_evt_DialEnd(j_evt);
    }
    else if(strcmp(event, "Hangup") == 0) {
        ami_evt_Hangup(j_evt);
    }

    return;
}


static void ami_evt_Newchannel(struct ast_json* j_evt)
{
//    {
//        "event": "Newchannel",
//        "calleridname": "<unknown>",
//        "channelstatedesc": "Down",
//        "priority": "1",
//        "privilege": "call,all",
//        "channel": "SIP/trunk_test_1-00000000",
//        "calleridnum": "<unknown>",
//        "linkedid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "channelstate": "0",
//        "uniqueid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "connectedlinenum": "<unknown>",
//        "connectedlinename": "<unknown>",
//        "language": "en",
//        "accountcode": "",
//        "exten": "s",
//        "context": "from_provider"
//    }

    const char* tmp_const;
    rb_dialing* dialing;
    struct ast_json* j_tmp;
    char* tmp;

    // get rb_dialing
    tmp_const = ast_json_string_get(ast_json_object_get(j_evt, "uniqueid"));
    dialing = rb_dialing_find_uuid_chan(tmp_const);
    if(dialing == NULL) {
        return;
    }

    tmp = get_utc_timestamp();
    j_tmp = ast_json_deep_copy(j_evt);
    ast_json_object_del(j_tmp, "event");
    ast_json_object_set(j_tmp, "tm_create", ast_json_string_create(tmp));
    ast_free(tmp);

    ast_json_object_update(dialing->j_chan, j_tmp);
    ast_json_unref(j_tmp);
}

static void ami_evt_Newexten(struct ast_json* j_evt)
{
//    {
//        "event": "Newexten",
//        "calleridname": "<unknown>",
//        "channelstatedesc": "Down",
//        "privilege": "call,all",
//        "priority": "1",
//        "channel": "SIP/trunk_test_1-00000000",
//        "calleridnum": "<unknown>",
//        "channelstate": "0",
//        "linkedid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "uniqueid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "extension": "",
//        "connectedlinenum": "<unknown>",
//        "connectedlinename": "<unknown>",
//        "language": "en",
//        "accountcode": "",
//        "exten": "",
//        "context": "from_provider",
//        "appdata": "(Outgoing Line)",
//        "application": "AppDial2"
//    }

    const char* tmp_const;
    rb_dialing* dialing;
    struct ast_json* j_tmp;

    // get rb_dialing
    tmp_const = ast_json_string_get(ast_json_object_get(j_evt, "uniqueid"));
    dialing = rb_dialing_find_uuid_chan(tmp_const);
    if(dialing == NULL) {
        return;
    }

    j_tmp = ast_json_deep_copy(j_evt);
    ast_json_object_del(j_tmp, "event");

    ast_json_object_update(dialing->j_chan, j_tmp);
    ast_json_unref(j_tmp);
}

static void ami_evt_Newstate(struct ast_json* j_evt)
{
//    {
//        "event": "Newstate",
//        "calleridname": "<unknown>",
//        "channelstatedesc": "Up",
//        "priority": "1",
//        "privilege": "call,all",
//        "channel": "SIP/trunk_test_1-00000000",
//        "calleridnum": "<unknown>",
//        "linkedid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "channelstate": "6",
//        "uniqueid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "connectedlinenum": "<unknown>",
//        "connectedlinename": "<unknown>",
//        "language": "en",
//        "accountcode": "",
//        "exten": "",
//        "context": "from_provider"
//    }

    const char* tmp_const;
    rb_dialing* dialing;
    struct ast_json* j_tmp;

    // get rb_dialing
    tmp_const = ast_json_string_get(ast_json_object_get(j_evt, "uniqueid"));
    dialing = rb_dialing_find_uuid_chan(tmp_const);
    if(dialing == NULL) {
        return;
    }

    j_tmp = ast_json_deep_copy(j_evt);
    ast_json_object_del(j_tmp, "event");

    ast_json_object_update(dialing->j_chan, j_tmp);
    ast_json_unref(j_tmp);
}

static void ami_evt_QueueCallerJoin(struct ast_json* j_evt)
{
//    {
//        "event": "QueueCallerJoin",
//        "calleridname": "<unknown>",
//        "channelstatedesc": "Up",
//        "privilege": "agent,all",
//        "priority": "1",
//        "channel": "SIP/trunk_test_1-00000000",
//        "calleridnum": "<unknown>",
//        "channelstate": "6",
//        "linkedid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "uniqueid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "connectedlinenum": "<unknown>",
//        "connectedlinename": "<unknown>",
//        "position": "1",
//        "language": "en",
//        "accountcode": "",
//        "exten": "",
//        "context": "from_provider",
//        "queue": "TestQueue",
//        "count": "1"
//    }

    const char* tmp_const;
    rb_dialing* dialing;
    struct ast_json* j_tmp;
    char* tmp;

    // get rb_dialing
    tmp_const = ast_json_string_get(ast_json_object_get(j_evt, "uniqueid"));
    dialing = rb_dialing_find_uuid_chan(tmp_const);
    if(dialing == NULL) {
        return;
    }

    tmp = get_utc_timestamp();
    j_tmp = ast_json_deep_copy(j_evt);
    ast_json_object_del(j_tmp, "event");
    ast_json_object_set(j_tmp, "tm_enter", ast_json_string_create(tmp));
    ast_free(tmp);

    // append
    ast_json_array_append(dialing->j_queues, j_tmp);
}

static void ami_evt_QueueCallerLeave(struct ast_json* j_evt)
{
//    {
//        "event": "QueueCallerLeave",
//        "calleridname": "<unknown>",
//        "channelstatedesc": "Up",
//        "privilege": "agent,all",
//        "priority": "1",
//        "channel": "SIP/trunk_test_1-00000000",
//        "calleridnum": "<unknown>",
//        "channelstate": "6",
//        "linkedid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "uniqueid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "connectedlinenum": "<unknown>",
//        "connectedlinename": "<unknown>",
//        "position": "1",
//        "language": "en",
//        "accountcode": "",
//        "exten": "",
//        "context": "from_provider",
//        "queue": "TestQueue",
//        "count": "0"
//    }

    const char* tmp_const;
    rb_dialing* dialing;
    struct ast_json* j_tmp;
    char* tmp;
    int i;

    // get rb_dialing
    tmp_const = ast_json_string_get(ast_json_object_get(j_evt, "uniqueid"));
    dialing = rb_dialing_find_uuid_chan(tmp_const);
    if(dialing == NULL) {
        return;
    }

    // find correct queue json.
    tmp = get_utc_timestamp();
    for(i = 0; i < ast_json_array_size(dialing->j_queues); i++) {
        j_tmp = ast_json_array_get(dialing->j_queues, i);
        if(j_tmp == NULL) {
            continue;
        }

        tmp_const = ast_json_string_get(ast_json_object_get(j_tmp, "queue"));
        if(tmp_const == NULL) {
            continue;
        }
        if(strcmp(tmp_const, ast_json_string_get(ast_json_object_get(j_evt, "queue"))) != 0) {
            continue;
        }

        ast_json_object_set(j_tmp, "tm_leave", ast_json_string_create(tmp));
        break;
    }
    ast_free(tmp);
}

static void ami_evt_AgentCalled(struct ast_json* j_evt)
{
//    {
//        "event": "AgentCalled",
//        "calleridname": "<unknown>",
//        "channelstatedesc": "Up",
//        "priority": "1",
//        "privilege": "agent,all",
//        "channel": "SIP/trunk_test_1-00000000",
//        "calleridnum": "<unknown>",
//        "linkedid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "channelstate": "6",
//        "destconnectedlinename": "<unknown>",
//        "membername": "test 04",
//        "uniqueid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "connectedlinenum": "<unknown>",
//        "destchannelstatedesc": "Down",
//        "connectedlinename": "<unknown>",
//        "language": "en",
//        "destcalleridnum": "<unknown>",
//        "accountcode": "",
//        "destconnectedlinenum": "<unknown>",
//        "destlanguage": "en",
//        "exten": "",
//        "context": "from_provider",
//        "destchannel": "Local/test-04@common-incoming-00000000;1",
//        "destchannelstate": "0",
//        "destcalleridname": "<unknown>",
//        "destaccountcode": "",
//        "destcontext": "common-incoming",
//        "destexten": "",
//        "destpriority": "1",
//        "destuniqueid": "1447457889.1",
//        "destlinkedid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "queue": "TestQueue",
//        "interface": "Local/test-04@common-incoming"
//    }

    const char* tmp_const;
    rb_dialing* dialing;
    struct ast_json* j_tmp;
    char* tmp;

    // get rb_dialing
    tmp_const = ast_json_string_get(ast_json_object_get(j_evt, "uniqueid"));
    dialing = rb_dialing_find_uuid_chan(tmp_const);
    if(dialing == NULL) {
        return;
    }

    tmp = get_utc_timestamp();
    j_tmp = ast_json_deep_copy(j_evt);
    ast_json_object_del(j_tmp, "event");
    ast_json_object_set(j_tmp, "tm_agent_called", ast_json_string_create(tmp));
    ast_free(tmp);

    // append
    ast_json_array_append(dialing->j_agents, j_tmp);
    ast_json_object_update(dialing->j_chan, j_tmp);
    ast_json_object_del(dialing->j_chan, "tm_agent_connect");
}

static void ami_evt_AgentConnect(struct ast_json* j_evt)
{
//    {
//        "event": "AgentConnect",
//        "calleridname": "<unknown>",
//        "channelstatedesc": "Up",
//        "priority": "1",
//        "privilege": "agent,all",
//        "channel": "SIP/trunk_test_1-00000000",
//        "calleridnum": "<unknown>",
//        "linkedid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "channelstate": "6",
//        "destconnectedlinename": "<unknown>",
//        "membername": "test 04",
//        "uniqueid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "connectedlinenum": "<unknown>",
//        "destchannelstatedesc": "Up",
//        "connectedlinename": "<unknown>",
//        "language": "en",
//        "destcalleridnum": "<unknown>",
//        "accountcode": "",
//        "destconnectedlinenum": "<unknown>",
//        "destlanguage": "en",
//        "exten": "",
//        "context": "from_provider",
//        "destchannel": "Local/test-04@common-incoming-00000000;1",
//        "destchannelstate": "6",
//        "destcalleridname": "<unknown>",
//        "destaccountcode": "",
//        "destcontext": "common-incoming",
//        "destexten": "",
//        "destpriority": "1",
//        "ringtime": "5",
//        "destuniqueid": "1447457889.1",
//        "destlinkedid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "queue": "TestQueue",
//        "interface": "Local/test-04@common-incoming",
//        "holdtime": "6"
//    }

    const char* tmp_const;
    rb_dialing* dialing;
    struct ast_json* j_tmp;
    struct ast_json* j_copy;
    char* tmp;
    int size;
    int i;

    // get rb_dialing
    tmp_const = ast_json_string_get(ast_json_object_get(j_evt, "uniqueid"));
    dialing = rb_dialing_find_uuid_chan(tmp_const);
    if(dialing == NULL) {
        return;
    }

    tmp = get_utc_timestamp();
    j_copy = ast_json_deep_copy(j_evt);
    ast_json_object_del(j_copy, "event");
    ast_json_object_set(j_copy, "tm_agent_connect", ast_json_string_create(tmp));
    ast_free(tmp);

    size = ast_json_array_size(dialing->j_agents);
    for(i = 0; i < size; i++) {
        j_tmp = ast_json_array_get(dialing->j_agents, i);
        if(j_tmp == NULL) {
            continue;
        }

        tmp_const = ast_json_string_get(ast_json_object_get(j_tmp, "destuniqueid"));
        if(tmp_const == NULL) {
            continue;
        }

        if(strcmp(tmp_const, ast_json_string_get(ast_json_object_get(j_copy, "destuniqueid"))) != 0) {
            continue;
        }

        // update
        ast_json_object_update(j_tmp, j_copy);
        ast_json_object_update(dialing->j_chan, j_copy);

        break;
    }
    ast_json_unref(j_copy);
}

static void ami_evt_DialBegin(struct ast_json* j_evt)
{
//    {
//        "event": "DialBegin",
//        "calleridname": "<unknown>",
//        "channelstatedesc": "Up",
//        "privilege": "call,all",
//        "priority": "1",
//        "channel": "SIP/trunk_test_1-00000000",
//        "calleridnum": "<unknown>",
//        "channelstate": "6",
//        "linkedid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "destconnectedlinename": "<unknown>",
//        "uniqueid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "destchannelstatedesc": "Down",
//        "connectedlinenum": "<unknown>",
//        "connectedlinename": "<unknown>",
//        "destcalleridnum": "<unknown>",
//        "language": "en",
//        "destconnectedlinenum": "<unknown>",
//        "accountcode": "",
//        "destlanguage": "en",
//        "exten": "",
//        "dialstring": "Local/test-04@common-incoming",
//        "context": "from_provider",
//        "destchannel": "Local/test-04@common-incoming-00000000;1",
//        "destchannelstate": "0",
//        "destcalleridname": "<unknown>",
//        "destaccountcode": "",
//        "destcontext": "common-incoming",
//        "destexten": "test-04",
//        "destpriority": "1",
//        "destlinkedid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "destuniqueid": "1447457889.1"
//    }

    const char* tmp_const;
    rb_dialing* dialing;
    struct ast_json* j_tmp;
    struct ast_json* j_copy;
    char* tmp;
    int i;
    int size;

    // get rb_dialing
    tmp_const = ast_json_string_get(ast_json_object_get(j_evt, "uniqueid"));
    dialing = rb_dialing_find_uuid_chan(tmp_const);
    if(dialing == NULL) {
        return;
    }

    tmp = get_utc_timestamp();
    j_copy = ast_json_deep_copy(j_evt);
    ast_json_object_del(j_copy, "event");
    ast_json_object_set(j_copy, "tm_dial_begin", ast_json_string_create(tmp));
    ast_free(tmp);

    size = ast_json_array_size(dialing->j_agents);
    for(i = 0; i < size; i++) {
        j_tmp = ast_json_array_get(dialing->j_agents, i);
        if(j_tmp == NULL) {
            continue;
        }

        tmp_const = ast_json_string_get(ast_json_object_get(j_tmp, "destuniqueid"));
        if(tmp_const == NULL) {
            continue;
        }

        if(strcmp(tmp_const, ast_json_string_get(ast_json_object_get(j_evt, "destuniqueid"))) != 0) {
            continue;
        }

        // update
        ast_json_object_update(j_tmp, j_copy);

        break;
    }
    ast_free(tmp);
}

static void ami_evt_DialEnd(struct ast_json* j_evt)
{
//    {
//        "event": "DialEnd",
//        "calleridname": "<unknown>",
//        "channelstatedesc": "Up",
//        "privilege": "call,all",
//        "priority": "1",
//        "channel": "SIP/trunk_test_1-00000000",
//        "calleridnum": "<unknown>",
//        "channelstate": "6",
//        "linkedid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "destconnectedlinename": "<unknown>",
//        "uniqueid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "destchannelstatedesc": "Up",
//        "connectedlinenum": "<unknown>",
//        "connectedlinename": "<unknown>",
//        "destcalleridnum": "<unknown>",
//        "language": "en",
//        "destconnectedlinenum": "<unknown>",
//        "accountcode": "",
//        "destlanguage": "en",
//        "exten": "",
//        "context": "from_provider",
//        "destchannel": "Local/test-04@common-incoming-00000000;1",
//        "dialstatus": "ANSWER",
//        "destchannelstate": "6",
//        "destcalleridname": "<unknown>",
//        "destaccountcode": "",
//        "destcontext": "common-incoming",
//        "destexten": "",
//        "destpriority": "1",
//        "destlinkedid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "destuniqueid": "1447457889.1"
//    }

    const char* tmp_const;
    rb_dialing* dialing;
    struct ast_json* j_tmp;
    char* tmp;
    int i;
    int size;

    // get rb_dialing
    tmp_const = ast_json_string_get(ast_json_object_get(j_evt, "uniqueid"));
    dialing = rb_dialing_find_uuid_chan(tmp_const);
    if(dialing == NULL) {
        return;
    }

    tmp = get_utc_timestamp();
    size = ast_json_array_size(dialing->j_agents);
    for(i = 0; i < size; i++) {
        j_tmp = ast_json_array_get(dialing->j_agents, i);
        if(j_tmp == NULL) {
            continue;
        }

        tmp_const = ast_json_string_get(ast_json_object_get(j_tmp, "destuniqueid"));
        if(tmp == NULL) {
            continue;
        }

        if(strcmp(tmp_const, ast_json_string_get(ast_json_object_get(j_evt, "destuniqueid"))) != 0) {
            continue;
        }

        // update
        ast_json_object_update(j_tmp, j_evt);
        ast_json_object_del(j_tmp, "event");
        ast_json_object_set(j_tmp, "tm_dial_end", ast_json_string_create(tmp));

        break;
    }
    ast_free(tmp);
}


static void ami_evt_OriginateResponse(struct ast_json* j_evt)
{
//    {
//        "calleridname": "<unknown>",
//        "event": "OriginateResponse",
//        "privilege": "call,all",
//        "response": "Success",
//        "uniqueid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "calleridnum": "<unknown>",
//        "channel": "SIP/trunk_test_1-00000000",
//        "context": "",
//        "exten": "",
//        "reason": "4"
//    }

    const char* uuid;
    rb_dialing* dialing;
    char* tmp;
    const char* tmp_const;

    // get rb_dialing
    uuid = ast_json_string_get(ast_json_object_get(j_evt, "uniqueid"));
    dialing = rb_dialing_find_uuid_chan(uuid);
    if(dialing == NULL) {
        return;
    }

    tmp = get_utc_timestamp();

    // update channel
    ast_json_object_update(dialing->j_chan, j_evt);
    ast_json_object_del(dialing->j_chan, "event");
    ast_json_object_set(dialing->j_chan, "tm_dial_end", ast_json_string_create(tmp));

    // update result
    ast_json_object_set(dialing->j_res, "dial_channel", ast_json_ref(ast_json_object_get(j_evt, "channel")));
    tmp_const = ast_json_string_get(ast_json_object_get(j_evt, "reason"));
    ast_json_object_set(dialing->j_res, "res_dial", ast_json_integer_create(atoi(tmp_const)));
    ast_json_object_set(dialing->j_res, "tm_dial_end", ast_json_string_create(tmp));
    ast_free(tmp);

    // update status
    rb_dialing_set_status(uuid, E_DIALING_ORIGINATE_RESPONSE);
}

static void ami_evt_Hangup(struct ast_json* j_evt)
{
//    {
//        "event": "Hangup",
//        "calleridname": "<unknown>",
//        "channelstatedesc": "Up",
//        "privilege": "call,all",
//        "priority": "1",
//        "channel": "SIP/trunk_test_1-00000000",
//        "calleridnum": "<unknown>",
//        "channelstate": "6",
//        "linkedid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "uniqueid": "dee9d42f-972c-4f87-b5fb-ff8edf1e6f35",
//        "cause": "16",
//        "connectedlinenum": "<unknown>",
//        "connectedlinename": "<unknown>",
//        "language": "en",
//        "accountcode": "",
//        "exten": "",
//        "context": "from_provider",
//        "cause-txt": "Normal Clearing"
//    }

    const char* uuid;
    rb_dialing* dialing;
    char* tmp;
    const char* tmp_const;

    // get rb_dialing
    uuid = ast_json_string_get(ast_json_object_get(j_evt, "uniqueid"));
    dialing = rb_dialing_find_uuid_chan(uuid);
    if(dialing == NULL) {
        return;
    }

    tmp = get_utc_timestamp();

    // update channel
    ast_json_object_update(dialing->j_chan, j_evt);
    ast_json_object_set(dialing->j_chan, "tm_hangup", ast_json_string_create(tmp));
    ast_json_object_del(dialing->j_chan, "event");

    // update result json
    ast_json_object_set(dialing->j_res, "tm_hangup", ast_json_string_create(tmp));
    tmp_const = ast_json_string_get(ast_json_object_get(j_evt, "cause"));
    ast_json_object_set(dialing->j_res, "res_hangup", ast_json_integer_create(atoi(tmp_const)));

    ast_free(tmp);

    // update status
    rb_dialing_set_status(uuid, E_DIALING_HANGUP);
}
