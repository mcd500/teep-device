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
 * This is the libteep REE userland library.
 *
 * It provides services for REE Applications to communicate with a remote TAM,
 * and the OTrP PTA in the Secure World.
 *
 * The return convention is 0 means all is OK.  Nonzero indicates an error.
 */

#include "libteep.h"
#include <libwebsockets.h>
#include <pthread.h>

static const TEEC_UUID uuid_aist_otrp_ta =
        { 0x68373894, 0x5bb3, 0x403c,
                { 0x9e, 0xec, 0x31, 0x14, 0xa1, 0xf5, 0xd3, 0xfc } };

/* this is wholly opaque to library users */

struct libteep_ctx {
	TEEC_Context		tee_context;
	TEEC_Session		tee_session;
	pthread_mutex_t 	lock;
	struct lws_context	*lws_ctx;
	struct libteep_async	*laoa_list_head; /* protected by .lock */
	enum libteep_teep_ver	teep_ver;
	char	 		*tam_url;
};

/* This is the user-opaque internal representation of an ongoing TAM message */

//static unsigned int rrlen;
//static uint8_t *rr;

struct libteep_async {
	struct libteep_async	*laoa_list_next; /* protected by .lock */
	struct libteep_ctx	*ctx;
	struct lao_rpc_io	*io;
	struct lws		*wsi;
	size_t			max_out_len;
	int			http_resp;
	tam_result		result;
};

static const char *teep_media_type(int teep_ver) {
	switch (teep_ver) {
	case LIBTEEP_TEEP_VER_TEEP:
		return "application/teep+json";
	case LIBTEEP_TEEP_VER_OTRP:
		return "application/otrp+json";	
	default:
		return NULL;
	}
}

static int
callback_teep(struct lws *wsi, enum lws_callback_reasons reason, void *user,
	     void *in, size_t len)
{
	struct libteep_async *laoa = (struct libteep_async *)user;
	char buffer[1024 + LWS_PRE];
	char *px = buffer + LWS_PRE;
	int lenx = sizeof(buffer) - LWS_PRE;
	lwsl_info("callback %d\n", reason);
	switch (reason) {
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		lwsl_err("%s: CONNECTION_ERROR: %d\n", __func__, TR_FAIL_CONN_ERR);
		laoa->result = TR_FAIL_CONN_ERR;
		break;
	case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
		if (laoa->result == TR_ONGOING)
			laoa->result = TR_FAIL_CLOSED;
		break;
	case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
		laoa->http_resp = lws_http_client_http_response(wsi);
		lwsl_info("%s: established, resp %d\n", __func__,
				laoa->http_resp);
		if (laoa->http_resp == 204) {
			laoa->result = TR_OKAY;
			return 0;
		}
		break;
	case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
		{
			unsigned char **p = (unsigned char **)in;
			unsigned char *end = (*p) + len;
			const char *media_type = teep_media_type(laoa->ctx->teep_ver);
			if (lws_add_http_header_by_name(wsi,
						(unsigned char *)"Accept:",
						(unsigned char *)media_type, strlen(media_type), p, end)) {
				lwsl_err("%s: Append header error\n", __func__);
				return -1;
			}
			// need to add http request body
			// int to char*
			char buf_len[256];
			size_t len;
			len = snprintf(buf_len, sizeof(buf_len) - 1, "%d", (int)laoa->io->in_len);
			if (lws_add_http_header_by_token(wsi,
						WSI_TOKEN_HTTP_CONTENT_LENGTH,
						(unsigned char*)buf_len, len, p, end)) {
				lwsl_err("%s: Append header error\n", __func__);
				return -1;
			}
			if (lws_add_http_header_by_token(wsi,
						WSI_TOKEN_HTTP_CONTENT_TYPE,
						(unsigned char*)media_type, strlen(media_type), p, end)) {
				lwsl_err("%s: Append header error\n", __func__);
				return -1;
			}

			lws_client_http_body_pending(wsi, 1);
			lws_callback_on_writable(wsi);
		}
		break;
	case LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
		lwsl_info("%s: completed; read %d\n", __func__,
			    (int)laoa->io->out_len);
		laoa->result = TR_OKAY;
		break;
	case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ:
		if (laoa->io->out_len + len > laoa->max_out_len) {
			lwsl_err("%s: too large\n", __func__);
			laoa->result = TR_FAIL_OVERSIZE;
			return -1;
		}
		memcpy(laoa->io->out + laoa->io->out_len, in, len);
		laoa->io->out_len += len;
		break;
	case LWS_CALLBACK_RECEIVE_CLIENT_HTTP:
		if (lws_http_client_read(wsi, &px, &lenx) < 0)
			return -1;
		return 0;
	case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
		if (lws_write(wsi, laoa->io->in, laoa->io->in_len, LWS_WRITE_HTTP) < 0) {
			lwsl_err("%s: Write body error\n", __func__);
			return -1;
		}
		lws_client_http_body_pending(wsi, 0);
		break;
	default:
		break;
	}
	return lws_callback_http_dummy(wsi, reason, user, in, len);
}

