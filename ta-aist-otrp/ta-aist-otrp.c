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

#define STR_TRACE_USER_TA "AIST_OTrP"
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <libwebsockets.h>

#include "pta_secstor_ta_mgmt.h"
#include "teep_message.h"

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

TEE_Result
TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx,
			uint32_t cmd_id,
			uint32_t param_types,
			TEE_Param params[TEE_NUM_PARAMS])
{
	int res;
	switch (cmd_id) {
	case 1: /* interpret OTrP message */
		if ((TEE_PARAM_TYPE_GET(type, 0) != TEE_PARAM_TYPE_MEMREF_INPUT) ||
				(TEE_PARAM_TYPE_GET(type, 1) != TEE_PARAM_TYPE_VALUE_INPUT) ||
				(TEE_PARAM_TYPE_GET(type, 2) != TEE_PARAM_TYPE_MEMREF_OUTPUT) ||
				(TEE_PARAM_TYPE_GET(type, 3) != TEE_PARAM_TYPE_VALUE_INOUT))
			return TEE_ERROR_BAD_PARAMETERS;
		res = otrp(param_types, params);
		if (res != 0) {
			return TEE_ERROR_COMMUNICATION;
		}
		return TEE_SUCCESS;
	case 2: /* interpret TEEP message */
		/* TODO */
		return TEE_ERROR_NOT_IMPLEMENTED;
	deafalt:
		return TEE_ERROR_NOT_IMPLEMENTED;
	}
}
