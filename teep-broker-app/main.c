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
static uint8_t http_req_buf[5 * 1024];

static const char *uri = "http://127.0.0.1:3000/api/tam"; // TAM server uri
static enum libteep_teep_ver teep_ver = LIBTEEP_TEEP_VER_TEEP; // protocol
static const char *talist = ""; // installed TA list
static bool jose = false;

enum teep_message_type {
	QUERY_REQUEST = 1,
	QUERY_RESPONSE = 2,
	TRUSTED_APP_INSTALL = 3,
	TRUSTED_APP_DELETE = 4,
	SUCCESS = 5,
	ERROR = 6
};

static void
usage(void)
{
	fprintf(stderr, "aist-otrp-testapp [--tamurl http://tamserver:port] [-d]\n");
	fprintf(stderr, "     --tamurl: TAM server url \n"
			"     --jose: enable encryption and sign \n"
			"     --talist: installed ta list \n"
			"     -p: teep protocol otrp or teep \n");
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
	tmp = lws_cmdline_option(argc, argv, "-p");
	if (tmp) {
		if (!strcmp(tmp, "otrp")) {
			teep_ver = LIBTEEP_TEEP_VER_OTRP_V3;
		} else if (!strcmp(tmp, "teep")) {
			teep_ver = LIBTEEP_TEEP_VER_TEEP;
		} else {
			usage();
		}
	}

	/* request the TAM ask the TEE to delete the test TA */
	/* protocol (teep or otrp) */
	tmp = lws_cmdline_option(argc, argv, "--jose");
	if (tmp) {
		jose = true;
	}

	/* ta-list */
	tmp = lws_cmdline_option(argc, argv, "--talist");
	if (tmp)
		talist = tmp;

}

static int io_copy(struct lao_rpc_io *io) {
	if (io->in_len > io->out_len) {
		return 1;
	}
	memmove(io->out, io->in, io->in_len);
	io->out_len = io->in_len;
	return 0;
}

static int unwrap_teep_request(struct libteep_ctx *lao_ctx, struct lao_rpc_io *io) {
	if (jose) {
		lwsl_notice("unwrap teep message");
		return libteep_msg_unwrap(lao_ctx, io);
	} else {
		return io_copy(io);
	}
}

static int wrap_teep_request(struct libteep_ctx *lao_ctx, struct lao_rpc_io *io) {
	if (jose) {
		lwsl_notice("wrap teep message");
		return libteep_msg_wrap(lao_ctx, io);
	} else {
		return io_copy(io);
	}
}


static const char * const reason_names[] = {
	"LEJPCB_CONSTRUCTED",
	"LEJPCB_DESTRUCTED",
	"LEJPCB_START",
	"LEJPCB_COMPLETE",
	"LEJPCB_FAILED",
	"LEJPCB_PAIR_NAME",
	"LEJPCB_VAL_TRUE",
	"LEJPCB_VAL_FALSE",
	"LEJPCB_VAL_NULL",
	"LEJPCB_VAL_NUM_INT",
	"LEJPCB_VAL_NUM_FLOAT",
	"LEJPCB_VAL_STR_START",
	"LEJPCB_VAL_STR_CHUNK",
	"LEJPCB_VAL_STR_END",
	"LEJPCB_ARRAY_START",
	"LEJPCB_ARRAY_END",
	"LEJPCB_OBJECT_START",
	"LEJPCB_OBJECT_END",
};

struct teep_mesg {
	int type;
	char *token;
	size_t token_max_len;
};

static signed char
parse_type_token_cb(struct lejp_ctx *ctx, char reason)
{
	char buf[1024];
	char *p = buf;
	char *end = &buf[sizeof(buf)];
	struct teep_mesg *m = (void *)ctx->user;
	if (!strcmp(ctx->path, "TYPE") && reason == LEJPCB_VAL_NUM_INT) {
		m->type = atoi(ctx->buf);
		lwsl_notice("TYPE: %d\n", m->type);
		return 0;
	}

	if (!strcmp(ctx->path, "TOKEN")) {
		if (reason == LEJPCB_VAL_STR_START) {
			lwsl_notice("TOKEN start: %s\n", ctx->buf);
			lws_snprintf(m->token, m->token_max_len, "%s", ctx->buf);
			return 0;
		}
		if (reason == LEJPCB_VAL_STR_CHUNK) {
			lwsl_notice("TOKEN chunk: %s\n", ctx->buf);
			lws_snprintf(m->token, m->token_max_len, "%s%s", m->token, ctx->buf);
			return 0;
		}
		if (reason == LEJPCB_VAL_STR_END) {
			lwsl_notice("TOKEN end: %s\n", ctx->buf);
			lws_snprintf(m->token, m->token_max_len, "%s%s", m->token, ctx->buf);
			lwsl_notice("TOKEN: %s\n", m->token);
			return 0;
		}
	}
	
	if (reason & LEJP_FLAG_CB_IS_VALUE) {
		p += lws_snprintf(p, p - end, "   value '%s' ", ctx->buf);
		if (ctx->ipos) {
			int n;
			p += lws_snprintf(p, p - end, "(array indexes: ");
			for (n = 0; n < ctx->ipos; n++)
				p += lws_snprintf(p, p - end, "%d ", ctx->i[n]);
			p += lws_snprintf(p, p - end, ") ");
		}
		lwsl_notice("%s (%s)\r\n", buf,
		       reason_names[(unsigned int)
			(reason) & (LEJP_FLAG_CB_IS_VALUE - 1)]);

		(void)reason_names; /* NO_LOGS... */
		return 0;
	}

	switch (reason) {
	case LEJPCB_COMPLETE:
		lwsl_notice("Parsing Completed (LEJPCB_COMPLETE)\n");
		break;
	case LEJPCB_PAIR_NAME:
		lwsl_notice("path: '%s' (LEJPCB_PAIR_NAME)\n", ctx->path);
		break;
	}

	return 0;
}

