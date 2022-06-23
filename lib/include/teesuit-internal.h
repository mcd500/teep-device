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
#include "nocbor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct suit_component suit_component_t;
typedef struct suit_envelope suit_envelope_t;
typedef struct suit_manifest suit_manifest_t;

typedef struct suit_dependency_binder
{
    suit_envelope_t *source;
    int dependency_index;
    suit_envelope_t *target;
} suit_dependency_binder_t;

enum suit_next_action
{
    SUIT_NEXT_EXECUTE_SEQUENCE,
    SUIT_NEXT_EXECUTE_COMMAND,
    SUIT_NEXT_EXECUTE_TRY_ENTRY,
    SUIT_NEXT_EXIT
};

typedef struct suit_continuation
{
    suit_manifest_t *manifest;
    enum suit_next_action next;
    union {
        struct {
            nocbor_context_t common;
            nocbor_context_t target;
        } sequence;
        struct {
            uint64_t command;
            nocbor_range_t param;
            bool is_component; // component or dependency
            bool selected_all;
            union {
                uint64_t index;  // selected all
                nocbor_context_t array; // selected array
            };
        } command;
        struct {
            nocbor_context_t try_entries;
        } try_entry;
        struct {
        } exit;
    };
} suit_continuation_t;

#ifdef __cplusplus
}
#endif
