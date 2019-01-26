/*
 * Copyright (C) 2019  National Institute of Advanced Industrial Science
 *                     and Technology (AIST）
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
 *
 * This is the user api for the libaistotrp (lao) REE userland library.
 *
 * It provides services for REE Applications to communicate with a remote TAM,
 * and the OTrP PTA in the Secure World.
 *
 * The return convention is 0 means all is OK.  Nonzero indicates an error.
 */

#if !defined(__LIBAISTOTRP__)
#define __LIBAISTOTRP__

#include <tee_client_api.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

struct libaistotrp_ctx;

struct lao_rpc_io {
	void	*in;
	size_t	in_len;
	void	*out;
	size_t	out_len;
};

/**
 * libaistotrp_init() - Initialize a libaistotrp context
 *
 * \param ctx: pointer to a pointer to your lao
 * \param tam_url_base: the DNS name a url path preamble to use to connect to
 *			the remote TAM over http, eg,
 *			"https://tam.mydomain.com:445/mytamapp".  A copy is made
 *			of this string before returning.
 *
 * Allocates an opaque structure containing the context for communications with
 * both the OTrP PTA in TEE-side and network communications to the TAM.
 *
 * The other apis of the library require a context created by this to operate.
 *
 * *ctx points to the allocated context on successful return.
 *
 * Returns 0 if created OK, else error and nothing was allocated.
 */
int
libaistotrp_init(struct libaistotrp_ctx **ctx, const char *tam_url_base);

/**
 * libaistotrp_destroy() - Destroy a libaistotrp context
 *
 * \param ctx: pointer to a pointer to your lao
 *
 * Destroys the context created by libaistotrp_init().  *ctx is set to NULL
 * on return.
 */
void
libaistotrp_destroy(struct libaistotrp_ctx **ctx);

/**
 * libaistotrp_pta_msg() - Send a message to the TEE OTrP PTA and get the result
 *
 * \param ctx: pointer to your lao
 * \param cmd: Message cmd index to send to PTA
 * \param io: pointer to struct pointing to in and out buffers and lengths.  On
 * 		entry, \p io.out_len must be set to the maximum length that can
 * 		be written to \p io.out.  On exit, it has been set to the number
 * 		of bytes actually used at \p io.out.
 *
 * Communication to the TEE is blocking by design.
 *
 * Returns 0 if the communication went OK, else error.
 */
int
libaistotrp_pta_msg(struct libaistotrp_ctx *ctx, uint32_t cmd,
		    struct lao_rpc_io *io);


typedef enum tam_result {
	TR_ONGOING		=  1,
	TR_OKAY			=  0,
	TR_FAIL_START		= -1,
	TR_FAIL_CONN_ERR	= -2,
	TR_FAIL_REFUSED		= -3,
	TR_FAIL_OVERSIZE	= -4,
	TR_FAIL_CLOSED		= -5,
} tam_result;

/**
 * libaistotrp_tam_msg() - Send a network message to the TAM and get the result
 *
 * \param ctx:    pointer to your lao
 * \param urlinfo: right-hand side of URL, added to the left-hand size given
 *		   at context-creation time to form the GET URL given to the TAM
 * \param io:     pointer to struct pointing to in and out buffers and lengths.
 *		  On entry, \p io.out_len must be set to the maximum length that
 *		  can be written to \p io.out.  On exit, it has been set to the
 *		  number of bytes actually used at \p io.out.
 *
 * This blocks until communication with the TAM was either successful, failed,
 * or timed out.
 *
 * \p io itself and its \p io.in and \p io.out buffers must be reserved after a
 * successful return from libaistotrp_tam_msg(), until the completion callback
 * is called one way or the other.
 *
 * Returns 0 if the communication was started OK, else error.
 */
int
libaistotrp_tam_msg(struct libaistotrp_ctx *ctx, const char *urlinfo,
		    struct lao_rpc_io *io);

#endif

