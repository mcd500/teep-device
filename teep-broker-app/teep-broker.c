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
 * This is a REE test application to test operation of libteep.
 */
#include <libteep.h>
#include <libwebsockets.h>

static uint8_t http_res_buf[6 * 1024 * 1024];
static char teep_req_buf[5 * 1024];
static char teep_res_buf[5 * 1024];
static char teep_tmp_buf[800 * 1024];
static uint8_t http_req_buf[5 * 1024];
static char ta_url_list_buf[1024];

const char *uri = "http://127.0.0.1:3000/api/tam"; // TAM server uri
enum libteep_teep_ver teep_ver = LIBTEEP_TEEP_VER_TEEP; // protocol
const char *talist = ""; // installed TA list
bool jose = false;

static void
usage(void)
{
	fprintf(stderr, "aist-otrp-testapp [--tamurl http://tamserver:port] [-d] [-p otrp]\n");
	fprintf(stderr, "     --tamurl: TAM server url \n"
			"     --jose: enable encryption and sign \n"
			"     --talist: installed ta list \n"
			"     -p: teep protocol otrp or teep \n");
	exit(1);
}

void
cmdline_parse(int argc, const char *argv[])
{
	const char *tmp;
	if (lws_cmdline_option(argc, argv, "--help"))
		usage();

	/* override the remote TAM URL */
	tmp = lws_cmdline_option(argc, argv, "--tamurl");
	if (tmp)
		uri = tmp;

	/* request the TAM ask the TEE to delete the test TA */
	/* protocol (teep or otrp) */
	tmp = lws_cmdline_option(argc, argv, "-p");
	if (tmp) {
		if (!strcmp(tmp, "otrp")) {
			teep_ver = LIBTEEP_TEEP_VER_OTRP;
		} else if (!strcmp(tmp, "teep")) {
			teep_ver = LIBTEEP_TEEP_VER_TEEP;
		} else {
			// usage();
			// use default protocol is TEEP
			teep_ver = LIBTEEP_TEEP_VER_TEEP;
		}
	}

	/* request the TAM ask the TEE to delete the test TA */
	/* protocol (teep or otrp) */
	tmp = lws_cmdline_option(argc, argv, "--jose");
	if (tmp) {
		jose = true;
	}

	/* ta-list */
	tmp = lws_cmdline_option(argc, argv, "--talist");
	if (tmp)
		talist = tmp;

	fprintf(stderr, "%s compiled at %s %s\n", __FILE__, __DATE__, __TIME__);
	fprintf(stderr, "uri = %s, teep_ver = %d, talist=%s\n", uri, teep_ver, talist);
}

enum otrp_message_type {
	OTRP_GET_DEVICE_STATE_REQUEST = 1,
	OTRP_INSTALL_TA_REQUEST = 2,
	OTRP_DELETE_TA_REQUEST = 3
};


static int io_copy(void *out, size_t outlen, void *in, size_t inlen) {
	if (inlen > outlen) {
		return -1;
	}
	memmove(out, in, inlen);
	return inlen;
}

static int verify_otrp_request(struct libteep_ctx *lao_ctx, void *out, size_t outlen, void *in, size_t inlen) {
	if (jose) {
		lwsl_notice("verify(unwrap) otrp message\n");
		return libteep_msg_verify(lao_ctx, out, outlen, in, inlen);
	} else {
		return io_copy(out, outlen, in, inlen);
	}
}

static int decrypt_otrp_request(struct libteep_ctx *lao_ctx, void *out, size_t outlen, void *in, size_t inlen) {
	if (jose) {
		lwsl_notice("decrypt(unwrap) otrp message\n");
		return libteep_msg_decrypt(lao_ctx, out, outlen, in, inlen);
	} else {
		return io_copy(out, outlen, in, inlen);
	}
}

static int encrypt_otrp_response(struct libteep_ctx *lao_ctx, void *out, size_t outlen, void *in, size_t inlen) {
	if (jose) {
		lwsl_notice("encrypt(wrap) otrp message\n");
		return libteep_msg_encrypt(lao_ctx, out, outlen, in, inlen);
	} else {
		return io_copy(out, outlen, in, inlen);
	}
}

static int sign_otrp_response(struct libteep_ctx *lao_ctx, void *out, size_t outlen, void *in, size_t inlen) {
	if (jose) {
		lwsl_notice("sign(wrap) otrp message\n");
		return libteep_msg_sign(lao_ctx, out, outlen, in, inlen);
	} else {
		return io_copy(out, outlen, in, inlen);
	}
}

