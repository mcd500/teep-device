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
#include <tee_client_api.h>
#include "teep_message.h"
#include "ta-store.h"
#include "teep-agent-ta.h"

/* TEEC Stub */

TEEC_Result TEEC_InitializeContext(const char *name, TEEC_Context *context)
{
	lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE
//		| LLL_INFO | LLL_DEBUG | LLL_PARSER | LLL_HEADER | LLL_CLIENT
		, NULL);
	lwsl_user("%s: stub called\n", __func__);
	return TEEC_SUCCESS;
}

void TEEC_FinalizeContext(TEEC_Context *context)
{
	lwsl_user("%s: stub called\n", __func__);
}

TEEC_Result TEEC_OpenSession(TEEC_Context *context,
		TEEC_Session *session,
		const TEEC_UUID *destination,
		uint32_t connectionMethod,
		const void *connectionData,
		TEEC_Operation *operation,
		uint32_t *returnOrigin)
{
	lwsl_user("%s: stub called\n", __func__);
	return TEEC_SUCCESS;
}

void TEEC_CloseSession(TEEC_Session *session)
{
	lwsl_user("%s: stub called\n", __func__);
}

TEEC_Result TEEC_InvokeCommand(TEEC_Session *session,
		uint32_t commandID,
		TEEC_Operation *operation,
		uint32_t *returnOrigin)
{
	lwsl_user("%s: stub called\n", __func__);
	uint32_t type = operation->paramTypes;
	TEEC_Parameter *params = operation->params;
	switch (commandID) {
	case TEEP_AGENT_TA_WRAP_MESSAGE: /* wrap TEEP message*/
		if ((TEEC_PARAM_TYPE_GET(type, 0) != TEEC_MEMREF_TEMP_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 1) != TEEC_VALUE_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 2) != TEEC_MEMREF_TEMP_OUTPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 3) != TEEC_VALUE_INOUT))
			return TEEC_ERROR_BAD_PARAMETERS;
		return teep_message_wrap(params[0].tmpref.buffer, params[1].value.a,
				params[2].tmpref.buffer, (size_t *)&params[3].value.a);
	case TEEP_AGENT_TA_SIGN_MESSAGE: /* sign(wrap) OTrP message*/
		if ((TEEC_PARAM_TYPE_GET(type, 0) != TEEC_MEMREF_TEMP_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 1) != TEEC_VALUE_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 2) != TEEC_MEMREF_TEMP_OUTPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 3) != TEEC_VALUE_INOUT))
			return TEEC_ERROR_BAD_PARAMETERS;
		return otrp_message_sign(params[0].tmpref.buffer, params[1].value.a,
				params[2].tmpref.buffer, (size_t *)&params[3].value.a);
	case TEEP_AGENT_TA_ENCRYPT_MESSAGE: /* encrypt(wrap) OTrP message*/
		if ((TEEC_PARAM_TYPE_GET(type, 0) != TEEC_MEMREF_TEMP_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 1) != TEEC_VALUE_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 2) != TEEC_MEMREF_TEMP_OUTPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 3) != TEEC_VALUE_INOUT))
			return TEEC_ERROR_BAD_PARAMETERS;
		return otrp_message_encrypt(params[0].tmpref.buffer, params[1].value.a,
				params[2].tmpref.buffer, (size_t *)&params[3].value.a);
	case TEEP_AGENT_TA_UNWRAP_MESSAGE: /* unwrap TEEP message*/
		if ((TEEC_PARAM_TYPE_GET(type, 0) != TEEC_MEMREF_TEMP_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 1) != TEEC_VALUE_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 2) != TEEC_MEMREF_TEMP_OUTPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 3) != TEEC_VALUE_INOUT))
			return TEEC_ERROR_BAD_PARAMETERS;
		return teep_message_unwrap(params[0].tmpref.buffer, params[1].value.a,
				params[2].tmpref.buffer, (size_t *)&params[3].value.a);
	case TEEP_AGENT_TA_VERIFY_MESSAGE: /* verify(unwrap) OTrP message*/
		if ((TEEC_PARAM_TYPE_GET(type, 0) != TEEC_MEMREF_TEMP_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 1) != TEEC_VALUE_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 2) != TEEC_MEMREF_TEMP_OUTPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 3) != TEEC_VALUE_INOUT))
			return TEEC_ERROR_BAD_PARAMETERS;
		return otrp_message_verify(params[0].tmpref.buffer, params[1].value.a,
				params[2].tmpref.buffer, (size_t *)&params[3].value.a);
	case TEEP_AGENT_TA_DECRYPT_MESSAGE: /* decrypt(unwrap) OTrP message*/
		if ((TEEC_PARAM_TYPE_GET(type, 0) != TEEC_MEMREF_TEMP_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 1) != TEEC_VALUE_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 2) != TEEC_MEMREF_TEMP_OUTPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 3) != TEEC_VALUE_INOUT))
			return TEEC_ERROR_BAD_PARAMETERS;
		return otrp_message_decrypt(params[0].tmpref.buffer, params[1].value.a,
				params[2].tmpref.buffer, (size_t *)&params[3].value.a);
	case TEEP_AGENT_TA_INSTALL: /* Install TA */
		if ((TEEC_PARAM_TYPE_GET(type, 0) != TEEC_MEMREF_TEMP_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 1) != TEEC_VALUE_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 2) != TEEC_NONE) ||
				(TEEC_PARAM_TYPE_GET(type, 3) != TEEC_NONE))
			return TEEC_ERROR_BAD_PARAMETERS;
		return ta_store_install(params[0].tmpref.buffer, params[1].value.a);
	case TEEP_AGENT_TA_DELETE: /* Delete TA */
		if ((TEEC_PARAM_TYPE_GET(type, 0) != TEEC_MEMREF_TEMP_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 1) != TEEC_VALUE_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 2) != TEEC_NONE) ||
				(TEEC_PARAM_TYPE_GET(type, 3) != TEEC_NONE))
			return TEEC_ERROR_BAD_PARAMETERS;
		return ta_store_delete(params[0].tmpref.buffer, params[1].value.a);
	case TEEP_AGENT_TA_UNWRAP_TA_IMAGE: /* unwrap TEEP message*/
		if ((TEEC_PARAM_TYPE_GET(type, 0) != TEEC_MEMREF_TEMP_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 1) != TEEC_VALUE_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 2) != TEEC_MEMREF_TEMP_OUTPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 3) != TEEC_VALUE_INOUT))
			return TEEC_ERROR_BAD_PARAMETERS;
		return teep_message_unwrap_ta_image(params[0].tmpref.buffer, params[1].value.a,
				params[2].tmpref.buffer, (size_t *)&params[3].value.a);

	default:
		return TEEC_ERROR_NOT_IMPLEMENTED;
	}
}

TEEC_Result TEEC_RegisterSharedMemory(TEEC_Context *context,
		TEEC_SharedMemory *sharedMem)
{
	lwsl_user("%s: stub called\n", __func__);
	return TEEC_SUCCESS;
}

TEEC_Result TEEC_AllocateSharedMemory(TEEC_Context *context,
		TEEC_SharedMemory *sharedMem)
{
	lwsl_user("%s: stub called\n", __func__);
	return TEEC_SUCCESS;
}

void TEEC_ReleaseSharedMemory(TEEC_SharedMemory *sharedMemory)
{
	lwsl_user("%s: stub called\n", __func__);
}

void TEEC_RequestCancellation(TEEC_Operation *operation)
{
	lwsl_user("%s: stub called\n", __func__);
}
