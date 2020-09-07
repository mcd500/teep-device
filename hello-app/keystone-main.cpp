#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/random.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <string>
#include <cstring>

#include "edger/Enclave_u.h"
#ifdef APP_PERF_ENABLE
#include "profiler/profiler.h"
#endif

#include "tee_client_api.h"

int main(int argc, char** argv)
{
    if (argc != 3) {
        printf("Usage: %s enclave-file runtime-file\n", argv[0]);
        return 1;
    }
    Keystone enclave;
    Params params;
    params.setFreeMemSize(1024*1024);
    params.setUntrustedMem(DEFAULT_UNTRUSTED_PTR, 1024*1024);
    if(enclave.init(argv[1], argv[2], params) != KEYSTONE_SUCCESS){
        printf("%s: Unable to start enclave\n", argv[0]);
        exit(-1);
    }

    enclave.registerOcallDispatch(incoming_call_dispatch);

    register_functions();
        
    edge_call_init_internals((uintptr_t)enclave.getSharedBuffer(),
                            enclave.getSharedBufferSize());

    enclave.run();
    return 0;
}

EDGE_EXTERNC_BEGIN

invoke_command_t ocall_pull_invoke_command()
{
}

param_buffer_t ocall_read_invoke_param(int index, unsigned int offset)
{
    param_buffer_t ret;
    return ret;
}

void ocall_write_invoke_param(int index, unsigned int offset, unsigned int size, const char *buf)
{

}

void ocall_put_invoke_command_result(invoke_command_t cmd, unsigned int result)
{
}

EDGE_EXTERNC_END

TEEC_Result TEEC_InitializeContext(const char *name, TEEC_Context *context)
{

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

}

void TEEC_CloseSession(TEEC_Session *session)
{

}

TEEC_Result TEEC_InvokeCommand(TEEC_Session *session,
			       uint32_t commandID,
			       TEEC_Operation *operation,
			       uint32_t *returnOrigin)
{

}
