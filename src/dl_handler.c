/*
 * dl_handler.c
 *
 *  Created on: Nov 29, 2015
 *      Author: pchero
 */

#include "asterisk.h"
#include "asterisk/causes.h"
#include "asterisk/logger.h"
#include "asterisk/utils.h"

#include "db_handler.h"
#include "dl_handler.h"
#include "campaign_handler.h"
#include "cli_handler.h"
#include "event_handler.h"

#include <stdbool.h>

static char* create_chan_addr_for_dial(struct ast_json* j_plan, struct ast_json* j_dl_list, int dial_num_point);
static char* get_dial_number(struct ast_json* j_dlist, const int cnt);
static char* create_view_name(const char* uuid);
static bool create_dlma_view(const char* uuid, const char* view_name);


/**
 * Get dl_list from database.
 * @param j_dlma
 * @param j_plan
 * @return
 */
struct ast_json* get_dl_available_predictive(struct ast_json* j_dlma, struct ast_json* j_plan)
{
    char* sql;
    db_res_t* db_res;
    struct ast_json* j_res;

    ast_asprintf(&sql, "select "
            "*, "
            "trycnt_1 + trycnt_2 + trycnt_3 + trycnt_4 + trycnt_5 + trycnt_6 + trycnt_7 + trycnt_8 as trycnt, "
            "case when number_1 is null then 0 when trycnt_1 < %d then 1 else 0 end as num_1, "
            "case when number_2 is null then 0 when trycnt_2 < %d then 1 else 0 end as num_2, "
            "case when number_3 is null then 0 when trycnt_3 < %d then 1 else 0 end as num_3, "
            "case when number_4 is null then 0 when trycnt_4 < %d then 1 else 0 end as num_4, "
            "case when number_5 is null then 0 when trycnt_5 < %d then 1 else 0 end as num_5, "
            "case when number_6 is null then 0 when trycnt_6 < %d then 1 else 0 end as num_6, "
            "case when number_7 is null then 0 when trycnt_7 < %d then 1 else 0 end as num_7, "
            "case when number_8 is null then 0 when trycnt_8 < %d then 1 else 0 end as num_8 "
            "from %s "
            "having "
            "status = %d "
            "and num_1 + num_2 + num_3 + num_4 + num_5 + num_6 + num_7 + num_8 > 0 "
            "and res_hangup != %d "
            "order by trycnt asc "
            "limit 1"
            ";",
            (int)ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_1")),
            (int)ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_2")),
            (int)ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_3")),
            (int)ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_4")),
            (int)ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_5")),
            (int)ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_6")),
            (int)ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_7")),
            (int)ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_8")),
            ast_json_string_get(ast_json_object_get(j_dlma, "dl_table")),
            E_DL_IDLE,
            AST_CAUSE_NORMAL_CLEARING
            );

    db_res = db_query(sql);
    ast_free(sql);
    if(db_res == NULL) {
        ast_log(LOG_ERROR, "Could not get dial list info.");
        return NULL;
    }

    j_res = db_get_record(db_res);
    db_free(db_res);
    if(j_res == NULL) {
        return NULL;
    }

    return j_res;
}

