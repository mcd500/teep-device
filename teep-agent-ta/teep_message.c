#include <libwebsockets.h>
#include "teep_message.h"
#include "ta-store.h"
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
otrp_unwrap_message(const char *msg, int msg_len, char *out, int *out_len) {
	struct lws_context_creation_info info;
	static struct lws_context *context = NULL;
	struct lws_jwk jwk_pubkey_tam;
	int temp_len = sizeof(temp_buf);
	struct lws_jws jws;
	struct lws_jwe jwe;
	int n;


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

	n = lws_jwk_import(&jwk_pubkey_tam, NULL, NULL, tam_id_pubkey_jwk, strlen(tam_id_pubkey_jwk));
	if (n < 0) {
		lwsl_err("%s: unable to import tam jwk\n", __func__);
		goto bail;
	}
	n = lws_jws_sig_confirm_json(msg, msg_len, &jws, &jwk_pubkey_tam, context, temp_buf, &temp_len);
	if (n < 0) {
		lwsl_err("%s: confirm rsa sig failed\n", __func__);
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
	if (jwe.jws.map.len[LJWE_CTXT] > *out_len) {
		lwsl_err("%s: output buffer is small (in, out) = (%d, %d)\n", __func__, jwe.jws.map.len[LJWE_CTXT], *out_len);
	}
	memcpy(out, jwe.jws.map.buf[LJWE_CTXT], jwe.jws.map.len[LJWE_CTXT]);
	*out_len = jwe.jws.map.len[LJWE_CTXT];
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
	int buf_len = TEMP_BUF_SIZE;
	int res;
	if (!buf) {
		lwsl_err("%s: out of memory\n", __func__);
		return -1;
	}
	res = otrp_unwrap_message(msg, msg_len, buf, &buf_len);
	if (res < 0) {
		lwsl_err("%s: failed to unwrap message\n", __func__);
		goto bail;
	}

	if (!strncmp(buf, "{\"delete-ta\":\"", 14)) {
		res = delete_ta(buf + 14);
	} else {
		res = install_ta(buf, buf_len);
	}
bail:
	free(buf);
	return res;
}

