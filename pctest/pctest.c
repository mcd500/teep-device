#include <libwebsockets.h>
#include <tee_client_api.h>
#include "teep_message.h"

// TA Store Stub

int
install_ta(const char *ta_image, size_t ta_image_len)
{
	lwsl_user("%s: stub called\n", __func__);
	return 0;
}

int
delete_ta(const char *uuid_string)
{
	lwsl_user("%s: stub called\n", __func__);
	return 0;
}

// TEEC Stub

TEEC_Result TEEC_InitializeContext(const char *name, TEEC_Context *context)
{
	lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_INFO | LLL_NOTICE, NULL);
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
	case 1: /* OTrP */
		if ((TEEC_PARAM_TYPE_GET(type, 0) != TEEC_MEMREF_TEMP_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 1) != TEEC_VALUE_INPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 2) != TEEC_MEMREF_TEMP_OUTPUT) ||
				(TEEC_PARAM_TYPE_GET(type, 3) != TEEC_VALUE_INOUT))
			return TEEC_ERROR_BAD_PARAMETERS;
		return otrp(params[0].tmpref.buffer, params[1].value.a, params[2].tmpref.buffer, params[3].value.a);
	case 2: /* TEEP */
		return TEEC_ERROR_NOT_IMPLEMENTED;
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