int check_more_dl_list(struct ast_json* j_dlma, struct ast_json* j_plan)
{
    struct ast_json* j_res;
    db_res_t* db_res;
    char* sql;

    ast_asprintf(&sql, "select *, "
            "case when number_1 is null then 0 when trycnt_1 < %ld then 1 else 0 end as num_1, "
            "case when number_2 is null then 0 when trycnt_2 < %ld then 1 else 0 end as num_2, "
            "case when number_3 is null then 0 when trycnt_3 < %ld then 1 else 0 end as num_3, "
            "case when number_4 is null then 0 when trycnt_4 < %ld then 1 else 0 end as num_4, "
            "case when number_5 is null then 0 when trycnt_5 < %ld then 1 else 0 end as num_5, "
            "case when number_6 is null then 0 when trycnt_6 < %ld then 1 else 0 end as num_6, "
            "case when number_7 is null then 0 when trycnt_7 < %ld then 1 else 0 end as num_7, "
            "case when number_8 is null then 0 when trycnt_8 < %ld then 1 else 0 end as num_8 "
            "from %s "
            "having "
            "res_hangup != %d "
            "and num_1 + num_2 + num_3 + num_4 + num_5 + num_6 + num_7 + num_8 > 0 "
            ";",

            ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_1")),
            ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_2")),
            ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_3")),
            ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_4")),
            ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_5")),
            ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_6")),
            ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_7")),
            ast_json_integer_get(ast_json_object_get(j_plan, "max_retry_cnt_8")),
            ast_json_string_get(ast_json_object_get(j_dlma, "dl_table")),
            AST_CAUSE_NORMAL_CLEARING
            );
//    ast_log(LOG_DEBUG, "Check sql. sql[%s]\n", sql);
    db_res = db_query(sql);
    ast_free(sql);
    if(db_res == NULL) {
        return false;
    }

    j_res = db_get_record(db_res);
    db_free(db_res);
//    ast_log(LOG_DEBUG, "Get dial records. uuid[%s]\n", ast_json_string_get(ast_json_object_get(j_res, "uuid")));
    if(j_res == NULL) {
        return false;
    }
    ast_json_unref(j_res);

    return true;
}

/**
 *
 * @param uuid
 */
void clear_dl_list_dialing(const char* uuid)
{
    struct ast_json* j_tmp;

    if(uuid == NULL) {
        return;
    }

    j_tmp = ast_json_pack("{s:i, s:s, s:O, s:O, s:O}",
            "status",               E_DL_IDLE,
            "uuid",                 uuid,
            "dialing_uuid",         ast_json_null(),
            "dialing_camp_uuid",    ast_json_null(),
            "dialing_plan_uuid",    ast_json_null()
            );
    update_dl_list(j_tmp);
    ast_json_unref(j_tmp);

    return;
}

/**
 * Update dl list info.
 * @param j_dlinfo
 * @return
 */
bool update_dl_list(struct ast_json* j_dl)
{
    char* sql;
    int ret;
    char* tmp;

    tmp = db_get_update_str(j_dl);
    if(tmp == NULL) {
        ast_log(LOG_ERROR, "Could not get update sql.\n");
        return false;
    }

    ast_asprintf(&sql, "update dl_list set %s where uuid = \"%s\";\n",
            tmp, ast_json_string_get(ast_json_object_get(j_dl, "uuid"))
            );
    ast_free(tmp);

    ret = db_exec(sql);
    ast_free(sql);
    if(ret == false) {
        ast_log(LOG_ERROR, "Could not update dl_list info.");
        return false;
    }

    return true;
}

/**
 * Return dialing count currently.
 * @param camp_uuid
 * @param dl_table
 * @return
 */
int get_current_dialing_dl_cnt(const char* camp_uuid, const char* dl_table)
{
    char* sql;
    db_res_t* db_res;
    struct ast_json* j_tmp;
    int ret;

    if((camp_uuid == NULL) || (dl_table == NULL)) {
        ast_log(LOG_ERROR, "Invalid input parameters.\n");
        return -1;
    }

    ast_asprintf(&sql, "select count(*) from %s where dialing_camp_uuid = \"%s\" and status = \"%s\";",
            dl_table, camp_uuid, "dialing"
            );

    db_res = db_query(sql);
    ast_free(sql);
    if(db_res == NULL) {
        ast_log(LOG_ERROR, "Could not get dialing count.\n");
        return 0;
    }

    j_tmp = db_get_record(db_res);
    db_free(db_res);
    if(j_tmp == NULL) {
        // shouldn't be reach to here.
        ast_log(LOG_ERROR, "Could not get dialing count.");
        return false;
    }

    ret = ast_json_integer_get(ast_json_object_get(j_tmp, "count(*)"));
    ast_json_unref(j_tmp);

    return ret;
}

/**
 * Get dial number index
 * @param j_dl_list
 * @param j_plan
 * @return
 */
