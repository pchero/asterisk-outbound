/*
 * dialing_handler.c
 *
 *  Created on: Nov 21, 2015
 *      Author: pchero
 */

#include "asterisk.h"
#include "asterisk/utils.h"
#include "asterisk/astobj2.h"

#include <stdbool.h>

#include "dialing_handler.h"
#include "event_handler.h"
#include "cli_handler.h"

AST_MUTEX_DEFINE_STATIC(g_rb_dialing_mutex);

static int rb_dialing_cmp_cb(void* obj, void* arg, int flags);
static int rb_dialing_sort_cb(const void* o_left, const void* o_right, int flags);
static void rb_dialing_destructor(void* obj);
static bool rb_dialing_update(rb_dialing* dialing);

static struct ao2_container* g_rb_dialings = NULL;  ///< dialing container

/**
 * Initiate rb_diailing.
 * @return
 */
int init_rb_dialing(void)
{
    g_rb_dialings = ao2_container_alloc_rbtree(AO2_ALLOC_OPT_LOCK_MUTEX, AO2_CONTAINER_ALLOC_OPT_DUPS_REJECT, rb_dialing_sort_cb, rb_dialing_cmp_cb);
    if(g_rb_dialings == NULL) {
        ast_log(LOG_ERROR, "Could not create rbtree.\n");
        return false;
    }
   ast_log(LOG_NOTICE, "Initiated dialing handler.\n");

    return true;
}

/**
 *
 * @param o_left
 * @param o_right
 * @param flags
 * @return
 */
static int rb_dialing_sort_cb(const void* o_left, const void* o_right, int flags)
{
    const rb_dialing* dialing_left;
    const char* key;

    dialing_left = (rb_dialing*)o_left;

    if(flags & OBJ_SEARCH_KEY) {
        key = (char*)o_right;

        return strcmp(dialing_left->uuid, key);
    }
    else if(flags & OBJ_SEARCH_PARTIAL_KEY) {
        key = (char*)o_right;

        return strcmp(ast_json_string_get(ast_json_object_get(dialing_left->j_res, "dl_list_uuid")), key);
    }
    else {
        const rb_dialing* dialing_right;

        dialing_right = (rb_dialing*)o_right;
        return strcmp(dialing_left->uuid, dialing_right->uuid);
    }
}

/**
 *
 * @param obj
 * @param arg
 * @param flags
 * @return
 */
static int rb_dialing_cmp_cb(void* obj, void* arg, int flags)
{
    rb_dialing* dialing;
    const char *uuid;

    dialing = (rb_dialing*)obj;

    if(flags & OBJ_SEARCH_KEY) {
        uuid = (const char*)arg;

        // channel id
        if(dialing->uuid == NULL) {
            return 0;
        }
        if(strcmp(dialing->uuid, uuid) == 0) {
            return CMP_MATCH;
        }
        return 0;
    }
    else if(flags & OBJ_SEARCH_PARTIAL_KEY) {
        uuid = (const char*)arg;

        // uuid_dl
        if(strcmp(ast_json_string_get(ast_json_object_get(dialing->j_res, "dl_list_uuid")), uuid) == 0) {
            return CMP_MATCH;
        }
        return 0;
    }
    else {
        // channel id
        rb_dialing* dialing_right;

        dialing_right = (rb_dialing*)arg;
        if(strcmp(dialing->uuid, dialing_right->uuid) == 0) {
            return CMP_MATCH;
        }
        return 0;
    }
}

/**
 * Create dialing obj.
 * @param j_camp
 * @param j_plan
 * @param j_dl
 * @return
 */
