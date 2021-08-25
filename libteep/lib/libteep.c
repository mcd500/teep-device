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

#include <stdlib.h>
#include "libteep.h"
#include "qcbor-ext.h"

static int parse_uint32_array(struct teep_uint32_array *p, QCBORDecodeContext *DC, const QCBORItem *option)
{
	if (option->uDataType != QCBOR_TYPE_ARRAY) {
		return 0;
	}
	p->len = option->val.uCount;
	p->array = malloc(p->len * sizeof (p->array[0]));
	if (!p->array) {
		return 0;
	}
	for (size_t i = 0; i < p->len; i++) {
		QCBORItem Item;
		QCBORDecode_GetNext(DC, &Item);
		if (Item.uDataType == QCBOR_TYPE_INT64) {
			if (QCBOR_Int64ToUInt32(Item.val.int64, &p->array[i]) < 0) {
				return 0;
			}
		} else {
			return 0;
		}
	}
	p->have_value = 1;
	return 1;
}

static int parse_uint32_option(struct teep_uint32_option *p, QCBORDecodeContext *DC, const QCBORItem *option)
{
	if (option->uDataType == QCBOR_TYPE_INT64) {
		if (QCBOR_Int64ToUInt32(option->val.int64, &p->value) < 0) {
			return 0;
		}
	} else {
		return 0;
	}
	p->have_value = 1;
	return 1;
}

static int parse_buffer_array(struct teep_buffer_array *p, QCBORDecodeContext *DC, const QCBORItem *option, bool binary)
{
	if (option->uDataType != QCBOR_TYPE_ARRAY) {
		return 0;
	}
	p->len = option->val.uCount;
	p->array = malloc(p->len * sizeof (p->array[0]));
	if (!p->array) {
		return 0;
	}
	for (size_t i = 0; i < p->len; i++) {
		QCBORItem Item;
		QCBORDecode_GetNext(DC, &Item);
		if (Item.uDataType == QCBOR_TYPE_BYTE_STRING && binary) {
			p->array[i] = Item.val.string;
		} else if (Item.uDataType == QCBOR_TYPE_TEXT_STRING && !binary) {
			p->array[i] = Item.val.string;
		} else {
			return 0;
		}
	}
	p->have_value = 1;
	return 1;
}

static int parse_component_id(teep_component_id *p, QCBORDecodeContext *DC, const QCBORItemWithOffset *component_id)
{
	if (component_id->item.uDataType != QCBOR_TYPE_ARRAY) {
		return 0;
	}
	uint32_t len = component_id->item.val.uCount;
	for (uint32_t i = 0; i < len; i++) {
		QCBORItem Item;
		QCBORDecode_GetNext(DC, &Item);
		if (Item.uDataType != QCBOR_TYPE_BYTE_STRING && Item.uDataType != QCBOR_TYPE_TEXT_STRING) {
			return 0;
		}
	}
	*p = QCBORDecode_Slice(DC, component_id->offset, QCBORDecode_Tell(DC));
	return 1;
}

static int parse_tc_info_array(struct teep_tc_info_array *p, QCBORDecodeContext *DC, const QCBORItem *option, bool requested)
{
	if (option->uDataType != QCBOR_TYPE_ARRAY) {
		return 0;
	}
	p->len = option->val.uCount;
	p->array = malloc(p->len * sizeof (p->array[0]));
	if (!p->array) {
		return 0;
	}
	memset(p->array, 0, p->len * sizeof (p->array[0]));
	for (size_t i = 0; i < p->len; i++) {
		QCBORItem Map;
		QCBORDecode_GetNext(DC, &Map);
		if (Map.uDataType != QCBOR_TYPE_MAP) {
			return 0;
		}
		for (size_t j = 0; j < Map.val.uCount; j++) {
			QCBORItemWithOffset Item;
			QCBORDecode_GetNextWithOffset(DC, &Item);
			if (Item.item.uLabelType != QCBOR_TYPE_INT64) {
				return 0;
			}
			int64_t label = Item.item.label.int64;
			switch (label) {
			case TEEP_OPTION_COMPONENT_ID:
				{
					if (!parse_component_id(&p->array[i].component_id, DC, &Item)) {
						return 0;
					}
				}
				break;
			case TEEP_OPTION_TC_MANIFEST_SEQUENCE_NUMBER:
				{
					if (!parse_uint32_option(&p->array[i].tc_manifest_sequence_number, DC, &Item.item)) {
						return 0;
					}
				}
				break;
			case TEEP_OPTION_HAVE_BINARY:
				{
					if (!requested) {
						return 0;
					}
					if (!parse_uint32_option(&p->array[i].have_binary, DC, &Item.item)) {
						return 0;
					}
				}
			default:
				return 0;
			}
		}
	}
	p->have_value = 1;
	return 1;
}

