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

static int rb_dialing_cmp_cb(void* obj, void* arg, int flags);
static int rb_dialing_sort_cb(const void* o_left, const void* o_right, int flags);
static void rb_dialing_destructor(void* obj);

static struct ao2_container* g_rb_dialings = NULL;  ///< dialing container

int init_rb_dialing(void)
{
    g_rb_dialings = ao2_container_alloc_rbtree(AO2_ALLOC_OPT_LOCK_MUTEX, AO2_CONTAINER_ALLOC_OPT_DUPS_REJECT, rb_dialing_sort_cb, rb_dialing_cmp_cb);
    if(g_rb_dialings == NULL) {
        ast_log(LOG_ERROR, "Could not create rbtree.\n");
        return false;
    }
    return true;
}

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

        return strcmp(ast_json_string_get(ast_json_object_get(dialing_left->j_dl, "uuid_dl")), key);
    }
    else {
        const rb_dialing* dialing_right;

        dialing_right = (rb_dialing*)o_right;
        return strcmp(dialing_left->uuid, dialing_right->uuid);
    }
}

static int rb_dialing_cmp_cb(void* obj, void* arg, int flags)
{
    rb_dialing* dialing;
    const char *uuid;

    dialing = (rb_dialing*)obj;

//    ast_log(LOG_DEBUG, "Find. uuid[%s], flag[%d]s\n", uuid, flags);
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
        if(strcmp(ast_json_string_get(ast_json_object_get(dialing->j_dl, "uuid_dl")), uuid) == 0) {
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

rb_dialing* rb_dialing_create(struct ast_json* j_dl)
{
    rb_dialing* dialing;
    const char* tmp_const;

    tmp_const = ast_json_string_get(ast_json_object_get(j_dl, "channelid"));
    if(tmp_const == NULL) {
        return NULL;
    }

    dialing = ao2_alloc(sizeof(rb_dialing), rb_dialing_destructor);

    dialing->uuid = ast_strdup(tmp_const);
    dialing->j_dl = ast_json_deep_copy(j_dl);

    if(ao2_link(g_rb_dialings, dialing) == 0) {
        ast_log(LOG_DEBUG, "Could not register the dialing. uuid[%s]\n", dialing->uuid);
        rb_dialing_destructor(dialing);
        return NULL;
    }
    return dialing;
}

void rb_dialing_destory(rb_dialing* dialing)
{
    ao2_ref(dialing, -1);
}

static void rb_dialing_destructor(void* obj)
{
    rb_dialing* dialing;

    dialing = (rb_dialing*)obj;

    if(dialing->uuid != NULL)   ast_free(dialing->uuid);
    if(dialing->j_dl != NULL)   ast_json_unref(dialing->j_dl);
    if(dialing->j_chan != NULL) ast_json_unref(dialing->j_chan);
    if(dialing->j_queue != NULL) ast_json_unref(dialing->j_queue);

    ast_log(LOG_DEBUG, "Called destoryer.\n");
//    ao2_ref(dialing, -1);
}

rb_dialing* rb_dialing_find_uuid_dl(const char* chan)
{
    rb_dialing* dialing;
    dialing = ao2_find(g_rb_dialings, chan, OBJ_SEARCH_PARTIAL_KEY);
    ao2_ref(dialing, -1);

    return dialing;
}

rb_dialing* rb_dialing_find_uuid_chan(const char* uuid)
{
    rb_dialing* dialing;

    ast_log(LOG_DEBUG, "rb_dialing_find_uuid. uuid[%s]\n", uuid);
    dialing = ao2_find(g_rb_dialings, uuid, OBJ_SEARCH_KEY);
    ao2_ref(dialing, -1);

    return dialing;
}

struct ao2_iterator rb_dialing_iter_init(void)
{
    ast_log(LOG_DEBUG, "rd_dialing count. count[%d]\n", ao2_container_count(g_rb_dialings));
    return ao2_iterator_init(g_rb_dialings, 0);
}
