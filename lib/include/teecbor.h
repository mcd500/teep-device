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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tcbor_range
{
    const uint8_t *begin;
    const uint8_t *end;
} tcbor_range_t;

static inline tcbor_range_t tcbor_range_null()
{
    return (tcbor_range_t){ NULL, NULL };
}

static inline bool tcbor_range_is_null(tcbor_range_t r)
{
    return r.begin == NULL;
}

static inline bool tcbor_range_equal(tcbor_range_t x, tcbor_range_t y)
{
    if (tcbor_range_is_null(x) || tcbor_range_is_null(y)) {
        return tcbor_range_is_null(x) == tcbor_range_is_null(y);
    }
    if (x.end - x.begin != y.begin - y.end) return false;
    return memcmp(x.begin, y.begin, x.end - x.begin) == 0;
}

typedef enum tcbor_error
{
    tcbor_OK,
    tcbor_MALFORMED,
    tcbor_NEED_DATA,
    tcbor_NOT_SUPPORTED,
    tcbor_NOT_MATCH,
    tcbor_END_OF_CONTEXT,
    tcbor_OVERFLOW
} tcbor_error_t;

typedef struct tcbor_any
{
    // cbor first byte (3bit tag and 5bit value)
    //
    // note: 5bit value is also saved in value field if value < 24.
    //   5bit value in header field need to distinguish simple type and floating point.
    uint8_t header;
    union {
        // cbor integer value
        uint64_t value;
        // cbor trailing bytes
        tcbor_range_t bytes;
    };
} tcbor_any_t;

static inline bool tcbor_is_uint(tcbor_any_t any)
{
    return (any.header >> 5) == 0;
}

static inline bool tcbor_is_nint(tcbor_any_t any)
{
    return (any.header >> 5) == 1;
}

static inline bool tcbor_is_bstr(tcbor_any_t any)
{
    return (any.header >> 5) == 2;
}

static inline bool tcbor_is_tstr(tcbor_any_t any)
{
    return (any.header >> 5) == 3;
}

static inline bool tcbor_is_array(tcbor_any_t any)
{
    return (any.header >> 5) == 4;
}

static inline bool tcbor_is_map(tcbor_any_t any)
{
    return (any.header >> 5) == 5;
}

static inline bool tcbor_is_tag(tcbor_any_t any)
{
    return (any.header >> 5) == 6;
}

static inline bool tcbor_is_simple(tcbor_any_t any)
{
    return (any.header >> 5) == 7 && (any.header & 0x1F) <= 24;
}

static inline bool tcbor_is_false(tcbor_any_t any)
{
    return tcbor_is_simple(any) && any.value  == 20;
}

static inline bool tcbor_is_true(tcbor_any_t any)
{
    return tcbor_is_simple(any) && any.value  == 21;
}

static inline bool tcbor_is_bool(tcbor_any_t any)
{
    return tcbor_is_false(any) || tcbor_is_true(any);
}

static inline bool tcbor_is_null(tcbor_any_t any)
{
    return tcbor_is_simple(any) && any.value  == 22;
}

static inline bool tcbor_is_undefined(tcbor_any_t any)
{
    return tcbor_is_simple(any) && any.value  == 23;
}

/*
 * cbor parser primitives
 *
 * * tcbor_primitive_read_any(): read one cbor object
 */
static inline bool tcbor_primitive_read_any(tcbor_range_t *r, tcbor_any_t *dst, tcbor_error_t *e)
{
    const uint8_t *p = r->begin;

    if (p == r->end) goto need_data;

    {
        uint8_t header = *p++;
        uint8_t mt = header >> 5;
        uint8_t ai = header & 0x1F;
        uint64_t value = ai;
        tcbor_range_t bytes = { NULL, NULL };

        switch (ai) {
        case 24: case 25: case 26: case 27:
            value = 0;
            for (int i = 0; i < (1 << (ai - 24)); i++) {
                if (p == r->end) goto need_data;
                value = (value << 8) | *p++;
            }
            break;
        case 28: case 29: case 30:
            goto malformed;
        case 31:
            goto not_supported;
        }

        if (mt == 7) {
            if (ai == 24 && value < 32) goto malformed;
        }

        if (mt == 2 || mt == 3) {
            if (r->end - p < value) goto need_data;
            bytes.begin = p;
            bytes.end = p + value;
            p += value;
        }

        if (dst) {
            dst->header = header;
            if (mt == 2 || mt == 3) {
                dst->bytes = bytes;
            } else {
                dst->value = value;
            }
        }
        r->begin = p;
        *e = tcbor_OK;
    }
    return true;
need_data:
    *e = tcbor_NEED_DATA;
    return false;
malformed:
    *e = tcbor_MALFORMED;
    return false;
not_supported:
    *e = tcbor_NOT_SUPPORTED;
    return false;
}

