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

#include <libwebsockets.h>
#include <qcbor/qcbor.h>
#include "teep_message.h"
#include "ta-store.h"

char *strncat(char *dest, const char *src, size_t n);


/* the TAM server public key as a JWK */
static const char * const tam_id_pubkey_jwk =
#include "tam_id_pubkey_jwk.h"
;

/* the TEE private key as a JWK */
static const char * const tee_id_privkey_jwk =
#include "tee_id_privkey_jwk.h"
;

enum teep_message_type {
	QUERY_REQUEST = 1,
	QUERY_RESPONSE = 2,
	TRUSTED_APP_INSTALL = 3,
	TRUSTED_APP_DELETE = 4,
	SUCCESS = 5,
	ERROR = 6
};

static char ta_list_buf[256];

struct teep_mesg_cbor {
	int32_t type;
	int64_t token;
	union {
		struct {
			//... TODO
			int64_t data_item_requested;
		} query_request;
		struct {
			UsefulBufC manifest_list[10];
			size_t manifest_list_len;
		} trusted_app_install;
		struct {
			UsefulBufC ta_list[10];
			size_t ta_list_len;
		} trusted_app_delete;
	};
};

static int parse_teep_message_cbor(struct teep_mesg_cbor *m, UsefulBufC cbor)
{
	m->type = -1;

	QCBORDecodeContext DC;
	QCBORDecode_Init(&DC, cbor, QCBOR_DECODE_MODE_NORMAL);

	QCBORItem Item;
	QCBORDecode_GetNext(&DC, &Item);
	if (Item.uDataType != QCBOR_TYPE_ARRAY || Item.val.uCount < 3) {
		lwsl_err("CBOR parse error: array is expected\n");
		return -1;
	}

	QCBORDecode_GetNext(&DC, &Item);
	if (Item.uDataType != QCBOR_TYPE_INT64) {
		lwsl_err("CBOR parse error: message type is expected\n");
		return -1;
	}
	if (QCBOR_Int64ToInt32(Item.val.int64, &m->type) < 0) {
		lwsl_err("CBOR parse error: message type out of range\n");
		return -1;
	}
	switch (m->type) {
	case QUERY_REQUEST:
		break;
	case TRUSTED_APP_INSTALL:
		m->trusted_app_install.manifest_list_len = 0;
		break;
	case TRUSTED_APP_DELETE:
		m->trusted_app_delete.ta_list_len = 0;
		break;
	default:
		lwsl_err("CBOR parse error: unknown message type\n");
	}

	QCBORDecode_GetNext(&DC, &Item);
	if (Item.uDataType != QCBOR_TYPE_INT64) {
		lwsl_err("CBOR parse error: token is expected\n");
		return -1;
	}
	m->token = Item.val.int64;

	QCBORItem options;
	QCBORDecode_GetNext(&DC, &options);
	if (options.uDataType != QCBOR_TYPE_MAP) {
		lwsl_err("CBOR parse error: map is expected\n");
		return -1;
	}

	for (int i = 0; i < options.val.uCount; i++) {
		QCBORItem option;
		QCBORDecode_GetNext(&DC, &option);
		if (option.uLabelType != QCBOR_TYPE_INT64) {
			lwsl_err("CBOR parse error: int64 label is expected\n");
			return -1;

		}
		if (option.label.int64 == 10 && m->type == TRUSTED_APP_INSTALL) {
			if (option.uDataType != QCBOR_TYPE_ARRAY) {
				lwsl_err("CBOR parse error: array is expected\n");
				return -1;
			}
			for (int j = 0; j < option.val.uCount; j++) {
				QCBORItem manifest;
				QCBORDecode_GetNext(&DC, &manifest);

				if (manifest.uDataType != QCBOR_TYPE_TEXT_STRING) {
					lwsl_err("CBOR parse error: string is expected\n");
					return -1;
				}

				if (j >= 10) {
					lwsl_warn("CBOR parse: manifest_list is too long\n");
				} else {
					m->trusted_app_install.manifest_list[j] = manifest.val.string;
					m->trusted_app_install.manifest_list_len++;
				}
			}
		} else if (option.label.int64 == 8 && m->type == TRUSTED_APP_DELETE) {
			if (option.uDataType != QCBOR_TYPE_ARRAY) {
				lwsl_err("CBOR parse error: array is expected\n");
				return -1;
			}
			for (int j = 0; j < option.val.uCount; j++) {
				QCBORItem ta;
				QCBORDecode_GetNext(&DC, &ta);

				if (ta.uDataType != QCBOR_TYPE_TEXT_STRING) {
					lwsl_err("CBOR parse error: string is expected\n");
					return -1;
				}

				if (j >= 10) {
					lwsl_warn("CBOR parse: ta_list is too long\n");
				} else {
					m->trusted_app_delete.ta_list[j] = ta.val.string;
					m->trusted_app_delete.ta_list_len++;
				}
			}
		} else {
			lwsl_warn("CBOR parse: unknown option\n");
			while (option.uNextNestLevel != options.uNextNestLevel) {
				QCBORDecode_GetNext(&DC, &option);
				if (option.uDataType == QCBOR_TYPE_NONE) {
					return -1;
				}
			}
		}
	}

	switch (m->type) {
	case QUERY_REQUEST:
		QCBORDecode_GetNext(&DC, &Item);
		if (Item.uDataType != QCBOR_TYPE_INT64) {
			lwsl_err("CBOR parse error: data_item_requested is expected\n");
			return -1;
		}
		m->query_request.data_item_requested = Item.val.int64;
		break;
	default:
		break;
	}

	if (QCBORDecode_Finish(&DC) != QCBOR_SUCCESS) {
		lwsl_err("CBOR parse error: end of input is expected\n");
		return -1;
	}

	return 0;
}

