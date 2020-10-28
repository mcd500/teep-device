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

#ifndef PCTEST
#include <tee_internal_api.h>
#ifndef PLAT_KEYSTONE
#include <pta_secstor_ta_mgmt.h>
#else
#include <edger/Enclave_t.h>
#endif
#endif
#include <libwebsockets.h>
#include "teep-agent-ta.h"
#include "ta-store.h"

#define TEMP_BUF_SIZE (800 * 1024)
static char temp_buf[TEMP_BUF_SIZE];

/* the TEE private key as a JWK */
static const char * const tee_id_privkey_jwk =
#include "tee_id_privkey_jwk.h"
;

/* SP public key as a JWK */
static const char * const sp_pubkey_jwk =
#include "sp_pubkey_jwk.h"
;

int hex(char c)
{
	if (c >= 'A' && c <= 'F')
		return 10 + (c - 'A');

	if (c >= 'a' && c <= 'f')
		return 10 + (c - 'a');

	if (c >= '0' && c <= '9')
		return c - '0';

	return -1;
}

/*
 * input should be like this
 *
 * 8d82573a-926d-4754-9353-32dc29997f74.ta
 *
 * return will be 0, 16 bytes of octets16 will be filled.
 *
 * On error, return is -1.
 */

int
string_to_uuid_octets(const char *s, uint8_t *octets16)
{
	const char *end = s + 36;
	uint8_t b, flip = 0;
	int a;

	while (s < end) {
		if (*s != '-') {
			a = hex(*s);
			if (a < 0)
				return -1;
			if (flip)
				*(octets16++) = (b << 4) | a;
			else
				b = a;

			flip ^= 1;
		}

		s++;
	}

	return 0;
}

static struct lws_context *get_lws_context()
{
	static struct lws_context *context = NULL;
#ifdef PCTEST
	// calling lws_create_context on tee environment causes a lot of link error
	// lws_create_context must be called in pc environment to avoid SEGV on decrypt
	if (!context) {
		struct lws_context_creation_info info;
		memset(&info, 0, sizeof(info));
		info.port = CONTEXT_PORT_NO_LISTEN;
		info.options = 0;
		context = lws_create_context(&info);
		if (!context) {
			lwsl_err("lws_create_context failed\n");
		}
	}
#endif
	return context;
}

