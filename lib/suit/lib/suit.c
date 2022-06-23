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

#include <stdio.h>
#include <string.h>
#include "mbedtls/pk.h"
#include "nocbor.h"
#include "teesuit.h"
#include "teerng.h"
#include "teelog.h"

static nocbor_range_t empty_array()
{
    static uint8_t array[] = {
        0x80
    };
    return (nocbor_range_t){ array, array + 1 };
}

static bool push_program_counter(suit_runner_t *runner)
{
    if (runner->n_stack == SUIT_STACK_MAX) return false;

    runner->stack[runner->n_stack] = runner->program_counter;
    runner->n_stack++;

    return true;
}

static bool pop_program_counter(suit_runner_t *runner)
{
    if (runner->n_stack == 0) return false;

    runner->n_stack--;
    runner->program_counter = runner->stack[runner->n_stack];

    return true;
}

static bool get_command_sequence_body(suit_runner_t *runner, suit_manifest_t *manifest, nocbor_range_t *ret)
{
    switch (runner->sequence) {
    case SUIT_COMMAND_SEQUENCE_INSTALL:
        *ret = manifest->install.body; // TODO: check severed
        return true;
    // TODO: other case
    default:
        return false;
    }
}

static bool call_command_sequence(suit_runner_t *runner, suit_manifest_t *manifest)
{
    nocbor_range_t common_sequence = manifest->common.command_sequence;
    if (nocbor_range_is_null(common_sequence)) {
        common_sequence = empty_array();
    }
    nocbor_range_t target_sequence;
    if (!get_command_sequence_body(runner, manifest, &target_sequence)) return false;
    if (nocbor_range_is_null(target_sequence)) {
        target_sequence = empty_array();
    }

    nocbor_context_t target_contxt = nocbor_toplevel(target_sequence);
    nocbor_context_t common_context = nocbor_toplevel(common_sequence);

    nocbor_context_t target_array;
    nocbor_context_t common_array;
    if (!nocbor_read_array(&target_contxt, &target_array)) return false;
    if (!nocbor_read_array(&common_context, &common_array)) return false;

    if (!push_program_counter(runner)) return false;
    runner->program_counter.manifest = manifest;
    runner->program_counter.next = SUIT_NEXT_EXECUTE_SEQUENCE;
    runner->program_counter.sequence.common = common_array;
    runner->program_counter.sequence.target = target_array;

    return true;
}

static bool digest_equal(suit_digest_t dx, suit_digest_t dy)
{
    if (dx.algorithm_id != dy.algorithm_id) return false;
    return nocbor_range_equal(dx.bytes, dy.bytes);
}

static bool call_dependency(suit_runner_t *runner, suit_manifest_t *manifest)
{
    if (!call_command_sequence(runner, manifest)) return false;
    return true;
}

void suit_runner_init(suit_runner_t *runner, suit_context_t *context, enum suit_command_sequence sequence, const suit_callbacks_t *callbacks, void *user)
{
    memset(runner, 0, sizeof *runner);
    runner->context = context;
    runner->callbacks = callbacks;
    runner->user = user;
    runner->sequence = sequence;
    static const uint8_t cbor_true[] = {
        0xF5
    };
    runner->selected_components = (nocbor_range_t) {
        .begin = cbor_true,
        .end = cbor_true + 1
    };
    runner->selected_dependencies = nocbor_range_null();
    suit_manifest_t *manifest = suit_context_get_root_manifest(runner->context);
    runner->program_counter.manifest = manifest;
    runner->program_counter.next = SUIT_NEXT_EXIT;
    if (!call_dependency(runner, manifest)) {
        // TODO
        return;
    }
}

struct command_handler
{
    enum suit_command command;
    bool allow_common;
    bool (*handler)(suit_runner_t *runner, const suit_object_t *target, enum suit_command command, nocbor_range_t param);
};