static int parse_component_id_array(struct teep_component_id_array *p, QCBORDecodeContext *DC, const QCBORItem *option)
{
	if (option->uDataType != QCBOR_TYPE_ARRAY) {
		return 0;
	}
	p->len = option->val.uCount;
	p->array = malloc(p->len * sizeof (p->array[0]));
	if (!p->array) {
		return 0;
	}
	memset(p->array, 0, p->len * sizeof (p->array[0]));
	for (size_t i = 0; i < p->len; i++) {
		QCBORItemWithOffset Item;
		QCBORDecode_GetNextWithOffset(DC, &Item);
		if (!parse_component_id(&p->array[i], DC, &Item)) {
			return 0;
		}
	}
	p->have_value = 1;
	return 1;
}

static int parse_option(struct teep_message *m, QCBORDecodeContext *DC, QCBORItem *option, uint8_t nest_level)
{
	if (option->uLabelType != QCBOR_TYPE_INT64) {
		goto err;
	}
	int64_t label = option->label.int64;

	switch (label) {
	case TEEP_OPTION_SUPPORTED_CIPHER_SUITS:
		{
			if (m->type == TEEP_QUERY_REQUEST) {
				if (!parse_uint32_array(&m->query_request.supported_cipher_suits, DC, option)) {
					goto err;
				}
			} else if (m->type == TEEP_ERROR) {
				if (!parse_uint32_array(&m->teep_error.supported_cipher_suits, DC, option)) {
					goto err;
				}
			} else {
				goto skip;
			}
		}
		break;
	case TEEP_OPTION_CHALLENGE:
	case TEEP_OPTION_OCSP_DATA:
		{
			if (m->type != TEEP_QUERY_REQUEST) {
				goto skip;
			}
			if (option->uDataType != QCBOR_TYPE_BYTE_STRING) {
				goto err;
			}
			if (label == TEEP_OPTION_CHALLENGE) {
				m->query_request.challenge = option->val.string;
			} else {
				m->query_request.ocsp_data = option->val.string;
			}
		}
		break;
	case TEEP_OPTION_VERSIONS:
		{
			if (m->type == TEEP_QUERY_REQUEST) {
				if (!parse_uint32_array(&m->query_request.versions, DC, option)) {
					goto err;
				}
			} else if (m->type == TEEP_ERROR) {
				if (!parse_uint32_array(&m->teep_error.versions, DC, option)) {
					goto err;
				}
			} else {
				goto skip;
			}
		}
		break;
	case TEEP_OPTION_SELECTED_CIPHER_SUIT:
		{
			if (m->type != TEEP_QUERY_RESPONSE) {
				goto skip;
			}
			if (!parse_uint32_option(&m->query_response.selected_cipher_suit, DC, option)) {
				goto err;
			}
		}
		break;
	case TEEP_OPTION_SELECTED_VERSION:
		{
			if (m->type != TEEP_QUERY_RESPONSE) {
				goto skip;
			}
			if (!parse_uint32_option(&m->query_response.selected_version, DC, option)) {
				goto err;
			}
		}
		break;
	case TEEP_OPTION_EVIDENCE_FORMAT:
		{
			if (m->type != TEEP_QUERY_RESPONSE) {
				goto skip;
			}
			if (option->uDataType != QCBOR_TYPE_TEXT_STRING) {
				goto err;
			}
			m->query_response.evidence_format = option->val.string;
		}
		break;
	case TEEP_OPTION_EVIDENCE:
		{
			if (m->type != TEEP_QUERY_RESPONSE) {
				goto skip;
			}
			if (option->uDataType != QCBOR_TYPE_BYTE_STRING) {
				goto err;
			}
			m->query_response.evidence = option->val.string;
		}
		break;
	case TEEP_OPTION_TC_LIST:
		{
			if (m->type != TEEP_QUERY_RESPONSE) {
				goto skip;
			}
			if (!parse_tc_info_array(&m->query_response.tc_list, DC, option, false)) {
				goto err;
			}
		}
		break;
	case TEEP_OPTION_REQUESTED_TC_LIST:
		{
			if (m->type != TEEP_QUERY_RESPONSE) {
				goto skip;
			}
			if (!parse_tc_info_array(&m->query_response.requested_tc_list, DC, option, true)) {
				goto err;
			}
		}
		break;
	case TEEP_OPTION_UNNEEDED_TC_LIST:
		{
			if (m->type == TEEP_QUERY_RESPONSE) {
				if (!parse_component_id_array(&m->query_response.unneeded_tc_list, DC, option)) {
					goto err;
				}
			} else {
				goto skip;
			}
		}
		break;
	case TEEP_OPTION_EXT_LIST:
		{
			if (m->type != TEEP_QUERY_RESPONSE) {
				goto skip;
			}
			if (!parse_uint32_array(&m->query_response.ext_list, DC, option)) {
				goto err;
			}
		}
		break;
	case TEEP_OPTION_MANIFEST_LIST:
		{
			if (m->type != TEEP_UPDATE) {
				goto skip;
			}
			if (!parse_buffer_array(&m->teep_update.manifest_list, DC, option, true)) {
				goto err;
			}
		}
		break;
	case TEEP_OPTION_MSG:
		{
			if (m->type != TEEP_SUCCESS) {
				goto skip;
			}
			if (option->uDataType != QCBOR_TYPE_TEXT_STRING) {
				goto err;
			}
			m->teep_success.msg = option->val.string;
		}
		break;
	case TEEP_OPTION_ERR_MSG:
		{
			if (m->type != TEEP_ERROR) {
				goto skip;
			}
			if (option->uDataType != QCBOR_TYPE_TEXT_STRING) {
				goto err;
			}
			m->teep_error.err_msg = option->val.string;
		}
		break;
	case TEEP_OPTION_SUIT_REPORTS:
		{
			if (m->type == TEEP_SUCCESS) {
				if (!parse_buffer_array(&m->teep_success.suit_reports, DC, option, true)) {
					goto err;
				}
			} else if (m->type == TEEP_ERROR) {
				if (!parse_buffer_array(&m->teep_error.suit_reports, DC, option, true)) {
					goto err;
				}
			} else {
				goto skip;
			}
		}
		break;
	case TEEP_OPTION_TOKEN:
		{
			uint64_t token;
			if (option->uDataType != QCBOR_TYPE_BYTE_STRING) {
				goto err;
			}
			m->token = option->val.string;
		}
		break;
	case TEEP_OPTION_SUPPORTED_FRESHNESS_MECHANISMS:
		{
			if (m->type == TEEP_QUERY_REQUEST) {
				if (!parse_uint32_array(&m->query_request.supported_freshness_mechanisms, DC, option)) {
					goto err;
				}
			} else if (m->type == TEEP_ERROR) {
				if (!parse_uint32_array(&m->teep_error.supported_freshness_mechanisms, DC, option)) {
					goto err;
				}
			} else {
				goto skip;
			}
		}
		break;
	default:
	skip:
		{
			uint8_t level = option->uNextNestLevel;	
			// skip unknown option
			while (level != nest_level) {
				QCBORItem Item;
				QCBORDecode_GetNext(DC, &Item);
				if (Item.uDataType == QCBOR_TYPE_NONE) {
					goto err;
				}
				level = Item.uNextNestLevel;
			}
		}
		break;
	}
	return 1;
err:
	return 0;
}