int get_dial_num_point(struct ast_json* j_dl_list, struct ast_json* j_plan)
{
    int i;
    int dial_num_point;
    int cur_trycnt;
    int max_trycnt;
    char* tmp;
    const char* tmp_const;

    // get dial number
    dial_num_point = -1;
    for(i = 1; i < 9; i++) {
        ast_asprintf(&tmp, "number_%d", i);
        tmp_const = ast_json_string_get(ast_json_object_get(j_dl_list, tmp));
        ast_free(tmp);
        if(tmp_const == NULL) {
            // No number set.
            continue;
        }

        ast_asprintf(&tmp, "trycnt_%d", i);
        cur_trycnt = ast_json_integer_get(ast_json_object_get(j_dl_list, tmp));
        ast_free(tmp);

        ast_asprintf(&tmp, "max_retry_cnt_%d", i);
        max_trycnt = ast_json_integer_get(ast_json_object_get(j_plan, tmp));
        ast_free(tmp);

        if(cur_trycnt < max_trycnt) {
            dial_num_point = i;
            break;
        }
    }

    return dial_num_point;
}

/**
 * Get dial try count for this time.
 * @param j_dl_list
 * @param dial_num_point
 * @return
 */
int get_dial_try_cnt(struct ast_json* j_dl_list, int dial_num_point)
{
    int cur_trycnt;
    char* tmp;

    ast_asprintf(&tmp, "trycnt_%d", dial_num_point);
    if(ast_json_object_get(j_dl_list, tmp) == NULL) {
        return -1;
    }

    cur_trycnt = ast_json_integer_get(ast_json_object_get(j_dl_list, tmp));
    ast_free(tmp);
    cur_trycnt++;   // for this time.

    return cur_trycnt;
}

/**
 *
 * @param uuid
 * @return
 */
struct ast_json* get_dlmas_all(void)
{
    char* sql;
    struct ast_json* j_res;
    struct ast_json* j_tmp;
    db_res_t* db_res;

    ast_asprintf(&sql, "%s", "select * from dl_list_ma where in_use=1;");

    db_res = db_query(sql);
    ast_free(sql);
    if(db_res == NULL) {
        ast_log(LOG_ERROR, "Could not get dl_list_ma info.\n");
        return NULL;
    }

    j_res = ast_json_array_create();
    while(1) {
        j_tmp = db_get_record(db_res);
        if(j_tmp == NULL) {
            break;
        }
        ast_json_array_append(j_res, j_tmp);
    }
    db_free(db_res);

    return j_res;
}

/**
 * Create dl_list_ma.
 * @param j_dlma
 * @return
 */
bool create_dlma(const struct ast_json* j_dlma)
{
    int ret;
    char* uuid;
    char* view_name;
    char* tmp;
    struct ast_json* j_tmp;

    if(j_dlma == NULL) {
        return false;
    }

    j_tmp = ast_json_deep_copy(j_dlma);

    // uuid
    uuid = gen_uuid();
    ast_json_object_set(j_tmp, "uuid", ast_json_string_create(uuid));

    // create view_table
    view_name = create_view_name(uuid);
    create_dlma_view(uuid, view_name);
    ast_json_object_set(j_tmp, "dl_table", ast_json_string_create(view_name));
    ast_free(view_name);

    // create timestamp
    tmp = get_utc_timestamp();
    ast_json_object_set(j_tmp, "tm_create", ast_json_string_create(tmp));
    ast_free(tmp);

    ast_log(LOG_NOTICE, "Create dlma. uuid[%s], name[%s]\n",
            ast_json_string_get(ast_json_object_get(j_tmp, "uuid")),
            ast_json_string_get(ast_json_object_get(j_tmp, "name"))
            );

    ret = db_insert("dl_list_ma", j_tmp);
    ast_json_unref(j_tmp);
    if(ret == false) {
        ast_free(uuid);
        return false;
    }

    // send ami event
    j_tmp = get_dlma(uuid);
    ast_free(uuid);
    send_manager_evt_out_dlma_create(j_tmp);
    ast_json_unref(j_tmp);

    return true;
}

/**
 * Update dl_list_ma.
 * @param j_dlma
 * @return
 */
