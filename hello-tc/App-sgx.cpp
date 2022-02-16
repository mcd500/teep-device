/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (C) 2019 National Institute of Advanced Industrial Science
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

#include <stdio.h>
#include <string.h>
#include <assert.h>

# include <unistd.h>
# include <pwd.h>
# define MAX_PATH FILENAME_MAX

# include <sys/stat.h>
# include <fcntl.h>

# include <time.h>

#include "sgx_urts.h"
#include "edger/Enclave_u.h"
#include "types.h"

#define ENCLAVE_FILENAME "enclave.signed.so"
#define TA_REF_RUN_HELLO 0x11111111

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

/**
 * main() - Performs the enclave operation by creating and destroying enclave.
 * 
 * This function is used for initializing the enclave and calling TA inside
 * the enclave. The enclave will destroy based on the success of TA.
 * 
 * @param argc		Argument Count is int and stores number of command-line 
 *			arguments passed by the user including the name of the 
 *			program.
 * @param argv		Argument Vector is array of character pointers listing all
 *			the arguments.
 *
 * @return 0		If success, else error occured.
 */
int SGX_CDECL main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);
    int ret = SGX_ERROR_UNEXPECTED;

    /* Initialize the enclave */
    if(initialize_enclave() < 0){
	ret = -1;
        goto main_destory_out; 
    }

    /* Calling Trusted Application */
    ret = ecall_ta_main(global_eid, TA_REF_RUN_HELLO);
    if (ret != SGX_SUCCESS)
        goto main_out;

    main_destory_out:
    /* Destroy the enclave */
    sgx_destroy_enclave(global_eid);
    
    printf("Info: Enclave successfully returned.\n");

main_out:
    return ret;
}