static int
callback_download_ta_image(struct lws *wsi, enum lws_callback_reasons reason, void *user,
	     void *in, size_t len)
{
	struct libteep_async *laoa = (struct libteep_async *)user;
	char buffer[1024 + LWS_PRE];
	char *px = buffer + LWS_PRE;
	int lenx = sizeof(buffer) - LWS_PRE;
	lwsl_info("callback %d\n", reason);
	switch (reason) {
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		lwsl_err("%s: CONNECTION_ERROR: %d\n", __func__, TR_FAIL_CONN_ERR);
		laoa->result = TR_FAIL_CONN_ERR;
		break;
	case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
		if (laoa->result == TR_ONGOING)
			laoa->result = TR_FAIL_CLOSED;
		break;
	case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
		laoa->http_resp = lws_http_client_http_response(wsi);
		lwsl_info("%s: established, resp %d\n", __func__,
				laoa->http_resp);
		if (laoa->http_resp != 200) {
			laoa->result = TR_FAIL_REFUSED;
			return -1;
		}
		break;
	case LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
		lwsl_info("%s: completed; read %d\n", __func__,
			    (int)laoa->io->out_len);
		laoa->result = TR_OKAY;
		break;
	case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ:
		if (laoa->io->out_len + len > laoa->max_out_len) {
			lwsl_err("%s: too large\n", __func__);
			laoa->result = TR_FAIL_OVERSIZE;
			return -1;
		}
		memcpy(laoa->io->out + laoa->io->out_len, in, len);
		laoa->io->out_len += len;
		break;
	case LWS_CALLBACK_RECEIVE_CLIENT_HTTP:
		if (lws_http_client_read(wsi, &px, &lenx) < 0)
			return -1;
		return 0;
	case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
		if (lws_write(wsi, laoa->io->in, laoa->io->in_len, LWS_WRITE_HTTP) < 0) {
			lwsl_err("%s: Write body error\n", __func__);
			return -1;
		}
		lws_client_http_body_pending(wsi, 0);
		break;
	default:
		break;
	}
	return lws_callback_http_dummy(wsi, reason, user, in, len);
}


static const struct lws_protocols protocols[] = {
	{ "teep", callback_teep, 0, 4096, },
	{ "download_ta_image", callback_download_ta_image, 0, 4096, },
	{ NULL, NULL, 0, 0 }
};

static char ta_image_buf[4 * 1024 * 1024];



