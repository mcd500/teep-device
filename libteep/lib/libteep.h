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
 *
 */

#if !defined(__LIBAISTTEEP__)
#define __LIBAISTTEEP__

#include <stdint.h>
#include <stdbool.h>
#include <qcbor/qcbor.h>

#ifdef __cplusplus
extern "C" {
#endif

enum teep_message_type {
	QUERY_REQUEST = 1,
	QUERY_RESPONSE = 2,
	TRUSTED_APP_INSTALL = 3,
	TRUSTED_APP_DELETE = 4,
	SUCCESS = 5,
	ERROR = 6
};

enum teep_suite {
	TEEP_AES_CCM_16_64_128_HMAC256_256_X25519_EdDSA = 1,
	TEEP_AES_CCM_16_64_128_HMAC256_256_P_256_ES256  = 2,
};

struct teep_message {
	enum teep_message_type type;
	uint64_t token;
	union {
		struct {
			enum teep_suite supported_cipher_suit;
			unsigned char nonce[64];
			size_t nonce_len;
			uint32_t version;
			UsefulBufC ocsp_data;
			int64_t data_item_requested;
		} query_request;
		struct {
			enum teep_suite selected_cipher_suit;
			uint32_t selected_version;
			UsefulBufC eat;
			UsefulBufC ta_list[10];
			size_t ta_list_len;
			UsefulBufC ext_list[10];
			size_t ext_list_len;
		} query_response;
		struct {
			UsefulBufC *manifest_list;
			size_t manifest_list_len;
		} trusted_app_install;
		struct {
			UsefulBufC *ta_list;
			size_t ta_list_len;
		} trusted_app_delete;
		struct {
			UsefulBufC err_msg;
			enum teep_suite cipher_suit;
			uint32_t version;
			uint64_t err_code;
		} error;
		struct {
			UsefulBufC msg;
		} success;
	};
};

// TODO: cose support
struct teep_message *parse_teep_message(UsefulBufC cbor);
void free_teep_message(struct teep_message *message);

struct teep_message_encoder
{
	QCBOREncodeContext EC;
};

void teep_message_encoder_init(struct teep_message_encoder *encoder, UsefulBuf buffer);
void teep_message_encoder_add_header(struct teep_message_encoder *encoder,
	enum teep_message_type type,
	uint64_t token);

void teep_message_encoder_open_options(struct teep_message_encoder *encoder);
void teep_message_encoder_open_ta_list(struct teep_message_encoder *encoder);
void teep_message_encoder_add_ta_to_ta_list(struct teep_message_encoder *encoder, const char *ta);
void teep_message_encoder_close_ta_list(struct teep_message_encoder *encoder);
void teep_message_encoder_close_options(struct teep_message_encoder *encoder);

void teep_message_encoder_add_err_code(struct teep_message_encoder *encoder, uint64_t err_code);

QCBORError teep_message_encoder_finish(struct teep_message_encoder *encoder, UsefulBufC *encoded);

#ifdef __cplusplus
}
#endif

#endif

