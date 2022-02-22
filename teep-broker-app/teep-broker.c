/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (C) 2019 National Institute of Advanced Industrial Science
 *                           and Technology (AIST)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <libteep.h>
#include <libwebsockets.h>
#include <tee_client_api.h>
#include "http.h"
#include "ta-interface.h"
#include <stdio.h>
#include <stdlib.h>


static const TEEC_UUID uuid_aist_otrp_ta =
        { 0x68373894, 0x5bb3, 0x403c,
                { 0x9e, 0xec, 0x31, 0x14, 0xa1, 0xf5, 0xd3, 0xfc } };

static uint8_t http_res_buf[6 * 1024 * 1024];

const char *uri = "http://127.0.0.1:3000/api/tam"; // TAM server uri
const char *talist = ""; // installed TA list
bool cose = false;

struct broker_ctx {
	TEEC_Context		tee_context;
	TEEC_Session		tee_session;

};

static int broker_ctx_init(struct broker_ctx *ctx)
{
	TEEC_Result r;

	r = TEEC_InitializeContext(NULL, &ctx->tee_context);
	if (r != TEEC_SUCCESS) {
		fprintf(stderr, "%s: tee_context init failed 0x%x\n",
			__func__, r);
		goto bail1;
	}

	TEEC_Operation op;
	memset(&op, 0, sizeof op);
	r = TEEC_OpenSession(&ctx->tee_context, &ctx->tee_session,
			     &uuid_aist_otrp_ta, TEEC_LOGIN_PUBLIC,
			     NULL, &op, NULL);
	if (r != TEEC_SUCCESS) {
		fprintf(stderr, "%s: tee open session failed 0x%x\n",
			__func__, r);
		goto bail2;
	}

	return 0;
bail2:
	TEEC_FinalizeContext(&ctx->tee_context);
bail1:
	return -1;
}

static void broker_ctx_destroy(struct broker_ctx *ctx)
{
	TEEC_CloseSession(&ctx->tee_session);
	TEEC_FinalizeContext(&ctx->tee_context);
}

static int set_agent_dev_option(struct broker_ctx *ctx, enum agent_dev_option option, const char *value)
{
	TEEC_Result n;
	TEEC_Operation op;

	memset(&op, 0, sizeof(TEEC_Operation));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_NONE,
					 TEEC_NONE);

	op.params[0].value.a    	= option;
	op.params[0].value.b		= 0;
	op.params[1].tmpref.buffer 	= (void *)value;
	op.params[1].tmpref.size	= strlen(value) + 1;

	n = TEEC_InvokeCommand(&ctx->tee_session, TEEP_AGENT_SET_DEV_OPTION, &op, NULL);
	if (n != TEEC_SUCCESS) {
		lwsl_err("%s: TEEC_InvokeCommand "
		        "failed (0x%08x)\n", __func__, n);
		return (int)n;
	}
	return n;
}

static int broker_task_done(struct broker_ctx *ctx, const void *in, size_t in_len)
{
	TEEC_Result n;
	TEEC_Operation op;

	memset(&op, 0, sizeof(TEEC_Operation));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_NONE,
					 TEEC_NONE,
					 TEEC_NONE);

	op.params[0].tmpref.buffer 	= (void *)in;
	op.params[0].tmpref.size	= in_len;

	n = TEEC_InvokeCommand(&ctx->tee_session, TEEP_AGENT_BROKER_TASK_DONE, &op, NULL);
	if (n != TEEC_SUCCESS) {
		lwsl_err("%s: TEEC_InvokeCommand "
		        "failed (0x%08x)\n", __func__, n);
		return (int)n;
	}
	return n;
}

static int agent_query_next_broker_task(struct broker_ctx *ctx, struct broker_task *task)
{
	TEEC_Result n;
	TEEC_Operation op;

	memset(&op, 0, sizeof(TEEC_Operation));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_NONE);

	op.params[1].tmpref.buffer 	= task->uri;
	op.params[1].tmpref.size	= sizeof (task->uri);
	op.params[2].tmpref.buffer 	= task->post_data;
	op.params[2].tmpref.size	= sizeof (task->post_data);

	n = TEEC_InvokeCommand(&ctx->tee_session, TEEP_AGENT_QUERY_NEXT_BROKER_TASK, &op, NULL);
	if (n != TEEC_SUCCESS) {
		lwsl_err("%s: TEEC_InvokeCommand "
		        "failed (0x%08x)\n", __func__, n);
		return (int)n;
	}

	task->command = op.params[0].value.a;
	size_t uri_len = op.params[1].tmpref.size;
	if (task->uri[uri_len - 1] != 0) {
		return -1;
	}
	task->post_data_len = op.params[2].tmpref.size;

	return n;
}

