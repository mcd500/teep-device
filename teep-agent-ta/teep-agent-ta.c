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

#include <tee_internal_api.h>
#include <libwebsockets.h>
#include <libteep.h>
#include "ta-interface.h"
#include "teep-agent-ta.h"
#include "ta-store.h"

enum agent_state
{
	AGENT_INIT,
	AGENT_POSTING_INITIAL_REQUEST,
	AGENT_POSTING_QUERY_RESPONSE,
	AGENT_POSTING_SUCCESS,
	AGENT_POSTING_ERROR,
	AGENT_DOWNLOAD_TA,
	AGENT_FINISH
};

struct ta_manifest
{
	bool requested;
	bool unneeded;
	char id[128];
	char uri[TEEP_MAX_URI_LEN];
};

struct teep_agent_session
{
	enum agent_state state;
	char tam_uri[TEEP_MAX_URI_LEN];
	struct broker_task *on_going_task;
	struct broker_task task_buffer;

	UsefulBufC token;

	struct ta_manifest *manifests;
	size_t manifests_len;
	size_t download_ta_index;

	uint64_t data_item_requested;
};

static struct teep_agent_session *
teep_agent_session_create()
{
	struct teep_agent_session *session = malloc(sizeof *session);
	if (!session) {
		return session;
	}
	session->state = AGENT_INIT;
	session->on_going_task = NULL;
	session->token = NULLUsefulBufC;
	session->manifests = NULL;
	session->manifests_len = 0;

	return session;
}

static void
teep_agent_session_destroy(struct teep_agent_session *session)
{
	if (!session) {
		return;
	}
	if (session->manifests) {
		free(session->manifests);
	}
	free(session);
}

static TEE_Result
set_dev_option(struct teep_agent_session *session, enum agent_dev_option option, const char *value)
{
	if (session->state != AGENT_INIT) return TEE_ERROR_BAD_STATE;

	// XXX: disable developper options on production.
	switch (option) {
	case AGENT_OPTION_SET_TAM_URI:
		if (sizeof session->tam_uri < strlen(value) + 1) {
			return TEE_ERROR_BAD_PARAMETERS;
		}
		strcpy(session->tam_uri, value);
		return TEE_SUCCESS;
	case AGENT_OPTION_SET_CURRENT_TA_LIST:
		if (session->manifests) {
			free(session->manifests);
		}
		session->manifests = NULL;
		session->manifests_len = 0;
		if (strlen(value) == 0) {
			return TEE_SUCCESS;
		}
		session->manifests_len++;
		for (const char *s = value; *s; s++) {
			if (*s == ',') {
				session->manifests_len++;
			}
		}
		session->manifests = malloc(session->manifests_len * sizeof (struct ta_manifest));
		if (!session->manifests) {
			return TEE_ERROR_OUT_OF_MEMORY;
		}
		size_t index = 0;
		for (const char *s = value; index < session->manifests_len; index++) {
			memset(session->manifests[index].id, 0, 32);
			session->manifests[index].unneeded = false;
			session->manifests[index].requested = false;
			if (*s == '+') {
				s++;
				session->manifests[index].requested = true;
			} else if (*s == '-') {
				s++;
				session->manifests[index].unneeded = true;
			}
			for (size_t i = 0;; i++) {
				if (*s == 0) {
					break;
				} else if (*s == ',') {
					s++;
					break;
				} else {
					if (i < 31) {
						session->manifests[index].id[i] = *s;
					}
					s++;
				}
			}
			strcpy(session->manifests[index].uri, "");
		}
		return TEE_SUCCESS;

	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}
}

static void
teep_error(struct teep_agent_session *session, const char *message)
{
	session->state = AGENT_POSTING_ERROR;
}

static int
set_manifest_from_uri(struct ta_manifest *manifest, UsefulBufC src_uri)
{
	UsefulOutBuf uri;
	UsefulOutBuf_Init(&uri, UsefulBuf_FROM_BYTE_ARRAY(manifest->uri));
	UsefulOutBuf_AppendUsefulBuf(&uri, src_uri);
	UsefulOutBuf_AppendByte(&uri, 0);
	if (UsefulOutBuf_GetError(&uri) != 0) {
		return 0;
	}

	char *id_start = strrchr(manifest->uri, '/');
	if (!id_start) {
		id_start = manifest->uri;
	} else {
		id_start++;
	}
	UsefulOutBuf id;
	UsefulOutBuf_Init(&id, UsefulBuf_FROM_BYTE_ARRAY(manifest->id));
	for (char *p = id_start; *p; p++) {
		if (*p == '.') break;
		UsefulOutBuf_AppendByte(&id, *p);
	}
	UsefulOutBuf_AppendByte(&id, 0);
	if (UsefulOutBuf_GetError(&id) != 0) {
		return 0;
	}
	return 1;
}

