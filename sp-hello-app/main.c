/*
 * Copyright (C) 2019  National Institute of Advanced Industrial Science
 *                     and Technology (AIST)
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

#include <tee_client_api.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

/* UUID of sp-hello-ta, the remotely-installed TA */

const TEEC_UUID uuid =
        { 0x8d82573a, 0x926d, 0x4754, \
                { 0x93, 0x53, 0x32, 0xdc, 0x29, 0x99, 0x7f, 0x74 } };
 
TEEC_Context *context;
TEEC_Session *session;
TEEC_SharedMemory shm;
uint8_t *filecontents;
size_t file_length;


TEEC_Result
sp_hello_app()
{
	TEEC_Result n;
	TEEC_Operation op;
	uint8_t result[256];
	int m;

	memset(&op, 0, sizeof(TEEC_Operation));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE,
			TEEC_NONE);

	n = TEEC_InvokeCommand(session, 1, &op, NULL);
	if (n != TEEC_SUCCESS) {
		fprintf(stderr, "[aist_otrp_client] %s: TEEC_InvokeCommand "
		      "failed (%08x), %d\n", __func__, n, __LINE__);
		return n;
	}

	for (m = 0; m < op.params[3].value.a; m++)
		fprintf(stderr, "%02X", result[m]);

	fprintf(stderr, "\n");

//	fprintf(stderr, "%s: done\n", __func__);

	return n;
}

int main(int argc, char *argv[])
{
	int tries;
	TEEC_Result nResult;
	TEEC_Operation op;

//	printf("START: sp-hello-app\n");

	context = (TEEC_Context *)malloc(sizeof(*context));
	if (!context) {
		printf("OOM\n");

		return 1;
	}
	memset(context, 0, sizeof(*context));

	nResult = TEEC_InitializeContext(NULL, context);
	if (nResult != TEEC_SUCCESS) {
		printf("context open failed 0x%x\n", nResult);
		goto bail1;
	}

	session = (TEEC_Session *)malloc(sizeof(TEEC_Session));
	if (session == NULL) {
		printf("OOM2 \n");

		goto bail2;
	}
	memset(session, 0, sizeof(*session));
	memset(&op, 0, sizeof(TEEC_Operation));

	op.paramTypes = 0;
	nResult = TEEC_OpenSession(context,
			session,             /* OUT session */
			&uuid,                /* destination UUID*/
			TEEC_LOGIN_PUBLIC,    /* connectionMethod */
			NULL,                 /* connectionData */
			&op,          /* IN OUT operation */
			NULL                  /* OUT returnOrigin, opt*/
	);

	if (nResult != TEEC_SUCCESS) {
		fprintf(stderr, "Could not open session with TA\n");
		goto bail3;
	}

	nResult = sp_hello_app();

	if (nResult != TEEC_SUCCESS) {
		fprintf(stderr, "Could not send command to TA\n");
		goto bail4;
	}

	return 0;
bail4:
	TEEC_CloseSession(session);
bail3:
	free(session);
bail2:
	TEEC_FinalizeContext(context);
bail1:
	free(context);

	return 0;
}

