/*
 * utils.h
 *
 *  Created on: Oct 19, 2016
 *      Author: pchero
 */

#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#define AST_JSON_UNREF(a)	{ast_json_unref(a); a = NULL;}

char* gen_uuid(void);
char* get_utc_timestamp(void);
char* get_utc_timestamp_using_timespec(struct timespec timeptr);
char* get_variables_info_ami_str(struct ast_json* j_obj, const char* name);
struct ast_json* get_variables_info_json_object(struct ast_json* j_obj, const char* name);
char* get_variables_info_ami_str_from_json_array(struct ast_json* j_arr);
char* get_variables_info_ami_str_from_string(const char* str);

#endif /* SRC_UTILS_H_ */
