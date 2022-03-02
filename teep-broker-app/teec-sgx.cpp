#include "tee_client_api.h"

#include <libteep.h>
#include <libwebsockets.h>
#include "sgx_urts.h"
#include "edger/Enclave_u.h"
#include "types.h"

#include "ta-interface.h"

#define ENCLAVE_FILENAME "teep-agent-ta.signed.so"


/**
 * print_error_message() - Used for printing the error message.
 * 
 * This function prints the error message in sgx_errlist list 
 * and checks error conditions for loading enclave.
 * 
 * @param ret		A list containing all possible values of sgx_status_t data type.
 */
void print_error_message(sgx_status_t ret)
{
    size_t idx = 0;
    size_t ttl = sizeof sgx_errlist/sizeof sgx_errlist[0];

    for (idx = 0; idx < ttl; idx++) {
        if(ret == sgx_errlist[idx].err) {
            if(NULL != sgx_errlist[idx].sug)
                printf("Info: %s\n", sgx_errlist[idx].sug);
            printf("Error: %s\n", sgx_errlist[idx].msg);
            break;
        }
    }
    
    if (idx == ttl)
    	printf("Error code is 0x%X. Please refer to the \"Intel SGX SDK Developer Reference\" for more details.\n", ret);
}

/**
 * initialize_enclave() - Initializes an enclave by calling sgx_create_enclave().
 * 
 * This function returns 0 on the success initialization of enclave.If enclave 
 * is not created properly then it will return -1 on error.
 * 
 * @return 0		If success, else error occured.
 */
int initialize_enclave(void)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    
    /* Call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL, &global_eid, NULL);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }

    return 0;
}

TEEC_Result TEEC_InitializeContext(const char *name, TEEC_Context *context)
{
    lwsl_info("TEEC_InitializeContext\n");
    if (initialize_enclave() < 0) {
        return TEEC_ERROR_GENERIC;
    }
    return TEEC_SUCCESS;
}

void TEEC_FinalizeContext(TEEC_Context *context)
{
    lwsl_info("TEEC_FinalizeContext\n");
    sgx_destroy_enclave(global_eid);
}

TEEC_Result TEEC_OpenSession(TEEC_Context *context,
			     TEEC_Session *session,
			     const TEEC_UUID *destination,
			     uint32_t connectionMethod,
			     const void *connectionData,
			     TEEC_Operation *operation,
			     uint32_t *returnOrigin)
{
    lwsl_info("TEEC_OpenSession\n");
    ecall_TA_OpenSessionEntryPoint(global_eid, returnOrigin);
    return TEEC_SUCCESS;
}

void TEEC_CloseSession(TEEC_Session *session)
{
    lwsl_info("TEEC_CloseSession\n");
    ecall_TA_CloseSessionEntryPoint(global_eid);
}

static void prepare_params(
	TEEC_Operation *operation,
	uint32_t *types,
	struct command_param params[4])
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
        params[i].a = NULL;
        params[i].b = NULL;
        params[i].size = NULL;
        params[i].p = NULL;
		switch (TEE_PARAM_TYPE_GET(*types, i)) {
		default:
			break;
		case TEE_PARAM_TYPE_VALUE_INPUT:
		case TEE_PARAM_TYPE_VALUE_INOUT:
		case TEE_PARAM_TYPE_VALUE_OUTPUT:
			params[i].a = &operation->params[i].value.a;
			params[i].b = &operation->params[i].value.b;
			break;
		case TEE_PARAM_TYPE_MEMREF_INPUT:
		case TEE_PARAM_TYPE_MEMREF_INOUT:
		case TEE_PARAM_TYPE_MEMREF_OUTPUT:
            params[i].size = &operation->params[i].tmpref.size;
            params[i].p = operation->params[i].tmpref.buffer;
			break;
		}
	}
}

TEEC_Result TEEC_InvokeCommand(TEEC_Session *session,
			       uint32_t commandID,
			       TEEC_Operation *operation,
			       uint32_t *returnOrigin)
{
    lwsl_info("TEEC_InvokeCommand\n");
    uint32_t r = TEEC_SUCCESS;
    uint32_t param_types = 0;
    struct command_param params[4];

    prepare_params(operation, &param_types, params);

    sgx_status_t ret = ecall_TA_InvokeCommandEntryPoint(
        global_eid, &r, 0, commandID, param_types, params
    );
    return r;
}
