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

#include <libwebsockets.h>
#include "http.h"

typedef enum tam_result {
	TR_ONGOING		=  1,
	TR_OKAY			=  0,
	TR_FAIL_START		= -1,
	TR_FAIL_CONN_ERR	= -2,
	TR_FAIL_REFUSED		= -3,
	TR_FAIL_OVERSIZE	= -4,
	TR_FAIL_CLOSED		= -5,
} tam_result;

struct lao_rpc_io {
       void    *in;
       size_t  in_len;
       void    *out;
       size_t  out_len;
};

struct libteep_async {
//	struct libteep_async	*laoa_list_next; /* protected by .lock */
//	struct libteep_ctx	*ctx;
	struct lao_rpc_io	*io;
	struct lws		*wsi;
	size_t			max_out_len;
	int			http_resp;
	tam_result		result;
};

static int
callback_teep(struct lws *wsi, enum lws_callback_reasons reason, void *user,
	     void *in, size_t len)
{
	struct libteep_async *laoa = (struct libteep_async *)user;
	char buffer[1024 + LWS_PRE];
	char *px = buffer + LWS_PRE;
	int lenx = sizeof(buffer) - LWS_PRE;

	lwsl_debug("%s: reason=%d\n", __func__, reason);
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
			const char *media_type = "application/teep+cbor";
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

	lwsl_debug("%s: reason=%d\n", __func__, reason);
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

#if 0
static int do_http(int is_get, const char *uri, void *out, size_t *out_len)
{
    char *uri_tmp = NULL;
    char *path = NULL;

    uri_tmp = strdup(uri);
    if (!uri_tmp) {
        goto err;
    }
    const char *proto;
	const char *address;
	int port;
	const char *path_slash_dropped;

	if (lws_parse_uri(uri_tmp, &proto, &address, &port, &path_slash_dropped)) {
		fprintf(stderr, "%s: Failed to parse URL %s\n", __func__, uri);
		goto err;
	}
    path = malloc(strlen(path_slash_dropped) + 2);
    if (!path) {
        goto err;
    }
    path[0] = '/';
    strcpy(path + 1, path_slash_dropped);

	struct lws_client_connect_info conn_info = {
		.context = ctx->lws_ctx,
		.address = address,
		.port = port,
		.path = path,
		.method = "POST",
		.userdata = laoa,
		.protocol = protocols[0].name,
	};



err:
    if (path) free(path);
    if (uri_tmp) free(path);
    return -1;
}
#endif

int http_get(const char *url, void *out, size_t *out_len)
{
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
	info.port = CONTEXT_PORT_NO_LISTEN;
	info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
	info.protocols = protocols;

	struct lws_context *lws_ctx = lws_create_context(&info);
    if (!lws_ctx) {
        return -1;
    }

	char tmp_url[200];
	const char *proto;
	const char *address;
	int port;
	const char *path_slash_dropped;

	strncpy(tmp_url, url, sizeof(tmp_url));
	if (lws_parse_uri(tmp_url, &proto, &address, &port, &path_slash_dropped)) {
		fprintf(stderr, "%s: Failed to parse tam URL base %s\n", __func__, url);
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
		.out = out,
		.out_len = 0
	};
	laoa->io = &io;
	laoa->max_out_len = *out_len;
	laoa->wsi = NULL;

#if 0
	pthread_mutex_lock(&ctx->lock); /* ++++++++++++++++++++++++ ctx->lock */
	laoa->laoa_list_next = ctx->laoa_list_head;
	ctx->laoa_list_head = laoa;
	pthread_mutex_unlock(&ctx->lock); /* ctx->lock ---------------------- */
#endif

	struct lws_client_connect_info conn_info = {
		.context = lws_ctx,
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

	while (!lws_service(lws_ctx, 1000) && laoa->result == TR_ONGOING)
		;
	if (laoa->result < 0) {
		return laoa->result;
	}
    *out_len = io.out_len;
    return 0;
}

int http_post(const char *url, const void *in, size_t in_len, void *out, size_t *out_len)
{
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
	info.port = CONTEXT_PORT_NO_LISTEN;
	info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
	info.protocols = protocols;

	struct lws_context *lws_ctx = lws_create_context(&info);
    if (!lws_ctx) {
        return -1;
    }

 	char tmp_url[200];
	const char *proto;
	const char *address;
	int port;
	const char *path_slash_dropped;

	strncpy(tmp_url, url, sizeof(tmp_url));
	if (lws_parse_uri(tmp_url, &proto, &address, &port, &path_slash_dropped)) {
		fprintf(stderr, "%s: Failed to parse tam URL base %s\n", __func__, url);
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
		.in = (void *)in,
		.in_len = in_len,
		.out = out,
		.out_len = 0
	};
	laoa->io = &io;
	laoa->max_out_len = *out_len;
	laoa->wsi = NULL;

#if 0
	pthread_mutex_lock(&ctx->lock); /* ++++++++++++++++++++++++ ctx->lock */
	laoa->laoa_list_next = ctx->laoa_list_head;
	ctx->laoa_list_head = laoa;
	pthread_mutex_unlock(&ctx->lock); /* ctx->lock ---------------------- */
#endif

	struct lws_client_connect_info conn_info = {
		.context = lws_ctx,
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

	while (!lws_service(lws_ctx, 1000) && laoa->result == TR_ONGOING)
		;
	if (laoa->result < 0) {
		return laoa->result;
	}
    *out_len = io.out_len;
    return 0;
}

