#include <libwebsockets.h>
#include "teep_message.h"

#define TEMP_BUF_SIZE (800 * 1024)

static char temp_buf[TEMP_BUF_SIZE];

/* the TAM root cert we trust */
static const char * const tam_root_cert_pem =
#include "tam_root_cert.h"
;
/* the TAM server cert public key as a JWK */

static const char * const tam_id_pubkey_jwk =
#include "tam_id_pubkey_jwk.h"
;

/* our TEE private key as a JWK */
static const char * const tee_privkey_jwk =
#include "tee_privkey_jwk.h"
;

int
otrp_unwrap_message(const char *msg, int msg_len, char *out, int out_len) {
	struct lws_context_creation_info info;
	static struct lws_context *context = NULL;
	struct lws_jwk jwk_pubkey_tam;
	int temp_len = sizeof(temp_buf);
	struct lws_jws jws;
	struct lws_jwe jwe;
	int n;

	lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE, NULL);

	lwsl_notice("%s: msg len %d\n", __func__, msg_len);
	memset(&info, 0, sizeof(info));
	info.port = CONTEXT_PORT_NO_LISTEN;
	info.options = 0;
#if 0
	// calling lws_create_context causes a lot of link error
	context = lws_create_context(&info);
	if (!context) {
		lwsl_err("lws init failed\n");
		return -1;
	}
#endif

	lws_jws_init(&jws, &jwk_pubkey_tam, context);
	lws_jwe_init(&jwe, context);

	n = lws_jwk_import(&jwk_pubkey_tam, NULL, NULL, tam_id_pubkey_jwk, strlen(tam_id_pubkey_jwk));
	if (n < 0) {
		lwsl_err("%s: unable to import tam jwk\n", __func__);
		goto bail;
	}
	n = lws_jws_sig_confirm_json(msg, msg_len, &jws, &jwk_pubkey_tam, context, temp_buf, &temp_len);
	if (n < 0) {
		lwsl_notice("%s: confirm rsa sig failed\n", __func__);
		goto bail;
	}
	lwsl_user("Signature OK\n");
	n = lws_jwe_json_parse(&jwe, (uint8_t *)jws.map.buf[LJWS_PYLD],
				jws.map.len[LJWS_PYLD],
				lws_concat_temp(temp_buf, temp_len), &temp_len);
	if (n < 0) {
		lwsl_err("%s: lws_jwe_json_parse failed\n", __func__);
		goto bail1;
	}

	n = lws_jwk_import(&jwe.jwk, NULL, NULL, tee_privkey_jwk, strlen(tee_privkey_jwk));
	if (n < 0) {
		lwsl_err("%s: unable to import tee jwk\n", __func__);
		goto bail1;
	}
	/* JWS payload is a JWE */
	n = lws_jwe_auth_and_decrypt(&jwe, lws_concat_temp(temp_buf, temp_len), &temp_len);
	if (n < 0) {
		lwsl_err("%s: lws_jwe_auth_and_decrypt failed\n", __func__);
		goto bail1;
	}

	lwsl_user("Decrypt OK: length %d\n", n);
	if (jwe.jws.map.len[LJWE_CTXT] > out_len) {
		lwsl_err("%s: output buffer is small (in, out) = (%d, %d)\n", __func__, jwe.jws.map.len[LJWE_CTXT], out_len);
	}
	memcpy(out, jwe.jws.map.buf[LJWE_CTXT], jwe.jws.map.len[LJWE_CTXT]);
	n = 0;
bail1:
	lws_jwe_destroy(&jwe);
bail:
	lws_jwk_destroy(&jwk_pubkey_tam);
	lws_jws_destroy(&jws);
	return n;
}

int
otrp(const char *msg, int msg_len, uint8_t *resp, int resp_len) {
	char *buf = malloc(TEMP_BUF_SIZE);
	int res = otrp_unwrap_message(msg, msg_len, buf, TEMP_BUF_SIZE);
	if (res) {
		lwsl_err("%s: failed to unwrap message\n", __func__);
		return -1;
	}
	if (!strncmp(buf, "{\"delete-ta\":\"", 14)) {
		lwsl_notice("TODO delete TA\n");
#if 0
		uint8_t uuid_octets[16];
		TEE_TASessionHandle sess = TEE_HANDLE_NULL;
		const TEE_UUID secstor_uuid = PTA_SECSTOR_TA_MGMT_UUID;
		TEE_Param pars[TEE_NUM_PARAMS];
		TEE_Result res;

		lwsl_user("Recognized TA delete request\n");

		if (string_to_uuid_octets((const char *)jwe.jws.map.buf[LJWE_CTXT] + 14, uuid_octets)) {
			lwsl_err("%s: problem parsing UUID\n", __func__);
			goto bail1;
		}

		res = TEE_OpenTASession(&secstor_uuid, 0, 0, NULL, &sess, NULL);
		if (res != TEE_SUCCESS) {
			lwsl_err("%s: Unable to open session to secstor\n",
				 __func__);
			goto bail1;
		}

		memset(pars, 0, sizeof(pars));
		pars[0].memref.buffer = (void *)uuid_octets;
		pars[0].memref.size = 16;
		res = TEE_InvokeTACommand(sess, 0,
					  PTA_SECSTOR_TA_MGMT_DELETE_TA,
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
		lwsl_notice("Deleted TA from secure storage\n");
#endif
	}
	else
	{
		lwsl_notice("TODO install TA\n");
#if 0
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
		lwsl_notice("Wrote TA to secure storage\n");
#endif
	}
	return 0;
}

