/*
 * Copyright (C) 2019  National Institute of Advanced Industrial Science
 *                     and Technology (AIST)
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
 *
 * This is a REE test application to test operation of libteep.
 */
#include <libteep.h>
#include <libwebsockets.h>

static uint8_t http_res_buf[6 * 1024 * 1024];
static char teep_req_buf[5 * 1024];
static char teep_res_buf[5 * 1024];
static char teep_tmp_buf[800 * 1024];
static uint8_t http_req_buf[5 * 1024];
static char ta_url_list_buf[1024];

const char *uri = "http://127.0.0.1:3000/api/tam"; // TAM server uri
const char *talist = ""; // installed TA list
bool cose = false;

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

void
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

int loop_teep(struct libteep_ctx *lao_ctx)
{
	libteep_set_ta_list(lao_ctx, talist);
	size_t tam_request_len = 0;
	do {
		int n = libteep_tam_msg(lao_ctx, http_res_buf, sizeof (http_res_buf), http_req_buf, tam_request_len);
		if (n <= 0) break;
		size_t tam_response_len = n;
		tam_request_len = sizeof (http_req_buf);
		if (libteep_agent_msg(lao_ctx, cose, http_req_buf, &tam_request_len, ta_url_list_buf, sizeof (ta_url_list_buf), http_res_buf, tam_response_len) < 0) {
			break;
		}
		char *p = ta_url_list_buf;
		for (;;) {
			size_t len = strlen(p);
			if (len == 0) break;
			libteep_download_and_install_ta_image(lao_ctx, p);
			p += len + 1;
		}
	} while (tam_request_len > 0);
	lwsl_notice("receive empty body to finish teep protocol\n");
	return 0;
}

int broker_main()
{
	struct libteep_ctx *lao_ctx = NULL;

	int res = libteep_init(&lao_ctx, LIBTEEP_TEEP_VER_TEEP, uri);
	if (res != TR_OKAY) {
		fprintf(stderr, "%s: Unable to create lao\n", __func__);
		return 1;
	}

	loop_teep(lao_ctx);

	/* ask the TAM to give us an encrypted, signed TA... we can't
	 * decrypt it because it's encrypted using the TEE's pubkey */

	libteep_destroy(&lao_ctx);
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