static bool read_rep_policy(nocbor_context_t *ctx)
{
    uint64_t policy;
    if (nocbor_read_uint(ctx, &policy)) {
        return true;
    } else if (nocbor_read_null(ctx)) {
        return true;
    } else {
        return false;
    }
}

static bool parse_rep_policy(nocbor_range_t param)
{
    // TODO
    return true;
}

static bool condition_vendor_identifier(suit_runner_t *runner, const suit_object_t *target, enum suit_command command, nocbor_range_t param)
{
    if (!parse_rep_policy(param)) return false;
    tee_log_trace("execute suit-condition-vendor-identifier\n");
    return true;
}

static bool condition_class_identifier(suit_runner_t *runner, const suit_object_t *target, enum suit_command command, nocbor_range_t param)
{
    if (!parse_rep_policy(param)) return false;
    tee_log_trace("execute suit-condition-class-identifier\n");
    return true;
}

static bool condition_image_match(suit_runner_t *runner, const suit_object_t *target, enum suit_command command, nocbor_range_t param)
{
    if (!parse_rep_policy(param)) return false;
    tee_log_trace("execute suit-condition-image-match\n");
    return true;
}

static suit_binder_t *lookup_binder(suit_runner_t *runner, const suit_object_t *target, uint64_t key)
{
    for (int i = 0; i < runner->n_binder; i++) {
        suit_binder_t *binder = &runner->bindings[i];
        // TODO: compare target by component_id
        if (binder->target == target && binder->key == key) {
            return binder;
        }
    }
    return NULL;
}

static suit_binder_t *alloc_binder(suit_runner_t *runner, const suit_object_t *target, uint64_t key)
{
    if (runner->n_binder == SUIT_BINDER_MAX) return NULL;
    suit_binder_t *binder = &runner->bindings[runner->n_binder++];
    binder->target = target;
    binder->key = key;
    return binder;
}

static bool set_parameter(suit_runner_t *runner, const suit_object_t *target, uint64_t key, nocbor_range_t value, bool override)
{
    suit_binder_t *binder = lookup_binder(runner, target, key);
    if (binder && !override) return true;
    if (!binder) {
        binder = alloc_binder(runner, target, key);
        if (!binder) return false;
    }
    binder->value_cbor = value;
    return true;
}

bool suit_runner_get_parameter(suit_runner_t *runner, const suit_object_t *target, uint64_t key, nocbor_range_t *dst)
{
    suit_binder_t *binder = lookup_binder(runner, target, key);
    if (!binder) {
        return false;
    }
    *dst = binder->value_cbor;
    return true;
}

static bool directive_set_parameters(suit_runner_t *runner, const suit_object_t *target, enum suit_command command, nocbor_range_t param)
{
    tee_log_trace("execute suit-set-parameters\n");
    bool is_override = command == SUIT_DIRECTIVE_OVERRIDE_PARAMETERS;
    
    nocbor_context_t ctx = nocbor_toplevel(param);

    nocbor_context_t map;
    if (!nocbor_read_map(&ctx, &map)) goto err;
    while (!nocbor_is_end_of_context(map)) {
        uint64_t key;
        nocbor_range_t value;
        if (!nocbor_read_uint(&map, &key)) goto err;
        if (!nocbor_read_subobject(&map, &value)) goto err;
        if (!set_parameter(runner, target, key, value, is_override)) goto err;
    }
    if (!nocbor_close(&ctx, map)) goto err;

    return true;
err:
    return false;
}

static bool directive_process_dependency(suit_runner_t *runner, const suit_object_t *target, enum suit_command command, nocbor_range_t param)
{
    if (!parse_rep_policy(param)) return false;
    tee_log_trace("execute suit-directive-process-dependency\n");
    // TODO
    return true;
}

