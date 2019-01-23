/*
 * Copyright (C) 2019  National Institute of Advanced Industrial Science
 *                     and Technology (AIST）
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
static const char *duri = "http://buddy.home.warmcat.com:3000";

int
main(int argc, char *argv[])
{
	struct libaistotrp_ctx *lao_ctx = NULL;
	struct lao_rpc_io io;
	uint8_t result[64];
	char *uri = duri;
	int n;

	/* our remote TAM */

	if (argc > 1)
		duri = argv[1];

	if (libaistotrp_init(&lao_ctx, uri)) {
		fprintf(stderr, "%s: Unable to create lao\n", __func__);

		return 1;
	}

	/* ask the TAM to give us an encrypted, signed TA... we can't
	 * decrypt it because it's encrypted using the TEE's pubkey */

	io.in = "";
	io.in_len = 0;
	io.out = pkt;
	io.out_len = sizeof(pkt);

	n = libaistotrp_tam_msg(lao_ctx, "enc-ta.jwe.jws", &io);
	if (n != TR_OKAY) {
		fprintf(stderr, "%s: libaistotrp_tam_msg: %d\n", __func__, n);

		return 1;
	}

	lwsl_notice("%s: received TAM pkt len %d\n", __func__, (int)io.out_len);

	/* pass the encrypted, signed TA into the TEE to deal with */

	io.in = pkt;
	io.in_len = io.out_len;
	io.out = result;
	io.out_len = sizeof(result);

	n = libaistotrp_pta_msg(lao_ctx, 1, &io);
	if (n) {
		lwsl_err("%s: libaistotrp_pta_msg: fail %d\n", __func__, n);
	} else
		lwsl_notice("%s: libaistotrp_pta_msg: OK %d\n", __func__,
				(int)io.out_len);

	libaistotrp_destroy(&lao_ctx);

	return 0;
}