bool update_dlma(const struct ast_json* j_dlma)
{
    char* tmp;
    const char* tmp_const;
    char* sql;
    struct ast_json* j_tmp;
    char* uuid;
    int ret;

    j_tmp = ast_json_deep_copy(j_dlma);

    tmp_const = ast_json_string_get(ast_json_object_get(j_tmp, "uuid"));
    if(tmp_const == NULL) {
        ast_log(LOG_WARNING, "Could not get uuid.\n");
        ast_json_unref(j_tmp);
        return false;
    }
    uuid = ast_strdup(tmp_const);

    // update timestamp
    tmp = get_utc_timestamp();
    ast_json_object_set(j_tmp, "tm_update", ast_json_string_create(tmp));
    ast_free(tmp);

    tmp = db_get_update_str(j_tmp);
    ast_json_unref(j_tmp);
    if(tmp == NULL) {
        ast_log(LOG_WARNING, "Could not get update str.\n");
        ast_free(uuid);
        return false;
    }

    ast_asprintf(&sql, "update dl_list_ma set %s where uuid=\"%s\";", tmp, uuid);
    ast_free(tmp);

    ret = db_exec(sql);
    ast_free(sql);
    if(ret == false) {
        ast_log(LOG_WARNING, "Could not get updated dlma. uuid[%s]\n", uuid);
        ast_free(uuid);
        return false;
    }

    j_tmp = get_dlma(uuid);
    ast_free(uuid);
    if(j_tmp == NULL) {
        ast_log(LOG_WARNING, "Could not get updated dlma info\n");
        return false;
    }
    send_manager_evt_out_dlma_update(j_tmp);
    ast_json_unref(j_tmp);

    return true;
}

bool delete_dlma(const char* uuid)
{
    struct ast_json* j_tmp;
    char* tmp;
    char* sql;
    int ret;

    if(uuid == NULL) {
        // invalid parameter.
        return false;
    }

    j_tmp = ast_json_object_create();
    tmp = get_utc_timestamp();
    ast_json_object_set(j_tmp, "tm_delete", ast_json_string_create(tmp));
    ast_json_object_set(j_tmp, "in_use", ast_json_integer_create(0));
    ast_free(tmp);

    tmp = db_get_update_str(j_tmp);
    ast_json_unref(j_tmp);
    ast_asprintf(&sql, "update dl_list_ma set %s where uuid=\"%s\";", tmp, uuid);
    ast_free(tmp);

    ret = db_exec(sql);
    ast_free(sql);
    if(ret == false) {
        ast_log(LOG_WARNING, "Could not delete dlma. uuid[%s]\n", uuid);
        return false;
    }

    // send notification
    send_manager_evt_out_dlma_delete(uuid);

    return true;
}


/**
 *
 * @param uuid
 * @return
 */
struct ast_json* get_dlma(const char* uuid)
{
    char* sql;
    struct ast_json* j_res;
    db_res_t* db_res;

    if(uuid == NULL) {
        ast_log(LOG_WARNING, "Invalid input paramters.\n");
        return NULL;
    }

    ast_asprintf(&sql, "select * from dl_list_ma where uuid=\"%s\" and in_use=1;", uuid);

    db_res = db_query(sql);
    ast_free(sql);
    if(db_res == NULL) {
        ast_log(LOG_ERROR, "Could not get dl_list_ma info. uuid[%s]\n", uuid);
        return NULL;
    }

    j_res = db_get_record(db_res);
    db_free(db_res);

    return j_res;
}

/**
 * Get dl_list from database.
 * @param j_dlma
 * @param j_plan
 * @return
 */
struct ast_json* get_dl_lists(const char* dlma_uuid, int count)
{
    char* sql;
    db_res_t* db_res;
    struct ast_json* j_res;
    struct ast_json* j_tmp;
    struct ast_json* j_dlma;

    if((dlma_uuid == NULL) || (count <= 0)) {
        return NULL;
    }

    j_dlma = get_dlma(dlma_uuid);
    if(j_dlma == NULL) {
        return NULL;
    }

    ast_asprintf(&sql, "select * from %s where in_use=1 limit %d;",
            ast_json_string_get(ast_json_object_get(j_dlma, "dl_table")),
            count
            );