static int
set_manifest_from_suit_install(struct ta_manifest *manifest, UsefulBufC suit_install)
{
	QCBORDecodeContext DC;
	QCBORDecode_Init(&DC, suit_install, QCBOR_DECODE_MODE_NORMAL);

	QCBORItem SuitInstall;
	QCBORDecode_GetNext(&DC, &SuitInstall);
	if (SuitInstall.uDataType != QCBOR_TYPE_ARRAY) {
		return 0;
	}

	if (SuitInstall.val.uCount % 2) {
		return 0;
	}

	for (size_t i = 0; i < SuitInstall.val.uCount; i += 2) {
		QCBORItem SuitCommand;
		QCBORDecode_GetNext(&DC, &SuitCommand);
		if (SuitCommand.uDataType != QCBOR_TYPE_INT64) {
			return 0;
		}
		QCBORItem CommandParam;
		QCBORDecode_GetNext(&DC, &CommandParam);

		if (SuitCommand.uDataType == QCBOR_TYPE_NONE) {
			return 0;
		}

		switch (SuitCommand.val.int64) {
		case 19: // suit-directive-set-parameters
			{
				if (CommandParam.uDataType != QCBOR_TYPE_MAP) {
					return 0;
				}
				for (size_t j = 0; j < CommandParam.val.uCount; j++) {
					QCBORItem SuitParam;
					QCBORDecode_GetNext(&DC, &SuitParam);

					if (SuitParam.uLabelType != QCBOR_TYPE_INT64) {
						return 0;
					}

					switch (SuitParam.label.int64) {
					case 21: // suit-parameter-uri
						if (SuitParam.uDataType != QCBOR_TYPE_TEXT_STRING) {
							return 0;
						}
						return set_manifest_from_uri(manifest, SuitParam.val.string);
					default:
						while (SuitParam.uNextNestLevel != CommandParam.uNextNestLevel) {
							QCBORDecode_GetNext(&DC, &SuitParam);
							if (SuitParam.uDataType == QCBOR_TYPE_NONE) {
								return 0;
							}
						}
					}
				}
			}
			break;
		default:
			while (CommandParam.uNextNestLevel != SuitInstall.uNextNestLevel) {
				QCBORDecode_GetNext(&DC, &CommandParam);
				if (CommandParam.uDataType == QCBOR_TYPE_NONE) {
					return 0;
				}
			}
		}

	}
	return 0;
}

static int
set_manifest_from_suit_manifest(struct ta_manifest *manifest, UsefulBufC suit_manifest)
{
	QCBORDecodeContext DC;
	QCBORDecode_Init(&DC, suit_manifest, QCBOR_DECODE_MODE_NORMAL);

	QCBORItem SuitManifest;
	QCBORDecode_GetNext(&DC, &SuitManifest);
	if (SuitManifest.uDataType != QCBOR_TYPE_MAP) {
		return 0;
	}

	for (size_t i = 0; i < SuitManifest.val.uCount; i++) {
		QCBORItem ManifestItem;
		QCBORDecode_GetNext(&DC, &ManifestItem);

		if (ManifestItem.uLabelType != QCBOR_TYPE_INT64) {
			return 0;
		}

		switch (ManifestItem.label.int64) {
		case 9: // suit-install
			if (ManifestItem.uDataType != QCBOR_TYPE_BYTE_STRING) {
				return 0;
			}
			return set_manifest_from_suit_install(manifest, ManifestItem.val.string);
		default:
			while (ManifestItem.uNextNestLevel != SuitManifest.uNextNestLevel) {
				QCBORDecode_GetNext(&DC, &ManifestItem);
				if (ManifestItem.uDataType == QCBOR_TYPE_NONE) {
					return 0;
				}
			}
		}
	}
	return 0;
}

