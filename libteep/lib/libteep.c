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

struct teep_message *parse_teep_message(UsefulBufC cbor)
{
	QCBORDecodeContext DC;
	QCBORDecode_Init(&DC, cbor, QCBOR_DECODE_MODE_NORMAL);

	QCBORItem Item;
	QCBORDecode_GetNext(&DC, &Item);

	if (Item.uDataType != QCBOR_TYPE_ARRAY || Item.val.uCount < 3) {
		return NULL;
	}

	QCBORDecode_GetNext(&DC, &Item);
	if (Item.uDataType != QCBOR_TYPE_INT64) {
		return NULL;
	}
	int64_t type_val = Item.val.int64;
	switch (type_val) {
	case QUERY_REQUEST:
	case QUERY_RESPONSE:
	case TRUSTED_APP_INSTALL:
	case TRUSTED_APP_DELETE:
	case ERROR:
	case SUCCESS:
		break;
	default:
		return NULL;
	}

	QCBORDecode_GetNext(&DC, &Item);
	uint64_t token;
	if (Item.uDataType == QCBOR_TYPE_INT64) {
		if (QCBOR_Int64ToUInt64(Item.val.int64, &token) < 0) {
			return NULL;
		}
	} else if (Item.uDataType == QCBOR_TYPE_INT64) {
		token = Item.val.uint64;
	} else {
		return NULL;
	}

	QCBORItem options;
	QCBORDecode_GetNext(&DC, &options);
	if (options.uDataType != QCBOR_TYPE_MAP) {
		return NULL;
	}

	struct teep_message *m = malloc(sizeof *m);
	if (!m) {
		return NULL;
	}
	memset(m, 0, sizeof *m);
	m->type = type_val;
	m->token = token;

	for (int i = 0; i < options.val.uCount; i++) {
		QCBORItem option;
		QCBORDecode_GetNext(&DC, &option);
		if (option.uLabelType != QCBOR_TYPE_INT64) {
			goto err;
		}
		if (option.label.int64 == 8 || option.label.int64 == 10) {
			if (option.uDataType != QCBOR_TYPE_ARRAY) {
				goto err;
			}
			size_t len = option.val.uCount;
			UsefulBufC *p = malloc(len * sizeof (UsefulBufC));
			if (!p) {
				goto err;
			}
			for (size_t j = 0; j < len; j++) {
				QCBORItem ArrayItem;
				QCBORDecode_GetNext(&DC, &ArrayItem);

				if (ArrayItem.uDataType != QCBOR_TYPE_TEXT_STRING) {
					free(p);
					goto err;
				}
				p[i] = ArrayItem.val.string;
			}
			if (option.label.int64 == 10 && m->type == TRUSTED_APP_INSTALL) {
				m->trusted_app_install.manifest_list = p;
				m->trusted_app_install.manifest_list_len = len;
			} else if (option.label.int64 == 8 && m->type == TRUSTED_APP_DELETE) {
				m->trusted_app_delete.ta_list = p;
				m->trusted_app_delete.ta_list_len = len;
			} else {
				free(p);
				goto err;
			}
		} else { // TODO: handle more option
			while (option.uNextNestLevel != options.uNextNestLevel) {
				// skip unhandled option
				QCBORDecode_GetNext(&DC, &option);
				if (option.uDataType == QCBOR_TYPE_NONE) {
					goto err;
				}
			}
		}
	}

	switch (m->type) {
	case QUERY_REQUEST:
		QCBORDecode_GetNext(&DC, &Item);
		if (Item.uDataType != QCBOR_TYPE_INT64) {
			goto err;
		}
		m->query_request.data_item_requested = Item.val.int64;
		break;
		// TODO: err_code
	default:
		break;
	}

	if (QCBORDecode_Finish(&DC) != QCBOR_SUCCESS) {
		goto err;
	}

	return m;
err:
	free_teep_message(m);
	return NULL;
}

void free_teep_message(struct teep_message *message)
{
	if (!message) {
		return;
	}
	switch (message->type) {
	case QUERY_REQUEST:
		break;
	case QUERY_RESPONSE:
		break;
	case TRUSTED_APP_INSTALL:
		free(message->trusted_app_install.manifest_list);
		break;
	case TRUSTED_APP_DELETE:
		free(message->trusted_app_delete.ta_list);
		break;
	case SUCCESS:
		break;
	case ERROR:
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
	enum teep_message_type type,
	uint64_t token)
{
	QCBOREncode_AddInt64(&encoder->EC, type);
	QCBOREncode_AddUInt64(&encoder->EC, token);
}

void teep_message_encoder_open_options(struct teep_message_encoder *encoder)
{
	QCBOREncode_OpenMap(&encoder->EC);
}

void teep_message_encoder_open_ta_list(struct teep_message_encoder *encoder)
{
	QCBOREncode_OpenArrayInMapN(&encoder->EC, 8);
}

void teep_message_encoder_add_ta_to_ta_list(struct teep_message_encoder *encoder, const char *ta)
{
	QCBOREncode_AddSZString(&encoder->EC, ta);
}

void teep_message_encoder_close_ta_list(struct teep_message_encoder *encoder)
{
	QCBOREncode_CloseArray(&encoder->EC);
}

void teep_message_encoder_close_options(struct teep_message_encoder *encoder)
{
	QCBOREncode_CloseMap(&encoder->EC);
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
