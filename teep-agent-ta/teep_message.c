/*
 * Copyright (C) 2017 - 2019 National Institute of Advanced Industrial Science
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
#include "teep_message.h"
#define TEMP_BUF_SIZE (800 * 1024)

static char temp_buf[TEMP_BUF_SIZE];

/* the TAM server public key as a JWK */
static const char * const tam_id_pubkey_jwk =
#include "tam_id_pubkey_jwk.h"
;

/* the TEE private key as a JWK */
static const char * const tee_id_privkey_jwk =
#include "tee_id_privkey_jwk.h"
;

/* SP public key as a JWK */
static const char * const sp_pubkey_jwk =
#include "sp_pubkey_jwk.h"
;

int
teep_message_wrap(const char *msg, int msg_len, unsigned char *out, unsigned int *out_len) {
	lwsl_err("%s: TODO implementation", __func__);
	return 0;
}

int
teep_message_unwrap(const char *msg, int msg_len, unsigned char *out, unsigned int *out_len) {
	struct lws_context_creation_info info;
	static struct lws_context *context = NULL;
	struct lws_jwk jwk_pubkey_tam;
	int temp_len = sizeof(temp_buf);
	struct lws_jws jws;
	struct lws_jwe jwe;
	int n = 0;
	lwsl_user("%s: msg len %d\n", __func__, msg_len);
	memset(&info, 0, sizeof(info));
	info.port = CONTEXT_PORT_NO_LISTEN;
	info.options = 0;
#ifdef PCTEST
	// calling lws_create_context on tee environment causes a lot of link error
	// lws_create_context must be called in pc environment to avoid SEGV on decrypt
	context = lws_create_context(&info);
	if (!context) {
		lwsl_err("lws init failed\n");
		return -1;
	}
#endif

	lws_jws_init(&jws, &jwk_pubkey_tam, context);
	lws_jwe_init(&jwe, context);

	/* JWS payload is a JWE */

	lwsl_user("Decrypt\n");

	n = lws_jwe_json_parse(&jwe, (void *)msg,
				msg_len,
				lws_concat_temp(temp_buf, temp_len), &temp_len);
	if (n < 0) {
		lwsl_err("%s: lws_jwe_json_parse failed\n", __func__);
		goto bail1;
	}
	n = lws_jwk_import(&jwe.jwk, NULL, NULL, tee_id_privkey_jwk, strlen(tee_id_privkey_jwk));
	if (n < 0) {
		lwsl_err("%s: unable to import tee jwk\n", __func__);
		goto bail1;
	}
	n = lws_jwe_auth_and_decrypt(&jwe, lws_concat_temp(temp_buf, temp_len), &temp_len);
	if (n < 0) {
		lwsl_err("%s: lws_jwe_auth_and_decrypt failed\n", __func__);
		goto bail1;
	}
	lwsl_user("Decrypt OK: length %d\n", n);

	lwsl_user("Verify\n");
	n = lws_jwk_import(&jwk_pubkey_tam, NULL, NULL, tam_id_pubkey_jwk, strlen(tam_id_pubkey_jwk));
	if (n < 0) {
		lwsl_err("%s: unable to import tam jwk\n", __func__);
		goto bail;
	}
	n = lws_jws_sig_confirm_json(jwe.jws.map.buf[LJWE_CTXT], jwe.jws.map.len[LJWE_CTXT], &jws, &jwk_pubkey_tam, context, temp_buf, &temp_len);
	if (n < 0) {
		lwsl_err("%s: confirm rsa sig failed\n", __func__);
		goto bail;
	}
	lwsl_user("Signature OK %d %d\n", n, jws.map.len[LJWS_PYLD]);

	if (jws.map.len[LJWS_PYLD] > *out_len) {
		lwsl_err("%s: output buffer is small (in, out) = (%d, %d)\n", __func__, jws.map.len[LJWS_PYLD], *out_len);
	}
	memcpy(out, jws.map.buf[LJWS_PYLD], jws.map.len[LJWS_PYLD]);
	*out_len = jws.map.len[LJWS_PYLD];
	n = 0;
bail1:
	lws_jwe_destroy(&jwe);
bail:
	lws_jwk_destroy(&jwk_pubkey_tam);
	lws_jws_destroy(&jws);
	return n;
}