static int
set_manifest_from_suit(struct ta_manifest *manifest, UsefulBufC suit_envelope)
{
	QCBORDecodeContext DC;
	QCBORDecode_Init(&DC, suit_envelope, QCBOR_DECODE_MODE_NORMAL);

	QCBORItem Envelope;
	QCBORDecode_GetNext(&DC, &Envelope);
	if (Envelope.uDataType != QCBOR_TYPE_MAP) {
		return 0;
	}

	for (size_t i = 0; i < Envelope.val.uCount; i++) {
		QCBORItem EnvelopeItem;
		QCBORDecode_GetNext(&DC, &EnvelopeItem);

		if (EnvelopeItem.uLabelType != QCBOR_TYPE_INT64) {
			return 0;
		}

		switch (EnvelopeItem.label.int64) {
		case 3: // suit-manifest
			if (EnvelopeItem.uDataType != QCBOR_TYPE_BYTE_STRING) {
				return 0;
			}
			return set_manifest_from_suit_manifest(manifest, EnvelopeItem.val.string);
		default:
			while (EnvelopeItem.uNextNestLevel != Envelope.uNextNestLevel) {
				QCBORDecode_GetNext(&DC, &EnvelopeItem);
				if (EnvelopeItem.uDataType == QCBOR_TYPE_NONE) {
					return 0;
				}
			}
		}
	}
	return 0;
}

static void
handle_tam_message(struct teep_agent_session *session, const void *buffer, size_t len)
{
	if (len == 0) {
		session->state = AGENT_FINISH;
		return;
	}
	struct teep_message *m = parse_teep_message((UsefulBufC){ buffer, len });
	if (!m) {
		teep_error(session, "invalid teep message");
		return;
	}
	switch (m->type) {
	case TEEP_QUERY_RESPONSE:
	case TEEP_SUCCESS:
	case TEEP_ERROR:
		teep_error(session, "invalid teep message type");
		goto err;
	case TEEP_QUERY_REQUEST:
		if (session->state != AGENT_POSTING_INITIAL_REQUEST) {
			teep_error(session, "invalid teep message type");
			goto err;
		}
		session->state = AGENT_POSTING_QUERY_RESPONSE;
		session->token = m->token;
		session->data_item_requested = m->query_request.data_item_requested;
		break;
	case TEEP_UPDATE:
		{
			if (session->state != AGENT_POSTING_QUERY_RESPONSE) {
				teep_error(session, "invalid teep message type");
				goto err;
			}

			{
				struct teep_component_id_array *p = &m->teep_update.unneeded_tc_list;
				for (size_t i = 0; i < p->len; i++) {
					UsefulBufC *ta = &p->array[i];
					// TODO: handle error
					ta_store_delete(ta->ptr, ta->len);
				}
			}

			{
				free(session->manifests);
				struct teep_buffer_array *p = &m->teep_update.manifest_list;
				// TODO: handle error
				session->manifests = malloc(p->len * sizeof (struct ta_manifest));
				session->manifests_len = p->len;
				for (size_t i = 0; i < p->len; i++) {
					// TODO: parse SUIT_Envelope
					if (!set_manifest_from_suit(&session->manifests[i], p->array[i])) {
						teep_error(session, "too long URI");
						goto err;
					}
				}
			}
			session->state = AGENT_DOWNLOAD_TA;
			session->download_ta_index = 0;
		}
		break;
	}
err:
	free_parsed_teep_message(m);
}

static void
handle_ta_download(struct teep_agent_session *session, const void *buffer, size_t len, const char *uri)
{
	struct ta_manifest *manifest = &session->manifests[session->download_ta_index];
	// TODO: handle error
	ta_store_install(buffer, len, manifest->id, strlen(manifest->id));
	session->download_ta_index++;
}

static TEE_Result
broker_task_done(struct teep_agent_session *session, const void *buffer, size_t len)
{
	struct broker_task *task = session->on_going_task;
	if (!task) return TEE_ERROR_BAD_STATE;

	switch (task->command) {
	default:
		return TEE_ERROR_BAD_STATE;
	case BROKER_HTTP_POST:
		handle_tam_message(session, buffer, len);
		break;
	case BROKER_HTTP_GET:
		handle_ta_download(session, buffer, len, task->uri);
		break;
	}
	session->on_going_task = NULL;
	return TEE_SUCCESS;
}

