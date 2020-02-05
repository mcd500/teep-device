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

static int io_copy(void *out, size_t outlen, void *in, size_t inlen) {
	if (inlen > outlen) {
		return -1;
	}
	memmove(out, in, inlen);
	return inlen;
}

static int unwrap_teep_request(struct libteep_ctx *lao_ctx, void *out, size_t outlen, void *in, size_t inlen) {
	if (jose) {
		lwsl_notice("unwrap teep message\n");
		return libteep_msg_unwrap(lao_ctx, out, outlen, in, inlen);
	} else {
		return io_copy(out, outlen, in, inlen);
	}
}

static int wrap_teep_response(struct libteep_ctx *lao_ctx, void *out, size_t outlen, void *in, size_t inlen) {
	if (jose) {
		lwsl_notice("wrap teep message\n");
		return libteep_msg_wrap(lao_ctx, out, outlen, in, inlen);
	} else {
		return io_copy(out, outlen, in, inlen);
	}
}

struct teep_mesg {
	int type;
	char *token;
	size_t token_max_len;
};


struct ta_list {
	size_t len;
	char uuid[10][37];
};

struct manifest_list {
	size_t len;
	char url[10][256];
};

#if 0
static signed char
parse_ta_list(struct lejp_ctx *ctx, char reason)
{
	struct ta_list *l = (void *)ctx->user;
	if (!strcmp(ctx->path, "TA_LIST[].Class_ID")) {
		size_t i = *ctx->i;
		if (i >= 10) {
			lwsl_err("TA_LIST is too long\n");
			return 0;
		}
		if (reason == LEJPCB_VAL_STR_START) {
			l->len = i+1;
			strncpy(l->uuid[i], ctx->buf, 36);
			return 0;
		}
		if (reason == LEJPCB_VAL_STR_CHUNK) {
			strncat(l->uuid[i], ctx->buf, 36 - strlen(l->uuid[i]));
			return 0;
		}
		if (reason == LEJPCB_VAL_STR_END) {
			strncat(l->uuid[i], ctx->buf, 36 - strlen(l->uuid[i]));
			lwsl_user("uuid[i]: %s\n", l->uuid[i]);
			return 0;
		}
	}
	return 0;
}
#endif
static signed char
parse_manifest_list(struct lejp_ctx *ctx, char reason)
{
	struct manifest_list *l = (void *)ctx->user;
	if (!strcmp(ctx->path, "MANIFEST_LIST[]")) {
		size_t i = *ctx->i;
		if (reason == LEJPCB_VAL_STR_START) {
			l->len = i+1;
			strncpy(l->url[i], ctx->buf, 255);
			return 0;
		}
		if (reason == LEJPCB_VAL_STR_CHUNK) {
			strncat(l->url[i], ctx->buf, 255 - strlen(l->url[i]));
			return 0;
		}
		if (reason == LEJPCB_VAL_STR_END) {
			strncat(l->url[i], ctx->buf, 255 - strlen(l->url[i]));
			lwsl_user("url[i]: %s\n", l->url[i]);
			return 0;
		}
	}
	return 0;
}

static signed char
parse_type_token_cb(struct lejp_ctx *ctx, char reason)
{
	struct teep_mesg *m = (void *)ctx->user;
	if (!strcmp(ctx->path, "TYPE") && reason == LEJPCB_VAL_NUM_INT) {
		m->type = atoi(ctx->buf);
		lwsl_user("TYPE: %d\n", m->type);
		return 0;
	}

	if (!strcmp(ctx->path, "TOKEN")) {
		if (reason == LEJPCB_VAL_STR_START) {
			lws_snprintf(m->token, m->token_max_len, "%s", ctx->buf);
			return 0;
		}
		if (reason == LEJPCB_VAL_STR_CHUNK) {
			lws_snprintf(m->token, m->token_max_len, "%s%s", m->token, ctx->buf);
			return 0;
		}
		if (reason == LEJPCB_VAL_STR_END) {
			lws_snprintf(m->token, m->token_max_len, "%s%s", m->token, ctx->buf);
			lwsl_user("TOKEN: %s\n", m->token);
			return 0;
		}
	}
	return 0;
}