    db_res = db_query(sql);
    ast_free(sql);
    if(db_res == NULL) {
        ast_log(LOG_ERROR, "Could not get dial list info.");
        return NULL;
    }

    j_res = ast_json_array_create();
    while(1) {
        j_tmp = db_get_record(db_res);
        if(j_tmp == NULL) {
            break;
        }

        ast_json_array_append(j_res, j_tmp);
    }
    db_free(db_res);

    return j_res;
}

/**
 * Get dl_list from database.
 * @param j_dlma
 * @param j_plan
 * @return
 */
struct ast_json* get_dl_lists_(struct ast_json* j_dlma, int count)
{
    char* sql;
    db_res_t* db_res;
    struct ast_json* j_res;
    struct ast_json* j_tmp;

    if((j_dlma == NULL) || (count <= 0)) {
        return NULL;
    }

    ast_asprintf(&sql, "select * from %s limit %d;",
            ast_json_string_get(ast_json_object_get(j_dlma, "dl_table")),
            count
            );

    db_res = db_query(sql);
    ast_free(sql);
    if(db_res == NULL) {
        ast_log(LOG_ERROR, "Could not get dial list info.");
        return NULL;
    }

    j_res = ast_json_array_create();
    while(1) {
        j_tmp = db_get_record(db_res);
        if(j_tmp == NULL) {
            break;
        }

        ast_json_array_append(j_res, j_tmp);
    }
    db_free(db_res);

    return j_res;
}

struct ast_json* get_dl_list(const char* uuid)
{
    char* sql;
    db_res_t* db_res;
    struct ast_json* j_res;

    if(uuid == NULL) {
        return NULL;
    }

    ast_asprintf(&sql, "select * from dl_list where in_use=1 and uuid=\"%s\"", uuid);
    db_res = db_query(sql);
    ast_free(sql);

    j_res = db_get_record(db_res);
    db_free(db_res);

    return j_res;
}

/**
 * Create dialing json object
 * @param j_camp
 * @param j_plan
 * @param j_dlma
 * @param j_dl_list
 * @return
 */
struct ast_json* create_dial_info(
        struct ast_json* j_plan,
        struct ast_json* j_dl_list,
        E_DIAL_TYPE dial_type,
        const char* data_1,     ///< extension | application name
        const char* data_2      ///< context | data
        )
{
    struct ast_json* j_dial;
    char* dial_addr;
    char* dial_channel;
    int dial_num_point;
    char tmp_timeout[10];
    char* channel_id;
    char* other_channel_id;
    int dial_count;

    // get dial number point
    dial_num_point = get_dial_num_point(j_dl_list, j_plan);
    if(dial_num_point < 0) {
        ast_log(LOG_ERROR, "Could not find correct number count.\n");
        return NULL;
    }

    // get dial count
    dial_count = get_dial_try_cnt(j_dl_list, dial_num_point);
    if(dial_count == -1) {
        ast_log(LOG_ERROR, "Could not get correct dial count number.\n");
        return NULL;
    }

    dial_addr = get_dial_number(j_dl_list, dial_num_point);
    if(dial_addr == NULL) {
        ast_log(LOG_ERROR, "Could not get correct dial address.\n");
        return NULL;
    }

    // create destination channel address.
    dial_channel = create_chan_addr_for_dial(j_plan, j_dl_list, dial_num_point);
    if(dial_channel == NULL) {
        ast_log(LOG_ERROR, "Could not get correct channel address.\n");
        ast_free(dial_addr);
        return NULL;
    }

    sprintf(tmp_timeout, "%ld", ast_json_integer_get(ast_json_object_get(j_plan, "dial_timeout")));
    channel_id = gen_uuid();
    other_channel_id = gen_uuid();

    j_dial = ast_json_pack("{"
            "s:s, "
            "s:i, s:s, s:s, s:s, s:s, s:s, "
            "s:i, s:i, s:i"
            "}",
            // identity info
            "uuid",      ast_json_string_get(ast_json_object_get(j_dl_list, "uuid")),

            // channel set
            "dial_type",        dial_type,
            "dial_channel",     dial_channel,   ///< dialing channel
            "dial_addr",        dial_addr,      ///< dialing address(number)
            "timeout",          tmp_timeout,
            "channelid",        channel_id,                                                     ///< Channel unique ID
            "otherchannelid",   other_channel_id,                                               ///< Other channel unique id.

            // other info
            "dial_index",       dial_num_point,
            "dial_trycnt",      dial_count,
            "dial_type",        dial_type
            );
    if(dial_type == E_DIAL_EXTEN) {
        ast_json_object_set(j_dial, "dial_exten", ast_json_string_create(data_1));
        ast_json_object_set(j_dial, "dial_context", ast_json_string_create(data_2));
    }
    else if(dial_type == E_DIAL_APPL) {
        ast_json_object_set(j_dial, "dial_application", ast_json_string_create(data_1));
        ast_json_object_set(j_dial, "dial_data", ast_json_string_create(data_2));
    }
    // caller id
    if(ast_json_string_get(ast_json_object_get(j_plan, "caller_id")) != NULL) {
        ast_json_object_set(j_dial, "callerid", ast_json_ref(ast_json_object_get(j_plan, "caller_id")));
    }

    ast_free(dial_channel);
    ast_free(dial_addr);
    ast_free(channel_id);
    ast_free(other_channel_id);

    return j_dial;
}

