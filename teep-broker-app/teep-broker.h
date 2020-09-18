#pragma once

#include <libteep.h>
#include <libwebsockets.h>

#ifdef __cplusplus
extern "C" {
#endif

void cmdline_parse(int argc, const char *argv[]);

int broker_main(void);

#ifdef __cplusplus
}
#endif
