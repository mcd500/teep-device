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

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <libwebsockets.h>
#include <pta_secstor_ta_mgmt.h>
#include "ta-store.h"

static int hex(char c)
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

static int
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

/* install given a TA Image into secure storage using optee pta*/
int
ta_store_install(const char *ta_image, size_t ta_image_len)
{
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
	pars[0].memref.buffer = (void *)ta_image;
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
}

/* delete a TA Image corresponds to UUID from secure storage using optee pta */
int
ta_store_delete(const char *uuid_string, size_t uuid_string_len)
{
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
	lwsl_notice("Deleted TA from secure storage\n");
	return 0;
}
