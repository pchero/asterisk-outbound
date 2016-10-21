/*
 * utils.c
 *
 *  Created on: Oct 19, 2016
 *      Author: pchero
 */


#include "asterisk.h"

#include "asterisk/uuid.h"
#include "asterisk/utils.h"

#include "utils.h"

/**
 * Generate uuid.
 * Return value should be free after used.
 * @param prefix
 * @return
 */
char* gen_uuid(void)
{
	char tmp[AST_UUID_STR_LEN];
	char* res;

	ast_uuid_generate_str(tmp, sizeof(tmp));
	res = ast_strdup(tmp);

	return res;
}

/**
 * return utc time.
 * YYYY-MM-DDTHH:mm:ssZ
 * @return
 */
char* get_utc_timestamp(void)
{
	char	timestr[128];
	char*   res;
	struct  timespec timeptr;
	time_t  tt;
	struct tm *t;

	clock_gettime(CLOCK_REALTIME, &timeptr);
	tt = (time_t)timeptr.tv_sec;
	t = gmtime(&tt);

	strftime(timestr, sizeof(timestr), "%Y-%m-%dT%H:%M:%S", t);
	ast_asprintf(&res, "%s.%ldZ", timestr, timeptr.tv_nsec);

	return res;
}

/**
 * return utc time.
 * YYYY-MM-DDTHH:mm:ssZ
 * @return
 */
char* get_utc_timestamp_using_timespec(struct timespec timeptr)
{
	char	timestr[128];
	char*   res;
	time_t  tt;
	struct tm *t;

	tt = (time_t)timeptr.tv_sec;
	t = gmtime(&tt);

	strftime(timestr, sizeof(timestr), "%Y-%m-%dT%H:%M:%S", t);
	ast_asprintf(&res, "%s.%ldZ", timestr, timeptr.tv_nsec);

	return res;
}