int
libteep_tam_msg(struct libteep_ctx *ctx, void *res, size_t reslen, void *req, size_t reqlen)
{
	char tmp_url[200];
	const char *proto;
	const char *address;
	int port;
	const char *path_slash_dropped;

	strncpy(tmp_url, ctx->tam_url, sizeof(tmp_url));
	if (lws_parse_uri(tmp_url, &proto, &address, &port, &path_slash_dropped)) {
		fprintf(stderr, "%s: Failed to parse tam URL base %s\n", __func__, ctx->tam_url);
		return -1;
	}

	char path[200] = "/";
	strncpy(&path[1], path_slash_dropped, sizeof(path) - 1);

	struct libteep_async *laoa = malloc(sizeof(*laoa));

	if (!laoa) {
		fprintf(stderr, "%s: Failed to alloc memory\n", __func__);
		return -1; /* OOM */
	}

	lws_explicit_bzero(laoa, sizeof(*laoa));

	struct lao_rpc_io io = {
		.in = req,
		.in_len = reqlen,
		.out = res,
		.out_len = 0
	};
	laoa->ctx = ctx;
	laoa->io = &io;
	laoa->max_out_len = reslen;
	laoa->wsi = NULL;

	pthread_mutex_lock(&ctx->lock); /* ++++++++++++++++++++++++ ctx->lock */
	laoa->laoa_list_next = ctx->laoa_list_head;
	ctx->laoa_list_head = laoa;
	pthread_mutex_unlock(&ctx->lock); /* ctx->lock ---------------------- */


	struct lws_client_connect_info conn_info = {
		.context = ctx->lws_ctx,
		.address = address,
		.port = port,
		.alpn = "http/1.1",
		.path = path,
		.host = "example.com",
		.origin = "192.168.11.3",
		.method = "POST",
		.userdata = laoa,
		.protocol = protocols[0].name,
		.pwsi = &laoa->wsi,
	};

	if (!strcmp(proto, "https")) {
		conn_info.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK ;
	} else if (!strcmp(proto, "http")) {
		;
	} else {
		fprintf(stderr, "%s: Unsupported protocol %s\n", __func__, proto);
		return -1;
	}

//	lws_client_http_multipart(i.pwsi, NULL, NULL, c_type, i.userdata, i.userdata + 200);

	lwsl_notice("%s://%s:%d%s\n" , proto, conn_info.address, conn_info.port, conn_info.path);
	if (!lws_client_connect_via_info(&conn_info)) {
		lwsl_err("%s: connect failed\n", __func__);
		return -1;
	}

	laoa->result = TR_ONGOING;

	/* spin the event loop until we're done */

	while (!lws_service(ctx->lws_ctx, 1000) && laoa->result == TR_ONGOING)
		;
	if (laoa->result < 0) {
		return laoa->result;
	}
	return io.out_len;
}

int
libteep_ta_store_install(struct libteep_ctx *ctx, char *ta_image, size_t ta_image_len)
{
	TEEC_Result n;
	TEEC_Operation op;

	memset(&op, 0, sizeof(TEEC_Operation));

	static char ta_image_dec[200*1024];
	int ta_image_dec_len = sizeof(ta_image_dec);

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_VALUE_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_VALUE_INOUT);


	op.params[0].tmpref.buffer 	= ta_image;
	op.params[0].tmpref.size	= ta_image_len;
	op.params[1].value.a		= ta_image_len;
	op.params[2].tmpref.buffer	= ta_image_dec;
	op.params[2].tmpref.size	= ta_image_dec_len;
	op.params[3].value.a		= ta_image_dec_len;
	n = TEEC_InvokeCommand(&ctx->tee_session, 301, &op, NULL);
	if (n != TEEC_SUCCESS) {
		lwsl_err("%s: TEEC_InvokeCommand "
		        "failed (0x%08x)\n", __func__, n);
		return (int)n;
	}

	ta_image = op.params[2].tmpref.buffer;
	ta_image_dec_len = op.params[3].value.a;

	memset(&op, 0, sizeof(TEEC_Operation));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_VALUE_INPUT,
					 TEEC_NONE,
					 TEEC_NONE);

	op.params[0].tmpref.buffer 	= ta_image_dec;
	op.params[0].tmpref.size	= ta_image_dec_len;
	op.params[1].value.a		= ta_image_dec_len;

	n = TEEC_InvokeCommand(&ctx->tee_session, 101, &op, NULL);
	if (n != TEEC_SUCCESS) {
		lwsl_err("%s: TEEC_InvokeCommand "
		        "failed (0x%08x)\n", __func__, n);
		return (int)n;
	}
	return n;
}