static inline bool tcbor_primitive_skip(tcbor_range_t *r, uint64_t count, tcbor_error_t *e)
{
    tcbor_range_t s = *r;
    while (count > 0) {
        tcbor_any_t any;
        if (!tcbor_primitive_read_any(&s, &any, e)) return false;
        if (tcbor_is_tag(any)) continue;
        count--;
        if (tcbor_is_array(any)) {
            uint64_t c = any.value;
            if (count + c < count) goto overflow;
            count += c;
            continue;
        } else if (tcbor_is_map(any)) {
            uint64_t c = any.value;
            if (count + c < count) goto overflow;
            count += c;
            if (count + c < count) goto overflow;
            count += c;
            continue;
        }
    }
    *r = s;
    return true;
overflow:
    *e = tcbor_OVERFLOW;
    return false;
}

typedef struct tcbor_context
{
    tcbor_range_t r;
    uint64_t index;
    uint64_t count;
    tcbor_error_t error;
} tcbor_context_t;

static inline bool tcbor_fail(tcbor_context_t *ctx, tcbor_context_t saved)
{
    ctx->r = saved.r;
    ctx->index = saved.index;
    ctx->count = saved.count;
    return false;
}

static inline bool tcbor_is_end_of_context(tcbor_context_t c)
{
    return c.index == c.count;
}

static inline tcbor_context_t tcbor_toplevel(tcbor_range_t r)
{
    return (tcbor_context_t) {
        .r = r,
        .index = 0,
        .count = 1,
        .error = tcbor_OK
    };
}

static inline void tcbor_open(tcbor_context_t parent, tcbor_context_t *child, uint64_t count)
{
    child->r = parent.r;
    child->index = 0;
    child->count = count;
    child->error = tcbor_OK;
}

static inline bool tcbor_close(tcbor_context_t *parent, tcbor_context_t child)
{
    if (!tcbor_is_end_of_context(child)) {
        parent->error = tcbor_END_OF_CONTEXT;
        return false;
    }
    parent->r = child.r;
    return true;
}

static inline bool tcbor_read_any(tcbor_context_t *ctx, tcbor_any_t *dst)
{
    if (tcbor_is_end_of_context(*ctx)) {
        ctx->error = tcbor_END_OF_CONTEXT;
        return false;
    } else {
        if (tcbor_primitive_read_any(&ctx->r, dst, &ctx->error)) {
            if (!tcbor_is_tag(*dst)) {
                ctx->index++;
            }
            return true;
        } else {
            return false;
        }
    }
}

static inline bool tcbor_skip(tcbor_context_t *ctx, uint64_t count)
{
    if (ctx->count - ctx->index < count) {
        ctx->error = tcbor_END_OF_CONTEXT;
        return false;
    }
    if (!tcbor_primitive_skip(&ctx->r, count, &ctx->error)) return false;
    ctx->index += count;
    return true;
}

static inline bool tcbor_skip_all(tcbor_context_t *ctx)
{
    return tcbor_skip(ctx, ctx->count - ctx->index);
}

//
// like tcbor_read_any, except returns value as cbor parseable binary string.
//
static inline bool tcbor_read_subobject(tcbor_context_t *ctx, tcbor_range_t *dst)
{
    tcbor_context_t saved = *ctx;
    if (!tcbor_skip(ctx, 1)) {
        return false;
    }
    *dst = (tcbor_range_t) {
        .begin = saved.r.begin,
        .end = ctx->r.begin
    };
    return true;
}