static int build_query_response(struct teep_agent_session *session, void *dst, size_t *dst_len)
{
	struct teep_message_encoder enc;
	teep_message_encoder_init(&enc, (UsefulBuf){ dst, *dst_len });
	teep_message_encoder_add_header(&enc, TEEP_QUERY_RESPONSE);
	teep_message_encoder_open_options(&enc);
	teep_message_encoder_add_token(&enc, session->token);

	uint64_t items = session->data_item_requested;
	if (items & TEEP_DATA_ATTESTATION) {
		// TODO
	} else if (items & TEEP_DATA_TRUSTED_COMPONENTS) {
		teep_message_encoder_open_tc_list(&enc);
		for (size_t i = 0; i < session->manifests_len; i++) {
			struct ta_manifest *m = &session->manifests[i];
			if (!m->requested && !m->unneeded) {
				teep_message_encoder_add_tc_to_tc_list(&enc, m->id);
			}
		}
		teep_message_encoder_close_tc_list(&enc);

		teep_message_encoder_open_requested_tc_list(&enc);
		for (size_t i = 0; i < session->manifests_len; i++) {
			struct ta_manifest *m = &session->manifests[i];
			if (m->requested) {
				teep_message_encoder_add_tc_to_requested_tc_list(&enc, m->id);
			}
		}
		teep_message_encoder_close_requested_tc_list(&enc);

		teep_message_encoder_open_unneeded_tc_list(&enc);
		for (size_t i = 0; i < session->manifests_len; i++) {
			struct ta_manifest *m = &session->manifests[i];
			if (m->unneeded) {
				teep_message_encoder_add_tc_to_unneeded_tc_list(&enc, m->id);
			}
		}
		teep_message_encoder_close_unneeded_tc_list(&enc);
	} else if (items & TEEP_DATA_EXTENSIONS) {
		// TODO
	} else if (items & TEEP_DATA_SUIT_COMMANDS) {
		// TODO
	}
	teep_message_encoder_close_options(&enc);
	UsefulBufC ret;
	if (teep_message_encoder_finish(&enc, &ret) != QCBOR_SUCCESS) {
		return 0;
	}
	*dst_len = ret.len;
	return 1;
}

static int build_success(struct teep_agent_session *session, void *dst, size_t *dst_len)
{
	struct teep_message_encoder enc;
	teep_message_encoder_init(&enc, (UsefulBuf){ dst, *dst_len });
	teep_message_encoder_add_header(&enc, TEEP_SUCCESS);
	teep_message_encoder_open_options(&enc);
	{
		teep_message_encoder_add_token(&enc, session->token);
	}
	teep_message_encoder_close_options(&enc);
	UsefulBufC ret;
	if (teep_message_encoder_finish(&enc, &ret) != QCBOR_SUCCESS) {
		return 0;
	}
	*dst_len = ret.len;
	return 1;
}

static int build_error(struct teep_agent_session *session, void *dst, size_t *dst_len)
{
	struct teep_message_encoder enc;
	teep_message_encoder_init(&enc, (UsefulBuf){ dst, *dst_len });
	teep_message_encoder_add_header(&enc, TEEP_ERROR);
	teep_message_encoder_open_options(&enc);
	{
		teep_message_encoder_add_token(&enc, session->token);
	}
	teep_message_encoder_close_options(&enc);
	teep_message_encoder_add_err_code(&enc, 42); // TODO
	UsefulBufC ret;
	if (teep_message_encoder_finish(&enc, &ret) != QCBOR_SUCCESS) {
		return 0;
	}
	*dst_len = ret.len;
	return 1;
}