int
teep_agent_message(int jose, const char *msg, int msg_len, char *out, uint32_t *out_len, char *ta_url_list, uint32_t *ta_url_list_len)
{
	lwsl_notice("teep_agent_message\n");
	lwsl_hexdump_notice(msg, msg_len);

	struct teep_mesg_cbor m;
	if (parse_teep_message_cbor(&m, (UsefulBufC){ msg, msg_len }) < 0) {
		return -1;
	}

	QCBOREncodeContext EC;
	QCBOREncode_Init(&EC, (UsefulBuf){ out, *out_len });

	switch (m.type) {
	case QUERY_REQUEST:
		lwsl_notice("detect QUERY_REQUEST\n");
		lwsl_notice("send QUERY_RESPONSE\n");
		{
			QCBOREncode_OpenArray(&EC);
			QCBOREncode_AddInt64(&EC, QUERY_RESPONSE);
			QCBOREncode_AddInt64(&EC, m.token);
			QCBOREncode_OpenMap(&EC);
			QCBOREncode_OpenArrayInMapN(&EC, 8);
			char *p = ta_list_buf;
			while (*p) {
				QCBOREncode_AddSZString(&EC, p);
				p += strlen(p) + 1;
			}
			QCBOREncode_CloseArray(&EC);
			QCBOREncode_CloseMap(&EC);
			QCBOREncode_CloseArray(&EC);

		}
		*ta_url_list_len = 0;
		break;
	case TRUSTED_APP_INSTALL:
		lwsl_notice("detect TRUSTED_APP_INSTALL\n");

		{
			uint32_t ofs = 0;
			for (int i = 0; i < m.trusted_app_install.manifest_list_len; i++) {
				UsefulBufC url = m.trusted_app_install.manifest_list[i];
				if (*ta_url_list_len < ofs + url.len + 1) return -1;
				memcpy(ta_url_list + ofs, url.ptr, url.len);
				ta_url_list[ofs + url.len] = 0;
				ofs += url.len + 1;
			}
			if (*ta_url_list_len < ofs + 1) return -1;
			strcpy(ta_url_list + ofs, "");
			ofs += 1;
			*ta_url_list_len = ofs;

			QCBOREncode_OpenArray(&EC);
			QCBOREncode_AddInt64(&EC, SUCCESS);
			QCBOREncode_AddInt64(&EC, m.token);
			QCBOREncode_CloseArray(&EC);
		}
		break;
	case TRUSTED_APP_DELETE:
		lwsl_notice("detect TRUSTED_APP_DELETE\n");
		for (int i = 0; i < m.trusted_app_delete.ta_list_len; i++) {
			UsefulBufC ta = m.trusted_app_delete.ta_list[i];
			ta_store_delete(ta.ptr, ta.len);
		}
		QCBOREncode_OpenArray(&EC);
		QCBOREncode_AddInt64(&EC, SUCCESS);
		QCBOREncode_AddInt64(&EC, m.token);
		QCBOREncode_CloseArray(&EC);
		*ta_url_list_len = 0;
		break;
	}
	UsefulBufC Encoded;
	if(QCBOREncode_Finish(&EC, &Encoded)) {
		return -1;
	}
	*out_len = Encoded.len;

	return 0;
}

int
teep_agent_set_ta_list(const char *ta_list, int ta_list_len)
{
	if (ta_list_len == 0) {
		ta_list_buf[0] = '\0';
	} else {
		if (ta_list[ta_list_len - 1] != 0) {
			return -1;
		}
		if (ta_list_len > 1 && ta_list[ta_list_len - 2] != 0) {
			return -1;
		}
		if (sizeof (ta_list_buf) < ta_list_len) {
			return -1;
		}
		memcpy(ta_list_buf, ta_list, ta_list_len);
	}
	return 0;
}