rb_dialing* rb_dialing_create(
        const char* dialing_uuid,
        struct ast_json* j_camp,
        struct ast_json* j_plan,
        struct ast_json* j_dlma,
        struct ast_json* j_dl
        )
{
    rb_dialing* dialing;
    char* tmp;

    if((dialing_uuid == NULL) || (j_camp == NULL) || (j_plan == NULL) || (j_dl == NULL)) {
        return NULL;
    }

    // create rb object
    dialing = ao2_alloc(sizeof(rb_dialing), rb_dialing_destructor);

    // init status
    dialing->status = E_DIALING_NONE;

    // init json info
    dialing->uuid = ast_strdup(dialing_uuid);
    dialing->j_chan = ast_json_object_create();
    dialing->j_queues = ast_json_array_create();
    dialing->j_agents = ast_json_array_create();
    dialing->j_res = ast_json_object_create();

    // result configure
    ast_json_object_set(dialing->j_res, "dialing_uuid", ast_json_string_create(dialing_uuid));
    ast_json_object_set(dialing->j_res, "camp_uuid",    ast_json_ref(ast_json_object_get(j_camp, "uuid")));
    ast_json_object_set(dialing->j_res, "plan_uuid",    ast_json_ref(ast_json_object_get(j_plan, "uuid")));
    ast_json_object_set(dialing->j_res, "dlma_uuid",    ast_json_ref(ast_json_object_get(j_dlma, "uuid")));
    ast_json_object_set(dialing->j_res, "dl_list_uuid", ast_json_ref(ast_json_object_get(j_dl, "uuid")));

    // info_camp
    tmp = ast_json_dump_string_format(j_camp, 0);
    ast_json_object_set(dialing->j_res, "info_camp", ast_json_string_create(tmp));
    ast_json_free(tmp);

    // info_plan
    tmp = ast_json_dump_string_format(j_plan, 0);
    ast_json_object_set(dialing->j_res, "info_plan", ast_json_string_create(tmp));
    ast_json_free(tmp);

    // info_plan
    tmp = ast_json_dump_string_format(j_dlma, 0);
    ast_json_object_set(dialing->j_res, "info_dlma", ast_json_string_create(tmp));
    ast_json_free(tmp);

    // info_dl
    tmp = ast_json_dump_string_format(j_dl, 0);
    ast_json_object_set(dialing->j_res, "info_dl", ast_json_string_create(tmp));
    ast_json_free(tmp);

    // timestamp
    dialing->tm_create = get_utc_timestamp();
    dialing->tm_update = NULL;
    dialing->tm_delete = NULL;
    clock_gettime(CLOCK_REALTIME, &dialing->timeptr_update);

    // insert into rb
    ast_mutex_lock(&g_rb_dialing_mutex);
    if(ao2_link(g_rb_dialings, dialing) == 0) {
        ast_log(LOG_DEBUG, "Could not register the dialing. uuid[%s]\n", dialing->uuid);
        rb_dialing_destructor(dialing);
        ast_mutex_unlock(&g_rb_dialing_mutex);
        return NULL;
    }

    // send event to all
    send_manager_evt_out_dialing_create(dialing);

    ast_mutex_unlock(&g_rb_dialing_mutex);

    return dialing;
}

void rb_dialing_destory(rb_dialing* dialing)
{
    ast_mutex_lock(&g_rb_dialing_mutex);

    ast_log(LOG_DEBUG, "Destroying dialing.\n");
    ao2_unlink(g_rb_dialings, dialing);
    ao2_ref(dialing, -1);

    ast_mutex_unlock(&g_rb_dialing_mutex);

    return;
}

static void rb_dialing_destructor(void* obj)
{
    rb_dialing* dialing;

    dialing = (rb_dialing*)obj;

    // send destroy
    send_manager_evt_out_dialing_delete(dialing);

    if(dialing->uuid != NULL)       ast_free(dialing->uuid);
    if(dialing->j_res != NULL)      ast_json_unref(dialing->j_res);
    if(dialing->j_chan != NULL)     ast_json_unref(dialing->j_chan);
    if(dialing->j_queues != NULL)   ast_json_unref(dialing->j_queues);
    if(dialing->j_agents != NULL)   ast_json_unref(dialing->j_agents);
    if(dialing->tm_create != NULL)  ast_free(dialing->tm_create);
    if(dialing->tm_update != NULL)  ast_free(dialing->tm_update);
    if(dialing->tm_delete != NULL)  ast_free(dialing->tm_delete);

    ast_log(LOG_DEBUG, "Called destroyer.\n");
}