static bool directive_fetch(suit_runner_t *runner, const suit_object_t *target, enum suit_command command, nocbor_range_t param)
{
    if (!parse_rep_policy(param)) return false;
    tee_log_trace("execute suit-directive-fetch\n");
    
    nocbor_range_t uri_cbor;
    if (!suit_runner_get_parameter(runner, target, SUIT_PARAMETER_URI, &uri_cbor)) {
        tee_log_trace("uri is not set\n");
        return false;
    }
    nocbor_context_t ctx = nocbor_toplevel(uri_cbor);
    nocbor_range_t uri;
    if (!nocbor_read_tstr(&ctx, &uri)) {
        tee_log_trace("uri not tstr\n");
        return false;
    }

    if (uri.begin == uri.end) {
        tee_log_trace("uri is empty\n");
    } else if (uri.begin[0] == '#') {
        tee_log_trace("local uri\n");
        nocbor_range_t binary;
        if (!suit_envelope_get_field_by_name(runner->program_counter.manifest->envelope_bstr, uri, &binary)) {
            tee_log_trace("local uri not found\n");
            return false;
        }
        return runner->callbacks->store(runner, runner->user, target, binary);
    } else {
        return runner->callbacks->fetch_and_store(runner, runner->user, target, uri);
    }
    return false;
}


static const struct command_handler command_handlers[] = {
    { SUIT_CONDITION_VENDOR_IDENTIFIER, true, condition_vendor_identifier },
    { SUIT_CONDITION_CLASS_IDENTIFIER, true, condition_class_identifier },
    { SUIT_CONDITION_IMAGE_MATCH, true, condition_image_match },

    { SUIT_DIRECTIVE_SET_PARAMETERS, true, directive_set_parameters },
    { SUIT_DIRECTIVE_OVERRIDE_PARAMETERS, true, directive_set_parameters },

    { SUIT_DIRECTIVE_PROCESS_DEPENDENCY, false, directive_process_dependency },
    { SUIT_DIRECTIVE_FETCH, false, directive_fetch },

    { 0, false, NULL},
};

static const struct command_handler *
find_handler(uint64_t command, bool is_common)
{
    for (int i = 0;; i++) {
        const struct command_handler *h = &command_handlers[i];
        if (!h->handler) return NULL;
        if (h->command == command) {
            if (is_common && !h->allow_common) return NULL;
            return h;
        }
    }
    return NULL;
}

static bool get_object(suit_runner_t *runner, uint64_t index, bool is_component, suit_object_t *dst)
{
    suit_continuation_t *pc = &runner->program_counter;
    if (is_component) {
        if (index < pc->manifest->common.n_components) {
            dst->is_component = true;
            dst->component = &pc->manifest->common.components[index];
            return true;
        }
    } else {
        if (index < pc->manifest->common.n_dependencies) {
            dst->is_component = false;
            dst->component = &pc->manifest->common.dependencies[index];
            return true;
        }
    }
    return false;
}

