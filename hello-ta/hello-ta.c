/*
 * Copyright (C) 2017 - 2019 National Institute of Advanced Industrial Science
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

#define STR_TRACE_USER_TA "AIST_TB"
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

/**
 * TA_CreateEntryPoint() - Trusted Application’s constructor,
 * 
 * This function is used to register instance data. the implementation of this 
 * constructor can use either global variables or the function
 * TEE_SetInstanceData.
 * 
 * @return        It returns TEE_SUCCESS.
 */ 
TEE_Result TA_CreateEntryPoint(void)
{
	return TEE_SUCCESS;
}

/**
 * TA_DestroyEntryPoint() - Trusted Application’s destructor.
 * 
 * When the function TA_DestroyEntryPoint is called, the Framework guarantees 
 * that no client session is currently open. Once the call to 
 * TA_DestroyEntryPoint has been completed, no other entry point of this
 * instance will ever be called.
 */ 
void TA_DestroyEntryPoint(void)
{
}

/**
 * TA_OpenSessionEntryPoint() - When a client requests to open a 
 * session with the Trusted Application.
 * 
 * This function client can specify parameters in an open operation which are 
 * passed to the Trusted Application instance in the arguments paramTypes and 
 * params.
 * 
 * @param param_types	The types of the four parameters
 * @param params	A pointer to an array of four parameters
 * @param sess_ctx	A pointer to a variable that can be filled by the
 *			Trusted Application instance with pointer. 
 *
 * @return		It returns TEEC_SUCCESS.
 */ 
TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
		TEE_Param __maybe_unused params[4],
		void __maybe_unused **sess_ctx)
{
	(void)param_types;
	return TEE_SUCCESS;
}

/**
 * TA_CloseSessionEntryPoint() - It is called when the client closes a session and
 * disconnects from the Trusted Application instance.
 * 
 * @param sess_ctx	The value of the void* opaque data pointer set by the 
 *			Trusted Application.
 */ 
void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx)
{
	(void)&sess_ctx;
}

/**
 * The TA_InvokeCommandEntryPoint() - When the client invokes a command 
 * within the given session.
 * 
 * The Trusted Application can access the parameters sent by the client through 
 * the paramTypes and params arguments. It can also use these arguments to 
 * transfer response data back to the client. A specification of how to handle 
 * the operation parameters. During the call to TA_InvokeCommandEntryPoint 
 * the client may request to cancel the operation.
 * 
 * @param sess_ctx	The value of the void* opaque data pointer set by the 
 * 			Trusted Application in the function 
 *			TA_OpenSessionEntryPoint
 * @param cmd_id	A Trusted Application-specific code that identifies 
 *			the command to be invoked
 * @param param_types	The types of the four parameters.
 * @param params	A pointer to an array of four parameters
 *
 * @return		Its return TEE_ERROR_NOT_IMPLEMENTED.
 */ 
TEE_Result TA_InvokeCommandEntryPoint(void __maybe_unused *sess_ctx,
			uint32_t cmd_id,
			uint32_t param_types,
			TEE_Param params[4])
{
	(void)cmd_id;
	(void)param_types;
	(void)params;

	switch (cmd_id) {
	case 1:
		printf("hello, world!\n");
		return TEE_SUCCESS;
	default:
		return TEE_ERROR_NOT_IMPLEMENTED;
	}
}

#ifdef KEYSTONE
#include <eapp_utils.h>
#include <edger/Enclave_t.h>
// TODO: should implemet in ref-ta/api???

/**
 * eapp_entry() - Prints hello TA message when it is invoked.
 * 
 * This function just calls ocall_print_string() to print the string given in the
 * bracket.
 */ 
void EAPP_ENTRY eapp_entry()
{
	ocall_print_string("hello TA\n");
	EAPP_RETURN(0);
}

#endif
