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

/**
@brief main() 			function is lunching the enclave by host.

@param[in] argc 		integer argument count.
@param[in] argv 		character type argument vector.

				The enclave parameters contain such as size of free memory and the address/size of the untrusted shared buffer.
In order to handle the edge calls (including system calls), the enclave must register the edge call handler and initialize the buffer addresses and finally the host lunches the enclave.

@return 			0 based on the enclave lunch.
*/
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

/**
@brief ocall_pull_invoke_command() is for returning invoke command.

@return invoke_command_t structure.
*/
invoke_command_t ocall_pull_invoke_command()
{
}

/**
@brief ocall_read_invoke_param() is for returning the size of the buffer.

@param[in] index    		integer type parameter position.
@param[in] offset   		indicating the distance between the beginning of the object.
 
return the param_buffer_t structure

@return 			param_buffer_t structure.
*/
param_buffer_t ocall_read_invoke_param(int index, size_t offset)
{
    param_buffer_t ret;
    return ret;
}

/**
@brief read_invoke_param() 	is used for callng the write_invoke_param().

@param[in] index    		integer type parameter position.
@param[in] offset   		indicating the distance between the beginning of the object.
*/
void ocall_write_invoke_param(int index, size_t, size_t size, const char *buf)
{

}

/**
@brief ocall_put_invoke_command_result() is returning the invoke command result 

@param[in] cmd      		invoke commnad.
@param[in] result   		invoke command result.

@return 			result based on invoke command output. 
*/
void ocall_put_invoke_command_result(invoke_command_t cmd, unsigned int result)
{
}

EDGE_EXTERNC_END

/**
@brief             		Initializes a context holding connection information on the specific TEE, designated by the name string.

@param[in] name         	A zero-terminated string identifying the TEE to connect to. If name is set to NULL, the default TEE is 					connected to. NULL is the only supported value in this version of the API implementation.
@param[in] context      	The context structure which is to be initialized.
 
@return TEEC_SUCCESS  		The initialization was successful.
@return TEEC_Result   
 */
TEEC_Result TEEC_InitializeContext(const char *name, TEEC_Context *context)
{

}

/**
@brief  			Destroys a context holding connection information on the specific TEE.

@param[in] context		The context to be destroyed.

				This function destroys an initialized TEE context, closing the connection
between the client application and the TEE. This function must only be called when all sessions related to this TEE context have been closed and all shared memory blocks have been released.
*/
void TEEC_FinalizeContext(TEEC_Context *context)
{

}

/**
@brief TEEC_OpenSession() 	Opens a new session with the specified trusted application.
 
@param[in] context		The initialized TEE context structure in which scope to open the session.
@param[in] session            	The session to initialize.
@param[in] destination        	A structure identifying the trusted application with which to open a session.
@param[in] connectionMethod   	The connection method to use.
@param[in] connectionData     	Any data necessary to connect with the chosen connection method. Not supported, should be set to 
                              	NULL.
@param[in] operation          	An operation structure to use in the session. May be set to NULL to signify no operation structure 
	                      	needed.
@param[in] returnOrigin       	A parameter which will hold the error origin if this function returns any value other than TEEC_SUCCESS.

@return TEEC_SUCCESS	      	OpenSession successfully opened a new session.
@return TEEC_Result           	Something failed.
*/
TEEC_Result TEEC_OpenSession(TEEC_Context *context,
			     TEEC_Session *session,
			     const TEEC_UUID *destination,
			     uint32_t connectionMethod,
			     const void *connectionData,
			     TEEC_Operation *operation,
			     uint32_t *returnOrigin)
{

}

/**
@brief TEEC_CloseSession()	Closes the session which has been opened with the specific trusted application.

@param[in] 			session The opened session to close.
*/
void TEEC_CloseSession(TEEC_Session *session)
{

}

/**
@brief TEEC_InvokeCommand()	Executes a command in the specified trusted application.

@param[in] session        	A handle to an open connection to the trusted application.
@param[in] commandID      	Identifier of the command in the trusted application to invoke.
@param[in] operation      	An operation structure to use in the invoke command.May be set to NULL to signify no operation structure
                       	  	needed.
@param[in] returnOrigin   	A parameter which will hold the error origin if this function returns any value other than TEEC_SUCCESS.

@return TEEC_SUCCESS  	  	OpenSession successfully opened a new session.
@return TEEC_Result   	  	Something failed.
 */
TEEC_Result TEEC_InvokeCommand(TEEC_Session *session,
			       uint32_t commandID,
			       TEEC_Operation *operation,
			       uint32_t *returnOrigin)
{

}
