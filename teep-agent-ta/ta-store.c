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

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "teep-agent-ta.h"
#include "ta-store.h"
#include "teelog.h"

#ifdef PLAT_KEYSTONE
#include <edger/Enclave_t.h>

static bool install_ta(const char *filename, const void *image, size_t image_len)
{
	bool ret = false;

	int fd = ocall_open_file(filename, O_CREAT | O_WRONLY, 0600);
	if (fd < 0) goto bail_1;
	int n = ocall_write_file_full(fd, image, image_len);
	if (n != image_len) goto bail_2;

	ret = true;
bail_2:
	ocall_close_file(fd);
bail_1:
	return ret;
}

#endif

#ifdef PLAT_OPTEE
#include <tee_internal_api.h>
#include <pta_secstor_ta_mgmt.h>
#include <time.h>
#include <string.h>

static bool install_ta(const char *filename, const void *image, size_t image_len)
{
	TEE_TASessionHandle sess = TEE_HANDLE_NULL;
	const TEE_UUID secstor_uuid = PTA_SECSTOR_TA_MGMT_UUID;
	TEE_Param pars[TEE_NUM_PARAMS];
	TEE_Result res;
	res = TEE_OpenTASession(&secstor_uuid, 0, 0, NULL, &sess, NULL);
	if (res != TEE_SUCCESS) {
		return false;
	}

	memset(pars, 0, sizeof(pars));
	pars[0].memref.buffer = (void *)image;
	pars[0].memref.size = image_len;
	res = TEE_InvokeTACommand(sess, 0,
			PTA_SECSTOR_TA_MGMT_BOOTSTRAP,
			TEE_PARAM_TYPES(
				TEE_PARAM_TYPE_MEMREF_INPUT,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE,
				TEE_PARAM_TYPE_NONE),
			pars, NULL);
	TEE_CloseTASession(sess);
	if (res != TEE_SUCCESS) {
		return false;
	}
	return true;
}

#endif

#ifdef PLAT_PC
#include<stdio.h>

static bool install_ta(const char *filename, const void *image, size_t image_len)
{
	FILE* fd = NULL;
	bool ret = true;
	size_t bytes = 0;

	fd = fopen(filename, "w+");
	if (NULL == fd) {
		tee_error("install_ta(): fopen(): ERROR\n");
		return false;
	}

	bytes = fwrite(image, 1, image_len, fd);
	if (bytes < image_len) {
		tee_error("install_ta(): fwrite(): ERROR\n");
		ret = false; goto bail_2;
	}

bail_2:
	fclose(fd);

	return ret;
}

#endif

#ifdef PLAT_SGX
#include "edger/Enclave_t.h"
#include <asm-generic/fcntl.h>
static bool install_ta(const char *filename, const void *image, size_t image_len)
{
	// Declare the return variable
	bool ret = false;
	// File descriptor and return value
	int fd, retval;
	long unsigned int n;

	// Open the file with the flags and permission
	retval = ocall_open_file(&fd, filename, O_RDWR | O_CREAT, 0600);
	tee_info("Return value of ocall_open_file is : retval  = %u\n", retval);
	if (retval < 0) {
		tee_error("Return value of ocall_open_file is not proper : retval  = %u\n", retval);
		goto bail_1;
	} 

	n = ocall_write_file(&retval, fd, image, image_len);
	if (retval != image_len) {
		tee_error("Return value of ocall_write_file is not proper : retval  = %u\n", retval);
		goto bail_2;
	} 

	// Set the return value to success
	ret = true;
bail_2:
	// Close the opened file 
	ocall_close_file(&retval, fd);
bail_1:
	// Return the status
	return ret;

}

#endif

bool store_component(const struct component_path *path, const void *image, size_t image_len)
{
	tee_trace("ta-store.c: store_component() store component\n");
	tee_trace("  device   = %s\n", path->device);
	tee_trace("  storage  = %s\n", path->storage);
	//tee_trace("  uuid     = %s\n", path.uuid);
	tee_trace("  filename = %s\n", path->filename);
	tee_trace("  image_len = %u\n", image_len);

	return install_ta(path->filename, image, image_len);
}
