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
	struct lws_context_creation_info info;
	static struct lws_context *context = NULL;
	struct lws_jwk jwk_privkey_tee;
	int temp_len = sizeof(temp_buf);
	struct lws_jws jws;
	struct lws_jwe jwe;
	struct lws_jose jose;
	static char sigbuf[200000];
	int sign_len = 0;
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

	lws_jose_init(&jose);
	lws_jws_init(&jws, &jwk_privkey_tee, context);
	lws_jwe_init(&jwe, context);
	n = lws_jwk_import(&jwe.jwk, NULL, NULL, tam_id_pubkey_jwk, strlen(tam_id_pubkey_jwk));
	if (n) {
		lwsl_err("lws init failed\n");
		return -1;
	}

	/* Encrypt json */

	lwsl_user("Encrypt\n");
	static const char *alg = "RSA1_5";
	static const char *enc = "A128CBC-HS256";
	n = lws_gencrypto_jwe_alg_to_definition(alg, &jwe.jose.alg);
	if (n) {
		lwsl_err("%s: alg_to_definition failed %s\n", __func__, alg);
		goto bail1;
	}
	n = lws_gencrypto_jwe_enc_to_definition(enc, &jwe.jose.enc_alg);
	if (n) {
		lwsl_err("%s: enc_to_definition failed %s\n", __func__, enc);
		goto bail1;
	}
	n = lws_jws_alloc_element(&jwe.jws.map, LJWS_JOSE,
			lws_concat_temp(temp_buf, temp_len),
			&temp_len, strlen(alg) +
			strlen(enc) + 32, 0);
	if (n) {
		lwsl_err("%s: temp space too small\n", __func__);
		goto bail1;
	}
	jwe.jws.map.len[LJWS_JOSE] = lws_snprintf(
			(char *)jwe.jws.map.buf[LJWS_JOSE], temp_len,
			"{\"alg\":\"%s\",\"enc\":\"%s\"}", alg, enc);

	// need manual padding
	// https://github.com/warmcat/libwebsockets/commit/63ad616941e080cbdb94f706e388b0cf8c5beb70#diff-69a3998d35803592c0e1c24b9c1b1757
	int pad = ((msg_len + 16) & ~15) - msg_len;
	memset((char*)msg + msg_len, pad, pad);
	msg_len += pad;

	jwe.jws.map.buf[LJWE_CTXT] = (void *)msg;
	jwe.jws.map.len[LJWE_CTXT] = msg_len;
	n = lws_gencrypto_bits_to_bytes(jwe.jose.enc_alg->keybits_fixed);
	if (lws_jws_randomize_element(context, &jwe.jws.map, LJWE_EKEY,
			lws_concat_temp(temp_buf, temp_len),
			&temp_len, n,
			LWS_JWE_LIMIT_KEY_ELEMENT_BYTES)) {
		lwsl_err("Problem getting random\n");
		goto bail1;
	}
	n = lws_jwe_encrypt(&jwe, lws_concat_temp(temp_buf, temp_len),
			&temp_len);
	if (n) {
		lwsl_err("%s: lws_jwe_encrypt failed %d\n", __func__, n);
		goto bail1;
	}
	n = lws_jwe_render_flattened(&jwe, (void *)sigbuf, sizeof(sigbuf));
	if (n < 0) {
		lwsl_err("%s: lws_jwe_render_flattened failed %d\n", __func__, n);
		goto bail1;
	}
	lwsl_user("Encrypt OK\n");
	sign_len = strlen((void *)sigbuf);

	/* sign json */

	lwsl_user("Sign\n");

	static const char *sign_alg = "RS256";

	if (lws_gencrypto_jws_alg_to_definition(sign_alg, &jose.alg)) {
		lwsl_err("alg_to_definition failed %s\n", sign_alg);
		return 1;
	}
	if (lws_jws_alloc_element(&jws.map, LJWS_JOSE,
			lws_concat_temp(temp_buf, temp_len),
			&temp_len, strlen(sign_alg) + 10, 0)) {
		lwsl_err("%s: temp space too small\n", __func__);
		return 1;
	}

	jws.map.len[LJWS_JOSE] = lws_snprintf((char *)jws.map.buf[LJWS_JOSE],
				temp_len, "{\"alg\":\"%s\"}", sign_alg);

	n = lws_jwk_import(&jwk_privkey_tee, NULL, NULL, tee_id_privkey_jwk, strlen(tee_id_privkey_jwk));
	if (n) {
		lwsl_err("lws jwk import failed\n");
		return -1;
	}

	jws.map.buf[LJWS_PYLD] = sigbuf;
	jws.map.len[LJWS_PYLD] = sign_len;

	if (lws_jws_encode_b64_element(&jws.map_b64, LJWS_PYLD,
			lws_concat_temp(temp_buf, temp_len),
			&temp_len, jws.map.buf[LJWS_PYLD],
			jws.map.len[LJWS_PYLD]))
		goto bail1;
	if (lws_jws_encode_b64_element(&jws.map_b64, LJWS_JOSE,
			lws_concat_temp(temp_buf, temp_len),
			&temp_len, jws.map.buf[LJWS_JOSE],
			jws.map.len[LJWS_JOSE]))
		goto bail1;

	/* prepare the space for the b64 signature in the map */

	if (lws_jws_alloc_element(&jws.map_b64, LJWS_SIG,
			lws_concat_temp(temp_buf, temp_len),
		    &temp_len, lws_base64_size(LWS_JWE_LIMIT_KEY_ELEMENT_BYTES), 0)) {
		lwsl_err("%s: temp space too small\n", __func__);
		goto bail1;
	}

	n = lws_jws_sign_from_b64(&jose, &jws,
			(char *)jws.map_b64.buf[LJWS_SIG],
			jws.map_b64.len[LJWS_SIG]);
	if (n < 0) {
		lwsl_err("%s: failed signing test packet\n", __func__);
		goto bail1;
	}

	/* set the actual b64 signature size */
	jws.map_b64.len[LJWS_SIG] = n;

	/* create the flattened representation */
	n = lws_jws_write_flattened_json(&jws, (void *)out, *out_len - 16);
	if (n < 0) {
		lwsl_err("%s: failed write flattened json\n", __func__);
		goto bail1;
	}
	*out_len = strlen(out);
	lwsl_user("Sign Ok %d\n", *out_len);

	return 0;
	