int
libteep_ta_store_delete(struct libteep_ctx *ctx, char *uuid, size_t uuid_len) {
	TEEC_Result n;
	TEEC_Operation op;

	memset(&op, 0, sizeof(TEEC_Operation));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_VALUE_INPUT,
					 TEEC_NONE,
					 TEEC_NONE);

	op.params[0].tmpref.buffer 	= uuid;
	op.params[0].tmpref.size	= uuid_len;
	op.params[1].value.a		= uuid_len;

	n = TEEC_InvokeCommand(&ctx->tee_session, 102, &op, NULL);
	if (n != TEEC_SUCCESS) {
		lwsl_err("%s: TEEC_InvokeCommand "
		        "failed (0x%08x)\n", __func__, n);
		return (int)n;
	}
	return n;
}

int
libteep_teep_agent_msg(struct libteep_ctx *ctx, uint32_t cmd,
		    struct lao_rpc_io *io)
{
	TEEC_Result n;
	TEEC_Operation op;

	memset(&op, 0, sizeof(TEEC_Operation));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_VALUE_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_VALUE_INOUT);

	op.params[0].tmpref.buffer 	= io->in;
	op.params[0].tmpref.size	= io->in_len;
	op.params[1].value.a		= io->in_len;
	op.params[2].tmpref.buffer	= io->out;
	op.params[2].tmpref.size	= io->out_len;
	op.params[3].value.a		= io->out_len;

	n = TEEC_InvokeCommand(&ctx->tee_session, cmd, &op, NULL);
	if (n != TEEC_SUCCESS) {
		lwsl_err("%s: TEEC_InvokeCommand "
		        "failed (0x%08x)\n", __func__, n);
		return (int)n;
	}

	io->out_len = op.params[3].value.a;
	lwsl_hexdump_notice(io->out, io->out_len);
	return n;
}

int
libteep_init(struct libteep_ctx **ctx, enum libteep_teep_ver ver, const char *tam_url)
{
	struct lws_context_creation_info info;
	TEEC_Operation op;
	TEEC_Result r;
	if (ver != LIBTEEP_TEEP_VER_TEEP && ver != LIBTEEP_TEEP_VER_OTRP) {
		lwsl_err("unsupported teep protocol version\n");
		return 1;
	}
	*ctx = malloc(sizeof(**ctx));
	if (!(*ctx)) {
		lwsl_err("out of memory\n");
		return 1;
	}

	lws_explicit_bzero(*ctx, sizeof(**ctx));

	(*ctx)->teep_ver = ver;
	(*ctx)->tam_url = strdup(tam_url);

	pthread_mutex_init(&(*ctx)->lock, NULL);

	/* start up TEEC in the context */

	r = TEEC_InitializeContext(NULL, &(*ctx)->tee_context);
	if (r != TEEC_SUCCESS) {
		fprintf(stderr, "%s: tee_context init failed 0x%x\n",
			__func__, r);
		goto bail1;
	}

	/* and open a session to the AIST OTrP PTA in the context */

	memset(&op, 0, sizeof(TEEC_Operation));
	r = TEEC_OpenSession(&(*ctx)->tee_context, &(*ctx)->tee_session,
			     &uuid_aist_otrp_ta, TEEC_LOGIN_PUBLIC,
			     NULL, &op, NULL);
	if (r != TEEC_SUCCESS) {
		fprintf(stderr, "%s: tee open session failed 0x%x\n",
			__func__, r);
		goto bail2;
	}
	/*
	 * Create an lws context in there as well, so we can do http client
	 * connections at will later.
	 */

	memset(&info, 0, sizeof(info));
	info.port = CONTEXT_PORT_NO_LISTEN;
	info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
	info.protocols = protocols;

	(*ctx)->lws_ctx = lws_create_context(&info);

	if (!(*ctx)->lws_ctx)
		goto bail3;
	return 0;
bail3:
	TEEC_CloseSession(&(*ctx)->tee_session);
bail2:
	TEEC_FinalizeContext(&(*ctx)->tee_context);
bail1:
	pthread_mutex_destroy(&(*ctx)->lock);
	free((*ctx)->tam_url);
	free(*ctx);
	*ctx = NULL;
	return 1;
}