static int parse_options(struct teep_message *m, QCBORDecodeContext *DC)
{
	QCBORItem options;
	QCBORDecode_GetNext(DC, &options);
	if (options.uDataType != QCBOR_TYPE_MAP) {
			return 0;
	}

	for (int i = 0; i < options.val.uCount; i++) {
		QCBORItem option;
		QCBORDecode_GetNext(DC, &option);
		if (!parse_option(m, DC, &option, options.uNextNestLevel)) {
			return 0;
		}
	}
	return 1;
}

struct teep_message *parse_teep_message(UsefulBufC cbor)
{
	QCBORDecodeContext DC;
	QCBORDecode_Init(&DC, cbor, QCBOR_DECODE_MODE_NORMAL);

	QCBORItem Item;
	QCBORDecode_GetNext(&DC, &Item);

	if (Item.uDataType != QCBOR_TYPE_ARRAY) {
		return NULL;
	}

	QCBORDecode_GetNext(&DC, &Item);
	if (Item.uDataType != QCBOR_TYPE_INT64) {
		return NULL;
	}
	int64_t type_val = Item.val.int64;
	switch (type_val) {
	case TEEP_QUERY_REQUEST:
	case TEEP_QUERY_RESPONSE:
	case TEEP_UPDATE:
	case TEEP_SUCCESS:
	case TEEP_ERROR:
		break;
	default:
		return NULL;
	}