int loop(struct libteep_ctx *lao_ctx) {
	lwsl_notice("send empty body to begin teep protocol\n");
	int n;
	n = libteep_tam_msg(lao_ctx, http_res_buf, sizeof(http_res_buf), "", 0);
	if (n < 0) {
		lwsl_err( "%s: libteep_tam_msg: %d\n", __func__, n);
		return n;
	}
	struct manifest_list ml = {.len = 0};
	// struct ta_list tl = {.len = 0};
	while (n > 0) { // if n == 0 then zero packet
		n = unwrap_teep_request(lao_ctx, teep_req_buf, sizeof(teep_req_buf), http_res_buf, (size_t)n);
		if (n < 0) {
			lwsl_err("%s: unwrap_teep_request: fail %d\n", __func__, n);
			return n;
		}
		lwsl_notice("%s: received message: %*s\n", __func__, n, (char *)teep_req_buf);
		struct lejp_ctx jp_ctx;
		char token[100] = "";
		struct teep_mesg m = {
			.type = -1,
			.token = token,
			.token_max_len = 99
		};

		lejp_construct(&jp_ctx, parse_type_token_cb, &m, NULL, 0);
		lejp_parse(&jp_ctx, (void *)teep_req_buf, n);
		char tmp[1000];
		switch (m.type) {
		case QUERY_REQUEST:
			lwsl_notice("detect QUERY_REQUEST\n");
			/* TODO: check entries in REQUEST */

			lwsl_notice("send QUERY_RESPONSE\n");
			lws_snprintf(tmp, sizeof(tmp),
				"{\"Vendor_ID\":\"%s\",\"Class_ID\":\"%s\",\"Device_ID\":\"%s\"}",
				"ietf-teep-wg", "3cfa03b5-d4b1-453a-9104-4e4bef53b37e", "teep-device");
			lws_snprintf(teep_res_buf, sizeof(teep_res_buf), 
				"{\"TYPE\":%d,\"TOKEN\":\"%s\",\"TA_LIST\":[%s]}", QUERY_RESPONSE, m.token, tmp);
			lwsl_notice("json: %s, len: %zd\n", teep_res_buf, strlen(teep_res_buf));
			n = wrap_teep_response(lao_ctx, http_req_buf, sizeof(http_req_buf), teep_res_buf, strlen(teep_res_buf));
			if (n < 0) {
				lwsl_err("%s: wrap_teep_response failed %d", __func__, n);
				return n;
			}
			lwsl_notice("body: %s, len: %zd\n", http_req_buf, (size_t)n);
			n = libteep_tam_msg(lao_ctx, http_res_buf, sizeof(http_res_buf), http_req_buf, (size_t)n);
			if (n < 0) {
				lwsl_err( "%s: libteep_tam_msg: %d\n", __func__, n);
				return n;
			}
			break;
		case TRUSTED_APP_INSTALL:
			lwsl_notice("detect TRUSTED_APP_INSTALL\n");
			/* TODO implement GET TA image */

			lejp_construct(&jp_ctx, parse_manifest_list, &ml, NULL, 0);
			lejp_parse(&jp_ctx, (void *)teep_req_buf, n);

			for (int i = 0; i < ml.len; i++) {
				libteep_download_and_install_ta_image(lao_ctx, ml.url[i]);
			}

			lwsl_notice("send SUCCESS\n");
			lws_snprintf(teep_res_buf, sizeof(teep_res_buf), 
				"{\"TYPE\":%d,\"TOKEN\":\"%s\"}", SUCCESS, m.token);
			lwsl_notice("json: %s, len: %zd\n", teep_res_buf, strlen(teep_res_buf));
			n = wrap_teep_response(lao_ctx, http_req_buf, sizeof(http_req_buf), teep_res_buf, strlen(teep_res_buf));
			if (n < 0) {
				lwsl_err("%s: wrap_teep_response failed, %d", __func__, n);
				return n;
			}
			n = libteep_tam_msg(lao_ctx, http_res_buf, sizeof(http_res_buf), http_req_buf, (size_t)n);
			if (n < 0) {
				lwsl_err( "%s: libteep_tam_msg: %d\n", __func__, n);
				return n;
			}
			break;
		case TRUSTED_APP_DELETE:
			lwsl_notice("detect TRUSTED_APP_DELETE\n");
			/* TODO implement delete TA image */

			lwsl_notice("send SUCCESS\n");
			lws_snprintf(teep_res_buf, sizeof(teep_res_buf),
				"{\"TYPE\":%d,\"TOKEN\":\"%s\"}", SUCCESS, m.token);
			lwsl_notice("json: %s, len: %zd\n", teep_res_buf, strlen(teep_res_buf));
			n = wrap_teep_response(lao_ctx, http_req_buf, sizeof(http_req_buf), teep_res_buf, strlen(teep_res_buf));
			if (n < 0) {
				lwsl_err("%s: wrap_teep_response failed, %d", __func__, n);
				return n;
			}
			n = libteep_tam_msg(lao_ctx, http_res_buf, sizeof(http_res_buf), http_req_buf, (size_t)n);
			if (n < 0) {
				lwsl_err( "%s: libteep_tam_msg: %d\n", __func__, n);
				return n;
			}
		default:
			lwsl_err("%s: requested message type is invalid %d\n", __func__, m.type);
			return -1;
		}
	}
	lwsl_notice("receive empty body to finish teep protocol\n");
	return n;
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


