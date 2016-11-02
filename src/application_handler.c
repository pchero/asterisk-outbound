/*
 * application_handler.c
 *
 *  Created on: Oct 31, 2016
 *      Author: pchero
 */

#include "asterisk.h"
#include "asterisk/app.h"
#include "asterisk/strings.h"
#include "asterisk/pbx.h"
#include "asterisk/module.h"

#include "res_outbound.h"
#include "dl_handler.h"
#include "utils.h"

/*** DOCUMENTATION
	<application name="OutDlCreate" language="en_US">
		<synopsis>
			Create dl.
		</synopsis>
		<syntax>
			<parameter name="dlma_uuid">
				<para>dlma uuid.</para>
			</parameter>
			<parameter name="numbers" required="true" argsep=":">
				<para>numbers</para>
				<argument name="number" required="true">
					<para>Colon-separated list of files to announce. The word
					<literal>PARKED</literal> will be replaced by a say_digits of the extension in which
					the call is parked.</para>
				</argument>
				<argument name="number1" multiple="true" />
			</parameter>
			<parameter name="name">
				<para>dl name</para>
			</parameter>
			<parameter name="detail">
				<para>dl detail</para>
			</parameter>
			<parameter name="email">
				<para>dl email</para>
			</parameter>
			<parameter name="ukey">
				<para>dl ukey</para>
			</parameter>
			<parameter name="variables" argsep=":">
				<para>variable</para>
				<argument name="number" required="true">
					<para>Colon-separated list of files to announce. The word
					<literal>PARKED</literal> will be replaced by a say_digits of the extension in which
					the call is parked.</para>
				</argument>
				<argument name="variable1" multiple="true" />
			</parameter>
		</syntax>
		<description>
			<para>Used to park yourself (typically in combination with an attended
			transfer to know the parking space).</para>
		</description>
	</application>
 ***/


#define  DEF_APPLICATION_OUTDLCREATE "OutDlCreate"


#define DEF_MAX_PARSE_SIZE	20
#define DEF_DELIMETER_COLON ':'

static int outdlcreate_exec(struct ast_channel *chan, const char *data);

static char** parsing_msg_delimiter(const char* msg, const char deli);
static void destroy_parsing(char** parse);
static char* create_variables_str(const char* msg);

/**
 * Application handler
 * OutDlCreate()
 * \param chan
 * \param data
 * \return
 */
static int outdlcreate_exec(struct ast_channel *chan, const char *data)
{
	char *data_copy;
	struct ast_json* j_dl;
	char** parse_numbers;
	char* dl_uuid;
	char* variables;

	AST_DECLARE_APP_ARGS(args,
		AST_APP_ARG(dlma_uuid);
		AST_APP_ARG(numbers);
		AST_APP_ARG(name);
		AST_APP_ARG(detail);
		AST_APP_ARG(email);
		AST_APP_ARG(ukey);
		AST_APP_ARG(variables);
	);

	if (ast_strlen_zero(data) == 1) {
		ast_log(LOG_WARNING, "OUTDLCREATE requires an argument.\n");
		return -1;
	}

	data_copy = ast_strdupa(data);
	AST_STANDARD_APP_ARGS(args, data_copy);

	ast_log(LOG_VERBOSE, "Application outdlcreate. dlma_uuid[%s], numbers[%s], name[%s], detail[%s], email[%s], ukey[%s], variables[%s]\n",
			args.dlma_uuid? :"",
			args.numbers? :"",
			args.name? :"",
			args.detail? :"",
			args.email? :"",
			args.ukey? :"",
			args.variables? :""
			);

	// validate args
	if(ast_strlen_zero(args.dlma_uuid) == 1) {
		ast_log(LOG_WARNING, "OUTDLCREATE requires a dlma_uuid.\n");
		return -1;
	}
	else if(ast_strlen_zero(args.numbers) == 1) {
		ast_log(LOG_WARNING, "OUTDLCREATE requires a numbers.\n");
		return -1;
	}

	// parse numbers
	parse_numbers = parsing_msg_delimiter(args.numbers, ':');
	if(parse_numbers == NULL) {
		ast_log(LOG_WARNING, "OUTDLCREATE requires at lest one number.\n");
	}

	// variables
	variables = create_variables_str(args.variables);

	// create request json
	j_dl = ast_json_pack("{"
			" s:s,"
			" s:s, s:s, s:s, s:s, s:s, s:s, s:s, s:s,"
			" s:s, s:s, s:s, s:s,"
			" s:s"
			"}",
			"dlma_uuid", 	args.dlma_uuid,

			"number_1", 	parse_numbers[0]? : "",
			"number_2", 	parse_numbers[1]? : "",
			"number_3", 	parse_numbers[2]? : "",
			"number_4", 	parse_numbers[3]? : "",
			"number_5", 	parse_numbers[4]? : "",
			"number_6", 	parse_numbers[5]? : "",
			"number_7", 	parse_numbers[6]? : "",
			"number_8", 	parse_numbers[7]? : "",

			"name",			args.name? : "",
			"detail",		args.detail? : "",
			"email",		args.email? : "",
			"ukey",			args.ukey? : "",

			"variables",	variables? : ""
			);
	destroy_parsing(parse_numbers);
	ast_free(variables);

	dl_uuid = create_dl_list(j_dl);
	AST_JSON_UNREF(j_dl);
	if(dl_uuid == NULL) {
		pbx_builtin_setvar_helper(chan, "OUTSTATUS", "FAILED");
		pbx_builtin_setvar_helper(chan, "OUTDETAIL", "");
	}
	else {
		pbx_builtin_setvar_helper(chan, "OUTSTATUS", "SUCCESS");
		pbx_builtin_setvar_helper(chan, "OUTDETAIL", dl_uuid);
	}

	ast_free(dl_uuid);
	return 0;
}

