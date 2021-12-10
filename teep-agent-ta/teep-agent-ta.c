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

#include <inttypes.h>
#include <tee_internal_api.h>
#include <libwebsockets.h>
#include <libteep.h>
#include <teesuit.h>

#include "ta-interface.h"
#include "teep-agent-ta.h"
#include "ta-store.h"

#ifdef IMSG
#define printf(...) IMSG(__VA_ARGS__)
#endif

enum agent_state
{
	AGENT_INIT,
	AGENT_POSTING_INITIAL_REQUEST,
	AGENT_POSTING_QUERY_RESPONSE,
	AGENT_POSTING_SUCCESS,
	AGENT_POSTING_ERROR,
	AGENT_INIT_RUNNER,
	AGENT_RUN_SUIT_RUNNER,
	AGENT_FETCH_COMPONENT,
	AGENT_FINISH
};

struct suit_manifest_storage
{
	UsefulBufC envelope;
};

struct suit_component_storage
{
	UsefulBufC component;
};

struct suit_manifest_storage suit_manifests[2]; // TODO: multiple manifest
struct suit_component_storage suit_components[1][1]; // suit_components[manifest_index][component_index]


static bool store_bstr(UsefulBufC *p, const void *buf, size_t len)
{
	if (!UsefulBuf_IsNULLC(*p)) {
		free(p->ptr);
	}
	void *q = malloc(len);
	if (!q) {
		return false;
	}
	memcpy(q, buf, len);
	*p = (UsefulBufC){ q, len };
	return true;
}

static bool store_envelope(size_t index, const void *buf, size_t len)
{
	if (index < 2) {
		// TODO: check sig before store envelope is better
		return store_bstr(&suit_manifests[index].envelope, buf, len);
	} else {
		return false;
	}
}

static bool store_suit_envelope(size_t index, UsefulBufC envelope)
{
	return store_envelope(index, envelope.ptr, envelope.len);
}

static bool store_component(size_t manifest_index, size_t component_index, const void *buf, size_t len)
{
	if (manifest_index < 1 && component_index < 1) {
		// TODO: decrypt & hash
		return store_bstr(&suit_components[manifest_index][component_index].component, buf, len);
	} else { 
		return false;
	}
}

struct teep_manifest_request
{
	bool requested;
	bool unneeded;
	char id[128];
};

struct teep_agent_session
{
	enum agent_state state;
	char tam_uri[TEEP_MAX_URI_LEN];
	struct broker_task *on_going_task;
	struct broker_task task_buffer;

	UsefulBufC token;

	int n_requests;
	struct teep_manifest_request requests[16];

	uint64_t data_item_requested;

	suit_context_t suit_context;
	suit_runner_t suit_runner;

	nocbor_range_t fetch_uri;
};

static suit_callbacks_t suit_callbacks;

static struct teep_agent_session *
teep_agent_session_create()
{
	struct teep_agent_session *session = malloc(sizeof *session);
	if (!session) {
		return NULL;
	}
	session->state = AGENT_INIT;
	session->on_going_task = NULL;
	session->token = NULLUsefulBufC;
	session->n_requests = 0;

	suit_context_init(&session->suit_context);

	return session;
}

static void
teep_agent_session_destroy(struct teep_agent_session *session)
{
	if (!session) {
		return;
	}
	free((void *)session->token.ptr);
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
		if (strlen(value) == 0) {
			return TEE_SUCCESS;
		}
		int n = 1;
		for (const char *s = value; *s; s++) {
			if (*s == ',') {
				n++;
			}
		}
		if (n > 16) {
			return TEE_ERROR_BAD_PARAMETERS;
		}
		int index = 0;
		for (const char *s = value; index < n; index++) {
			size_t len = sizeof session->requests[index].id;
			memset(session->requests[index].id, 0, len);
			session->requests[index].unneeded = false;
			session->requests[index].requested = false;
			if (*s == '+') {
				s++;
				session->requests[index].requested = true;
			} else if (*s == '-') {
				s++;
				session->requests[index].unneeded = true;
			}
			for (size_t i = 0;; i++) {
				if (*s == 0) {
					break;
				} else if (*s == ',') {
					s++;
					break;
				} else {
					if (i < len - 1) {
						session->requests[index].id[i] = *s;
					}
					s++;
				}
			}
		}
		session->n_requests = n;
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
		UsefulBuf p = (UsefulBuf){malloc(m->token.len), m->token.len };
		// TODO: check
		UsefulBuf_Copy(p, m->token);
		session->token = UsefulBuf_Const(p);
		session->data_item_requested = m->query_request.data_item_requested;
		break;
	case TEEP_UPDATE:
		{
			if (session->state != AGENT_POSTING_QUERY_RESPONSE) {
				teep_error(session, "invalid teep message type");
				goto err;
			}

			struct teep_buffer_array *p = &m->teep_update.manifest_list;

			for (size_t i = 0; i < p->len; i++) {
				if (!store_suit_envelope(i, p->array[i])) {
					goto err;
				}
			}

			for (size_t i = 0; i < p->len; i++) {
				UsefulBufC e = suit_manifests[p->len - i - 1].envelope;
				nocbor_range_t r = {
					e.ptr, e.ptr + e.len
				};
				if (!suit_context_add_envelope(&session->suit_context, r)) {
					teep_error(session, "TODO");
					goto err;
				}
			}

			session->state = AGENT_INIT_RUNNER;
		}
		break;
	}
