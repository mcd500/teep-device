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
int command_result;

invoke_command_t pull_invoke_command()
{
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, []{ return invoking; });
    return command;
}

void put_invoke_command_result(int result)
{
    std::unique_lock<std::mutex> lock(mutex);
    command_result = result;
    invoking = false;
    lock.unlock();
    cv.notify_one();
}

void put_invoke_command(const invoke_command_t& c)
{
    std::unique_lock<std::mutex> lock(mutex);
    command = c;
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
    put_invoke_command(c);
    return pull_invoke_command_result();
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

    {
        invoke_command_t c;
        c.commandID = 42;
        int ret = my_TEEC_InvokeCommand(c);
        printf("result: %d\n", ret);

        if (teep_ver == LIBTEEP_TEEP_VER_TEEP)
            loop_teep(lao_ctx);
        else if (teep_ver == LIBTEEP_TEEP_VER_OTRP)
            loop_otrp(lao_ctx);
    }

    enclave_thread.join();

    return 0;
}

EDGE_EXTERNC_BEGIN

unsigned int ocall_print_string(const char* str){
  printf("%s",str);
  return strlen(str);
}

int ocall_open_file(const char* fname, int flags, int perm) {}
int ocall_close_file(int fdesc) {}
int ocall_write_file(int fdesc, const char *buf,  unsigned int len) {}
int ocall_invoke_command_callback_write(const char* str, const char *buf,  unsigned int len) {}
#if !defined(EDGE_OUT_WITH_STRUCTURE)
int ocall_read_file(int fdesc, char *buf, size_t len) {}
int ocall_ree_time(struct ree_time_t *timep) {}
ssize_t ocall_getrandom(char *buf, size_t len, unsigned int flags){}
#else
ob256_t ocall_read_file256(int fdesc) {}
ree_time_t ocall_ree_time(void) {}
ob16_t ocall_getrandom16(unsigned int flags) {}
ob196_t ocall_getrandom196(unsigned int flags) {}
invoke_command_t ocall_invoke_command_polling(void) {}
int ocall_invoke_command_callback(invoke_command_t cb_cmd) {}


invoke_command_t ocall_pull_invoke_command()
{
    return pull_invoke_command();
}

void ocall_put_invoke_command_result(int result)
{
    return put_invoke_command_result(result);
}

#endif

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