/**
 * Create dl_list's dial address.
 * @param j_camp
 * @param j_plan
 * @param j_dl_list
 * @return
 */
static char* create_chan_addr_for_dial(struct ast_json* j_plan, struct ast_json* j_dl_list, int dial_num_point)
{
    char* dest_addr;
    char* chan_addr;

    if(dial_num_point < 0) {
        ast_log(LOG_WARNING, "Wrong dial number point.\n");
        return NULL;
    }

    if(ast_json_string_get(ast_json_object_get(j_plan, "trunk_name")) == NULL) {
        ast_log(LOG_WARNING, "Could not get trunk_name info.\n");
        return NULL;
    }

    // get dial number
    dest_addr = get_dial_number(j_dl_list, dial_num_point);
    if(dest_addr == NULL) {
        ast_log(LOG_WARNING, "Could not get destination address.\n");
        return NULL;
    }

    // create dial addr
    ast_asprintf(&chan_addr, "SIP/%s@%s", dest_addr, ast_json_string_get(ast_json_object_get(j_plan, "trunk_name")));
    ast_log(LOG_DEBUG, "Created dialing channel address. chan_addr[%s].\n", chan_addr);
    ast_free(dest_addr);

    return chan_addr;
}

/**
 * Return dial number of j_dlist.
 * @param j_dlist
 * @param cnt
 * @return
 */
static char* get_dial_number(struct ast_json* j_dlist, const int cnt)
{
    char* res;
    char* tmp;

    ast_asprintf(&tmp, "number_%d", cnt);

    ast_asprintf(&res, "%s", ast_json_string_get(ast_json_object_get(j_dlist, tmp)));
    ast_free(tmp);

    return res;
}

/**
 * Create dl_result
 * @param dialing
 * @return
 */
struct ast_json* create_json_for_dl_result(rb_dialing* dialing)
{
    struct ast_json* j_res;
    char* tmp;

    j_res = ast_json_deep_copy(dialing->j_dialing);

    ast_log(LOG_DEBUG, "Check value. dialing_uuid[%s], camp_uuid[%s], plan_uuid[%s], dlma_uuid[%s], dl_list_uuid[%s]\n",
            ast_json_string_get(ast_json_object_get(j_res, "dialing_uuid")),
            ast_json_string_get(ast_json_object_get(j_res, "camp_uuid")),
            ast_json_string_get(ast_json_object_get(j_res, "plan_uuid")),
            ast_json_string_get(ast_json_object_get(j_res, "dlma_uuid")),
            ast_json_string_get(ast_json_object_get(j_res, "dl_list_uuid"))
            );

    // info_chan
    tmp = ast_json_dump_string_format(dialing->j_chan, 0);
    ast_json_object_set(j_res, "info_chan", ast_json_string_create(tmp));
    ast_json_free(tmp);