int loop(struct libteep_ctx *lao_ctx) {
	struct lao_rpc_io io = {
		.in = "",
		.in_len = 0,
		.out = http_res_buf,
		.out_len = sizeof(http_res_buf)
	};

	/* pass the encrypted, signed TA into the TEE to deal with */
	do {
		int res = libteep_tam_msg(lao_ctx, &io);
		if (res != TR_OKAY) {
			fprintf(stderr, "%s: libteep_tam_msg: %d\n", __func__, res);
			return 1;
		}
		lwsl_notice("%s: received TAM pkt len %d\n", __func__, (int)io.out_len);

		if (io.out_len == 0) { // io.in_len == 0 => finish
			lwsl_user("teep over http finish packet received\n");
			return 0;
		}
		io.in = http_res_buf;
		io.in_len = io.out_len;
		io.out = teep_req_buf;
		io.out_len = sizeof(teep_req_buf);
		res = unwrap_teep_request(lao_ctx, &io);
		if (res < 0) {
			lwsl_err("%s: unwrap_teep_request: fail %d\n", __func__, res);
			return res;
		}
		lwsl_notice("%s: unwrap_teep_request: OK %d\n", __func__, (int)io.out_len);
		lwsl_notice("%s: received message: %*s\n", __func__, (int)io.out_len, (char *)io.out);
		struct lejp_ctx jp_ctx;
		char token[100] = "";
		struct teep_mesg m = {
			.type = -1,
			.token = token,
			.token_max_len = 99
		};
		lejp_construct(&jp_ctx, parse_type_token_cb, &m, NULL, 0);
		lejp_parse(&jp_ctx, io.out, io.out_len);
		char ta_list[1000];
		switch (m.type) {
		case QUERY_REQUEST:
			lwsl_notice("detect QUERY_REQUEST");
			/* TODO: check entries in REQUEST */
			lws_snprintf(ta_list, sizeof(ta_list),
				"{\"Vendor_ID\":\"%s\",\"Class_ID\":\"%s\",\"Device_ID\":\"%s\"}",
				"ietf-teep-wg", "3cfa03b5-d4b1-453a-9104-4e4bef53b37e", "teep-device");
			lws_snprintf(teep_res_buf, sizeof(teep_res_buf), 
				"{\"TYPE\":%d,\"TOKEN\":\"%s\",\"TA_LIST\":[%s]}", QUERY_RESPONSE, m.token, ta_list);
			io.in = teep_res_buf;
			io.in_len = strlen(teep_res_buf);
			io.out = http_req_buf;
			io.out_len = sizeof(http_req_buf);
			res = wrap_teep_request(lao_ctx, &io);
			if (res < 0) {
				return 1;
			}
			io.in = io.out;
			io.in_len = io.out_len;
			io.out = http_res_buf;
			io.out_len = sizeof(http_res_buf);
			break;
		case TRUSTED_APP_INSTALL:
			lwsl_notice("detect TRUSTED_APP_INSTALL");
			/* TODO implement GET TA image */
			lws_snprintf(teep_res_buf, sizeof(teep_res_buf), 
				"{\"TYPE\":%d,\"TOKEN\":\"%s\"}", SUCCESS, m.token);
			io.in = teep_res_buf;
			io.in_len = strlen(teep_res_buf);
			io.out = http_req_buf;
			io.out_len = sizeof(http_req_buf);
			res = wrap_teep_request(lao_ctx, &io);
			if (res < 0) {
				return 1;
			}
			break;
		case TRUSTED_APP_DELETE:
			lwsl_err("%s: TODO implement TRUSTED_APP_DELETE\n", __func__);
			return 0;
		default:
			lwsl_err("%s: requested message type is invalid %d\n", __func__, m.type);
			return 1;
		}
	} while (1);
}

int
main(int argc, const char *argv[])
{
	struct libteep_ctx *lao_ctx = NULL;
	int res;

	cmdline_parse(argc, argv);
	fprintf(stderr, "%s compiled at %s %s\n", __FILE__, __DATE__, __TIME__);
	fprintf(stderr, "uri = %s, teep_ver = %d, talist=%s\n", uri, teep_ver, talist);

	res = libteep_init(&lao_ctx, teep_ver, uri);
	if (res != TR_OKAY) {
		fprintf(stderr, "%s: Unable to create lao\n", __func__);
		return 1;
	}

	loop(lao_ctx);

	/* ask the TAM to give us an encrypted, signed TA... we can't
	 * decrypt it because it's encrypted using the TEE's pubkey */

	libteep_destroy(&lao_ctx);
	return 0;
}


