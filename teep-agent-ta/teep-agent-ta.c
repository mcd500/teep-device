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
#include "teep-agent-ta.h"
#include "teep_message.h"
#include "ta-store.h"

/* in a real system this'd come from the encrypted packet */

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


TEE_Result TA_CreateEntryPoint(void)
{
	lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_INFO | LLL_NOTICE, NULL);
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
	case TEEP_AGENT_TA_WRAP_MESSAGE: /* wrap TEEP message */
		if ((TEE_PARAM_TYPE_GET(param_types, 0) != TEE_PARAM_TYPE_MEMREF_INPUT) ||
		    (TEE_PARAM_TYPE_GET(param_types, 1) != TEE_PARAM_TYPE_VALUE_INPUT) ||
		    (TEE_PARAM_TYPE_GET(param_types, 2) != TEE_PARAM_TYPE_MEMREF_OUTPUT) ||
		    (TEE_PARAM_TYPE_GET(param_types, 3) != TEE_PARAM_TYPE_VALUE_INOUT))
			return TEE_ERROR_BAD_PARAMETERS;
		return teep_message_wrap(params[0].memref.buffer, params[1].value.a,
				params[2].memref.buffer, (size_t *)&params[3].value.a);
	case TEEP_AGENT_TA_UNWRAP_MESSAGE: /* unwrap TEEP message */
		if ((TEE_PARAM_TYPE_GET(param_types, 0) != TEE_PARAM_TYPE_MEMREF_INPUT) ||
		    (TEE_PARAM_TYPE_GET(param_types, 1) != TEE_PARAM_TYPE_VALUE_INPUT) ||
		    (TEE_PARAM_TYPE_GET(param_types, 2) != TEE_PARAM_TYPE_MEMREF_OUTPUT) ||
		    (TEE_PARAM_TYPE_GET(param_types, 3) != TEE_PARAM_TYPE_VALUE_INOUT))
			return TEE_ERROR_BAD_PARAMETERS;
		return teep_message_unwrap(params[0].memref.buffer, params[1].value.a,
				params[2].memref.buffer, (size_t *)&params[3].value.a);
	case TEEP_AGENT_TA_VERIFY_MESSAGE: /* verify OTrP message */
		if ((TEE_PARAM_TYPE_GET(param_types, 0) != TEE_PARAM_TYPE_MEMREF_INPUT) ||
				(TEE_PARAM_TYPE_GET(param_types, 1) != TEE_PARAM_TYPE_VALUE_INPUT) ||
				(TEE_PARAM_TYPE_GET(param_types, 2) != TEE_PARAM_TYPE_MEMREF_OUTPUT) ||
				(TEE_PARAM_TYPE_GET(param_types, 3) != TEE_PARAM_TYPE_VALUE_INOUT))
			return TEE_ERROR_BAD_PARAMETERS;
		return otrp_message_verify(params[0].memref.buffer, params[1].value.a,
				params[2].memref.buffer, (size_t *)&params[3].value.a);
	case TEEP_AGENT_TA_INSTALL: /* Install TA */
		if ((TEE_PARAM_TYPE_GET(param_types, 0) != TEE_PARAM_TYPE_MEMREF_INPUT) ||
		    (TEE_PARAM_TYPE_GET(param_types, 1) != TEE_PARAM_TYPE_VALUE_INPUT) ||
		    (TEE_PARAM_TYPE_GET(param_types, 2) != TEE_PARAM_TYPE_MEMREF_INPUT) ||
		    (TEE_PARAM_TYPE_GET(param_types, 3) != TEE_PARAM_TYPE_VALUE_INPUT))
			return TEE_ERROR_BAD_PARAMETERS;
		return ta_store_install(params[0].memref.buffer, params[1].value.a,
				params[2].memref.buffer, params[3].value.a);
	case TEEP_AGENT_TA_DELETE: /* Delete TA */
		if ((TEE_PARAM_TYPE_GET(param_types, 0) != TEE_PARAM_TYPE_MEMREF_INPUT) ||
		    (TEE_PARAM_TYPE_GET(param_types, 1) != TEE_PARAM_TYPE_VALUE_INPUT) ||
		    (TEE_PARAM_TYPE_GET(param_types, 2) != TEE_PARAM_TYPE_NONE) ||
		    (TEE_PARAM_TYPE_GET(param_types, 3) != TEE_PARAM_TYPE_NONE))
			return TEE_ERROR_BAD_PARAMETERS;
		return ta_store_delete(params[0].memref.buffer, params[1].value.a);
	case TEEP_AGENT_TA_UNWRAP_TA_IMAGE:
		if ((TEE_PARAM_TYPE_GET(param_types, 0) != TEE_PARAM_TYPE_MEMREF_INPUT) ||
		    (TEE_PARAM_TYPE_GET(param_types, 1) != TEE_PARAM_TYPE_VALUE_INPUT) ||
		    (TEE_PARAM_TYPE_GET(param_types, 2) != TEE_PARAM_TYPE_MEMREF_OUTPUT) ||
		    (TEE_PARAM_TYPE_GET(param_types, 3) != TEE_PARAM_TYPE_VALUE_INOUT))
			return TEE_ERROR_BAD_PARAMETERS;
		return teep_message_unwrap_ta_image(params[0].memref.buffer, params[1].value.a,
				params[2].memref.buffer, (size_t *)&params[3].value.a);

	case TEEP_AGENT_TA_SIGN_MESSAGE: /* sign(wrap) OTrP message*/
		if ((TEE_PARAM_TYPE_GET(param_types, 0) != TEE_PARAM_TYPE_MEMREF_INPUT) ||
				(TEE_PARAM_TYPE_GET(param_types, 1) != TEE_PARAM_TYPE_VALUE_INPUT) ||
				(TEE_PARAM_TYPE_GET(param_types, 2) != TEE_PARAM_TYPE_MEMREF_OUTPUT) ||
				(TEE_PARAM_TYPE_GET(param_types, 3) != TEE_PARAM_TYPE_VALUE_INOUT))
			return TEE_ERROR_BAD_PARAMETERS;
		return otrp_message_sign(params[0].memref.buffer, params[1].value.a,
				params[2].memref.buffer, (size_t *)&params[3].value.a);
	case TEEP_AGENT_TA_ENCRYPT_MESSAGE: /* encrypt(wrap) OTrP message*/
		if ((TEE_PARAM_TYPE_GET(param_types, 0) != TEE_PARAM_TYPE_MEMREF_INPUT) ||
				(TEE_PARAM_TYPE_GET(param_types, 1) != TEE_PARAM_TYPE_VALUE_INPUT) ||
				(TEE_PARAM_TYPE_GET(param_types, 2) != TEE_PARAM_TYPE_MEMREF_OUTPUT) ||
				(TEE_PARAM_TYPE_GET(param_types, 3) != TEE_PARAM_TYPE_VALUE_INOUT))
			return TEE_ERROR_BAD_PARAMETERS;
		return otrp_message_encrypt(params[0].memref.buffer, params[1].value.a,
				params[2].memref.buffer, (size_t *)&params[3].value.a);
	case TEEP_AGENT_TA_DECRYPT_MESSAGE: /* decrypt(unwrap) OTrP message*/
		if ((TEE_PARAM_TYPE_GET(param_types, 0) != TEE_PARAM_TYPE_MEMREF_INPUT) ||
				(TEE_PARAM_TYPE_GET(param_types, 1) != TEE_PARAM_TYPE_VALUE_INPUT) ||
				(TEE_PARAM_TYPE_GET(param_types, 2) != TEE_PARAM_TYPE_MEMREF_OUTPUT) ||
				(TEE_PARAM_TYPE_GET(param_types, 3) != TEE_PARAM_TYPE_VALUE_INOUT))
			return TEE_ERROR_BAD_PARAMETERS;
		return otrp_message_decrypt(params[0].memref.buffer, params[1].value.a,
				params[2].memref.buffer, (size_t *)&params[3].value.a);

	default:
		return TEE_ERROR_NOT_IMPLEMENTED;
	}
}

