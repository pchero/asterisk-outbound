/*
 * utils.c
 *
 *  Created on: Oct 19, 2016
 *      Author: pchero
 */


#include "asterisk.h"

#include "asterisk/uuid.h"
#include "asterisk/utils.h"

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
