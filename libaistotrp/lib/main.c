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
 * This is the libaistotrp REE userland library.
 *
 * It provides services for REE Applications to communicate with a remote TAM,
 * and the OTrP PTA in the Secure World.
 *
 * The return convention is 0 means all is OK.  Nonzero indicates an error.
 */

#include "libaistotrp.h"
#include <libwebsockets.h>
#include <pthread.h>

static const TEEC_UUID uuid_aist_otrp_ta =
        { 0x68373894, 0x5bb3, 0x403c,
                { 0x9e, 0xec, 0x31, 0x14, 0xa1, 0xf5, 0xd3, 0xfc } };

/* this is wholly opaque to library users */

struct libaistotrp_ctx {
	TEEC_Context		 tee_context;
	TEEC_Session		 tee_session;
	pthread_mutex_t 	 lock;
	struct lws_context	 *lws_ctx;
	struct libaistotrp_async *laoa_list_head; /* protected by .lock */
	char			 *tam_url_base;
	int			 tam_port;
	const char		 *tam_address;
	const char		 *tam_protocol;
	const char		 *tam_path;
};

/* This is the user-opaque internal representation of an ongoing TAM message */

//static unsigned int rrlen;
//static uint8_t *rr;

struct libaistotrp_async {
	struct libaistotrp_async *laoa_list_next; /* protected by .lock */
	struct libaistotrp_ctx	 *ctx;
	struct lao_rpc_io	 *io;
	struct lws		 *wsi;

	size_t			 max_out_len;

	int			 http_resp;
	tam_result		 result;
};


static int
callback_tam(struct lws *wsi, enum lws_callback_reasons reason, void *user,
	     void *in, size_t len)
{
	struct libaistotrp_async *laoa = (struct libaistotrp_async *)user;
	char buffer[1024 + LWS_PRE];
	char *px = buffer + LWS_PRE;
	int lenx = sizeof(buffer) - LWS_PRE;

	switch (reason) {
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		lwsl_notice("%s: CONNECTION_ERROR: %d\n", __func__, TR_FAIL_CONN_ERR);
		laoa->result = TR_FAIL_CONN_ERR;
		break;
	case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
		if (laoa->result == TR_ONGOING)
			laoa->result = TR_FAIL_CLOSED;
		break;
	case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
		laoa->http_resp = lws_http_client_http_response(wsi);
		lwsl_notice("%s: established, resp %d\n", __func__,
				laoa->http_resp);
#if 0
		if (laoa->http_resp != 200) {
			laoa->result = TR_FAIL_REFUSED;

			return -1;
		}
#endif
		break;
	case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
	    {
		unsigned char **p = (unsigned char **)in, *end = (*p) + len;
		char *c_type_str = "application/otrp+json";

		if (lws_add_http_header_by_name(wsi,
					(const unsigned char *)"Accept:",
					(const unsigned char *)c_type_str,
					strlen(c_type_str), p, end)) {
			lwsl_notice("%s: Append header error\n", __func__);
			return -1;
		}
	    }
		break;
	case LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
		lwsl_notice("%s: completed; read %d\n", __func__,
			    (int)laoa->io->out_len);
		lwsl_notice("\n%s\n", (char *)(laoa->io->out));
		laoa->result = TR_OKAY;
		break;
	case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ:
		if (laoa->io->out_len + len > laoa->max_out_len) {
			lwsl_notice("%s: too large\n", __func__);
			laoa->result = TR_FAIL_OVERSIZE;
			return -1;
		}
		memcpy(laoa->io->out + laoa->io->out_len, in, len);
		laoa->io->out_len += len;
		break;
	case LWS_CALLBACK_RECEIVE_CLIENT_HTTP:
		if (lws_http_client_read(wsi, &px, &lenx) < 0)
			return -1;
		break;
	case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
		break;
#if 0
	case LWS_CALLBACK_RECEIVE_CLIENT_HTTP:
		px = buffer + LWS_PRE;
		lenx = sizeof(buffer) - LWS_PRE;

		if (lws_http_client_read(wsi, &px, &lenx) < 0) {
			lwsl_notice("lws_http_client_read returned < 0\n");
			return -1;
		}
		break;
	case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
		if (wsi == wsi_report) {
			lwsl_user("report LWS_CALLBACK_CLIENT_HTTP_WRITEABLE\n");
			/* we can be sure it was sent */
			if (!rrlen) {
				lwsl_user("report LWS_CALLBACK_CLIENT_HTTP_WRITEABLE\n");
                                return -1;
			}
			lws_write(wsi, rr, rrlen, LWS_WRITE_HTTP_FINAL);
			rrlen = 0;
			lws_client_http_body_pending(wsi, 0);
			lws_callback_on_writable(wsi);
		}
		break
#endif

	default:
		break;
	}
	return lws_callback_http_dummy(wsi, reason, user, in, len);
}

