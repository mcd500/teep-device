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
	TEEP_QUERY_REQUEST = 1,
	TEEP_QUERY_RESPONSE = 2,
	TEEP_INSTALL = 3,
	TEEP_DELETE = 4,
	TEEP_SUCCESS = 5,
	TEEP_ERROR = 6
};

enum teep_data_item {
	TEEP_DATA_ATTESTATION = 1,
	TEEP_DATA_TRUSTED_COMPONENTS = 2,
	TEEP_DATA_EXTENSIONS = 4,
	TEEP_DATA_SUIT_COMMANDS = 8,
};

enum teep_suite {
	TEEP_AES_CCM_16_64_128_HMAC256_256_X25519_EdDSA = 1,
	TEEP_AES_CCM_16_64_128_HMAC256_256_P_256_ES256  = 2,
};

enum teep_error {
	TEEP_ERR_ILLEGAL_PARAMETER = 1,
	TEEP_ERR_UNSUPPORTED_EXTENSION = 2,
	TEEP_ERR_REQUEST_SIGNATURE_FAILED = 3,
	TEEP_ERR_UNSUPPORTED_MSG_VERSION = 4,
	TEEP_ERR_UNSUPPORTED_CRYPTO_ALG = 5,
	TEEP_ERR_BAD_CERTIFICATE = 6,
	TEEP_ERR_UNSUPPORTED_CERTIFICATE = 7,
	TEEP_ERR_CERTIFICATE_REVOKED = 8,
	TEEP_ERR_CERTIFICATE_EXPIRED = 9,
	TEEP_ERR_INTERNAL_ERROR = 10,
	TEEP_ERR_TC_NOT_FOUND = 12,
	TEEP_ERR_MANIFEST_PROCESSING_FAILED = 17,
};

enum teep_option_key {
	TEEP_OPTION_SUPPORTED_CIPHER_SUITS = 1,
	TEEP_OPTION_CHALLENGE = 2,
	TEEP_OPTION_VERSIONS = 3,
	TEEP_OPTION_OCSP_DATA = 4,
	TEEP_OPTION_SELECTED_CIPHER_SUIT = 5,
	TEEP_OPTION_SELECTED_VERSION = 6,
	TEEP_OPTION_EVIDENCE = 7,
	TEEP_OPTION_TC_LIST = 8,
	TEEP_OPTION_EXT_LIST = 9,
	TEEP_OPTION_MANIFEST_LIST = 10,
	TEEP_OPTION_MSG = 11,
	TEEP_OPTION_ERR_MSG = 12,
	TEEP_OPTION_EVIDENCE_FORMAT = 13,
	TEEP_OPTION_REQUESTED_TC_LIST = 14,
	TEEP_OPTION_UNNEEDED_TC_LIST = 15,
	TEEP_OPTION_COMPONENT_ID = 16,
	TEEP_OPTION_TC_MANIFEST_SEQUENCE_NUMBER = 17,
	TEEP_OPTION_HAVE_BINARY = 18,
	TEEP_OPTION_SUIT_REPORTS = 19,
};

struct teep_uint32_array {
	bool have_value;
	uint32_t *array;
	size_t len;
};

struct teep_uint32_option {
	bool have_value;
	uint32_t value;
};

struct teep_buffer_array {
	bool have_value;
	UsefulBufC *array;
	size_t len;
};

struct teep_tc_info {
	UsefulBufC component_id;
	struct teep_uint32_option tc_manifest_sequence_number;
	struct teep_uint32_option have_binary;
};

struct teep_tc_info_array {
	bool have_value;
	struct teep_tc_info *array;
	size_t len;
};

struct teep_message {
	enum teep_message_type type;
	uint64_t token;
	union {
		struct {
			struct teep_uint32_array supported_cipher_suits;
			UsefulBufC challenge;
			struct teep_uint32_array versions;
			UsefulBufC ocsp_data;
			uint64_t data_item_requested;
		} query_request;
		struct {
			struct teep_uint32_option selected_cipher_suit;
			struct teep_uint32_option selected_version;
			UsefulBufC evidence_format;
			UsefulBufC evidence;
			struct teep_tc_info_array tc_list;
			struct teep_tc_info_array requested_tc_list;
			struct teep_buffer_array unneeded_tc_list;
			struct teep_uint32_array ext_list;
		} query_response;
		struct {
			struct teep_buffer_array manifest_list;
		} teep_install;
		struct {
			struct teep_buffer_array ta_list;
		} teep_delete;
		struct {
			UsefulBufC msg;
			struct teep_buffer_array suit_reports;
		} teep_success;
		struct {
			int64_t err_code;
			UsefulBufC err_msg;
			struct teep_uint32_array supported_cipher_suits;
			struct teep_uint32_array versions;
			struct teep_buffer_array suit_reports;
		} teep_error;
	};
};

// TODO: cose support
struct teep_message *parse_teep_message(UsefulBufC cbor);
void free_parsed_teep_message(struct teep_message *message);

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

