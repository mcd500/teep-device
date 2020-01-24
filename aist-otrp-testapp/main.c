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
 * This is a REE test application to test operation of libaistotrp.
 */
#include <libaistotrp.h>
#include <libwebsockets.h>

static uint8_t pkt[6 * 1024 * 1024];
static const char *uri = "http://127.0.0.1:3000"; // TAM server uri
static const char *path = "/api/tam"; // TAM TEEP/HTTP server path
static const char *proto = "otrp"; // protocol
static const char *talist = ""; // installed TA list

static void
usage(void)
{
	fprintf(stderr, "aist-otrp-testapp [--tamurl http://tamserver:port] [-d]\n");
	fprintf(stderr, "     -d: ask TAM to send an encrypted request \n"
			"         for the TEE to delete the test TA\n"
			"     -p: protocol otrp or teep \n");
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
	if (lws_cmdline_option(argc, argv, "-d"))
		path = "delete";

	/* protocol (teep or otrp) */
	tmp = lws_cmdline_option(argc, argv, "-p");
	if (tmp)
		proto = tmp;

	/* ta-list */
	tmp = lws_cmdline_option(argc, argv, "--talist");
	if (tmp)
		talist = tmp;

}

int
main(int argc, const char *argv[])
{
	struct libaistotrp_ctx *lao_ctx = NULL;
	struct lao_rpc_io io;
	uint8_t result[64];
	int res;

	fprintf(stderr, "%s compiled at %s %s\n", __FILE__, __DATE__, __TIME__);
	cmdline_parse(argc, argv);

	res = libaistotrp_init(&lao_ctx, uri);
	if (res != TR_OKAY) {
		fprintf(stderr, "%s: Unable to create lao\n", __func__);
		return 1;
	}

	/* ask the TAM to give us an encrypted, signed TA... we can't
	 * decrypt it because it's encrypted using the TEE's pubkey */

	io.in = "";
	io.in_len = 0;
	io.out = pkt;
	io.out_len = sizeof(pkt);

	res = libaistotrp_tam_msg(lao_ctx, path, &io);
	if (res != TR_OKAY) {
		fprintf(stderr, "%s: libaistotrp_tam_msg: %d\n", __func__, res);
		return 1;
	}

	lwsl_notice("%s: received TAM pkt len %d\n", __func__, (int)io.out_len);

	/* pass the encrypted, signed TA into the TEE to deal with */

	io.in = pkt;
	io.in_len = io.out_len;
	io.out = result;
	io.out_len = sizeof(result);

	res = libaistotrp_pta_msg(lao_ctx, 1, &io);
	if (res != TR_OKAY) {
		lwsl_err("%s: libaistotrp_pta_msg: fail %d\n", __func__, res);
	} else
		lwsl_notice("%s: libaistotrp_pta_msg: OK %d\n", __func__,
				(int)io.out_len);

	libaistotrp_destroy(&lao_ctx);

	return 0;
}

