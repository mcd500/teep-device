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
#include "ta-store.h"
#define TEMP_BUF_SIZE (800 * 1024)

char *strncat(char *dest, const char *src, size_t n);

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
teep_message_wrap(const char *msg, int msg_len, char *out, uint32_t *out_len) {
	struct lws_context_creation_info info;
	static struct lws_context *context = NULL;
	struct lws_jwk jwk_privkey_tee;
	int temp_len = TEMP_BUF_SIZE - 1;
	struct lws_jws jws;
	struct lws_jwe jwe;
	struct lws_jose jose;
	#define MSG_LEN 200000
	static char msgbuf[MSG_LEN];
	static char encbuf[MSG_LEN];
	int enc_len = sizeof(encbuf) - 1;
	int n = 0;

	lwsl_user("%s: msg len %d\n", __func__, msg_len);
	*temp_buf = '0';
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
	if (msg_len > MSG_LEN - 1) {
		lwsl_err("%s: msg_len too long %d\n", __func__, msg_len);
		goto bail1;
	}
	memcpy(msgbuf, msg, msg_len);
	int pad = ((msg_len + 16) & ~15) - msg_len;
	memset(msgbuf + msg_len, pad, pad);
	msg_len += pad;

	jwe.jws.map.buf[LJWE_CTXT] = (void *)msgbuf;
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
	n = lws_jwe_render_flattened(&jwe, (void *)encbuf, enc_len);

	if (n < 0) {
		lwsl_err("%s: lws_jwe_render_flattened failed %d\n", __func__, n);
		goto bail1;
	}

	enc_len = strlen((void *)encbuf);
	lwsl_user("Encrypt OK %d\n", enc_len);

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

	jws.map.buf[LJWS_PYLD] = encbuf;
	jws.map.len[LJWS_PYLD] = enc_len;


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

	n = lws_jws_write_flattened_json(&jws, (void *)out, *out_len);

	if (n < 0) {
		lwsl_err("%s: failed write flattened json\n", __func__);
		goto bail1;
	}
	*out_len = strlen((void *)out);
	lwsl_user("Sign Ok %u\n", *out_len);

	return 0;
	
bail1:
	return -1;
}

int
otrp_message_sign(const char *msg, int msg_len, char *out, uint32_t *out_len) {
	struct lws_context_creation_info info;
	static struct lws_context *context = NULL;
	struct lws_jwk jwk_privkey_tee;
	int temp_len = TEMP_BUF_SIZE - 1;
	struct lws_jws jws;
	struct lws_jose jose;
	int n = 0;

	lwsl_user("%s: msg len %d\n", __func__, msg_len);
	*temp_buf = '0';
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

	jws.map.buf[LJWS_PYLD] = msg;
	jws.map.len[LJWS_PYLD] = msg_len;


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

	/* sign the plaintext */

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
	*out_len = strlen((void *)out);
	lwsl_user("Sign Ok %u\n", *out_len);

	return 0;
	
bail1:
	return -1;
}

int
otrp_message_encrypt(const char *msg, int msg_len, char *out, uint32_t *out_len) {
	struct lws_context_creation_info info;
	static struct lws_context *context = NULL;
	int temp_len = TEMP_BUF_SIZE - 1;
	struct lws_jwe jwe;
	struct lws_jose jose;
	int n = 0;

	lwsl_user("%s: msg len %d\n", __func__, msg_len);
	*temp_buf = '0';
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
	lws_jwe_init(&jwe, context);
	n = lws_jwk_import(&jwe.jwk, NULL, NULL, tam_id_pubkey_jwk, strlen(tam_id_pubkey_jwk));
	if (n) {
		lwsl_err("lws init failed\n");
		return -1;
	}
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
	n = lws_jwe_render_flattened(&jwe, (void *)out, *out_len);
	if (n < 0) {
		lwsl_err("%s: lws_jwe_render_flattened failed %d\n", __func__, n);
		goto bail1;
	}
	lwsl_user("Encrypt OK\n");
	*out_len = strlen((void *)out);
	return 0;
	
bail1:
	return -1;
}