/**
 * Create parsed info.
 * \param msg
 * \param deli
 * \return
 */
static char** parsing_msg_delimiter(const char* msg, const char deli)
{
	char** res;
	char* org;
	const char* tmp_const;
	int i;

	if(msg == NULL) {
		ast_log(LOG_WARNING, "Wrong input parameter.\n");
	}

	org = ast_strdupa(msg);

	res = ast_std_calloc(DEF_MAX_PARSE_SIZE, sizeof(char*));

	for(i = 0; i < DEF_MAX_PARSE_SIZE; i++) {
		tmp_const = ast_strsep(&org, deli, AST_STRSEP_ALL);
		ast_log(LOG_VERBOSE, "Check value. tmp_const[%s]\n", tmp_const);
		if(tmp_const == NULL) {
			break;
		}

		res[i] = strdup(tmp_const);
	}

	return res;
}

/**
 * Destroy parsed info
 * \param parse
 */
static void destroy_parsing(char** parse)
{
	int i;

	if(parse == NULL) {
		return;
	}

	// free every each items
	for(i = 0; i < DEF_MAX_PARSE_SIZE; i++) {
		if(parse[i] == NULL) {
			break;
		}
		ast_free(parse[i]);
	}

	ast_free(parse);
	return;
}


/**
 * create string for variables.
 * input: "var1=val1 : var2 = var 2 : var 3 = va r 3"
 * output: "{\"var1\":"val1\", \"var2\":\"var 2\", \"var 3\":\"va r 3\"}"
 * \param msg
 * \return
 */
static char* create_variables_str(const char* msg)
{
	char* res;
	struct ast_json* j_tmp;
	char* tmp;
	char* org;
	int i;
	char* variable;
	char* var;
	char* val;

	if(msg == NULL) {
		return NULL;
	}

	org = ast_strdupa(msg);

	j_tmp = ast_json_object_create();
	for(i = 0; i < DEF_MAX_PARSE_SIZE; i++) {
		variable = ast_strsep(&org, DEF_DELIMETER_COLON, AST_STRSEP_ALL);
		if(variable == NULL) {
			break;
		}

		var = ast_strsep(&variable, '=', AST_STRSEP_ALL);
		val = ast_strsep(&variable, '=', AST_STRSEP_ALL);
		if((var == NULL) || (val == NULL)) {
			continue;
		}
		ast_json_object_set(j_tmp, var, ast_json_string_create(val));
	}

	tmp = ast_json_dump_string(j_tmp);
	AST_JSON_UNREF(j_tmp);
	if(tmp == NULL) {
		return NULL;
	}

	res = ast_strdup(tmp);
	ast_json_free(tmp);

	return res;
}

bool init_application_handler(void)
{
	int ret;

	ast_log(LOG_VERBOSE, "init_application_handler.\n");

	ret = ast_register_application2(DEF_APPLICATION_OUTDLCREATE, outdlcreate_exec, NULL, NULL, NULL);

	if(ret != 0) {
		return false;
	}

	return true;
}

bool term_application_handler(void)
{
	ast_log(LOG_VERBOSE, "term_application_handler.\n");

	ast_unregister_application(DEF_APPLICATION_OUTDLCREATE);

	return true;
}
