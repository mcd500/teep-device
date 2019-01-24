/*
 * Copyright (C) 2017 - 2019 National Institute of Advanced Industrial Science
 *                           and Technology (AIST）
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

#define STR_TRACE_USER_TA "AIST_OTrP"
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <libwebsockets.h>

#include "pta_secstor_ta_mgmt.h"

/* the TAM root cert we trust */

static const uint8_t const *tam_root_cert_pem =
	#include "tam_root_cert.h"
;

/* the TAM server cert public key as a JWK */

static const uint8_t const *tam_id_pubkey_jwk =
	#include "tam_id_pubkey_jwk.h"
;

/* our TEE private key as a JWK */

static const uint8_t const *tee_privkey_jwk =
	#include "tee_privkey_jwk.h"
;


static struct lws_context *context;

#define JW_TEMP_SIZE (800 * 1024)

#define TA_NAME		"aist-otrp.ta"
#define TA_PRINT_PREFIX	"AIST-OTRP: "


/* in a real system this'd come from the encrypted packet */

static const char *const ta_name =
		"8d82573a-926d-4754-9353-32dc29997f74.ta";

time_t time(time_t *tloc)
{
	TEE_Time t;

	TEE_GetREETime(&t);

	if (tloc)
		*tloc = t.seconds;

	return t.seconds;
}


int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	TEE_Time t;

	(void)tz;

	TEE_GetREETime(&t);

	tv->tv_sec = t.seconds;
	tv->tv_usec = t.millis * 1000;

	return 0;
}

TEE_Result TA_CreateEntryPoint(void)
{
	return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void)
{
}

TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
		TEE_Param __maybe_unused params[4],
		void __maybe_unused **sess_ctx)
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE,
						   TEE_PARAM_TYPE_NONE);
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	(void)&params;
	(void)&sess_ctx;

	return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx)
{
	(void)&sess_ctx;
}

static TEE_Result
otrp(uint32_t type, TEE_Param p[TEE_NUM_PARAMS])
{
	struct lws_genhash_ctx hash_ctx;
	struct lws_jwk jwk_pubkey_tam;
	int temp_len = JW_TEMP_SIZE;
	TEE_ObjectHandle handle;
	int msg_len, resp_len;
	const uint8_t *msg;
	TEE_Result res = 1;
	struct lws_jws jws;
	struct lws_jwe jwe;
	uint8_t *resp;
	char *temp;
	int n;

	if ((TEE_PARAM_TYPE_GET(type, 0) != TEE_PARAM_TYPE_MEMREF_INPUT) ||
	    (TEE_PARAM_TYPE_GET(type, 1) != TEE_PARAM_TYPE_VALUE_INPUT) ||
	    (TEE_PARAM_TYPE_GET(type, 2) != TEE_PARAM_TYPE_MEMREF_OUTPUT) ||
	    (TEE_PARAM_TYPE_GET(type, 3) != TEE_PARAM_TYPE_VALUE_INOUT))
		return TEE_ERROR_BAD_PARAMETERS;

	temp = malloc(JW_TEMP_SIZE);
	if (!temp) {
		EMSG("%s: can't allocate JWS temp\n", __func__);
		return TEE_ERROR_BAD_PARAMETERS;
	}

	msg = (const uint8_t *)p[0].memref.buffer;
	msg_len = (int)p[1].value.a;
	resp = (uint8_t *)p[2].memref.buffer;
	resp_len = (int)p[3].value.a;

	EMSG("%s: msg len %d\n", __func__, msg_len);

	lws_jws_init(&jws, &jwk_pubkey_tam, context);

	if (lws_jwk_import(&jwk_pubkey_tam, NULL, NULL, tam_id_pubkey_jwk,
			   strlen(tam_id_pubkey_jwk))) {
		lwsl_err("%s: unable to import tam jwk\n", __func__);

		goto bail;
	}

	if (lws_jws_sig_confirm_json(msg, msg_len, &jws, &jwk_pubkey_tam, context,
				     temp, &temp_len) < 0) {
		lwsl_notice("%s: confirm rsa sig failed\n",
		    __func__);
		goto bail;
	}

	lwsl_user("Signature OK\n");

	/*
	 * it's a bit expensive, but take a moment to reset the temp usage now
	 * the JWS part is confirmed.  Over-allocating large temp spaces is
	 * really expensive in OP-TEE world.
	 *
	 * First copy the part we care about, the base64-decoded JWS payload,
	 * to the start of the temp space.
	 */

	memmove(temp, jws.map.buf[LJWS_PYLD], jws.map.len[LJWS_PYLD]);

	/* Second, set the pointers to point to the copy at the start */

	jws.map.buf[LJWS_PYLD] = temp;

	/* Lastly, reset the remaining temp space.  Now all the JWS-related
	 * temp usage is wiped out except the JWE payload. */

	temp_len = JW_TEMP_SIZE - jws.map.len[LJWS_PYLD];

	lws_jwe_init(&jwe, context);
	if (lws_jwe_json_parse(&jwe, jws.map.buf[LJWS_PYLD],
			       jws.map.len[LJWS_PYLD],
			       lws_concat_temp(temp, temp_len), &temp_len)) {
		lwsl_err("%s: lws_jwe_json_parse failed\n",
						 __func__);
		goto bail1;
	}

	if (lws_jwk_import(&jwe.jwk, NULL, NULL, tee_privkey_jwk,
			   strlen(tee_privkey_jwk))) {
		lwsl_err("%s: unable to import tee jwk\n", __func__);

		goto bail1;
	}

	/* JWS payload is a JWE */

	n = lws_jwe_auth_and_decrypt(&jwe, lws_concat_temp(temp, temp_len),
				     &temp_len);
	if (n < 0) {
		lwsl_err("%s: lws_jwe_auth_and_decrypt failed\n",
			 __func__);
		goto bail1;
	}

	lwsl_user("Decrypt OK: length %d\n", n);
#if 0
	/* delete it if it already exists */

	if (TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE_REE, (void *)ta_name,
				     strlen(ta_name), TEE_DATA_FLAG_ACCESS_READ,
				     &handle) != TEE_ERROR_ITEM_NOT_FOUND) {
		lwsl_notice("%s: TA already existed, replacing\n", __func__);
		TEE_CloseAndDeletePersistentObject(handle);
	}

	/*
	 * in a real OTrP system the "filename" must be segregated by path
	 * per security domain
	 */

	res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE_REE,
				ta_name, strlen(ta_name),
				TEE_DATA_FLAG_ACCESS_WRITE |
				 TEE_DATA_FLAG_ACCESS_READ |
				 TEE_DATA_FLAG_ACCESS_WRITE_META,
				(TEE_ObjectHandle)(uintptr_t)NULL,
				jwe.jws.map.buf[LJWE_CTXT], n, &handle);
	if (res) {
		lwsl_err("Failed to write TA: 0x%x\n", (int)res);
		goto bail1;
	}