#ifdef KEYSTONE
#include <eapp_utils.h>
#include <edger/Enclave_t.h>
// TODO: should implemet in ref-ta/api???

void EAPP_ENTRY eapp_entry()
{
	lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_USER, NULL);
	ocall_print_string("hello agent ta\n");
	IMSG("agent ta IMSG %d\n", 42);
	{
		invoke_command_t c = ocall_pull_invoke_command();
		int ret = -1;
		if (c.commandID == 42) {
			IMSG("command 42\n");
			ret = 10080;
		}
		ocall_put_invoke_command_result(c, ret);
	}
	for (;;) {
		invoke_command_t c = ocall_pull_invoke_command();
		if (c.commandID == 1000) {
			ocall_put_invoke_command_result(c, 0);
			break;
		}
		TEE_Param params[4];
		for (int i = 0; i < 4; i++) {
			params[i].value.a = 0;
			params[i].value.b = 0;
			switch (TEE_PARAM_TYPE_GET(c.paramTypes, i)) {
			default:
				break;
			case TEE_PARAM_TYPE_VALUE_INPUT:
			case TEE_PARAM_TYPE_VALUE_INOUT:
				params[i].value.a = c.params[i].a;
				params[i].value.b = c.params[i].b;
				break;
			case TEE_PARAM_TYPE_MEMREF_INPUT:
			case TEE_PARAM_TYPE_MEMREF_INOUT:
				{
					uint32_t size = c.params[i].size;
					uint32_t offset;
					params[i].memref.size = size;
					params[i].memref.buffer = malloc(size); // TODO: check
					for (offset = 0; offset < size;) {
						param_buffer_t buf = ocall_read_invoke_param(i, offset);
						memcpy(params[i].memref.buffer + offset, buf.buf, buf.size);
						offset += buf.size;
					}
				}
				break;
			case TEE_PARAM_TYPE_MEMREF_OUTPUT:
				params[i].memref.size = c.params[i].size;
				params[i].memref.buffer = malloc(params[i].memref.size); // TODO: check
				memset(params[i].memref.buffer, 0, c.params[i].size);
				break;
			}
		}
		TEE_Result r = TA_InvokeCommandEntryPoint(NULL, c.commandID, c.paramTypes, params);
		for (int i = 0; i < 4; i++) {
			c.params[i].a = 0;
			c.params[i].b = 0;
			switch (TEE_PARAM_TYPE_GET(c.paramTypes, i)) {
			default:
				break;
			case TEE_PARAM_TYPE_VALUE_OUTPUT:
			case TEE_PARAM_TYPE_VALUE_INOUT:
				c.params[i].a = params[i].value.a;
				c.params[i].b = params[i].value.b;
				break;
			case TEE_PARAM_TYPE_MEMREF_OUTPUT:
			case TEE_PARAM_TYPE_MEMREF_INOUT:
				{
					uint32_t size = c.params[i].size;
					uint32_t offset;
					for (offset = 0; offset < size;) {
						uint32_t n = size - offset;
						if (n >= 256) n = 256;
						ocall_write_invoke_param(i, offset, n, params[i].memref.buffer + offset);
						offset += n;
					}
					free(params[i].memref.buffer);
				}
				break;
			case TEE_PARAM_TYPE_MEMREF_INPUT:
				free(params[i].memref.buffer);
				break;
			}
		}

		ocall_put_invoke_command_result(c, r);

	}

	EAPP_RETURN(0);
}

#endif
