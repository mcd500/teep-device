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

#include <err.h>
#include <stdio.h>
#include <string.h>

#include <tee_client_api.h>

// For the UUID
#include <edger/Enclave.h>

#define PRINT_BUF_SIZE 16384
static char print_buf[PRINT_BUF_SIZE];
#define TEEC_PARAM_TYPE1 TEEC_MEMREF_TEMP_OUTPUT

#define TA_UUID \
  { 0x8d82573a, 0x926d, 0x4754, { 0x93, 0x53, 0x32, 0xdc, 0x29, 0x99, 0x7f, 0x74}}

// Define the constant value
#define TA_REF_RUN_HELLO   0x11111111

/**
 * main() -To perform the TEEC operations for building 
 * TA inside TEE.
 * 
 * In this function the context is initialized for connecting to the TEE by calling
 * TEEC_InitializeContext(). After initialization of context the session is opened 
 * on TEEC_OpenSession() and then command is invoked in the TEE. Once the command 
 * is invoked the session is closed and the context is finalized. If the 
 * session is not opened properly, session_failed error appears.
 * 
 * @return 0	If success, else displays error message.
 */
int main(void)
{
    TEEC_Result res;
    TEEC_Context ctx;
    TEEC_Session sess;
    TEEC_Operation op;
    TEEC_UUID uuid = TA_UUID;
    uint32_t err_origin;

    // Initialize a context connecting us to the TEE
    res = TEEC_InitializeContext(NULL, &ctx);
    if (res != TEEC_SUCCESS)
      errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

    // Open "ref_ta"
    res = TEEC_OpenSession(&ctx, &sess, &uuid,
			   TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
    if (res != TEEC_SUCCESS)
      errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
	   res, err_origin);

    memset(&op, 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(
			TEEC_NONE,
			TEEC_PARAM_TYPE1,
			TEEC_NONE,
            TEEC_NONE
    );

    op.params[1].tmpref.buffer = (void*)print_buf;
    op.params[1].tmpref.size = PRINT_BUF_SIZE;

    // only TA_REF_RUN_ALL is defined
    res = TEEC_InvokeCommand(&sess, TA_REF_RUN_HELLO, &op,
			     &err_origin);

    if (res != TEEC_SUCCESS) {
      errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
	   res, err_origin);
    }

    // Done
    TEEC_CloseSession(&sess);

    TEEC_FinalizeContext(&ctx);

    return 0;
}