static struct broker_task *
query_next_broker_task(struct teep_agent_session *session)
{
	while (!session->on_going_task) {
		struct broker_task *task = &session->task_buffer;

		if (session->state == AGENT_INIT) {
			session->state = AGENT_POSTING_INITIAL_REQUEST;
		} else if (session->state == AGENT_POSTING_INITIAL_REQUEST) {
			task->command = BROKER_HTTP_POST;
			strcpy(task->uri, session->tam_uri);
			task->post_data_len = 0;
			session->on_going_task = task;
		} else if (session->state == AGENT_POSTING_QUERY_RESPONSE) {
			task->command = BROKER_HTTP_POST;
			strcpy(task->uri, session->tam_uri);
			task->post_data_len = sizeof (task->post_data);
			// TODO: handle return value
			build_query_response(session, task->post_data, &task->post_data_len);
			session->on_going_task = task;
		} else if (session->state == AGENT_POSTING_SUCCESS) {
			task->command = BROKER_HTTP_POST;
			strcpy(task->uri, session->tam_uri);
			task->post_data_len = sizeof (task->post_data);
			// TODO: handle return value
			build_success(session, task->post_data, &task->post_data_len);
			session->on_going_task = task;
		} else if (session->state == AGENT_POSTING_ERROR) {
			task->command = BROKER_HTTP_POST;
			strcpy(task->uri, session->tam_uri);
			task->post_data_len = sizeof (task->post_data);
			// TODO: handle return value
			build_error(session, task->post_data, &task->post_data_len);
			session->on_going_task = task;
		} else if (session->state == AGENT_DOWNLOAD_TA) {
			if (session->download_ta_index == session->manifests_len) {
				session->state = AGENT_POSTING_SUCCESS;
			} else {
				task->command = BROKER_HTTP_GET;
				strcpy(task->uri, session->manifests[session->download_ta_index].uri);
				task->post_data_len = 0;
				session->on_going_task = task;
			}
		} else {
			task->command = BROKER_FINISH;
			strcpy(task->uri, "");
			task->post_data_len = 0;
			session->on_going_task = task;
		}

	}
	return session->on_going_task;
}

/**
 * TA_CreateEntryPoint() - Creates the entry point for TA.
 * 
 * The function set the log level for TA.
 * 
 * @return TEE_SUCCESS for success, else any other value.
 */
TEE_Result TA_CreateEntryPoint(void)
{
	lws_set_log_level(0
		| LLL_USER
		| LLL_ERR
		| LLL_WARN
//		| LLL_INFO
		| LLL_NOTICE
//		| LLL_DEBUG
		, NULL);
	return TEE_SUCCESS;
}

/**
 * TA_DestroyEntryPoint() - Destroys the entry point for TA.  
 */
void TA_DestroyEntryPoint(void)
{
}

/**
 * TA_OpenSessionEntryPoint() - Opens the session entry point for TA.
 *    
 * The Framework calls the function TA_OpenSessionEntryPoint when a client
 * requests to open a session with the Trusted Application.
 * 
 * @param param_types	param_types is a numeric type that guarantees 32 bits.
 * @param params[]	A pointer to an array of four parameters
 * @param **sess_ctx	A pointer to a variable that can be filled by the Trusted
 * 			Application instance with an opaque void* data pointer
 * 
 * @return TEE_SUCCESS	If the session is successfully opened, else any other value.
 */
TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
		TEE_Param params[4],
		void **sess_ctx)
{
	(void)params;
	uint32_t exp_param_types = TEE_PARAM_TYPES(
		TEE_PARAM_TYPE_NONE,
		TEE_PARAM_TYPE_NONE,
		TEE_PARAM_TYPE_NONE,
		TEE_PARAM_TYPE_NONE);
	if (param_types != exp_param_types)
		return TEE_ERROR_BAD_PARAMETERS;

	struct teep_agent_session *session = teep_agent_session_create();
	if (!session) return TEE_ERROR_OUT_OF_MEMORY;
	*sess_ctx = session;

	return TEE_SUCCESS;
}

/**
 * TA_CloseSessionEntryPoint() - The Framework calls the function 
 * to close a client session.
 *
 * The Trusted Application implementation is responsible for 
 * freeing any resources consumed by the session.
 *
 * @param *sess_ctx	The value of the void* opaque data pointer set by the 
 *			Trusted Application in the function TA_OpenSessionEntryPoint
 *			for this session.
 * 
 * @return TEE_SUCCESS	for success, else any other value.
 */
void TA_CloseSessionEntryPoint(void *sess_ctx)
{
	struct teep_agent_session *session = sess_ctx;
	teep_agent_session_destroy(session);
}