static void
usage(void)
{
	fprintf(stderr, "teep-broker-app [--tamurl http://tamserver:port] [--cose] [--talist uuid1,uuid2,...]\n");
	fprintf(stderr,
			"     --tamurl: TAM server url\n"
			"     --cose: enable sign\n"
			"     --talist: installed ta list.  for testing\n");
	exit(1);
}

static void
cmdline_parse(int argc, const char *argv[])
{
	const char *tmp;
	if (lws_cmdline_option(argc, argv, "--help"))
		usage();

	/* override the remote TAM URL */
	tmp = lws_cmdline_option(argc, argv, "--tamurl");
	if (tmp)
		uri = tmp;

	/* request the TAM ask the TEE to delete the test TA */
	/* protocol (teep or otrp) */
	tmp = lws_cmdline_option(argc, argv, "--cose");
	if (tmp) {
		cose = true;
	}

	/* ta-list */
	tmp = lws_cmdline_option(argc, argv, "--talist");
	if (tmp)
		talist = tmp;

	fprintf(stderr, "%s compiled at %s %s\n", __FILE__, __DATE__, __TIME__);
	fprintf(stderr, "uri = %s, cose=%d, talist=%s\n", uri, cose, talist);
}

static int broker_http_post(struct broker_ctx *ctx, const struct broker_task *task)
{
	lwsl_notice("POST: %s\n", task->uri);
	lwsl_hexdump_notice(task->post_data, task->post_data_len);
	size_t len = sizeof (http_res_buf);
	int n = http_post(task->uri, task->post_data, task->post_data_len, http_res_buf, &len);
	if (n < 0) {
		return -1;
	}
	lwsl_hexdump_notice(http_res_buf, len);
	return broker_task_done(ctx, http_res_buf, len);
}

static int broker_http_get(struct broker_ctx *ctx, const struct broker_task *task)
{
	lwsl_notice("GET: %s\n", task->uri);
	size_t len = sizeof (http_res_buf);
	int n = http_get(task->uri, http_res_buf, &len);
	if (n < 0) {
		return -1;
	}
	return broker_task_done(ctx, http_res_buf, len);
}

/**
 * loop_teep - The teep message request.     
 *
 * This function has a loop. The loop condtion is based on
 * the tam message and for each iteration it will go through the every 
 * switch case and if the switch statement matches with the type it will 
 * invoke the respective function; if it does not match,then it executes 
 * the default case.
 * 
 * @param lao_ctx       It is an object of structure libteep context.    
 * 
 * @return 0            If success, else error occurred.
 */
static int loop_teep(struct broker_ctx *ctx)
{
	set_agent_dev_option(ctx, AGENT_OPTION_SET_TAM_URI, uri);
	set_agent_dev_option(ctx, AGENT_OPTION_SET_CURRENT_TA_LIST, talist);
	for (;;) {
		struct broker_task task;
		if (agent_query_next_broker_task(ctx, &task) < 0) {
			return -1;
		}
		if (task.command == BROKER_FINISH) {
			break;
		} else if (task.command == BROKER_HTTP_POST) {
			if (broker_http_post(ctx, &task) < 0) {
				return -1;
			}
		} else if (task.command == BROKER_HTTP_GET) {
			if (broker_http_get(ctx, &task) < 0) {
				return -1;
			}
		} else {
			return -1;
		}
	}
	return 0;
}

int broker_main()
{
	struct broker_ctx ctx;
	if (broker_ctx_init(&ctx) < 0) {
		return -1;
	}

	//set_ta_list(&ctx, talist);

	loop_teep(&ctx);

	broker_ctx_destroy(&ctx);
	return 0;
}

int main(int argc, const char** argv)
{
	lws_set_log_level(0
		| LLL_USER
		| LLL_ERR
		| LLL_WARN
//		| LLL_INFO
		| LLL_NOTICE
//		| LLL_DEBUG
		, NULL);

	cmdline_parse(argc, argv);
	return broker_main();
}
