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
#include <thread>
#include <mutex>
#include <condition_variable>

#include "edger/Enclave_u.h"
#ifdef APP_PERF_ENABLE
#include "profiler/profiler.h"
#endif

#include "tee_client_api.h"

#include <libteep.h>
#include <libwebsockets.h>

#include "teep-broker.h"
#include "teep-command-def.h"


/* We hardcode these for demo purposes. */
const char* enc_path = "teep-agent-ta";
const char* runtime_path = "eyrie-rt";

std::mutex mutex;
std::condition_variable cv;
bool invoking = false;
invoke_command_t command;
TEEC_Operation *operation;
unsigned int command_result;

invoke_command_t pull_invoke_command()
{
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, []{ return invoking; });
    return command;
}

param_buffer_t read_invoke_param(int index, unsigned int offset)
{
    //printf("%s: %u\n", __func__, offset);
    param_buffer_t ret;
    // TODO: check paramType
    TEEC_TempMemoryReference *ref = &operation->params[index].tmpref;
    if (offset > ref->size) {
        ret.size = 0;
    } else {
        unsigned int size = std::min(ref->size - offset, sizeof ret.buf);
        ret.size = size;
        memcpy(ret.buf, (const char *)ref->buffer + offset, size);
    }
    //printf("%s: %u %u\n", __func__, offset, ret.size);
    return ret;
}

void write_invoke_param(int index, unsigned int offset, unsigned int size, const char *buf)
{
    //printf("%s: %u\n", __func__, offset);
    // TODO: check paramType
    TEEC_TempMemoryReference *ref = &operation->params[index].tmpref;
    if (offset > ref->size) {
    } else {
        unsigned int n = std::min<size_t>(ref->size - offset, size);
        memcpy((char *)ref->buffer + offset, buf, n);
    }
}

void put_invoke_command_result(invoke_command_t cmd, unsigned int result)
{
    std::unique_lock<std::mutex> lock(mutex);
    for (int i = 0; i < 4; i++) {
        switch (TEEC_PARAM_TYPE_GET(cmd.paramTypes, i)) {
        default:
            break;
        case TEEC_VALUE_OUTPUT:
        case TEEC_VALUE_INOUT:
            operation->params[i].value.a = cmd.params[i].a;
            operation->params[i].value.b = cmd.params[i].b;
            break;
        }
    }
    command_result = result;
    operation = NULL;
    invoking = false;
    lock.unlock();
    cv.notify_one();
}

void put_invoke_command(const invoke_command_t& c, TEEC_Operation *op)
{
    std::unique_lock<std::mutex> lock(mutex);
    command = c;
    operation = op;
    invoking = true;
    lock.unlock();
    cv.notify_one();
}

int pull_invoke_command_result()
{
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, []{ return !invoking; });
    return command_result;
}

int my_TEEC_InvokeCommand(const invoke_command_t& c)
{
    put_invoke_command(c, NULL);
    return pull_invoke_command_result();
}


TEEC_Result TEEC_InvokeCommand(TEEC_Session *session,
			       uint32_t commandID,
			       TEEC_Operation *operation,
			       uint32_t *returnOrigin)
{
    fprintf(stderr, "TEEC_InvokeCommand\n");

    invoke_command_t c;
    c.commandID = commandID;
    c.paramTypes = operation->paramTypes;
    for (int i = 0; i < 4; i++) {
        c.params[i].a = 0;
        c.params[i].b = 0;
        c.params[i].size = 0;
        switch (TEEC_PARAM_TYPE_GET(c.paramTypes, i)) {
        default:
            break;
        case TEEC_VALUE_INPUT:
        case TEEC_VALUE_INOUT:
            c.params[i].a = operation->params[i].value.a;
            c.params[i].b = operation->params[i].value.b;
            break;
        case TEEC_MEMREF_TEMP_INPUT:
        case TEEC_MEMREF_TEMP_OUTPUT:
        case TEEC_MEMREF_TEMP_INOUT:
            c.params[i].size = operation->params[i].tmpref.size;
            break;
        }
    }
    put_invoke_command(c, operation);
    unsigned int ret = pull_invoke_command_result();

    return ret;
}

int main(int argc, const char** argv)
{
	cmdline_parse(argc, argv);

    Keystone enclave;
    Params params;
    params.setFreeMemSize(1024*1024);
    params.setUntrustedMem(DEFAULT_UNTRUSTED_PTR, 1024*1024);
    if(enclave.init(enc_path, runtime_path, params) != KEYSTONE_SUCCESS){
        printf("%s: Unable to start enclave\n", argv[0]);
        exit(-1);
    }

    enclave.registerOcallDispatch(incoming_call_dispatch);

    register_functions();
        
    edge_call_init_internals((uintptr_t)enclave.getSharedBuffer(),
                            enclave.getSharedBufferSize());

    std::thread enclave_thread(
        [&]{ enclave.run(); }
    );

    int ret = broker_main();
    {
        invoke_command_t c;
        c.commandID = TEEP_AGENT_TA_EXIT;
        c.paramTypes = 0;
        my_TEEC_InvokeCommand(c);
    }
    enclave_thread.join();

    return ret;
}

EDGE_EXTERNC_BEGIN

invoke_command_t ocall_pull_invoke_command()
{
    return pull_invoke_command();
}

param_buffer_t ocall_read_invoke_param(int index, unsigned int offset)
{
    return read_invoke_param(index, offset);
}

void ocall_write_invoke_param(int index, unsigned int offset, unsigned int size, const char *buf)
{
    //lwsl_hexdump_notice(buf, size);
    write_invoke_param(index, offset, size, buf);
}

void ocall_put_invoke_command_result(invoke_command_t cmd, unsigned int result)
{
    return put_invoke_command_result(cmd, result);
}

EDGE_EXTERNC_END

TEEC_Result TEEC_InitializeContext(const char *name, TEEC_Context *context)
{
    fprintf(stderr, "TEEC_InitializeContext\n");
    return TEEC_SUCCESS;
}

void TEEC_FinalizeContext(TEEC_Context *context)
{
    fprintf(stderr, "TEEC_FinalizeContext\n");
}

TEEC_Result TEEC_OpenSession(TEEC_Context *context,
			     TEEC_Session *session,
			     const TEEC_UUID *destination,
			     uint32_t connectionMethod,
			     const void *connectionData,
			     TEEC_Operation *operation,
			     uint32_t *returnOrigin)
{
    fprintf(stderr, "TEEC_OpenSession\n");
    return TEEC_SUCCESS;

}

void TEEC_CloseSession(TEEC_Session *session)
{
    fprintf(stderr, "TEEC_CloseSession\n");
}