static int teep_message_unwrap_ta_image(const char *msg, int msg_len, char *out, uint32_t *out_len) {
	struct lws_jwk jwk_pubkey_sp;
	int temp_len = TEMP_BUF_SIZE - 1;
	struct lws_jws jws;
	struct lws_jwe jwe;
	int n = 0;

	lwsl_user("%s: msg len %d\n", __func__, msg_len);
	*temp_buf = '0';

	lws_jws_init(&jws, &jwk_pubkey_sp, get_lws_context());
	lws_jwe_init(&jwe, get_lws_context());

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
	n = lws_jws_sig_confirm_json(jwe.jws.map.buf[LJWE_CTXT], jwe.jws.map.len[LJWE_CTXT], &jws, &jwk_pubkey_sp, get_lws_context(), temp_buf, &temp_len);
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

#ifdef PLAT_KEYSTONE

static int install_plain(const char *filename, const char *ta_image, size_t ta_image_len)
{
	int ret = -1;

	int fd = ocall_open_file(filename, O_CREAT | O_WRONLY, 0600);
	if (fd < 0) goto bail_1;
	int n = ocall_write_file_full(fd, ta_image, ta_image_len);
	if (n != ta_image_len) goto bail_2;

	ret = 0;
bail_2:
	ocall_close_file(fd);
bail_1:
	return ret;
}

static int install_secstor(const char *secstor_id, const char *ta_image, size_t ta_image_len)
{
	int ret = -1;
	TEE_Result res;
	TEE_ObjectHandle obj;

	res = TEE_CreatePersistentObject(0, secstor_id, strlen(secstor_id),
		TEE_DATA_FLAG_ACCESS_WRITE | TEE_DATA_FLAG_OVERWRITE,
		TEE_HANDLE_NULL, NULL, 0, &obj);
	if (res != TEE_SUCCESS) {
		lwsl_err("%s: OpenPersistentObject failed\n", __func__);
		goto bail_1;
	}
	size_t ta_image_len_16 = ta_image_len & ~15;
	size_t rest_len = ta_image_len & 15;
	res = TEE_WriteObjectData(obj, ta_image, ta_image_len_16);
	if (res != TEE_SUCCESS) {
		lwsl_err("%s: TEE_WriteObjectData failed\n", __func__);
		goto bail_2;
	}
	char padding[16];
	memcpy(padding, ta_image + ta_image_len_16, rest_len);
	memset(padding + rest_len, 16 - rest_len, 16 - rest_len);
	TEE_WriteObjectData(obj, padding, 16);
	if (res != TEE_SUCCESS) {
		lwsl_err("%s: TEE_WriteObjectData failed\n", __func__);
		goto bail_2;
	}

	ret = 0;
bail_2:
	TEE_CloseObject(obj);
bail_1:
	return ret;
}

static int install_secstor_plain(const char *filename, const char *secstor_id, size_t ta_image_len)
{
	int ret = -1;
	TEE_Result res;
	TEE_ObjectHandle obj;

	res = TEE_OpenPersistentObject(0, secstor_id, strlen(secstor_id),
		TEE_DATA_FLAG_ACCESS_READ, &obj);
	if (res != TEE_SUCCESS) {
		lwsl_err("%s: OpenPersistentObject failed\n", __func__);
		goto bail_1;
	}
	int fd = ocall_open_file(filename, O_CREAT | O_WRONLY, 0600);
	if (fd < 0) {
		lwsl_err("%s: ocall_open_file failed\n", __func__);
		goto bail_2;
	}
	int offset = 0;
	while (offset < ta_image_len) {
		char buf[256];
		uint32_t count;
		res = TEE_ReadObjectData(obj, buf, 256, &count);
		if (res != TEE_SUCCESS) {
			lwsl_err("%s: TEE_ReadObjectData failed\n", __func__);
			goto bail_3;
		}

		if (offset + count > ta_image_len) {
			count = ta_image_len - offset; // ignore padding
		}
		int n = ocall_write_file_full(fd, buf, count);
		if (n != count) {
			lwsl_err("%s: ocall_write_file failed\n", __func__);
			goto bail_3;
		}
		offset += n;
	}
	ret = 0;
bail_3:
	ocall_close_file(fd);
bail_2:
	TEE_CloseObject(obj);
bail_1:
	return ret;
}

#endif

static ta_image_buf[TEMP_BUF_SIZE];

/* install given a TA Image into secure storage using optee pta*/
int
ta_store_install(const char *ta_image_ciphertext, size_t ta_image_ciphertext_len, const char *ta_name, size_t ta_name_len)
{
	size_t ta_image_len = TEMP_BUF_SIZE;
	if (teep_message_unwrap_ta_image(ta_image_ciphertext, ta_image_ciphertext_len, ta_image_buf, &ta_image_len) < 0) {
		lwsl_err("%s: TA verification failed\n", __func__);
		return -1;
	}
#if defined(PCTEST)
	lwsl_user("%s: stub called ta_image_len = %zd\n", __func__, ta_image_len);
	return 0;
#elif defined(PLAT_KEYSTONE)
	lwsl_user("%s: ta_image_len = %zd ta_name=%s\n", __func__, ta_image_len, ta_name);

	char filename_ta[256];
	char filename_secstor[256];
	char filename_secstor_plain[256];
	snprintf(filename_ta, 256, "%s.ta", ta_name);
	snprintf(filename_secstor, 256, "%s.ta.secstor", ta_name);
	snprintf(filename_secstor_plain, 256, "%s.ta.secstor.plain", ta_name);

	install_plain(filename_ta, ta_image_buf, ta_image_len);
	install_secstor(filename_secstor, ta_image_buf, ta_image_len);
	install_secstor_plain(filename_secstor_plain, filename_secstor, ta_image_len);

	return 0;
#else
	(void)ta_name;
	(void)ta_name_len;
	TEE_TASessionHandle sess = TEE_HANDLE_NULL;
	const TEE_UUID secstor_uuid = PTA_SECSTOR_TA_MGMT_UUID;
	TEE_Param pars[TEE_NUM_PARAMS];
	TEE_Result res;
	lwsl_err("%s:TODO verify hand decryption", __func__);
	res = TEE_OpenTASession(&secstor_uuid, 0, 0, NULL, &sess, NULL);
	if (res != TEE_SUCCESS) {
		lwsl_err("%s: Unable to open session to secstor\n", __func__);
		return -1;
	}

	memset(pars, 0, sizeof(pars));
	pars[0].memref.buffer = (void *)ta_image_buf;
	pars[0].memref.size = ta_image_len;
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
		return -1;
	}
	lwsl_notice("Wrote TA to secure storage\n");
	return 0;
#endif
}

/* delete a TA Image corresponds to UUID from secure storage using optee pta */
int
ta_store_delete(const char *uuid_string, size_t uuid_string_len)
{
#if defined(PCTEST)
	lwsl_user("%s: stub called\n", __func__);
	return 0;
#elif defined(PLAT_KEYSTONE)
	char filename_ta[256];
	char filename_secstor[256];
	char filename_secstor_plain[256];
	snprintf(filename_ta, 256, "%s.ta", uuid_string);
	snprintf(filename_secstor, 256, "%s.ta.secstor", uuid_string);
	snprintf(filename_secstor_plain, 256, "%s.ta.secstor.plain", uuid_string);
	lwsl_user("%s: delete %s\n", __func__, filename_ta);
	ocall_unlink(filename_ta);
	ocall_unlink(filename_secstor);
	ocall_unlink(filename_secstor_plain);
	return 0;
#else
	(void)uuid_string_len;
	uint8_t uuid_octets[16];
	TEE_TASessionHandle sess = TEE_HANDLE_NULL;
	const TEE_UUID secstor_uuid = PTA_SECSTOR_TA_MGMT_UUID;
	TEE_Param pars[TEE_NUM_PARAMS];
	TEE_Result res;

	if (string_to_uuid_octets(uuid_string, uuid_octets)) {
		lwsl_err("%s: problem parsing UUID\n", __func__);
		return -1;
	}

	res = TEE_OpenTASession(&secstor_uuid, 0, 0, NULL, &sess, NULL);
	if (res != TEE_SUCCESS) {
		lwsl_err("%s: Unable to open session to secstor\n", __func__);
		return -1;
	}

	memset(pars, 0, sizeof(pars));
	pars[0].memref.buffer = (void *)uuid_octets;
	pars[0].memref.size = 16;
#ifndef TEE_TA
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
		return -1;
	}
#endif
	lwsl_notice("Deleted TA from secure storage\n");
	return 0;
#endif
}