bail1:
	return -1;
}

int teep_message_unwrap_ta_image(const char *msg, int msg_len, char *out, int *out_len) {
	struct lws_context_creation_info info;
	static struct lws_context *context = NULL;
	struct lws_jwk jwk_pubkey_sp;
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

	lws_jws_init(&jws, &jwk_pubkey_sp, context);
	lws_jwe_init(&jwe, context);

	lwsl_user("Decrypt\n");

	n = lws_jwe_json_parse(&jwe, (void *)msg,
				msg_len,
				lws_concat_temp(temp_buf, temp_len), &temp_len);
	if (n < 0) {
		lwsl_err("%s: lws_jwe_json_parse failed\n", __func__);
		goto bail;
	}
	n = lws_jwk_import(&jwe.jwk, NULL, NULL, tee_id_privkey_jwk, strlen(tee_id_privkey_jwk));
	if (n < 0) {
		lwsl_err("%s: unable to import tee jwk\n", __func__);
		goto bail;
	}
	n = lws_jwe_auth_and_decrypt(&jwe, lws_concat_temp(temp_buf, temp_len), &temp_len);
	if (n < 0) {
		lwsl_err("%s: lws_jwe_auth_and_decrypt failed\n", __func__);
		goto bail;
	}
	lwsl_user("Decrypt OK: length %d\n", n);

	lwsl_user("Verify\n");
	n = lws_jwk_import(&jwk_pubkey_sp, NULL, NULL, sp_pubkey_jwk, strlen(sp_pubkey_jwk));
	if (n < 0) {
		lwsl_err("%s: unable to import tam jwk\n", __func__);
		goto bail;
	}
	n = lws_jws_sig_confirm_json(jwe.jws.map.buf[LJWE_CTXT], jwe.jws.map.len[LJWE_CTXT], &jws, &jwk_pubkey_sp, context, temp_buf, &temp_len);
	if (n < 0) {
		lwsl_err("%s: confirm rsa sig failed\n", __func__);
		goto bail1;
	}
	lwsl_user("Signature OK %d %d\n", n, jws.map.len[LJWS_PYLD]);

	if (jws.map.len[LJWS_PYLD] > *out_len) {
		lwsl_err("%s: output buffer is small (in, out) = (%d, %d)\n", __func__, jws.map.len[LJWS_PYLD], *out_len);
	}
	memcpy(out, jws.map.buf[LJWS_PYLD], jws.map.len[LJWS_PYLD]);
	*out_len = jws.map.len[LJWS_PYLD];
	n = 0;
bail1:
	lws_jwk_destroy(&jwk_pubkey_sp);
bail:
	lws_jws_destroy(&jws);
	lws_jwe_destroy(&jwe);
	return n;

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

	lwsl_user("Verify\n");
	n = lws_jwk_import(&jwk_pubkey_tam, NULL, NULL, tam_id_pubkey_jwk, strlen(tam_id_pubkey_jwk));
	if (n < 0) {
		lwsl_err("%s: unable to import tam jwk\n", __func__);
		goto bail;
	}
	n = lws_jws_sig_confirm_json(msg, msg_len, &jws, &jwk_pubkey_tam, context, temp_buf, &temp_len);
	if (n < 0) {
		lwsl_err("%s: confirm rsa sig failed\n", __func__);
		goto bail1;
	}
	lwsl_user("Signature OK %d %d\n", n, jws.map.len[LJWS_PYLD]);

	if (jws.map.len[LJWS_PYLD] > *out_len) {
		lwsl_err("%s: output buffer is small (in, out) = (%d, %d)\n", __func__, jws.map.len[LJWS_PYLD], *out_len);
	}

	lwsl_user("Decrypt\n");

	n = lws_jwe_json_parse(&jwe, (void *)jws.map.buf[LJWS_PYLD],
				jws.map.len[LJWS_PYLD],
				lws_concat_temp(temp_buf, temp_len), &temp_len);
	if (n < 0) {
		lwsl_err("%s: lws_jwe_json_parse failed\n", __func__);
		goto bail;
	}

	n = lws_jwk_import(&jwe.jwk, NULL, NULL, tee_id_privkey_jwk, strlen(tee_id_privkey_jwk));
	if (n < 0) {
		lwsl_err("%s: unable to import tee jwk\n", __func__);
		goto bail;
	}
	n = lws_jwe_auth_and_decrypt(&jwe, lws_concat_temp(temp_buf, temp_len), &temp_len);
	if (n < 0) {
		lwsl_err("%s: lws_jwe_auth_and_decrypt failed\n", __func__);
		goto bail;
	}
	lwsl_user("Decrypt OK: length %d\n", n);

	memcpy(out, jwe.jws.map.buf[LJWE_CTXT], jwe.jws.map.len[LJWE_CTXT]);
	*out_len = jwe.jws.map.len[LJWE_CTXT];
	n = 0;
bail1:
	lws_jwk_destroy(&jwk_pubkey_tam);
bail:
	lws_jws_destroy(&jws);
	lws_jwe_destroy(&jwe);
	return n;
}