void
libteep_destroy(struct libteep_ctx **ctx)
{
	TEEC_CloseSession(&(*ctx)->tee_session);
	TEEC_FinalizeContext(&(*ctx)->tee_context);
	lws_context_destroy((*ctx)->lws_ctx);
	(*ctx)->lws_ctx = NULL;

	pthread_mutex_destroy(&(*ctx)->lock);
	free((*ctx)->tam_url);
	free(*ctx);
	*ctx = NULL;
}

int
libteep_msg_wrap(struct libteep_ctx *ctx, void *out, size_t outlen, void *in, size_t inlen)
{
	struct lao_rpc_io io = {
		.in = in,
		.in_len = inlen,
		.out = out,
		.out_len = outlen
	};
	int n = libteep_teep_agent_msg(ctx, 1, &io);
	if (n < 0) {
		return n;
	}
	return io.out_len;
}

int
libteep_msg_unwrap(struct libteep_ctx *ctx, void *out, size_t outlen, void *in, size_t inlen)
{
	struct lao_rpc_io io = {
		.in = in,
		.in_len = inlen,
		.out = out,
		.out_len = outlen
	};
	int n = libteep_teep_agent_msg(ctx, 2, &io);
	if (n < 0) {
		return n;
	}
	return io.out_len;
}

int
libteep_download_and_install_ta_image(struct libteep_ctx *ctx, char *url) {
	char tmp_url[200];
	const char *proto;
	const char *address;
	int port;
	const char *path_slash_dropped;

	strncpy(tmp_url, url, sizeof(tmp_url));
	if (lws_parse_uri(tmp_url, &proto, &address, &port, &path_slash_dropped)) {
		fprintf(stderr, "%s: Failed to parse tam URL base %s\n", __func__, ctx->tam_url);
		return -1;
	}

	char path[200] = "/";
	strncpy(&path[1], path_slash_dropped, sizeof(path) - 1);

	struct libteep_async *laoa = malloc(sizeof(*laoa));

	if (!laoa) {
		fprintf(stderr, "%s: Failed to alloc memory\n", __func__);
		return -1; /* OOM */
	}

	lws_explicit_bzero(laoa, sizeof(*laoa));

	struct lao_rpc_io io = {
		.in = "",
		.in_len = 0,
		.out = ta_image_buf,
		.out_len = 0
	};
	laoa->ctx = ctx;
	laoa->io = &io;
	laoa->max_out_len = sizeof(ta_image_buf);
	laoa->wsi = NULL;

	pthread_mutex_lock(&ctx->lock); /* ++++++++++++++++++++++++ ctx->lock */
	laoa->laoa_list_next = ctx->laoa_list_head;
	ctx->laoa_list_head = laoa;
	pthread_mutex_unlock(&ctx->lock); /* ctx->lock ---------------------- */


	struct lws_client_connect_info conn_info = {
		.context = ctx->lws_ctx,
		.address = address,
		.port = port,
		.alpn = "http/1.1",
		.path = path,
		.host = "example.com",
		.origin = "192.168.11.3",
		.method = "GET",
		.userdata = laoa,
		.protocol = protocols[1].name,
		.pwsi = &laoa->wsi,
	};

	if (!strcmp(proto, "https")) {
		conn_info.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK ;
	} else if (!strcmp(proto, "http")) {
		;
	} else {
		fprintf(stderr, "%s: Unsupported protocol %s\n", __func__, proto);
		return -1;
	}

//	lws_client_http_multipart(i.pwsi, NULL, NULL, c_type, i.userdata, i.userdata + 200);

	lwsl_notice("%s://%s:%d%s\n" , proto, conn_info.address, conn_info.port, conn_info.path);
	if (!lws_client_connect_via_info(&conn_info)) {
		lwsl_err("%s: connect failed\n", __func__);
		return -1;
	}

	laoa->result = TR_ONGOING;

	/* spin the event loop until we're done */

	while (!lws_service(ctx->lws_ctx, 1000) && laoa->result == TR_ONGOING)
		;
	if (laoa->result < 0) {
		return laoa->result;
	}
	return libteep_ta_store_install(ctx, io.out, io.out_len);
}
