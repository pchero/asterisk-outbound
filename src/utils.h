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

#endif /* SRC_UTILS_H_ */