    // info_queues
    tmp = ast_json_dump_string_format(dialing->j_queues, 0);
    ast_json_object_set(j_res, "info_queues", ast_json_string_create(tmp));
    ast_json_free(tmp);

    // info_agents
    tmp = ast_json_dump_string_format(dialing->j_agents, 0);
    ast_json_object_set(j_res, "info_agents", ast_json_string_create(tmp));
    ast_json_free(tmp);

    // delete current_*
    ast_json_object_del(j_res, "current_queue");
    ast_json_object_del(j_res, "current_agent");


    return j_res;
}

/**
 * Create view name using uuid.
 * @param uuid
 * @return
 */
static char* create_view_name(const char* uuid)
{
    char* tmp;
    int len;
    int i;
    int j;

    if(uuid == NULL) {
        return NULL;
    }

    len = strlen(uuid);
    tmp = ast_calloc(len + 1, sizeof(char));
    j = 0;
    for(i = 0; i < len; i++) {
        if(uuid[i] == '-') {
            tmp[j] = '_';
        }
        else {
            tmp[j] = uuid[i];
        }
        j++;
    }
    tmp[j] = '\0';
    return tmp;
}

/**
 * Create dl_list
 * @param j_dl
 * @return
 */
bool create_dl_list(struct ast_json* j_dl)
{
    int ret;
    char* uuid;
    char* tmp;
    struct ast_json* j_tmp;

    if(j_dl == NULL) {
        return false;
    }

    j_tmp = ast_json_deep_copy(j_dl);

    // uuid
    uuid = gen_uuid();
    ast_json_object_set(j_tmp, "uuid", ast_json_string_create(uuid));
    ast_free(uuid);

    // create timestamp
    tmp = get_utc_timestamp();
    ast_json_object_set(j_tmp, "tm_create", ast_json_string_create(tmp));
    ast_free(tmp);

    ast_log(LOG_NOTICE, "Create dl_list. dl_uuid[%s], dlma_uuid[%s], name[%s]\n",
            ast_json_string_get(ast_json_object_get(j_tmp, "uuid")),
            ast_json_string_get(ast_json_object_get(j_tmp, "dlma_uuid")),
            ast_json_string_get(ast_json_object_get(j_tmp, "name"))
            );

    ret = db_insert("dl_list", j_tmp);
    ast_json_unref(j_tmp);
    if(ret == false) {
        ast_free(uuid);
        return false;
    }

    // do not send any create event for dl_list.
    return true;
}

/**
 * delete dl_list
 * @param uuid
 * @return
 */
bool delete_dl_list(const char* uuid)
{
    struct ast_json* j_tmp;
    char* tmp;
    char* sql;
    int ret;

    if(uuid == NULL) {
        // invalid parameter.
        return false;
    }

    j_tmp = ast_json_object_create();
    tmp = get_utc_timestamp();
    ast_json_object_set(j_tmp, "tm_delete", ast_json_string_create(tmp));
    ast_json_object_set(j_tmp, "in_use", ast_json_integer_create(0));
    ast_free(tmp);

    tmp = db_get_update_str(j_tmp);
    ast_json_unref(j_tmp);
    ast_asprintf(&sql, "update dl_list set %s where uuid=\"%s\";", tmp, uuid);
    ast_free(tmp);

    ret = db_exec(sql);
    ast_free(sql);
    if(ret == false) {
        ast_log(LOG_WARNING, "Could not delete dl_list. uuid[%s]\n", uuid);
        return false;
    }

    // No notifications for dl list.

    return true;
}

static bool create_dlma_view(const char* uuid, const char* view_name)
{
    char* sql;
    int ret;

    if((uuid == NULL) || (view_name == NULL)) {
        return false;
    }

    ast_asprintf(&sql, "create view %s as select * from dl_list where dlma_uuid=\"%s\";", view_name, uuid);

    ret = db_exec(sql);
    ast_free(sql);
    if(ret == false) {
        ast_log(LOG_WARNING, "Could not create view. uuid[%s], view_name[%s]\n", uuid, view_name);
        return false;
    }

    return true;
}