/**
 * There's no mutex lock here.
 * locking is caller's responsibility.
 * @param dialing
 * @return
 */
static bool rb_dialing_update(rb_dialing* dialing)
{
    if(dialing == NULL) {
        return false;
    }

    if(dialing->tm_update != NULL) {
        ast_free(dialing->tm_update);
    }
    dialing->tm_update = get_utc_timestamp();

    clock_gettime(CLOCK_REALTIME, &dialing->timeptr_update);

    send_manager_evt_out_dialing_update(dialing);

    return true;
}

bool rb_dialing_update_chan_update(rb_dialing* dialing, struct ast_json* j_evt)
{
    int ret;

    if((dialing == NULL) || (j_evt == NULL)) {
        return false;
    }

    ast_mutex_lock(&g_rb_dialing_mutex);

    ast_json_object_update(dialing->j_chan, j_evt);

    ret = rb_dialing_update(dialing);
    ast_mutex_unlock(&g_rb_dialing_mutex);
    if(ret != true) {
        return false;
    }

    return true;
}

bool rb_dialing_update_agent_append(rb_dialing* dialing, struct ast_json* j_evt)
{
    int ret;

    if((dialing == NULL) || (j_evt == NULL)) {
        return false;
    }

    ast_mutex_lock(&g_rb_dialing_mutex);

    ast_json_array_append(dialing->j_agents, ast_json_ref(j_evt));
    ret = rb_dialing_update(dialing);
    ast_mutex_unlock(&g_rb_dialing_mutex);
    if(ret != true) {
        return false;
    }

    return true;
}

bool rb_dialing_update_agent_update(rb_dialing* dialing, struct ast_json* j_evt)
{
    int ret;
    int i;
    const char* tmp_const;
    unsigned int size;
    struct ast_json* j_tmp;

    if((dialing == NULL) || (j_evt == NULL)) {
        return false;
    }

    ast_mutex_lock(&g_rb_dialing_mutex);

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
        ast_json_object_update(j_tmp, j_evt);

        break;
    }

    ret = rb_dialing_update(dialing);
    ast_mutex_unlock(&g_rb_dialing_mutex);
    if(ret != true) {
        return false;
    }

    return true;
}

bool rb_dialing_update_queue_append(rb_dialing* dialing, struct ast_json* j_evt)
{
    int ret;

    if((dialing == NULL) || (j_evt == NULL)) {
        return false;
    }

    ast_mutex_lock(&g_rb_dialing_mutex);
    ast_json_array_append(dialing->j_queues, ast_json_ref(j_evt));
    ret = rb_dialing_update(dialing);
    ast_mutex_unlock(&g_rb_dialing_mutex);
    if(ret != true) {
        return false;
    }

    return true;
}

/**
 * Update dialing res
 * @param dialing
 * @param j_res
 * @return
 */
bool rb_dialing_update_res_update(rb_dialing* dialing, struct ast_json* j_res)
{
    int ret;

    if((dialing == NULL) || (j_res == NULL)) {
        return false;
    }

    ast_mutex_lock(&g_rb_dialing_mutex);
    ast_json_array_append(dialing->j_res, ast_json_ref(j_res));
    ret = rb_dialing_update(dialing);
    ast_mutex_unlock(&g_rb_dialing_mutex);
    if(ret != true) {
        return false;
    }

    return true;
}

bool rb_dialing_update_status(rb_dialing* dialing, E_DIALING_STATUS_T status)
{
    int ret;

    if(dialing == NULL) {
        return false;
    }

    ast_mutex_lock(&g_rb_dialing_mutex);
    dialing->status = status;
    ret = rb_dialing_update(dialing);
    ast_mutex_unlock(&g_rb_dialing_mutex);
    if(ret != true) {
        return false;
    }

    return true;
}