static void run_sequence_step(suit_runner_t *runner, bool *pop_pc)
{
    suit_continuation_t *pc = &runner->program_counter;
    nocbor_context_t *ctx;
    bool is_common;
    if (!nocbor_is_end_of_context(pc->sequence.common)) {
        ctx = &pc->sequence.common;
        is_common = true;
    } else if (!nocbor_is_end_of_context(pc->sequence.target)) {
        ctx = &pc->sequence.target;
        is_common = false;
    } else {
        *pop_pc = true;
        tee_log_trace("end of command seq\n");
        return;
    }

    uint64_t command;
    nocbor_range_t param;

    if (!nocbor_read_uint(ctx, &command)) goto err;
    if (!nocbor_read_subobject(ctx, &param)) goto err;

    tee_log_trace("command: %lu\n", command);

    if (command == SUIT_DIRECTIVE_SET_COMPONENT_INDEX) {
        runner->selected_components = param;
        runner->selected_dependencies = nocbor_range_null();
        return;
    } else if (command == SUIT_DIRECTIVE_SET_DEPENDENCY_INDEX) {
        runner->selected_components = nocbor_range_null();
        runner->selected_dependencies = param;
        return;
    }

    const struct command_handler *h = find_handler(command, is_common);
    if (!h) {
        tee_log_trace("unknown command %lu\n", command);
        goto err;
    }

    bool is_component = !nocbor_range_is_null(runner->selected_components);
    bool is_dependency = !nocbor_range_is_null(runner->selected_dependencies);
    if (is_component == is_dependency) goto err;

    suit_continuation_t newpc;
    newpc.manifest = pc->manifest;
    newpc.next = SUIT_NEXT_EXECUTE_COMMAND;
    newpc.command.command = command;
    newpc.command.param = param;
    newpc.command.is_component = is_component;

    nocbor_range_t selected = is_component ? runner->selected_components : runner->selected_dependencies;
    nocbor_context_t selected_ctx = nocbor_toplevel(selected);
    bool flag;
    uint64_t index;
    nocbor_context_t array;
    if (nocbor_read_bool(&selected_ctx, &flag)) {
        if (flag) {
            newpc.command.selected_all = true;
            newpc.command.index = 0;
        } else {
            return; // no object selected
        }
    } else if (nocbor_read_uint(&selected_ctx, &index)) {
        suit_object_t target;
        if (!get_object(runner, index, is_component, &target)) goto err;
        if (!h->handler(runner, &target, command, param)) goto err;
        return;
    } else if (nocbor_read_array(&selected_ctx, &array)) {
        newpc.command.selected_all = false;
        newpc.command.array = array;
    } else {
        goto err;
    }

    if (!push_program_counter(runner)) goto err;
    runner->program_counter = newpc;

    return;
err:
    suit_runner_mark_error(runner);
}

static void run_try_step(suit_runner_t *runner, bool *pop_pc)
{
    // TODO
}

static void run_command_step(suit_runner_t *runner, bool *pop_pc)
{
    suit_continuation_t *pc = &runner->program_counter;

    const struct command_handler *h = find_handler(pc->command.command, false);

    if (pc->command.selected_all) {
        suit_object_t target;
        if (!get_object(runner, pc->command.index, pc->command.is_component, &target)) {
            *pop_pc = true;
            return;
        }
        if (!h->handler(runner, &target, pc->command.command, pc->command.param)) goto err;
        pc->command.index++;
    } else {
        if (nocbor_is_end_of_context(pc->command.array)) {
            *pop_pc = true;
            return;
        }
        uint64_t index;
        if (!nocbor_read_uint(&pc->command.array, &index)) goto err;
        suit_object_t target;
        if (!get_object(runner, index, pc->command.is_component, &target)) goto err;
        if (!h->handler(runner, &target, pc->command.command, pc->command.param)) goto err;
    }
    return;
err:
    suit_runner_mark_error(runner);
    return;
}

void suit_runner_run(suit_runner_t *runner)
{
    if (runner->has_error) return;

    for (;;) {
        if (runner->suspended) return;

        suit_continuation_t *pc = &runner->program_counter;

        bool pop_pc = false;
        if (pc->next == SUIT_NEXT_EXIT) {
            break;
        } else if (pc->next == SUIT_NEXT_EXECUTE_SEQUENCE) {
            run_sequence_step(runner, &pop_pc);
        } else if (pc->next == SUIT_NEXT_EXECUTE_TRY_ENTRY) {
            run_try_step(runner, &pop_pc);
        } else if (pc->next == SUIT_NEXT_EXECUTE_COMMAND) {
            run_command_step(runner, &pop_pc);
        } else {
            goto err;
        }
        if (runner->has_error) {
            // TODO: unwind stack
            break;
        }
        if (pop_pc) {
            if (!pop_program_counter(runner)) goto err;
        }
    }
    return;
err:
    suit_runner_mark_error(runner); // syntax error
    return;
}

bool suit_runner_finished(suit_runner_t *runner)
{
    return !runner->has_error && !runner->suspended;
}