static TEE_Result
handle_TEEP_AGENT_SET_DEV_OPTION(
	struct teep_agent_session *session,
	uint32_t param_types,
	TEE_Param params[TEE_NUM_PARAMS])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(
		TEE_PARAM_TYPE_VALUE_INPUT,
		TEE_PARAM_TYPE_MEMREF_INPUT,
		TEE_PARAM_TYPE_NONE,
		TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types) return TEE_ERROR_BAD_PARAMETERS;

	enum agent_dev_option option = params[0].value.a;
	char *value = params[1].memref.buffer;
	size_t len = params[1].memref.size;
	if (value[len - 1] != 0) return TEE_ERROR_BAD_PARAMETERS;

	return set_dev_option(session, option, value);
}

static TEE_Result
handle_TEEP_AGENT_BROKER_TASK_DONE(
	struct teep_agent_session *session,
	uint32_t param_types,
	TEE_Param params[TEE_NUM_PARAMS])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(
		TEE_PARAM_TYPE_MEMREF_INPUT,
		TEE_PARAM_TYPE_NONE,
		TEE_PARAM_TYPE_NONE,
		TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types) return TEE_ERROR_BAD_PARAMETERS;

	return broker_task_done(session, params[0].memref.buffer, params[0].memref.size);
}

static int copyout_param(TEE_Param *param, const void *buffer, size_t len)
{
	if (param->memref.size < len) return 0;
	memcpy(param->memref.buffer, buffer, len);
	param->memref.size = len;
	return 1;
}

static TEE_Result
handle_TEEP_AGENT_QUERY_NEXT_BROKER_TASK(
	struct teep_agent_session *session,
	uint32_t param_types,
	TEE_Param params[TEE_NUM_PARAMS])
{
	uint32_t exp_param_types = TEE_PARAM_TYPES(
		TEE_PARAM_TYPE_VALUE_OUTPUT,
		TEE_PARAM_TYPE_MEMREF_OUTPUT,
		TEE_PARAM_TYPE_MEMREF_OUTPUT,
		TEE_PARAM_TYPE_NONE);

	if (param_types != exp_param_types) return TEE_ERROR_BAD_PARAMETERS;

	struct broker_task *task = query_next_broker_task(session);

	params[0].value.a = task->command;
	if (!copyout_param(&params[1], task->uri, strlen(task->uri) + 1)) {
		return TEE_ERROR_BAD_PARAMETERS;
	}
	if (!copyout_param(&params[2], task->post_data, task->post_data_len)) {
		return TEE_ERROR_BAD_PARAMETERS;
	}

	return TEE_SUCCESS;
}

/**
 * TA_InvokeCommandEntryPoint() - Invokes the command entry point for TA.    
 * 
 * Based on command id, verify the tee parameter type and invokes teep_message_wrap(),
 * teep_message_unwrap(),otrp_message_verify(),ta_store_install(),ta_store_delete(),
 * teep_message_unwrap_ta_image(),otrp_message_sign,otrp_message_encrypt(), and 
 * otrp_message_decrypt().
 * 
 * @param *sess_ctx			The value of the void* opaque data pointer set by the Trusted 
 *					Application in the function TA_OpenSessionEntryPoint.
 * @param cmd_id			A Trusted Application-specific code that identifies the command to be invoked.
 * @param param_types			The types of the four parameters
 * @param params[TEE_NUM_PARAMS]    	A pointer to an array of four parameters 
 * 
 * @return TEE_SUCCESS               	for success, else any other value.
 */
TEE_Result
TA_InvokeCommandEntryPoint(void *sess_ctx,
			uint32_t cmd_id,
			uint32_t param_types,
			TEE_Param params[TEE_NUM_PARAMS])
{
	struct teep_agent_session *session = sess_ctx;

	switch (cmd_id) {
	case TEEP_AGENT_SET_DEV_OPTION:
		return handle_TEEP_AGENT_SET_DEV_OPTION(session, param_types, params);
	case TEEP_AGENT_BROKER_TASK_DONE:
		return handle_TEEP_AGENT_BROKER_TASK_DONE(session, param_types, params);
	case TEEP_AGENT_QUERY_NEXT_BROKER_TASK:
		return handle_TEEP_AGENT_QUERY_NEXT_BROKER_TASK(session, param_types, params);

	default:
		return TEE_ERROR_NOT_IMPLEMENTED;
	}
}

#ifdef KEYSTONE
#include <eapp_utils.h>
#include <edger/Enclave_t.h>
// TODO: should implemet in ref-ta/api???