err:
	free_parsed_teep_message(m);
}

static void
handle_component_download(struct teep_agent_session *session, const void *buffer, size_t len, const char *uri)
{
	if (session->state != AGENT_FETCH_COMPONENT) {
		teep_error(session, "invalid state");
		return;
	}
	printf("component download %"PRIu64"\n", len);
	//suit_store_component(session->request_component, buffer, len);
	suit_runner_resume(&session->suit_runner, NULL);
	session->state = AGENT_RUN_SUIT_RUNNER;
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
		handle_component_download(session, buffer, len, task->uri);
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
		for (size_t i = 0; i < session->n_requests; i++) {
			struct teep_manifest_request *m = &session->requests[i];
			if (!m->requested && !m->unneeded) {
				teep_message_encoder_add_tc_to_tc_list(&enc, m->id);
			}
		}
		teep_message_encoder_close_tc_list(&enc);

		teep_message_encoder_open_requested_tc_list(&enc);
		for (size_t i = 0; i < session->n_requests; i++) {
			struct teep_manifest_request *m = &session->requests[i];
			if (m->requested) {
				teep_message_encoder_add_tc_to_requested_tc_list(&enc, m->id);
			}
		}
		teep_message_encoder_close_requested_tc_list(&enc);

		teep_message_encoder_open_unneeded_tc_list(&enc);
		for (size_t i = 0; i < session->n_requests; i++) {
			struct teep_manifest_request *m = &session->requests[i];
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

static void hexdump(nocbor_range_t r)
{
    if (!r.begin) {
        printf(" (NULL)\n");
        return;
    }
    for (const uint8_t *p = r.begin; p != r.end;) {
        for (int i = 0; i < 16 && p != r.end; i++, p++) {
            printf(" %2.2X", *p);
        }
        printf("\n");
    }
}

/*
static void severed(suit_severed_t s)
{
    if (!s.has_value) {
        printf(" (NULL)\n");
        return;
    }
    if (s.severed) {
        printf("  id=%"PRIu64"\n", s.digest.algorithm_id);
        hexdump(s.digest.bytes);
    } else {
        hexdump(s.body);
    }
}
*/


static bool check_vendor_id(suit_runner_t *runner, void *user, const suit_object_t *target, nocbor_range_t id)
{
    printf("check vendor id\n");
    return true;
}

struct component_path {
	const char *device;
	const char *storage;
	char uuid[17];
	char filename[32];
};

static bool parse_component_id(suit_component_t *component, struct component_path *dst)
{
	nocbor_range_t id_cbor = component->id_cbor;
	nocbor_context_t ctx = nocbor_toplevel(id_cbor);
	nocbor_context_t array;
	if (!nocbor_read_array(&ctx, &array)) return false;
	nocbor_range_t bstr;
	if (!nocbor_read_bstr(&array, &bstr)) return false;
	if (memcmp(bstr.begin, "TEEP-Device", bstr.end - bstr.begin) != 0) return false;
	dst->device = "TEEP-Device";

	if (!nocbor_read_bstr(&array, &bstr)) return false;
	if (memcmp(bstr.begin, "SecureFS", bstr.end - bstr.begin) != 0) return false;
	dst->storage = "SecureFS";

	if (!nocbor_read_bstr(&array, &bstr)) return false;
	if (bstr.end - bstr.begin != 16) return false;
	memcpy(dst->uuid, bstr.begin, 16);
	dst->uuid[16] = 0;

	if (!nocbor_read_bstr(&array, &bstr)) return false;
	if (memcmp(bstr.begin, "ta", bstr.end - bstr.begin) != 0) return false;
	snprintf(dst->filename, "%s.ta", dst->uuid);

	if (!nocbor_close(&ctx, array)) return false;

	return true;
}

static bool store(suit_runner_t *runner, void *user, const suit_object_t *target, nocbor_range_t body)
{
	if (target->is_component) {
		printf("store component\n");
		struct component_path path;
		if (!parse_component_id(target->component, &path)) return false;
		printf("  device   = %s", path.device);
		printf("  storage  = %s", path.storage);
		printf("  uuid     = %s", path.uuid);
		printf("  filename = %s", path.filename);
	} else {
		printf("store dependency\n");

	}
    return true;
}

static void fetch_complete(suit_runner_t *runner, void *user)
{
    printf("finish fetch\n");
}

static bool fetch_and_store(suit_runner_t *runner, void *user, const suit_object_t *target, nocbor_range_t uri)
{
    printf("fetch_and_store\n");
	struct teep_agent_session *session = (struct teep_agent_session *)user;
	session->fetch_uri = uri;
	suit_runner_suspend(runner, fetch_complete);
    return true;
}


static suit_callbacks_t suit_callbacks = {
    .check_vendor_id = check_vendor_id,
    .store = store,
    .fetch_and_store = fetch_and_store,
};

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
		} else if (session->state == AGENT_INIT_RUNNER) {
#if 0
			struct suit_envelope *ep = &session->suit_context.envelope_buf[0];

			printf("envelope:\n");
			hexdump(ep->binary);
			printf("envelope.delegation:\n");
			hexdump(ep->delegation);
			printf("envelope.authentication_wrapper:\n");
			//hexdump(ep->authentication_wrapper);

			printf("envelope.manifest:\n");
			hexdump(ep->manifest.binary);
			printf("envelope.manifest.version: %"PRIu64"\n", ep->manifest.version);
			printf("envelope.manifest.sequence_number: %"PRIu64"\n", ep->manifest.sequence_number);
			printf("envelope.manifest.common:\n");
			hexdump(ep->manifest.common);
			printf("envelope.manifest.reference_uri:\n");
			hexdump(ep->manifest.reference_uri);

			printf("envelope.manifest.dependency_resolution:\n");
			severed(ep->manifest.dependency_resolution);
			printf("envelope.manifest.payload_fetch:\n");
			severed(ep->manifest.payload_fetch);
			printf("envelope.manifest.install:\n");
			severed(ep->manifest.install);
			printf("envelope.manifest.text:\n");
			severed(ep->manifest.text);
			printf("envelope.manifest.coswid:\n");
			severed(ep->manifest.coswid);

			printf("envelope.manifest.validate:\n");
			hexdump(ep->manifest.validate);
			printf("envelope.manifest.load:\n");
			hexdump(ep->manifest.load);
			printf("envelope.manifest.run:\n");
			hexdump(ep->manifest.run);
#endif
			suit_runner_init(&session->suit_runner, &session->suit_context,
				SUIT_COMMAND_SEQUENCE_INSTALL, &suit_callbacks, session);

			session->state = AGENT_RUN_SUIT_RUNNER;
		} else if (session->state == AGENT_RUN_SUIT_RUNNER) {
			suit_runner_run(&session->suit_runner);

			if (suit_runner_finished(&session->suit_runner)) {
				session->state = AGENT_POSTING_SUCCESS;
			} else if (suit_runner_suspended(&session->suit_runner)) {
				session->state = AGENT_FETCH_COMPONENT;
			} else {
				teep_error(session, "suit error");
			}
		} else if (session->state == AGENT_FETCH_COMPONENT) {
			nocbor_range_t uri = session->fetch_uri;
			task->command = BROKER_HTTP_GET;
			memset(task->uri, 0, sizeof task->uri);
			// TODO: length
			memcpy(task->uri, uri.begin, uri.end - uri.begin);
			task->post_data_len = 0;
			session->on_going_task = task;
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