bool suit_runner_has_error(suit_runner_t *runner)
{
    return runner->has_error;
}

bool suit_runner_suspended(suit_runner_t *runner)
{
    return !runner->has_error && runner->suspended;
}


void suit_runner_mark_error(suit_runner_t *runner)
{
    runner->has_error = true;
}

bool suit_runner_get_error(const suit_runner_t *runner, void *error_enum_todo)
{
    return runner->has_error;
}

void suit_runner_suspend(suit_runner_t *runner, void (*on_resume)(suit_runner_t *, void *))
{
    runner->suspended = true;
    runner->on_resume = on_resume;
}

void suit_runner_resume(suit_runner_t *runner, void *user)
{
    if (runner->has_error) {
        return;
    }
    if (runner->suspended) {
        // allow on_resume to call suspend agein
        runner->suspended = false;
        void (*f)(suit_runner_t *, void *) = runner->on_resume;
        runner->on_resume = NULL;
        if (f) {
            f(runner, user);
        }
    }
}

void suit_check_mbedtls_pk(void)
{
    tee_log_trace("suit_check_mbedtls_pk\n");
    mbedtls_pk_context ctx;

    const unsigned char prv_key[] =
        "-----BEGIN EC PRIVATE KEY-----\n"
        "MHcCAQEEIDgSOT6RU/WLJrnJQOBQzNcCUtALyHpOPB7fsnUhDctsoAoGCCqGSM49\n"
        "AwEHoUQDQgAE6oNVxlIVnUynYAVBb5pUutPh3gi0tI0f4dBRq4xnEqqzJH+XexVQ\n"
        "egTVdiDSAdkTG/aEtOTHaWpC4rDLIzwplA==\n"
        "-----END EC PRIVATE KEY-----\n";
    const unsigned char pub_key[] =
        "-----BEGIN PUBLIC KEY-----\n"
        "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE6oNVxlIVnUynYAVBb5pUutPh3gi0\n"
        "tI0f4dBRq4xnEqqzJH+XexVQegTVdiDSAdkTG/aEtOTHaWpC4rDLIzwplA==\n"
        "-----END PUBLIC KEY-----\n";

    mbedtls_pk_init(&ctx);
    tee_log_trace("mbedtls_pk_init ok\n");

    int ret = mbedtls_pk_parse_key(&ctx, prv_key, sizeof prv_key, NULL, 0);
    if (ret != 0) {
        tee_log_trace("mbedtls_pk_parse_key failed\n");
        return;
    }
    tee_log_trace("mbedtls_pk_parse_key ok\n");

    unsigned char sign[MBEDTLS_ECDSA_MAX_LEN];
    size_t sign_len = sizeof sign;
    unsigned char hash[32];
    for (int i = 0; i < 32; i++) {
        hash[i] = i;
    }

    ret = mbedtls_pk_sign(&ctx, MBEDTLS_MD_SHA256,
        hash, 32, sign, &sign_len, teerng_read, NULL);
    if (ret != 0) {
        tee_log_trace("mbedtls_pk_sign failed\n");
        return;
    }
    tee_log_trace("mbedtls_pk_sign ok\n");

    mbedtls_pk_free(&ctx);
    tee_log_trace("mbedtls_pk_free ok\n");

    ret = mbedtls_pk_parse_public_key(&ctx, pub_key, sizeof pub_key);
    if (ret != 0) {
        tee_log_trace("mbedtls_pk_parse_public_key failed\n");
        return;
    }
    tee_log_trace("mbedtls_pk_parse_public_key ok\n");

    ret = mbedtls_pk_verify(&ctx, MBEDTLS_MD_SHA256, hash, 32, sign, sign_len);
    if (ret != 0) {
        tee_log_trace("mbedtls_pk_verify failed\n");
        return;
    }
    tee_log_trace("mbedtls_pk_verify ok\n");

    mbedtls_pk_free(&ctx);
    tee_log_trace("mbedtls_pk_free ok\n");
}