int teep_message_unwrap_ta_image(const char *msg, int msg_len, char *out, uint32_t *out_len) {
	struct lws_context_creation_info info;
	static struct lws_context *context = NULL;
	struct lws_jwk jwk_pubkey_sp;
	int temp_len = TEMP_BUF_SIZE - 1;
	struct lws_jws jws;
	struct lws_jwe jwe;
	int n = 0;

	lwsl_user("%s: msg len %d\n", __func__, msg_len);
	*temp_buf = '0';
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
		lwsl_err("%s: output buffer is small (in, out) = (%d, %u)\n", __func__, jws.map.len[LJWS_PYLD], *out_len);
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
teep_message_unwrap(const char *msg, int msg_len, char *out, uint32_t *out_len) {
	struct lws_context_creation_info info;
	static struct lws_context *context = NULL;
	struct lws_jwk jwk_pubkey_tam;
	int temp_len = TEMP_BUF_SIZE - 1;
	struct lws_jws jws;
	struct lws_jwe jwe;
	int n = 0;

	lwsl_user("%s: msg len %d\n", __func__, msg_len);
	*temp_buf = '0';
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
	lwsl_user("msg=%s len=%d\n", msg, msg_len);
	lwsl_user("len=%d\n", msg_len);
	lwsl_user("temp=%s len=%d\n", temp_buf, temp_len);
	n = lws_jws_sig_confirm_json(msg, msg_len, &jws, &jwk_pubkey_tam, context, temp_buf, &temp_len);
	if (n < 0) {
		lwsl_err("%s: confirm rsa sig failed\n", __func__);
		goto bail1;
	}
	lwsl_user("Signature OK %d %d\n", n, jws.map.len[LJWS_PYLD]);

	if (jws.map.len[LJWS_PYLD] > *out_len) {
		lwsl_err("%s: output buffer is small (in, out) = (%d, %u)\n", __func__, jws.map.len[LJWS_PYLD], *out_len);
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

int
otrp_message_verify(const char *msg, int msg_len, char *out, uint32_t *out_len) {
	struct lws_context_creation_info info;
	static struct lws_context *context = NULL;
	struct lws_jwk jwk_pubkey_tam;
	int temp_len = TEMP_BUF_SIZE - 1;
	struct lws_jws jws;
	int n = 0;

	lwsl_user("%s: msg len %d\n", __func__, msg_len);
	*temp_buf = '0';
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

	lwsl_user("Verify\n");
	n = lws_jwk_import(&jwk_pubkey_tam, NULL, NULL, tam_id_pubkey_jwk, strlen(tam_id_pubkey_jwk));
	if (n < 0) {
		lwsl_err("%s: unable to import tam jwk\n", __func__);
		goto bail;
	}
	n = lws_jws_sig_confirm_json((void *)msg,	msg_len, &jws, &jwk_pubkey_tam, context, temp_buf, &temp_len);
	if (n < 0) {
		lwsl_err("%s: confirm rsa sig failed\n", __func__);
		goto bail1;
	}
	lwsl_user("Signature OK %d %d\n", n, jws.map.len[LJWS_PYLD]);

	if (jws.map.len[LJWS_PYLD] > *out_len) {
		lwsl_err("%s: output buffer is small (in, out) = (%d, %u)\n", __func__, jws.map.len[LJWS_PYLD], *out_len);
	}
	memcpy(out, jws.map.buf[LJWS_PYLD], jws.map.len[LJWS_PYLD]);
	*out_len = jws.map.len[LJWS_PYLD];
	out[*out_len] = '\0';
	n = 0;
bail1:
	lws_jwk_destroy(&jwk_pubkey_tam);
bail:
	lws_jws_destroy(&jws);
	return n;
}

int
otrp_message_decrypt(const char *msg, int msg_len, char *out, uint32_t *out_len) {
	struct lws_context_creation_info info;
	static struct lws_context *context = NULL;
	int temp_len = TEMP_BUF_SIZE - 1;
	struct lws_jwe jwe;
	int n = 0;

	lwsl_user("%s: msg len %d\n", __func__, msg_len);
	*temp_buf = '0';
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
	memcpy(out, jwe.jws.map.buf[LJWE_CTXT], jwe.jws.map.len[LJWE_CTXT]);
	*out_len = jwe.jws.map.len[LJWE_CTXT];
	n = 0;
bail:
	lws_jwe_destroy(&jwe);
	return n;
}

static char teep_req_buf[5 * 1024];
static char teep_res_buf[5 * 1024];
static char ta_list_buf[256];

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
	if (!strcmp(ctx->path, "TA_LIST[]")) {
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

enum teep_message_type {
	QUERY_REQUEST = 1,
	QUERY_RESPONSE = 2,
	TRUSTED_APP_INSTALL = 3,
	TRUSTED_APP_DELETE = 4,
	SUCCESS = 5,
	ERROR = 6
};

int
teep_agent_message(int jose, const char *msg, int msg_len, char *out, uint32_t *out_len, char *ta_url_list, uint32_t *ta_url_list_len)
{
	lwsl_notice("teep_agent_message\n");
	int n;
	if (jose) {
		uint32_t len = sizeof (teep_req_buf);
		n = teep_message_unwrap(msg, msg_len, teep_req_buf, &len);
		if (n >= 0) n = len;
	} else {
		if (sizeof (teep_req_buf) < msg_len) {
			n = -1;
		} else {
			memmove(teep_req_buf, msg, msg_len);
			n = msg_len;
		}
	}
	if (n < 0) {
		lwsl_err("%s: unwrap_teep_request: fail %d\n", __func__, n);
		return n;
	}
	if (n == 0) {
		lwsl_notice("%s: received encrypted empty body\n", __func__);
		*out_len = 0;
		return 0;
	}
	lwsl_notice("%s: received message: %*s\n", __func__, n, (char *)teep_req_buf);

	static struct manifest_list manifests = {.len = 0};
	static struct ta_list tas = {.len = 0};
	struct lejp_ctx jp_ctx;
	static char token[100] = "";
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
			static char ta_id_list[1000] = "";
			char *p = ta_list_buf;
			while (*p) {
				static char tmp[300];
				lws_snprintf(tmp, sizeof(tmp),
					"\"%s\",", p);
				strncat(ta_id_list, tmp, sizeof(ta_id_list) - strlen(ta_id_list));
				p += strlen(p) + 1;
			}
			ta_id_list[strlen(ta_id_list) - 1] = '\0';
			lws_snprintf(teep_res_buf, sizeof(teep_res_buf), 
				"{\"TYPE\":%d,\"TOKEN\":\"%s\",\"TA_LIST\":[%s]}", QUERY_RESPONSE, m.token, ta_id_list);
		}
		lwsl_notice("json: %s, len: %zd\n", teep_res_buf, strlen(teep_res_buf));

		if (jose) {
			n = teep_message_wrap(teep_res_buf, strlen(teep_res_buf), out, out_len);
		} else {
			if (*out_len < strlen(teep_res_buf)) {
				n = -1;
			} else {
				memmove(out, teep_res_buf, strlen(teep_res_buf));
				*out_len = strlen(teep_res_buf);
				n = *out_len;
			}
		}
		if (n < 0) {
			lwsl_err("%s: wrap_teep_response failed %d\n", __func__, n);
			return n;
		}
		lwsl_notice("body: %s, len: %zd\n", out, (size_t)*out_len);
		break;
	case TRUSTED_APP_INSTALL:
		lwsl_notice("detect TRUSTED_APP_INSTALL\n");
		lejp_construct(&jp_ctx, parse_manifest_list, &manifests, NULL, 0);
		lejp_parse(&jp_ctx, (void *)teep_req_buf, n);
		lejp_destruct(&jp_ctx);

		lwsl_notice("send SUCCESS\n");
		lws_snprintf(teep_res_buf, sizeof(teep_res_buf), 
			"{\"TYPE\":%d,\"TOKEN\":\"%s\"}", SUCCESS, m.token);
		lwsl_notice("json: %s, len: %zd\n", teep_res_buf, strlen(teep_res_buf));

		if (jose) {
			n = teep_message_wrap(teep_res_buf, strlen(teep_res_buf), out, out_len);
		} else {
			if (*out_len < strlen(teep_res_buf)) {
				n = -1;
			} else {
				memmove(out, teep_res_buf, strlen(teep_res_buf));
				*out_len = strlen(teep_res_buf);
				n = *out_len;
			}
		}
		if (n < 0) {
			lwsl_err("%s: wrap_teep_response failed, %d", __func__, n);
			return n;
		}
		break;
	case TRUSTED_APP_DELETE:
		lwsl_notice("detect TRUSTED_APP_DELETE\n");
		lejp_construct(&jp_ctx, parse_ta_list, &tas, NULL, 0);
		lejp_parse(&jp_ctx, (void *)teep_req_buf, n);
		lejp_destruct(&jp_ctx);
		for (int i = 0; i < tas.len; i++) {
			ta_store_delete(tas.uuid[i], strlen(tas.uuid[i]));
		}
		lwsl_notice("send SUCCESS\n");
		lws_snprintf(teep_res_buf, sizeof(teep_res_buf),
			"{\"TYPE\":%d,\"TOKEN\":\"%s\"}", SUCCESS, m.token);
		lwsl_notice("json: %s, len: %zd\n", teep_res_buf, strlen(teep_res_buf));
		if (jose) {
			n = teep_message_wrap(teep_res_buf, strlen(teep_res_buf), out, out_len);
		} else {
			if (*out_len < strlen(teep_res_buf)) {
				n = -1;
			} else {
				memmove(out, teep_res_buf, strlen(teep_res_buf));
				*out_len = strlen(teep_res_buf);
				n = *out_len;
			}
		}
		if (n < 0) {
			lwsl_err("%s: wrap_teep_response failed, %d", __func__, n);
			return n;
		}
		*out_len = n;
		break;
	default:
		lwsl_err("%s: requested message type is invalid %d\n", __func__, m.type);
		return -1;
	}

	uint32_t ofs = 0;
	for (int i = 0; i < manifests.len; i++) {
		uint32_t len = strlen(manifests.url[i]) + 1;
		if (*ta_url_list_len < ofs + len) return -1;
		strcpy(ta_url_list + ofs, manifests.url[i]);
		ofs += len + 1;
	}
	if (*ta_url_list_len < ofs + 1) return -1;
	strcpy(ta_url_list + ofs, "");
	ofs += 1;
	*ta_url_list_len = ofs;

	return 0;
}

int
teep_agent_set_ta_list(const char *ta_list, int ta_list_len)
{
	if (ta_list_len == 0) {
		ta_list_buf[0] = '\0';
	} else {
		if (ta_list[ta_list_len - 1] != 0) {
			return -1;
		}
		if (ta_list_len > 1 && ta_list[ta_list_len - 2] != 0) {
			return -1;
		}
		if (sizeof (ta_list_buf) < ta_list_len) {
			return -1;
		}
		memcpy(ta_list_buf, ta_list, ta_list_len);
	}
	return 0;
}
