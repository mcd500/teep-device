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

static void
usage(void)
{
	fprintf(stderr, "aist-otrp-testapp [--tamurl http://tamserver:port] [-d] [-p otrp]\n");
	fprintf(stderr, "     --tamurl: TAM server url \n"
			"     --jose: enable encryption and sign \n"
			"     --talist: installed ta list \n"
			"     -p: teep protocol otrp or teep \n");
	exit(1);
}

static void
cmdline_parse(int argc, const char *argv[])
{
	const char *tmp;
	if (lws_cmdline_option(argc, argv, "--help"))
		usage();

	/* override the remote TAM URL */
	tmp = lws_cmdline_option(argc, argv, "--tamurl");
	if (tmp)
		uri = tmp;

	/* request the TAM ask the TEE to delete the test TA */
	/* protocol (teep or otrp) */
	tmp = lws_cmdline_option(argc, argv, "-p");
	if (tmp) {
		if (!strcmp(tmp, "otrp")) {
			teep_ver = LIBTEEP_TEEP_VER_OTRP;
		} else if (!strcmp(tmp, "teep")) {
			teep_ver = LIBTEEP_TEEP_VER_TEEP;
		} else {
			// usage();
			// use default protocol is TEEP
			teep_ver = LIBTEEP_TEEP_VER_TEEP;
		}
	}

	/* request the TAM ask the TEE to delete the test TA */
	/* protocol (teep or otrp) */
	tmp = lws_cmdline_option(argc, argv, "--jose");
	if (tmp) {
		jose = true;
	}

	/* ta-list */
	tmp = lws_cmdline_option(argc, argv, "--talist");
	if (tmp)
		talist = tmp;

}

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
        memcpy(ret.buf, ref->buffer + offset, size);
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
        memcpy(ref->buffer + offset, buf, n);
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
    struct libteep_ctx *lao_ctx = NULL;
	int res;

	cmdline_parse(argc, argv);
	fprintf(stderr, "%s compiled at %s %s\n", __FILE__, __DATE__, __TIME__);
	fprintf(stderr, "uri = %s, teep_ver = %d, talist=%s\n", uri, teep_ver, talist);

	res = libteep_init(&lao_ctx, teep_ver, uri);
	if (res != TR_OKAY) {
		fprintf(stderr, "%s: Unable to create lao\n", __func__);
		return 1;
	}

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

    if (teep_ver == LIBTEEP_TEEP_VER_TEEP)
        loop_teep(lao_ctx);
    else if (teep_ver == LIBTEEP_TEEP_VER_OTRP)
        loop_otrp(lao_ctx);

    {
        invoke_command_t c;
        c.commandID = 1000;
        c.paramTypes = 0;
        my_TEEC_InvokeCommand(c);
    }
    enclave_thread.join();

    return 0;
}

EDGE_EXTERNC_BEGIN

unsigned int ocall_print_string(const char* str){
  printf("%s",str);
  return strlen(str);
}

#undef APP_VERBOSE

int ocall_open_file(const char* fname, int flags, int perm)
{
    int desc = open(fname, flags, perm);
#ifdef APP_VERBOSE
    printf("@[SE] open file %s flags %x -> %d (%d)\n",fname,flags,desc,errno);
#endif
    return desc;
}

int ocall_close_file(int fdesc) 
{
    return close(fdesc);
}

int ocall_write_file(int fdesc, const char *buf,  unsigned int len) 
{
#ifdef APP_VERBOSE
    printf("@[SE] write desc %d buf %x len %d\n",fdesc,buf,len);
#endif
    return write(fdesc, buf, len);
}

int ocall_unlink(keyedge_str const char *path)
{
    return unlink(path);
}

int ocall_fstat_size(int fd)
{
    struct stat st;
    int ret = fstat(fd, &st);
    if (ret < 0) {
        return ret;
    }
    return st.st_size;
}

#if !defined(EDGE_OUT_WITH_STRUCTURE)
int ocall_read_file(int fdesc, char *buf, size_t len) 
{
    printf("%s\n", __func__);

}

int ocall_ree_time(struct ree_time_t *timep) 
{
    printf("%s\n", __func__);

}

ssize_t ocall_getrandom(char *buf, size_t len, unsigned int flags)
{
    printf("%s\n", __func__);

}

#else

ob256_t ocall_read_file256(int fdesc)
{
  ob256_t ret;
  int rtn = read(fdesc, ret.b, 256);
#ifdef APP_VERBOSE
  printf("@[SE] read desc %d buf %x len %d-> %d\n",fdesc,ret.b,256,rtn);
#endif
  ret.ret = rtn;
  return ret;
}

ree_time_t ocall_ree_time(void) 
{
    printf("%s\n", __func__);

}

ob16_t ocall_getrandom16(unsigned int flags) 
{
#ifdef APP_VERBOSE
    printf("%s\n", __func__);
#endif
}

ob196_t ocall_getrandom196(unsigned int flags) 
{
#ifdef APP_VERBOSE
    printf("%s\n", __func__);
#endif

}

invoke_command_t ocall_invoke_command_polling(void) 
{
    printf("%s\n", __func__);

}

int ocall_invoke_command_callback(invoke_command_t cb_cmd) 
{
    printf("%s\n", __func__);

}

int ocall_invoke_command_callback_write(const char* str, const char *buf,  unsigned int len)
{
    printf("%s\n", __func__);

}

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

#endif

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
