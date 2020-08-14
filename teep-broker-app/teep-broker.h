#pragma once

#include <libteep.h>
#include <libwebsockets.h>

#ifdef __cplusplus
extern "C" {
#endif

// XXX
extern const char *uri;
extern enum libteep_teep_ver teep_ver;
extern const char *talist;
extern bool jose;

int loop_teep(struct libteep_ctx *lao_ctx);
int loop_otrp(struct libteep_ctx *lao_ctx);

#ifdef __cplusplus
}
#endif