rb_dialing* rb_dialing_find_dl_uuid(const char* chan)
{
    rb_dialing* dialing;
    ast_mutex_lock(&g_rb_dialing_mutex);
    dialing = ao2_find(g_rb_dialings, chan, OBJ_SEARCH_PARTIAL_KEY);
    if(dialing == NULL) {
        ast_mutex_unlock(&g_rb_dialing_mutex);
        return NULL;
    }
    ao2_ref(dialing, -1);
    ast_mutex_unlock(&g_rb_dialing_mutex);

    return dialing;
}

rb_dialing* rb_dialing_find_chan_uuid(const char* uuid)
{
    rb_dialing* dialing;

    if(uuid == NULL) {
        return NULL;
    }

    ast_log(LOG_DEBUG, "rb_dialing_find_uuid. uuid[%s]\n", uuid);
    ast_mutex_lock(&g_rb_dialing_mutex);
    dialing = ao2_find(g_rb_dialings, uuid, OBJ_SEARCH_KEY);
    if(dialing == NULL) {
        ast_mutex_unlock(&g_rb_dialing_mutex);
        return NULL;
    }
    ao2_ref(dialing, -1);
    ast_mutex_unlock(&g_rb_dialing_mutex);

    return dialing;
}

bool rb_dialing_is_exist_uuid(const char* uuid)
{
    rb_dialing* dialing;

    if(uuid == NULL) {
        return false;
    }

    dialing = rb_dialing_find_chan_uuid(uuid);
    if(dialing == NULL) {
        return false;
    }

    return true;
}

struct ao2_iterator rb_dialing_iter_init(void)
{
    struct ao2_iterator iter;

    ast_mutex_lock(&g_rb_dialing_mutex);
    iter = ao2_iterator_init(g_rb_dialings, 0);
    ast_mutex_unlock(&g_rb_dialing_mutex);

    return iter;
}

void rb_dialing_iter_destroy(struct ao2_iterator* iter)
{
    ao2_iterator_destroy(iter);
    return;
}

rb_dialing* rb_dialing_iter_next(struct ao2_iterator *iter)
{
    rb_dialing* dialing;

    ast_mutex_lock(&g_rb_dialing_mutex);
    dialing = ao2_iterator_next(iter);
    if(dialing == NULL) {
        ast_mutex_unlock(&g_rb_dialing_mutex);
        return NULL;
    }
    ao2_ref(dialing, -1);
    ast_mutex_unlock(&g_rb_dialing_mutex);

    return dialing;
}

struct ast_json* rb_dialing_get_all_for_cli(void)
{
    struct ao2_iterator iter;
    rb_dialing* dialing;
    struct ast_json* j_res;
    struct ast_json* j_tmp;

    j_res = ast_json_array_create();
    iter = rb_dialing_iter_init();
    while(1) {
        dialing = rb_dialing_iter_next(&iter);
        if(dialing == NULL) {
            break;
        }

        j_tmp = ast_json_pack("{s:s, s:s, s:s, s:s, s:s, s:s}",
                "uuid",         dialing->uuid,
                "channelstate", ast_json_string_get(ast_json_object_get(dialing->j_chan, "channelstate")) ? : "",
                "channel",      ast_json_string_get(ast_json_object_get(dialing->j_chan, "channel")) ? : "",
                "queue",        ast_json_string_get(ast_json_object_get(dialing->j_chan, "queue")) ? : "",
                "membername",   ast_json_string_get(ast_json_object_get(dialing->j_chan, "membername")) ? : "",
                "tm_hangup",    ast_json_string_get(ast_json_object_get(dialing->j_chan, "tm_hangup")) ? : ""
                );

        ast_json_array_append(j_res, j_tmp);
    }
    rb_dialing_iter_destroy(&iter);

    return j_res;
}

/**
 * Get count of dialings
 * @return
 */
int rb_dialing_get_count(void)
{
    int ret;

    if(g_rb_dialings == NULL) {
        return 0;
    }

    ret = ao2_container_count(g_rb_dialings);

    return ret;
}
