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

#include "teep-command-def.h"


/* We hardcode these for demo purposes. */
const char* enc_path = "teep-agent-ta";
const char* runtime_path = "eyrie-rt";

struct Command
{
    invoke_command_t command;
    TEEC_Operation *operation;
    unsigned int command_result;
};

class CommandQueue
{
private:
    Command current;
    std::mutex mutex;
    std::condition_variable cv;
    bool invoking = false;
public:

    invoke_command_t pull_invoke_command()
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this]{ return invoking; });
        return current.command;
    }

    param_buffer_t read_invoke_param(int index, unsigned int offset)
    {
        param_buffer_t ret;
        // TODO: check paramType
        TEEC_TempMemoryReference *ref = &current.operation->params[index].tmpref;
        if (offset > ref->size) {
            ret.size = 0;
        } else {
            unsigned int size = std::min(ref->size - offset, sizeof ret.buf);
            ret.size = size;
            memcpy(ret.buf, (const char *)ref->buffer + offset, size);
        }
        return ret;
    }

    void write_invoke_param(int index, unsigned int offset, unsigned int size, const char *buf)
    {
        // TODO: check paramType
        TEEC_TempMemoryReference *ref = &current.operation->params[index].tmpref;
        if (offset > ref->size) {
        } else {
            unsigned int n = std::min<size_t>(ref->size - offset, size);
            memcpy((char *)ref->buffer + offset, buf, n);
        }
    }

    void put_invoke_command_result(const invoke_command_t& cmd, unsigned int result)
    {
        std::unique_lock<std::mutex> lock(mutex);
        for (int i = 0; i < 4; i++) {
            switch (TEEC_PARAM_TYPE_GET(cmd.paramTypes, i)) {
            default:
                break;
            case TEEC_VALUE_OUTPUT:
            case TEEC_VALUE_INOUT:
                current.operation->params[i].value.a = cmd.params[i].a;
                current.operation->params[i].value.b = cmd.params[i].b;
                break;
            case TEEC_MEMREF_TEMP_OUTPUT:
            case TEEC_MEMREF_TEMP_INOUT:
                current.operation->params[i].tmpref.size = cmd.params[i].size;
                break;
            }
        }
        current.command_result = result;
        current.operation = NULL;
        invoking = false;
        lock.unlock();
        cv.notify_one();
    }

    void put_invoke_command(int commandID, TEEC_Operation *operation)
    {
        invoke_command_t c;
        c.commandID = commandID;
        c.paramTypes = operation ? operation->paramTypes : 0;
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
        std::unique_lock<std::mutex> lock(mutex);
        current.command = c;
        current.operation = operation;
        invoking = true;
        lock.unlock();
        cv.notify_one();
    }

    int pull_invoke_command_result()
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this]{ return !invoking; });
        return current.command_result;
    }

};

// XXX: only one TEEC_Session is supported
static CommandQueue queue;
static Keystone enclave;
static std::thread enclave_thread;

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

    Params params;
    params.setFreeMemSize(1024*1024);
    params.setUntrustedMem(DEFAULT_UNTRUSTED_PTR, 1024*1024);
    if(enclave.init(enc_path, runtime_path, params) != KEYSTONE_SUCCESS){
        printf("Unable to start enclave\n");
        return TEEC_ERROR_GENERIC;
    }

    enclave.registerOcallDispatch(incoming_call_dispatch);

    register_functions();
        
    edge_call_init_internals((uintptr_t)enclave.getSharedBuffer(),
                            enclave.getSharedBufferSize());

    std::thread t(
        [&]{ enclave.run(); }
    );
    enclave_thread.swap(t);

    return TEEC_SUCCESS;
}

void TEEC_CloseSession(TEEC_Session *session)
{
    fprintf(stderr, "TEEC_CloseSession\n");
    TEEC_InvokeCommand(NULL, TEEP_AGENT_TA_EXIT, NULL, NULL);
    enclave_thread.join();
}

TEEC_Result TEEC_InvokeCommand(TEEC_Session *session,
			       uint32_t commandID,
			       TEEC_Operation *operation,
			       uint32_t *returnOrigin)
{
    fprintf(stderr, "TEEC_InvokeCommand\n");

    queue.put_invoke_command(commandID, operation);
    unsigned int ret = queue.pull_invoke_command_result();

    return ret;
}

EDGE_EXTERNC_BEGIN

invoke_command_t ocall_pull_invoke_command()
{
    return queue.pull_invoke_command();
}

param_buffer_t ocall_read_invoke_param(int index, unsigned int offset)
{
    return queue.read_invoke_param(index, offset);
}

void ocall_write_invoke_param(int index, unsigned int offset, unsigned int size, const char *buf)
{
    //lwsl_hexdump_notice(buf, size);
    queue.write_invoke_param(index, offset, size, buf);
}

void ocall_put_invoke_command_result(invoke_command_t cmd, unsigned int result)
{
    return queue.put_invoke_command_result(cmd, result);
}

EDGE_EXTERNC_END