#endif

	{
		TEE_TASessionHandle sess = TEE_HANDLE_NULL;
		const TEE_UUID secstor_uuid = PTA_SECSTOR_TA_MGMT_UUID;
		TEE_Param pars[TEE_NUM_PARAMS];
		TEE_Result res;

		res = TEE_OpenTASession(&secstor_uuid, 0, 0, NULL, &sess, NULL);
		if (res != TEE_SUCCESS) {
			lwsl_err("%s: Unable to open session to secstor\n",
				 __func__);
			goto bail1;
		}

		memset(pars, 0, sizeof(pars));
		pars[0].memref.buffer = (void *)jwe.jws.map.buf[LJWE_CTXT];
		pars[0].memref.size = jwe.jws.map.len[LJWE_CTXT];
		res = TEE_InvokeTACommand(sess, 0,
					  PTA_SECSTOR_TA_MGMT_BOOTSTRAP,
					  TEE_PARAM_TYPES(
						TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE),
					  pars, NULL);
		TEE_CloseTASession(sess);
		if (res != TEE_SUCCESS) {
			lwsl_err("%s: Command failed\n", __func__);
			goto bail1;
		}
	}

	lwsl_notice("Wrote TA to secure storage\n");

	/* the return of TEE_SUCCESS is enough */
	p[3].value.a = 0;

	res = TEE_SUCCESS;

bail1:
	lws_jwe_destroy(&jwe);
bail:
	lws_jwk_destroy(&jwk_pubkey_tam);
	lws_jws_destroy(&jws);

	free(temp);

	return res;
}

static struct lws_context *
create_lws_context(void)
{
	struct lws_context_creation_info info;
	struct lws_context *context;

	memset(&info, 0, sizeof info);
	info.port = CONTEXT_PORT_NO_LISTEN;
	context = lws_create_context(&info);
	if (!context) {
		lwsl_err("lws init failed\n");
		return NULL;
	}

	return context;
}

TEE_Result
TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx,
			uint32_t cmd_id,
			uint32_t param_types,
			TEE_Param params[TEE_NUM_PARAMS])
{
	switch (cmd_id) {

	case 1: /* interpret OTrP message */
		return otrp(param_types, params);

	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}
	return TEE_ERROR_NOT_IMPLEMENTED;
}
