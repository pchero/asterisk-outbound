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

#include <stdbool.h>


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
int update_dl_list(struct ast_json* j_dlinfo)
{
    char* sql;
    int ret;
    char* tmp;

    tmp = db_get_update_str(j_dlinfo);
    if(tmp == NULL) {
        ast_log(LOG_ERROR, "Could not get update sql.\n");
        return false;
    }

    ast_asprintf(&sql, "update dl_list set %s where uuid = \"%s\";\n",
            tmp, ast_json_string_get(ast_json_object_get(j_dlinfo, "uuid"))
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

    return cur_trycnt;
}

/**
 *
 * @param uuid
 * @return
 */
struct ast_json* get_dl_master_info_all(void)
{
    char* sql;
    struct ast_json* j_res;
    struct ast_json* j_tmp;
    db_res_t* db_res;

    ast_asprintf(&sql, "%s", "select * from dl_list_ma;");

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
 *
 * @param uuid
 * @return
 */
struct ast_json* get_dl_master_info(const char* uuid)
{
    char* sql;
    struct ast_json* j_res;
    db_res_t* db_res;

    if(uuid == NULL) {
        ast_log(LOG_WARNING, "Invalid input paramters.\n");
        return NULL;
    }

    ast_asprintf(&sql, "select * from dl_list_ma where uuid = \"%s\";", uuid);

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
struct ast_json* get_dl_lists(struct ast_json* j_dlma, int count)
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

    ast_asprintf(&sql, "select * from dl_list where uuid=\"%s\"", uuid);
    db_res = db_query(sql);
    ast_free(sql);

    j_res = db_get_record(db_res);
    db_free(db_res);

    return j_res;
}