struct otrp_mesg {
	int type;
	char *mes;
};


static int parse_otrp_request(char *out, size_t outlen, char *in, size_t inlen)
{
	char keyname[32];
	int type = -1;

	// parse first key and value(map) from JSON
	// detect keyname
	char *keyStart = strchr(in, (int)'"');
	if (keyStart == NULL) return -1;
  char *keyEnd = strchr(keyStart+1, (int)'"');
	if (keyEnd == NULL) return -1;

	strncpy(keyname, keyStart+1, keyEnd-keyStart-1);
	keyname[keyEnd-keyStart] = '\0';

	if (!strcmp(keyname, "GetDeviceStateRequest")) {
		type = OTRP_GET_DEVICE_STATE_REQUEST;
	} else if (!strcmp(keyname, "InstallTARequest")) {
		type = OTRP_INSTALL_TA_REQUEST;
	} else if (!strcmp(keyname, "DeleteTARequest")) {
		type = OTRP_DELETE_TA_REQUEST;
	} else {
		// Unknown message
		return -1;
	}
	lwsl_notice("keyname=%s, type=%d\n", keyname, type);

	// detect value(map)
	char *mapStart = strchr(keyEnd+1, (int)'{');
	if (mapStart == NULL) return -1;
	char *mapEnd = strchr(in, (int)'}');
	if (mapEnd == NULL) return -1;

	strncpy(out, mapStart, mapEnd-mapStart+1);
	out[mapEnd-mapStart+1] = '\0';

	return type;
}

static int parse_otrp_jwe_map(const char* key, char *out, size_t outlen, char *in, size_t inlen)
{
	// parse jwe(ex: encrypted_ta) and value(map) from JSON
	// detect jwe json key(ex: encrypted_ta)
	char *encTaStart = strstr(in, key);
	if (encTaStart == NULL) return -1;

	// detect value(map)
	char *mapStart = strchr(encTaStart, (int)'{');
	if (mapStart == NULL) return -1;
	char *mapEnd = strchr(encTaStart, (int)'}');
	if (mapEnd == NULL) return -1;

	strncpy(out, mapStart, mapEnd-mapStart+1);
	out[mapEnd-mapStart+1] = '\0';

//	printf("debug encrypted_ta start=%.256s\n", out);
//	printf("debug encrypted_ta end=%s\n", out + strlen(out) - 256);

	return 0;
}

static int parse_otrp_json_value(const char* key, char *out, size_t outlen, char *in, size_t inlen)
{
	// parse jwe(ex: ta_id) and value(map) from JSON
	// detect jwe json key(ex: ta_id)
	char *encTaStart = strstr(in, key);
	if (encTaStart == NULL) return -1;

	// detect value
	char *mapStart = strchr(encTaStart+strlen(key)+1, (int)'"');
	if (mapStart == NULL) return -1;
	mapStart++;
	char *mapEnd = strchr(mapStart, (int)'"');
	if (mapEnd == NULL) return -1;

	strncpy(out, mapStart, mapEnd-mapStart);
	out[mapEnd-mapStart] = '\0';

	return 0;
}

int loop_teep(struct libteep_ctx *lao_ctx)
{
	libteep_set_ta_list(lao_ctx, talist);
	size_t tam_request_len = 0;
	do {
		int n = libteep_tam_msg(lao_ctx, http_res_buf, sizeof (http_res_buf), http_req_buf, tam_request_len);
		if (n <= 0) break;
		size_t tam_response_len = n;
		tam_request_len = sizeof (http_req_buf);
		if (libteep_agent_msg(lao_ctx, jose, http_req_buf, &tam_request_len, ta_url_list_buf, sizeof (ta_url_list_buf), http_res_buf, tam_response_len) < 0) {
			break;
		}
		char *p = ta_url_list_buf;
		for (;;) {
			size_t len = strlen(p);
			if (len == 0) break;
			libteep_download_and_install_ta_image(lao_ctx, p);
			p += len + 1;
		}
	} while (tam_request_len > 0);
	lwsl_notice("receive empty body to finish teep protocol\n");
	return 0;
}