static inline bool tcbor_read_uint(tcbor_context_t *ctx, uint64_t *dst)
{
    tcbor_context_t saved = *ctx;
    tcbor_any_t any;
    if (!tcbor_read_any(ctx, &any)) goto err;
    if (!tcbor_is_uint(any)) goto mismatch;
    if (dst) *dst = any.value;
    return true;
mismatch:
    ctx->error = tcbor_NOT_MATCH;
err:
    return tcbor_fail(ctx, saved);
}

static inline bool tcbor_read_nint(tcbor_context_t *ctx, uint64_t *dst)
{
    tcbor_context_t saved = *ctx;
    tcbor_any_t any;
    if (!tcbor_read_any(ctx, &any)) goto err;
    if (!tcbor_is_nint(any)) goto mismatch;
    if (dst) *dst = any.value;
    return true;
mismatch:
    ctx->error = tcbor_NOT_MATCH;
err:
    return tcbor_fail(ctx, saved);
}

static inline bool tcbor_read_bstr(tcbor_context_t *ctx, tcbor_range_t *dst)
{
    tcbor_context_t saved = *ctx;
    tcbor_any_t any;
    if (!tcbor_read_any(ctx, &any)) goto err;
    if (!tcbor_is_bstr(any)) goto mismatch;
    if (dst) *dst = any.bytes;
    return true;
mismatch:
    ctx->error = tcbor_NOT_MATCH;
err:
    return tcbor_fail(ctx, saved);
}

static inline bool tcbor_read_tstr(tcbor_context_t *ctx, tcbor_range_t *dst)
{
    tcbor_context_t saved = *ctx;
    tcbor_any_t any;
    if (!tcbor_read_any(ctx, &any)) goto err;
    if (!tcbor_is_tstr(any)) goto mismatch;
    if (dst) *dst = any.bytes;
    return true;
mismatch:
    ctx->error = tcbor_NOT_MATCH;
err:
    return tcbor_fail(ctx, saved);
}

static inline bool tcbor_read_array(tcbor_context_t *ctx, tcbor_context_t *array)
{
    tcbor_context_t saved = *ctx;
    tcbor_any_t any;
    if (!tcbor_read_any(ctx, &any)) goto err;
    if (!tcbor_is_array(any)) goto mismatch;
    tcbor_open(*ctx, array, any.value);
    return true;
mismatch:
    ctx->error = tcbor_NOT_MATCH;
err:
    return tcbor_fail(ctx, saved);
}

static inline bool tcbor_read_map(tcbor_context_t *ctx, tcbor_context_t *map)
{
    tcbor_context_t saved = *ctx;
    tcbor_any_t any;
    if (!tcbor_read_any(ctx, &any)) goto err;
    if (!tcbor_is_map(any)) goto mismatch;
    if (any.value + any.value < any.value) {
        ctx->error = tcbor_OVERFLOW;
        goto err;
    }
    tcbor_open(*ctx, map, any.value + any.value);
    return true;
mismatch:
    ctx->error = tcbor_NOT_MATCH;
err:
    return tcbor_fail(ctx, saved);
}

static inline bool tcbor_read_null(tcbor_context_t *ctx)
{
    tcbor_context_t saved = *ctx;
    tcbor_any_t any;
    if (!tcbor_read_any(ctx, &any)) goto err;
    if (!tcbor_is_null(any)) goto mismatch;
    return true;
mismatch:
    ctx->error = tcbor_NOT_MATCH;
err:
    return tcbor_fail(ctx, saved);
}

static inline bool tcbor_read_bool(tcbor_context_t *ctx, bool *b)
{
    tcbor_context_t saved = *ctx;
    tcbor_any_t any;
    if (!tcbor_read_any(ctx, &any)) goto err;
    if (!tcbor_is_bool(any)) goto mismatch;
    *b = tcbor_is_true(any);
    return true;
mismatch:
    ctx->error = tcbor_NOT_MATCH;
err:
    return tcbor_fail(ctx, saved);
}


static inline bool tcbor_read_tag(tcbor_context_t *ctx, uint64_t *dst)
{
    tcbor_context_t saved = *ctx;
    tcbor_any_t any;
    if (!tcbor_read_any(ctx, &any)) goto err;
    if (!tcbor_is_tag(any)) goto mismatch;
    if (dst) *dst = any.value;
    return true;
mismatch:
    ctx->error = tcbor_NOT_MATCH;
err:
    return tcbor_fail(ctx, saved);
}

#ifdef __cplusplus
}
#endif
