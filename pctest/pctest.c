#include "libaistotrp.h"
#include <libwebsockets.h>

// from aist-otrp-testapp
static uint8_t pkt[6 * 1024 * 1024];

int main(int argc, const char *argv[])
{
	const char *path = "enc-ta.jwe.jws";
	char* uri = "http://127.0.0.1:3000";

	struct libaistotrp_ctx *lao_ctx = NULL;
	struct lws_context_creation_info info;

	struct lao_rpc_io io;
	uint8_t result[64];
	int n;

	libaistotrp_init(&lao_ctx, uri);

	// debug request from arg
	if (argc > 1) {
		io.in =  (void*)argv[1];
		io.in_len = strlen(argv[1]);
	} else {
		io.in = "";
		io.in_len = 0;
	}
	io.out = pkt;
	io.out_len = sizeof(pkt);

	n = libaistotrp_tam_msg(lao_ctx, path, &io);
	if (n != TR_OKAY) {
		fprintf(stderr, "%s: libaistotrp_tam_msg: %d\n", __func__, n);

	  libaistotrp_destroy(&lao_ctx);
		return 1;
	}

	libaistotrp_destroy(&lao_ctx);

	return 0;
}