/**
 * eapp_entry() - Defines the entry point that the runtime will start  in the enclave 
 * application.
 *   
 * Firstly  the function  set the log level for app and this acts as the entry point  
 * that the runtime will start in the enclave application,pull the invoke command and  
 * if command id is equal to 1000 then invoke the ocall_put_invoke_command_result().
 * Based on the parameter type it will execute the switch case if parameter type is 
 * "TEE_PARAM_TYPE_MEMREF_INOUT" it will allocate memory for size and read the invoke 
 * command and copies "buf.size" from memory area "buf.buf" to memory area
 * "params[i].memref.buffer + offset". If parameter type is a TEE_PARAM_TYPE_MEMREF_OUTPUT
 * then allocate the memory for params[i].memref.size. Finally invoke the command entry point
 * here Receive the commond id,param types and param and based on param type it will execute
 * the switch case,if parameter type is "TEE_PARAM_TYPE_MEMREF_INOUT" it will write the 
 * invoke parameter.
 * 
 * @return 0  for success, else error occurred.
 */
void EAPP_ENTRY eapp_entry()
{
	TA_CreateEntryPoint();

	void *session_ctx;

	TA_OpenSessionEntryPoint(0, NULL, &session_ctx);

	for (;;) {
		invoke_command_t c = ocall_pull_invoke_command();
		if (c.commandID == TEEP_AGENT_TA_EXIT) {
			ocall_put_invoke_command_result(c, 0);
			break;
		}
		TEE_Param params[4];
		for (int i = 0; i < 4; i++) {
			params[i].value.a = 0;
			params[i].value.b = 0;
			switch (TEE_PARAM_TYPE_GET(c.paramTypes, i)) {
			default:
				break;
			case TEE_PARAM_TYPE_VALUE_INPUT:
			case TEE_PARAM_TYPE_VALUE_INOUT:
				params[i].value.a = c.params[i].a;
				params[i].value.b = c.params[i].b;
				break;
			case TEE_PARAM_TYPE_MEMREF_INPUT:
			case TEE_PARAM_TYPE_MEMREF_INOUT:
				{
					uint32_t size = c.params[i].size;
					uint32_t offset;
					params[i].memref.size = size;
					params[i].memref.buffer = malloc(size); // TODO: check
					for (offset = 0; offset < size;) {
						param_buffer_t buf = ocall_read_invoke_param(i, offset);
						memcpy(params[i].memref.buffer + offset, buf.buf, buf.size);
						offset += buf.size;
					}
				}
				break;
			case TEE_PARAM_TYPE_MEMREF_OUTPUT:
				params[i].memref.size = c.params[i].size;
				params[i].memref.buffer = malloc(params[i].memref.size); // TODO: check
				memset(params[i].memref.buffer, 0, c.params[i].size);
				break;
			}
		}
		TEE_Result r = TA_InvokeCommandEntryPoint(session_ctx, c.commandID, c.paramTypes, params);
		for (int i = 0; i < 4; i++) {
			c.params[i].a = 0;
			c.params[i].b = 0;
			switch (TEE_PARAM_TYPE_GET(c.paramTypes, i)) {
			default:
				break;
			case TEE_PARAM_TYPE_VALUE_OUTPUT:
			case TEE_PARAM_TYPE_VALUE_INOUT:
				c.params[i].a = params[i].value.a;
				c.params[i].b = params[i].value.b;
				break;
			case TEE_PARAM_TYPE_MEMREF_OUTPUT:
			case TEE_PARAM_TYPE_MEMREF_INOUT:
				{
					c.params[i].size = params[i].memref.size;
					uint32_t size = c.params[i].size;
					uint32_t offset;
					for (offset = 0; offset < size;) {
						uint32_t n = size - offset;
						if (n >= 256) n = 256;
						ocall_write_invoke_param(i, offset, n, params[i].memref.buffer + offset);
						offset += n;
					}
					free(params[i].memref.buffer);
				}
				break;
			case TEE_PARAM_TYPE_MEMREF_INPUT:
				free(params[i].memref.buffer);
				break;
			}
		}

		ocall_put_invoke_command_result(c, r);

	}

	TA_CloseSessionEntryPoint(session_ctx);

	EAPP_RETURN(0);
}

#endif