int otrp_gen_dsi(char *out, int out_len) {
		char ta_id_list[1000] = "";
		char *talist_dup = strdup(talist);
		char *ta_uuid = strtok(talist_dup, ",");
		int ta_num = 1; // for ta dummy name
		while (ta_uuid) {
			char tmp[300];
			lws_snprintf(tmp, sizeof(tmp),
				"{\"taid\":\"%s\",\"taname\":\"sp-hello-ta-%d\"},",
				ta_uuid, ta_num);
			strncat(ta_id_list, tmp, sizeof(ta_id_list) - strlen(ta_id_list));
			ta_uuid = strtok(NULL, ",");
			ta_num++;
		}
		ta_id_list[strlen(ta_id_list) - 1] = '\0';
		// generate dsi before encrypted, only minimal parameters
		lws_snprintf(out, out_len, 
			"{\"dsi\":{\"tee\":{\"name\":\"aist-otrp\",\"ver\":\"1.0\",\"sdlist\":{\"cnt\":1,\"sd\":[{\"ta_list\":[%s]}]}}}}",
			ta_id_list);
		free(talist_dup);
		talist_dup = NULL;

		return 0;
}

int loop_otrp(struct libteep_ctx *lao_ctx) {
	lwsl_notice("send empty body to begin otrp protocol\n");
	int n;
	n = libteep_tam_msg(lao_ctx, http_res_buf, sizeof(http_res_buf), "", 0);
	if (n < 0) {
		lwsl_err( "%s: libteep_tam_msg: %d\n", __func__, n);
		return n;
	}
	while (n > 0) { // n == 0 means zero packet
		if (n < 0) {
			lwsl_err("%s: unwrap_teep_request: fail %d\n", __func__, n);
			return n;
		}
		if (n == 0) {
			lwsl_notice("%s: received encrypted empty body\n", __func__);
			break;
		}
		lwsl_notice("%s: received message: %*s\n", __func__, n, (char *)http_res_buf);
		struct otrp_mesg m = {
			.type = -1,
			.mes = NULL
		};

		m.type = parse_otrp_request(teep_tmp_buf, sizeof(teep_tmp_buf), (char*)http_res_buf, n);
		n = verify_otrp_request(lao_ctx, http_res_buf, strlen(teep_tmp_buf), teep_tmp_buf, strlen(teep_tmp_buf));

		switch (m.type) {
		case OTRP_GET_DEVICE_STATE_REQUEST:
			lwsl_notice("detect OTRP_GET_DEVICE_STATE_REQUEST\n");
			/* TODO: check entries in REQUEST */
			lwsl_notice("send OTRP_GET_DEVICE_STATE_RESPONSE\n");
			otrp_gen_dsi(teep_tmp_buf, sizeof(teep_tmp_buf));
			// encrypt dsi
			lwsl_notice("dsi(before encrypted) json: %s, len: %zd\n", teep_tmp_buf, strlen(teep_tmp_buf));
			n = encrypt_otrp_response(lao_ctx, http_req_buf, sizeof(http_req_buf), teep_tmp_buf, strlen(teep_tmp_buf));
			if (n < 0) {
				lwsl_err("%s: encrypt_otrp_response failed %d\n", __func__, n);
				return n;
			}
			lwsl_notice("dsi(encrypted) json: %s, len: %zd\n", http_req_buf, (size_t)n);
			// GetDeviceTEEStateTBSResponse
			// TODO: get rid from request
			lws_snprintf(teep_tmp_buf, sizeof(teep_tmp_buf), 
				"{\"GetDeviceTEEStateTBSResponse\":{\"ver\":\"1.0\",\"status\":\"pass\",\"rid\":\"1\",\"tid\":\"1\",\"signerreq\":false,\"edsi\":%s}}",
				http_req_buf);
			lwsl_notice("GetDeviceTEEStateTBSResponse(before sign) json: %s, len: %zd\n", teep_tmp_buf, strlen(teep_tmp_buf));
			// gen GetDeviceTEEStateResponse from siging GetDeviceTEEStateTBSResponse
			n = sign_otrp_response(lao_ctx, http_req_buf, sizeof(http_req_buf), teep_tmp_buf, strlen(teep_tmp_buf));
			if (n < 0) {
				lwsl_err("%s: sign_otrp_response failed %d\n", __func__, n);
				return n;
			}
			lwsl_notice("GetDeviceTEEStateResponse(signed) json: %s, len: %zd\n", http_req_buf, (size_t)n);
			lws_snprintf(teep_tmp_buf, sizeof(teep_tmp_buf), 
				"{\"GetDeviceStateResponse\":[{\"GetDeviceTEEStateResponse\": %s}]}",
				http_req_buf);
			lwsl_notice("GetDeviceStateResponse json: %s, len: %zd\n", teep_tmp_buf, strlen(teep_tmp_buf));
			n = libteep_tam_msg(lao_ctx, http_res_buf, 800*1024, teep_tmp_buf, strlen(teep_tmp_buf));
			if (n < 0) {
				lwsl_err( "%s: libteep_tam_msg: %d\n", __func__, n);
				return n;
			}
			break;
		case OTRP_INSTALL_TA_REQUEST:
			lwsl_notice("detect OTRP_INSTALL_TA_REQUEST\n");
			// parse encrypted_ta
			n = parse_otrp_jwe_map("encrypted_ta", teep_tmp_buf, sizeof(teep_tmp_buf), (char*)http_res_buf, n);
			if (n < 0) {
				lwsl_err( "%s: parse_otrp_jwe_map failed: %d\n", __func__, n);
				// TODO: error handling(return InstallTAResponse with status fail)
				return n;
			}
			// install encrypted ta
			n = libteep_ta_store_install(lao_ctx, teep_tmp_buf, strlen(teep_tmp_buf), "TA");
			if (n < 0) {
				lwsl_err( "%s: libteep_ta_store_install failed: %d\n", __func__, n);
				return n;
			}

			lwsl_notice("send OTRP_INSTALL_TA_RESPONSE\n");
			otrp_gen_dsi(teep_tmp_buf, sizeof(teep_tmp_buf));
			// encrypt dsi
			lwsl_notice("dsi(before encrypted) json: %s, len: %zd\n", teep_tmp_buf, strlen(teep_tmp_buf));
			n = encrypt_otrp_response(lao_ctx, http_req_buf, sizeof(http_req_buf), teep_tmp_buf, strlen(teep_tmp_buf));
			if (n < 0) {
				lwsl_err("%s: encrypt_otrp_response failed %d\n", __func__, n);
				return n;
			}
			lwsl_notice("dsi(encrypted) json: %s, len: %zd\n", http_req_buf, (size_t)n);
			// InstallTATBSResponse
			// TODO: get rid from request
			lws_snprintf(teep_tmp_buf, sizeof(teep_tmp_buf), 
				"{\"InstallTATBSResponse\":{\"ver\":\"1.0\",\"status\":\"pass\",\"rid\":\"1\",\"tid\":\"1\",\"content\":%s}}",
				http_req_buf);
			lwsl_notice("InstallTATBSResponse(before sign) json: %s, len: %zd\n", teep_tmp_buf, strlen(teep_tmp_buf));
			// gen InstallTAResponse from siging InstallTATBSResponse
			n = sign_otrp_response(lao_ctx, http_req_buf, sizeof(http_req_buf), teep_tmp_buf, strlen(teep_tmp_buf));
			if (n < 0) {
				lwsl_err("%s: sign_otrp_response failed %d\n", __func__, n);
				return n;
			}
			lwsl_notice("InstallTAResponse(signed) json: %s, len: %zd\n", http_req_buf, (size_t)n);
			lws_snprintf(teep_tmp_buf, sizeof(teep_tmp_buf), 
				"{\"InstallTAResponse\":%s}",
				http_req_buf);
			lwsl_notice("InstallTAResponse json: %s, len: %zd\n", teep_tmp_buf, strlen(teep_tmp_buf));
			n = libteep_tam_msg(lao_ctx, http_res_buf, 800*1024, teep_tmp_buf, strlen(teep_tmp_buf));
			if (n < 0) {
				lwsl_err( "%s: libteep_tam_msg: %d\n", __func__, n);
				return n;
			}
			break;
		case OTRP_DELETE_TA_REQUEST:
			lwsl_notice("detect OTRP_DELETE_TA_REQUEST\n");
			// parse encrypted_ta
			n = parse_otrp_jwe_map("content", teep_tmp_buf, sizeof(teep_tmp_buf), (char*)http_res_buf, n);
			if (n < 0) {
				lwsl_err( "%s: parse_otrp_jwe_map failed: %d\n", __func__, n);
				// TODO: error handling(return DeleteTAResponse with status fail)
				return n;
			}
			// decrypt content to get ta id
			n = decrypt_otrp_request(lao_ctx, teep_res_buf, sizeof(teep_res_buf), teep_tmp_buf, strlen(teep_tmp_buf));
			if (n < 0) {
				lwsl_err( "%s: decrypt_otrp_request failed: %d\n", __func__, n);
				// TODO: error handling(return DeleteTAResponse with status fail)
				return n;
			}
			// delete encrypted ta
			n = parse_otrp_json_value("taid", teep_tmp_buf, sizeof(teep_tmp_buf), (char*)teep_res_buf, strlen(teep_res_buf));
			if (n < 0) {
				lwsl_err( "%s: parse_otrp_value failed: %d\n", __func__, n);
				// TODO: error handling(return DeleteTAResponse with status fail)
				return n;
			}
			lwsl_notice("delete ta_id=%s\n", teep_tmp_buf);
			n = libteep_ta_store_delete(lao_ctx, teep_tmp_buf, strlen(teep_tmp_buf));
			if (n < 0) {
				lwsl_err( "%s: libteep_ta_store_delete failed: %d\n", __func__, n);
				// TODO: error handling(return DeleteTAResponse with status fail)
				return n;
			}

			lwsl_notice("send OTRP_DELETE_TA_RESPONSE\n");
			otrp_gen_dsi(teep_tmp_buf, sizeof(teep_tmp_buf));
			// encrypt dsi
			lwsl_notice("dsi(before encrypted) json: %s, len: %zd\n", teep_tmp_buf, strlen(teep_tmp_buf));
			n = encrypt_otrp_response(lao_ctx, http_req_buf, sizeof(http_req_buf), teep_tmp_buf, strlen(teep_tmp_buf));
			if (n < 0) {
				lwsl_err("%s: encrypt_otrp_response failed %d\n", __func__, n);
				return n;
			}
			lwsl_notice("dsi(encrypted) json: %s, len: %zd\n", http_req_buf, (size_t)n);
			// DeleteTATBSResponse
			// TODO: get rid from request
			lws_snprintf(teep_tmp_buf, sizeof(teep_tmp_buf), 
				"{\"DeleteTATBSResponse\":{\"ver\":\"1.0\",\"status\":\"pass\",\"rid\":\"1\",\"tid\":\"1\",\"content\":%s}}",
				http_req_buf);
			lwsl_notice("DeleteTATBSResponse(before sign) json: %s, len: %zd\n", teep_tmp_buf, strlen(teep_tmp_buf));
			// gen DeleteTAResponse from siging DeleteTATBSResponse
			n = sign_otrp_response(lao_ctx, http_req_buf, sizeof(http_req_buf), teep_tmp_buf, strlen(teep_tmp_buf));
			if (n < 0) {
				lwsl_err("%s: sign_otrp_response failed %d\n", __func__, n);
				return n;
			}
			lwsl_notice("DeleteTAResponse(signed) json: %s, len: %zd\n", http_req_buf, (size_t)n);
			lws_snprintf(teep_tmp_buf, sizeof(teep_tmp_buf), 
				"{\"DeleteTAResponse\":%s}",
				http_req_buf);
			lwsl_notice("DeleteTAResponse json: %s, len: %zd\n", teep_tmp_buf, strlen(teep_tmp_buf));
			n = libteep_tam_msg(lao_ctx, http_res_buf, 800*1024, teep_tmp_buf, strlen(teep_tmp_buf));
			if (n < 0) {
				lwsl_err( "%s: libteep_tam_msg: %d\n", __func__, n);
				return n;
			}
			break;
		default:
			lwsl_err("%s: requested message type is invalid %d\n", __func__, m.type);
			return -1;
		}
	}
	lwsl_notice("receive empty body to finish teep protocol\n");
	return n;
}

int broker_main()
{
	struct libteep_ctx *lao_ctx = NULL;

	int res = libteep_init(&lao_ctx, teep_ver, uri);
	if (res != TR_OKAY) {
		fprintf(stderr, "%s: Unable to create lao\n", __func__);
		return 1;
	}

	if (teep_ver == LIBTEEP_TEEP_VER_TEEP)
		loop_teep(lao_ctx);
	else if (teep_ver == LIBTEEP_TEEP_VER_OTRP)
		loop_otrp(lao_ctx);

	/* ask the TAM to give us an encrypted, signed TA... we can't
	 * decrypt it because it's encrypted using the TEE's pubkey */

	libteep_destroy(&lao_ctx);
	return 0;
}

int main(int argc, const char** argv)
{
	lws_set_log_level(0
		| LLL_USER
		| LLL_ERR
		| LLL_WARN
		| LLL_INFO
		| LLL_NOTICE
//		| LLL_DEBUG
		, NULL);

	cmdline_parse(argc, argv);
	return broker_main();
}
