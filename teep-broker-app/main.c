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
static const char *talist = "3cfa03b5-d4b1-453a-9104-4e4bef53b37e"; // installed TA list
static bool jose = false;

enum teep_message_type {
	QUERY_REQUEST = 1,
	QUERY_RESPONSE = 2,
	TRUSTED_APP_INSTALL = 3,
	TRUSTED_APP_DELETE = 4,
	SUCCESS = 5,
	ERROR = 6
};

enum otrp_message_type {
	OTRP_GET_DEVICE_STATE_REQUEST = 1,
	OTRP_INSTALL_TA_REQUEST = 2,
	OTRP_DELETE_TA_REQUEST = 3
};

static void
usage(void)
{
	fprintf(stderr, "aist-otrp-testapp [--tamurl http://tamserver:port] [-d] [-p otrp]\n");
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
			teep_ver = LIBTEEP_TEEP_VER_OTRP;
		} else if (!strcmp(tmp, "teep")) {
			teep_ver = LIBTEEP_TEEP_VER_TEEP;
		} else {
			// usage();
			// use default protocol is TEEP
			teep_ver = LIBTEEP_TEEP_VER_TEEP;
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

struct otrp_mesg {
	int type;
	char *mes;
};

struct ta_list {
	size_t len;
	char uuid[10][37];
};

struct manifest_list {
	size_t len;
	char url[10][256];
};

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
parse_otrp_type_cb(struct lejp_ctx *ctx, char reason)
{
	struct otrp_mesg *m = (void *)ctx->user;
	if (!strcmp(ctx->path, "GetDeviceStateRequest")) {
		m->type = OTRP_GET_DEVICE_STATE_REQUEST;
		lwsl_user("TYPE: %d, message: %s, %d\n", m->type, m->mes, reason);
		return 0;
	}
	if (!strcmp(ctx->path, "InstallTARequest")) {
		m->type = OTRP_INSTALL_TA_REQUEST;
		lwsl_user("TYPE: %d\n", m->type);
		return 0;
	}
	if (!strcmp(ctx->path, "DeleteTARequest")) {
		m->type = OTRP_DELETE_TA_REQUEST;
		lwsl_user("TYPE: %d\n", m->type);
		return 0;
	}
	lwsl_user("DEBUG: reason=%d buf=%s path=%s uni=%d npos=%d f=%d sp=%d ipos=%d pst_sp=%d\n",
			reason, ctx->buf, ctx->path, ctx->uni, ctx->npos, ctx->f, ctx->sp, ctx->ipos, ctx->pst_sp);
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

int loop_teep(struct libteep_ctx *lao_ctx) {
	lwsl_notice("send empty body to begin teep protocol\n");
	int n;
	n = libteep_tam_msg(lao_ctx, http_res_buf, sizeof(http_res_buf), "", 0);
	if (n < 0) {
		lwsl_err( "%s: libteep_tam_msg: %d\n", __func__, n);
		return n;
	}
	struct manifest_list manifests = {.len = 0};
	struct ta_list tas = {.len = 0};
	while (n > 0) { // if n == 0 then zero packet
		n = unwrap_teep_request(lao_ctx, teep_req_buf, sizeof(teep_req_buf), http_res_buf, (size_t)n);
		if (n < 0) {
			lwsl_err("%s: unwrap_teep_request: fail %d\n", __func__, n);
			return n;
		}
		if (n == 0) {
			lwsl_notice("%s: received encrypted empty body\n", __func__);
			break;
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
		lejp_destruct(&jp_ctx);
		switch (m.type) {
		case QUERY_REQUEST:
			lwsl_notice("detect QUERY_REQUEST\n");
			/* TODO: check entries in REQUEST */

			lwsl_notice("send QUERY_RESPONSE\n");
			{
				char ta_id_list[1000] = "";
				char *talist_dup = strdup(talist);
				char *ta_uuid = strtok(talist_dup, ",");
				while (ta_uuid) {
					char tmp[300];
					lws_snprintf(tmp, sizeof(tmp),
						"{\"Vendor_ID\":\"%s\",\"Class_ID\":\"%s\",\"Device_ID\":\"%s\"},",
						"ietf-teep-wg", ta_uuid, "teep-device");
					strncat(ta_id_list, tmp, sizeof(ta_id_list) - strlen(ta_id_list));
					ta_uuid = strtok(NULL, ",");
				}
				ta_id_list[strlen(ta_id_list) - 1] = '\0';
				lws_snprintf(teep_res_buf, sizeof(teep_res_buf), 
					"{\"TYPE\":%d,\"TOKEN\":\"%s\",\"TA_LIST\":[%s]}", QUERY_RESPONSE, m.token, ta_id_list);
				free(talist_dup);
				talist_dup = NULL;
			}
			lwsl_notice("json: %s, len: %zd\n", teep_res_buf, strlen(teep_res_buf));
			n = wrap_teep_response(lao_ctx, http_req_buf, sizeof(http_req_buf), teep_res_buf, strlen(teep_res_buf));
			if (n < 0) {
				lwsl_err("%s: wrap_teep_response failed %d\n", __func__, n);
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
			lejp_construct(&jp_ctx, parse_manifest_list, &manifests, NULL, 0);
			lejp_parse(&jp_ctx, (void *)teep_req_buf, n);
			lejp_destruct(&jp_ctx);
			for (int i = 0; i < manifests.len; i++) {
				libteep_download_and_install_ta_image(lao_ctx, manifests.url[i]);
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
			lejp_construct(&jp_ctx, parse_ta_list, &tas, NULL, 0);
			lejp_parse(&jp_ctx, (void *)teep_req_buf, n);
			lejp_destruct(&jp_ctx);
			for (int i = 0; i < tas.len; i++) {
				libteep_ta_store_delete(lao_ctx, tas.uuid[i], strlen(tas.uuid[i]));
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
		default:
			lwsl_err("%s: requested message type is invalid %d\n", __func__, m.type);
			return -1;
		}
	}
	lwsl_notice("receive empty body to finish teep protocol\n");
	return n;
}

int loop_otrp(struct libteep_ctx *lao_ctx) {
	lwsl_notice("send empty body to begin otrp protocol\n");
	int n;
	n = libteep_tam_msg(lao_ctx, http_res_buf, sizeof(http_res_buf), "", 0);
	if (n < 0) {
		lwsl_err( "%s: libteep_tam_msg: %d\n", __func__, n);
		return n;
	}
	while (n > 0) { // n == 0 means zero packet
		memcpy(teep_req_buf, http_res_buf, (size_t)n);
		if (n < 0) {
			lwsl_err("%s: unwrap_teep_request: fail %d\n", __func__, n);
			return n;
		}
		if (n == 0) {
			lwsl_notice("%s: received encrypted empty body\n", __func__);
			break;
		}
		lwsl_notice("%s: received message: %*s\n", __func__, n, (char *)teep_req_buf);
		struct lejp_ctx jp_ctx;
		struct otrp_mesg m = {
			.type = -1,
			.mes = NULL
		};

		lejp_construct(&jp_ctx, parse_otrp_type_cb, &m, NULL, 0);
		lejp_parse(&jp_ctx, (void *)teep_req_buf, n);
		lejp_destruct(&jp_ctx);
		switch (m.type) {
		case OTRP_GET_DEVICE_STATE_REQUEST:
			lwsl_notice("detect OTRP_GET_DEVICE_STATE_REQUEST\n");
			/* TODO: check entries in REQUEST */
			exit(1);
			break;
		case OTRP_INSTALL_TA_REQUEST:
			lwsl_notice("detect OTRP_INSTALL_TA_REQUEST\n");
			exit(1);
			break;
		case OTRP_DELETE_TA_REQUEST:
			lwsl_notice("detect OTRP_DELETE_TA_REQUEST\n");
			exit(1);
			break;
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

	if (teep_ver == LIBTEEP_TEEP_VER_TEEP)
		loop_teep(lao_ctx);
	else if (teep_ver == LIBTEEP_TEEP_VER_OTRP)
		loop_otrp(lao_ctx);

	/* ask the TAM to give us an encrypted, signed TA... we can't
	 * decrypt it because it's encrypted using the TEE's pubkey */

	libteep_destroy(&lao_ctx);
	return 0;
}


