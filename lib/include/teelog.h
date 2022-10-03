/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (C) 2019 National Institute of Advanced Industrial Science
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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum tee_log_level
{
    TEE_LOG_ERROR,
    TEE_LOG_WARN,
    TEE_LOG_INFO,
    TEE_LOG_DEBUG,
    TEE_LOG_TRACE
};

void tee_log(enum tee_log_level level, const char *msg, ...);

#define tee_error(...) tee_log(TEE_LOG_ERROR, "TERR:" __VA_ARGS__)
#define tee_warn(...)  tee_log(TEE_LOG_WARN, "TWRN:" __VA_ARGS__)
#define tee_info(...)  tee_log(TEE_LOG_INFO, "TINF:" __VA_ARGS__)
#ifdef DEBUG
#define tee_debug(...) tee_log(TEE_LOG_DEBUG, "TDGB:" __VA_ARGS__)
#define tee_trace(...) tee_log(TEE_LOG_TRACE, "TTRC:" __VA_ARGS__)
#else
#define tee_debug(...)
#define tee_trace(...)
#endif

#ifdef __cplusplus
}
#endif