	struct teep_message *m = malloc(sizeof *m);
	if (!m) {
		return NULL;
	}
	memset(m, 0, sizeof *m);
	m->type = type_val;
	m->token = NULLUsefulBufC;

	if (!parse_options(m, &DC)) {
		goto err;
	}

	switch (m->type) {
	case TEEP_QUERY_REQUEST:
		QCBORDecode_GetNext(&DC, &Item);
		if (Item.uDataType != QCBOR_TYPE_INT64) {
			goto err;
		}
		m->query_request.data_item_requested = Item.val.int64;
		break;
	case TEEP_ERROR:
		QCBORDecode_GetNext(&DC, &Item);
		if (Item.uDataType != QCBOR_TYPE_INT64) {
			goto err;
		}
		m->teep_error.err_code = Item.val.int64;
		break;
	default:
		break;
	}

	if (QCBORDecode_Finish(&DC) != QCBOR_SUCCESS) {
		goto err;
	}

	return m;
err:
	free_parsed_teep_message(m);
	return NULL;
}

void free_parsed_teep_message(struct teep_message *message)
{
	if (!message) {
		return;
	}
	switch (message->type) {
	case TEEP_QUERY_REQUEST:
		free(message->query_request.supported_cipher_suits.array);
		free(message->query_request.supported_freshness_mechanisms.array);
		free(message->query_request.versions.array);
		break;
	case TEEP_QUERY_RESPONSE:
		free(message->query_response.tc_list.array);
		free(message->query_response.requested_tc_list.array);
		free(message->query_response.unneeded_tc_list.array);
		free(message->query_response.ext_list.array);
		break;
	case TEEP_UPDATE:
		free(message->teep_update.manifest_list.array);
		break;
	case TEEP_SUCCESS:
		free(message->teep_success.suit_reports.array);
		break;
	case TEEP_ERROR:
		free(message->teep_error.supported_cipher_suits.array);
		free(message->teep_error.supported_freshness_mechanisms.array);
		free(message->teep_error.versions.array);
		free(message->teep_error.suit_reports.array);
		break;
	}
	free(message);
}