static const struct lws_protocols protocols[] = {
	{ "tam", callback_tam, 0, 4096, },
	{ "application/teep+json", callback_tam, 0, 4096, },
	{ NULL, NULL, 0, 0 }
};

int
libaistotrp_tam_msg(struct libaistotrp_ctx *ctx, const char *urlinfo,
		    struct lao_rpc_io *io)
{
	struct libaistotrp_async *laoa = malloc(sizeof(*laoa));
	struct lws_client_connect_info i;
	char path[200];

	if (!laoa)
		return -1; /* OOM */

	memset(laoa, 0, sizeof(struct libaistotrp_async));

	laoa->ctx = ctx;
	laoa->io = io;
	laoa->max_out_len = io->out_len;
	io->out_len = 0;
	laoa->wsi = NULL;

	pthread_mutex_lock(&ctx->lock); /* ++++++++++++++++++++++++ ctx->lock */
	laoa->laoa_list_next = ctx->laoa_list_head;
	ctx->laoa_list_head = laoa;
	pthread_mutex_unlock(&ctx->lock); /* ctx->lock ---------------------- */

	memset(&i, 0, sizeof(i));

	i.context = ctx->lws_ctx;
	if (!strcmp(ctx->tam_protocol, "https"))
		i.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED;

	i.port = ctx->tam_port;
	i.address = ctx->tam_address;
	i.alpn = "http/1.1";
	memset(path, 0, sizeof(path));
	if (ctx->tam_path[strlen(ctx->tam_path) - 1] == '/') {
		lws_snprintf(path, sizeof(path),
				"/TEEP HTTP/1.1\r\n"
				"Host: example.com\r\n"
				"Accept: application/otrp+json\r\n"
				"Content-Type: application/otrp+json\r\n"
				"Content-Length: 0\r\n"
				);
	} else {
		lws_snprintf(path, sizeof(path),
				"%s/%s", ctx->tam_path,  urlinfo);
	}

#if 1
	i.path = "/api/tam";
	i.host = "example.com";
	i.origin = "192.168.11.3";
	i.method = "POST";
	i.userdata = laoa;
	i.protocol = protocols[1].name;
#else

	i.path = path;
	i.host = i.address;
	i.origin = i.address;
//	i.method = "GET";
	i.method = "POST";
	i.userdata = laoa;
	i.protocol = protocols[0].name;
#endif
	i.pwsi = &laoa->wsi;

//	char *c_type = "application/teep+json";

//	lws_client_http_multipart(i.pwsi, NULL, NULL, c_type, i.userdata, i.userdata + 200);

	lwsl_notice(	"\n"
			"%s://%s:%d%s\n"
			, ctx->tam_protocol, ctx->tam_address,
					   ctx->tam_port, i.path);

	if (!lws_client_connect_via_info(&i)) {
		lwsl_err("%s: connect failed\n", __func__);
		return 1;
	}

	laoa->result = TR_ONGOING;

	/* spin the event loop until we're done */

	while (!lws_service(ctx->lws_ctx, 1000) && laoa->result == TR_ONGOING)
		;

#if 0
	char str[200];
	char ta_path[200];

	sleep(1);

	memset(laoa, 0, sizeof(struct libaistotrp_async));

	laoa->ctx = ctx;
	laoa->io = io;
	laoa->max_out_len = io->out_len;
	io->out_len = 0;
	laoa->wsi = NULL;

	pthread_mutex_lock(&ctx->lock); /* ++++++++++++++++++++++++ ctx->lock */
	laoa->laoa_list_next = ctx->laoa_list_head;
	ctx->laoa_list_head = laoa;
	pthread_mutex_unlock(&ctx->lock); /* ctx->lock ---------------------- */

	memset(&i, 0, sizeof(i));

	i.context = ctx->lws_ctx;
	if (!strcmp(ctx->tam_protocol, "https"))
		i.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED;

