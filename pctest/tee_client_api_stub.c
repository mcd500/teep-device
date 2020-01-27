#include <tee_client_api.h>
#include <libwebsockets.h>

#define EMSG(...) fprintf(stderr, __VA_ARGS__)

/* the TAM root cert we trust */
static const uint8_t * const tam_root_cert_pem =
#include "tam_root_cert.h"
;
/* the TAM server cert public key as a JWK */

static const uint8_t * const tam_id_pubkey_jwk =
#include "tam_id_pubkey_jwk.h"
;

/* our TEE private key as a JWK */
static const uint8_t * const tee_privkey_jwk =
#include "tee_privkey_jwk.h"
;



TEEC_Result TEEC_InitializeContext(const char *name, TEEC_Context *context)
{
	return TEEC_SUCCESS;
}

void TEEC_FinalizeContext(TEEC_Context *context)
{

}

TEEC_Result TEEC_OpenSession(TEEC_Context *context,
		TEEC_Session *session,
		const TEEC_UUID *destination,
		uint32_t connectionMethod,
		const void *connectionData,
		TEEC_Operation *operation,
		uint32_t *returnOrigin)
{
	return TEEC_SUCCESS;
}

void TEEC_CloseSession(TEEC_Session *session)
{

}

static struct lws_context *context;

#define JW_TEMP_SIZE (800 * 1024)

static TEEC_Result
otrp(const uint8_t *msg, int msg_len, uint8_t *resp, int resp_len) {
	struct lws_genhash_ctx hash_ctx;
	struct lws_jwk jwk_pubkey_tam;
	int temp_len = JW_TEMP_SIZE;
	TEEC_Result res = 1;
	struct lws_jws jws;
	struct lws_jwe jwe;
	char *temp;
	int n;

	lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE, NULL);

	temp = malloc(JW_TEMP_SIZE);
	if (!temp) {
		EMSG("%s: can't allocate JWS temp\n", __func__);
		return TEEC_ERROR_BAD_PARAMETERS;
	}

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

	if (!strncmp((const char *)jwe.jws.map.buf[LJWE_CTXT], "{\"delete-ta\":\"", 14)) {
		EMSG("TODO delete TA\n");
	}
	else
	{
		EMSG("TODO install TA\n");
	}


	res = TEEC_SUCCESS;

bail1:
	lws_jwe_destroy(&jwe);
bail:
	lws_jwk_destroy(&jwk_pubkey_tam);
	lws_jws_destroy(&jws);

	free(temp);

	return res;
}

TEEC_Result TEEC_InvokeCommand(TEEC_Session *session,
		uint32_t commandID,
		TEEC_Operation *operation,
		uint32_t *returnOrigin)
{
	uint32_t type = operation->paramTypes;
	TEEC_Parameter *params = operation->params;
	switch (commandID) {
	case 1: /* OTrP */
		if ((TEEC_PARAM_TYPE_GET(type, 0) != TEEC_MEMREF_TEMP_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 1) != TEEC_VALUE_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 2) != TEEC_MEMREF_TEMP_OUTPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 3) != TEEC_VALUE_INOUT))
			return TEEC_ERROR_BAD_PARAMETERS;
		return otrp(params[0].tmpref.buffer, params[1].value.a, params[2].tmpref.buffer, params[3].value.a);
	case 2: /* TEEP */
	default:
		return TEEC_ERROR_NOT_IMPLEMENTED;
	}
}

TEEC_Result TEEC_RegisterSharedMemory(TEEC_Context *context,
		TEEC_SharedMemory *sharedMem)
{
	return TEEC_SUCCESS;
}

TEEC_Result TEEC_AllocateSharedMemory(TEEC_Context *context,
		TEEC_SharedMemory *sharedMem)
{
	return TEEC_SUCCESS;
}

void TEEC_ReleaseSharedMemory(TEEC_SharedMemory *sharedMemory)
{

}

void TEEC_RequestCancellation(TEEC_Operation *operation)
{

}
