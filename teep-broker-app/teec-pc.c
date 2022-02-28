/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (C) 2019 National Institute of Advanced Industrial Science
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
#include <tee_internal_api.h>
#include "ta-store.h"
#include "teep-agent-ta.h"

/* TEEC Stub */

static void prepare_params(
	TEEC_Operation *operation,
	uint32_t *types,
	TEE_Param params[4])
{
	int type[4];
	for (int i = 0; i < 4; i++) {
		switch (TEEC_PARAM_TYPE_GET(operation->paramTypes, i)) {
		default:
			type[i] = TEE_PARAM_TYPE_NONE;
			break;
		case TEEC_VALUE_INPUT:
			type[i] = TEE_PARAM_TYPE_VALUE_INPUT;
			break;
		case TEEC_VALUE_OUTPUT:
			type[i] = TEE_PARAM_TYPE_VALUE_OUTPUT;
			break;
		case TEEC_VALUE_INOUT:
			type[i] = TEE_PARAM_TYPE_VALUE_INOUT;
			break;
		case TEEC_MEMREF_TEMP_INPUT:
			type[i] = TEE_PARAM_TYPE_MEMREF_INPUT;
			break;
		case TEEC_MEMREF_TEMP_OUTPUT:
			type[i] = TEE_PARAM_TYPE_MEMREF_OUTPUT;
			break;
		case TEEC_MEMREF_TEMP_INOUT:
			type[i] = TEE_PARAM_TYPE_MEMREF_INOUT;
			break;
		}
	}
	*types = TEE_PARAM_TYPES(type[0], type[1], type[2], type[3]);

	for (int i = 0; i < 4; i++) {
		params[i].value.a = 0;
		params[i].value.b = 0;
		switch (TEE_PARAM_TYPE_GET(*types, i)) {
		default:
			break;
		case TEE_PARAM_TYPE_VALUE_INPUT:
		case TEE_PARAM_TYPE_VALUE_INOUT:
			params[i].value.a = operation->params[i].value.a;
			params[i].value.b = operation->params[i].value.b;
			break;
		case TEE_PARAM_TYPE_MEMREF_INPUT:
		case TEE_PARAM_TYPE_MEMREF_INOUT:
			{
				size_t size = operation->params[i].tmpref.size;
				params[i].memref.size = size;
				params[i].memref.buffer = malloc(size); // TODO: check
				memcpy(params[i].memref.buffer, operation->params[i].tmpref.buffer, size);
			}
			break;
		case TEE_PARAM_TYPE_MEMREF_OUTPUT:
			params[i].memref.size = operation->params[i].tmpref.size;
			params[i].memref.buffer = malloc(params[i].memref.size); // TODO: check
			memset(params[i].memref.buffer, 0, params[i].memref.size);
			break;
		}
	}
}

static void writeback_params(
	TEEC_Operation *operation,
	uint32_t types,
	TEE_Param params[4])
{
	for (int i = 0; i < 4; i++) {
		switch (TEE_PARAM_TYPE_GET(types, i)) {
		default:
			break;
		case TEE_PARAM_TYPE_VALUE_OUTPUT:
		case TEE_PARAM_TYPE_VALUE_INOUT:
			operation->params[i].value.a = params[i].value.a;
			operation->params[i].value.b = params[i].value.b;
			break;
		case TEE_PARAM_TYPE_MEMREF_OUTPUT:
		case TEE_PARAM_TYPE_MEMREF_INOUT:
			{
				uint32_t size = params[i].memref.size;
				uint32_t offset;
				operation->params[i].tmpref.size = size;
				memcpy(operation->params[i].tmpref.buffer, params[i].memref.buffer, size);
				free(params[i].memref.buffer);
			}
			break;
		case TEE_PARAM_TYPE_MEMREF_INPUT:
			free(params[i].memref.buffer);
			break;
		}
	}
}

TEEC_Result TEEC_InitializeContext(const char *name, TEEC_Context *context)
{
	lwsl_user("%s: stub called\n", __func__);
	return TEEC_SUCCESS;
}

void TEEC_FinalizeContext(TEEC_Context *context)
{
	lwsl_user("%s: stub called\n", __func__);
}

// NOTE: onky single session supported
static void *session_ctx;

TEEC_Result TEEC_OpenSession(TEEC_Context *context,
		TEEC_Session *session,
		const TEEC_UUID *destination,
		uint32_t connectionMethod,
		const void *connectionData,
		TEEC_Operation *operation,
		uint32_t *returnOrigin)
{
	uint32_t types;
	TEE_Param params[4];

	prepare_params(operation, &types, params);
	TEE_Result r = TA_OpenSessionEntryPoint(types, params, &session_ctx);
	if (r == TEE_SUCCESS) {
		writeback_params(operation, types, params);
	}
	return r;
}

void TEEC_CloseSession(TEEC_Session *session)
{
	TA_CloseSessionEntryPoint(session_ctx);
}

TEEC_Result TEEC_InvokeCommand(TEEC_Session *session,
		uint32_t commandID,
		TEEC_Operation *operation,
		uint32_t *returnOrigin)
{
	uint32_t types;
	TEE_Param params[4];

	prepare_params(operation, &types, params);
	TEE_Result r = TA_InvokeCommandEntryPoint(session_ctx, commandID, types, params);
	if (r == TEE_SUCCESS) {
		writeback_params(operation, types, params);
	}
	return r;
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