	i.port = ctx->tam_port;
	i.address = ctx->tam_address;
	i.alpn = "http/1.1";
	if (ctx->tam_path[strlen(ctx->tam_path) - 1] == '/') {
		lws_snprintf(str, sizeof(str),
				"/api/tam HTTP/1.1\r\n"
				"Host: example.com\r\n"
				"Accept: application/otrp+json\r\n"
				"Content-Type: application/otrp+json\r\n"
				);
	} else {
		lws_snprintf(str, sizeof(str),
				"/api/tam HTTP/1.1\r\n"
				"%s/%s", ctx->tam_path,  urlinfo);
	}

	lws_snprintf(ta_path, sizeof(ta_path),
				"{ \"taname\": \"%s\" }"
//				"{ \"taname\": \"dummy\" }"
//				"hi"
				, urlinfo
		    );

	lws_snprintf(path, sizeof(path),
				"%s"
				"Content-Length: %ld\r\n"
//				"Content-Length: 0\r\n"
				"%s\n"
				, str
				, strlen(ta_path)
				, ta_path
		    );

	i.path = path;
	i.host = i.address;
	i.origin = i.address;
//	i.method = "GET";
	i.method = "POST";
	i.userdata = laoa;
	i.protocol = protocols[0].name;
	i.pwsi = &laoa->wsi;

	lwsl_notice(	"\n"
			"%s://%s:%d%s\n"
			, ctx->tam_protocol, ctx->tam_address,
					   ctx->tam_port, i.path);

	if (!lws_client_connect_via_info(&i)) {
//		lwsl_err("%s: connect failed\n", __func__);
//		return 1;
	}

	laoa->result = TR_ONGOING;

	/* spin the event loop until we're done */

	while (!lws_service(ctx->lws_ctx, 1000) && laoa->result == TR_ONGOING)
		;
#endif

	return laoa->result;
}

int
libaistotrp_pta_msg(struct libaistotrp_ctx *ctx, uint32_t cmd,
		    struct lao_rpc_io *io)
{
	TEEC_Result n;
	TEEC_Operation op;
	int m;

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
		fprintf(stderr, "%s: TEEC_InvokeCommand "
		        "failed (0x%08x)\n", __func__, n);
		return (int)n;
	}

	io->out_len = op.params[3].value.a;
	for (m = 0; m < op.params[3].value.a; m++)
		fprintf(stderr, "%02X", ((uint8_t *)io->out)[m]);

	fprintf(stderr, "\n");

	return n;
}

int
libaistotrp_init(struct libaistotrp_ctx **ctx, const char *tam_url_base)
{
	struct lws_context_creation_info info;
	TEEC_Operation op;
	TEEC_Result r;

	/* allocate an opaque libaistotrp context to the caller */

	*ctx = malloc(sizeof(**ctx));
	if (!(*ctx)) {
		printf("OOM\n");

		return 1;
	}

	memset(*ctx, 0, sizeof(**ctx));
	(*ctx)->tam_url_base = strdup(tam_url_base);
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

	memset(&info, 0, sizeof info);
	info.port = CONTEXT_PORT_NO_LISTEN;
	info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
	info.protocols = protocols;

	(*ctx)->lws_ctx = lws_create_context(&info);

	if (!(*ctx)->lws_ctx)
		goto bail3;

	if (lws_parse_uri((*ctx)->tam_url_base, &(*ctx)->tam_protocol,
			  &(*ctx)->tam_address, &(*ctx)->tam_port,
			  &(*ctx)->tam_path)) {
		fprintf(stderr, "%s: Failed to parse tam URL base %s\n",
			__func__, tam_url_base);

		goto bail4;
	}

	return 0;

bail4:
	lws_context_destroy((*ctx)->lws_ctx);
	(*ctx)->lws_ctx = NULL;
bail3:
	TEEC_CloseSession(&(*ctx)->tee_session);
bail2:
	TEEC_FinalizeContext(&(*ctx)->tee_context);
bail1:
	free((*ctx)->tam_url_base);
	pthread_mutex_destroy(&(*ctx)->lock);

	free(*ctx);
	*ctx = NULL;

	return 1;
}

void
libaistotrp_destroy(struct libaistotrp_ctx **ctx)
{
	TEEC_CloseSession(&(*ctx)->tee_session);
	TEEC_FinalizeContext(&(*ctx)->tee_context);
	lws_context_destroy((*ctx)->lws_ctx);
	(*ctx)->lws_ctx = NULL;

	pthread_mutex_destroy(&(*ctx)->lock);

	free(*ctx);
	*ctx = NULL;
}
