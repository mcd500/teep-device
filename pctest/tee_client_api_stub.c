#include <tee_client_api.h>

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