void teep_message_encoder_init(struct teep_message_encoder *encoder, UsefulBuf buffer)
{
	QCBOREncode_Init(&encoder->EC, buffer);
	QCBOREncode_OpenArray(&encoder->EC);
}

void teep_message_encoder_add_header(struct teep_message_encoder *encoder,
	enum teep_message_type type)
{
	QCBOREncode_AddInt64(&encoder->EC, type);
}

void teep_message_encoder_open_options(struct teep_message_encoder *encoder)
{
	QCBOREncode_OpenMap(&encoder->EC);
}

void teep_message_encoder_close_options(struct teep_message_encoder *encoder)
{
	QCBOREncode_CloseMap(&encoder->EC);
}

void teep_message_encoder_add_token(struct teep_message_encoder *encoder, UsefulBufC token)
{
	if (!UsefulBuf_IsNULLC(token)) {
		QCBOREncode_AddBytesToMapN(&encoder->EC, TEEP_OPTION_TOKEN, token);
	}
}

void teep_message_encoder_open_tc_list(struct teep_message_encoder *encoder)
{
	QCBOREncode_OpenArrayInMapN(&encoder->EC, TEEP_OPTION_TC_LIST);
}

void teep_message_encoder_add_tc_to_tc_list(struct teep_message_encoder *encoder, const char *ta)
{
	//QCBOREncode_OpenMap(&encoder->EC);
	//QCBOREncode_OpenArrayInMapN(&encoder->EC, TEEP_OPTION_COMPONENT_ID);
	QCBOREncode_AddSZString(&encoder->EC, ta);
	//QCBOREncode_CloseArray(&encoder->EC);
	//QCBOREncode_CloseMap(&encoder->EC);
}

void teep_message_encoder_close_tc_list(struct teep_message_encoder *encoder)
{
	QCBOREncode_CloseArray(&encoder->EC);
}

void teep_message_encoder_open_requested_tc_list(struct teep_message_encoder *encoder)
{
	QCBOREncode_OpenArrayInMapN(&encoder->EC, TEEP_OPTION_REQUESTED_TC_LIST);
}

void teep_message_encoder_add_tc_to_requested_tc_list(struct teep_message_encoder *encoder, const char *ta)
{
	QCBOREncode_OpenMap(&encoder->EC);
	QCBOREncode_OpenArrayInMapN(&encoder->EC, TEEP_OPTION_COMPONENT_ID);
	QCBOREncode_AddSZString(&encoder->EC, ta);
	QCBOREncode_CloseArray(&encoder->EC);
	QCBOREncode_CloseMap(&encoder->EC);
}

void teep_message_encoder_close_requested_tc_list(struct teep_message_encoder *encoder)
{
	QCBOREncode_CloseArray(&encoder->EC);
}

void teep_message_encoder_open_unneeded_tc_list(struct teep_message_encoder *encoder)
{
	QCBOREncode_OpenArrayInMapN(&encoder->EC, TEEP_OPTION_UNNEEDED_TC_LIST);
}

void teep_message_encoder_add_tc_to_unneeded_tc_list(struct teep_message_encoder *encoder, const char *ta)
{
	QCBOREncode_OpenArray(&encoder->EC);
	QCBOREncode_AddSZString(&encoder->EC, ta);
	QCBOREncode_CloseArray(&encoder->EC);
}

void teep_message_encoder_close_unneeded_tc_list(struct teep_message_encoder *encoder)
{
	QCBOREncode_CloseArray(&encoder->EC);
}


void teep_message_encoder_add_err_code(struct teep_message_encoder *encoder, uint64_t err_code)
{
	QCBOREncode_AddUInt64(&encoder->EC, err_code);
}

QCBORError teep_message_encoder_finish(struct teep_message_encoder *encoder, UsefulBufC *encoded)
{
	QCBOREncode_CloseArray(&encoder->EC);
	return QCBOREncode_Finish(&encoder->EC, encoded);
}
