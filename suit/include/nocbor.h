#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct nocbor_range
{
    const uint8_t *begin;
    const uint8_t *end;
} nocbor_range_t;

typedef enum nocbor_error
{
    NOCBOR_OK,
    NOCBOR_MALFORMED,
    NOCBOR_NEED_DATA,
    NOCBOR_NOT_SUPPORTED,
    NOCBOR_NOT_MATCH,
    NOCBOR_END_OF_CONTEXT,
    NOCBOR_OVERFLOW
} nocbor_error_t;

typedef struct nocbor_any
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
        nocbor_range_t bytes;
    };
} nocbor_any_t;

static inline bool nocbor_is_uint(nocbor_any_t any)
{
    return (any.header >> 5) == 0;
}

static inline bool nocbor_is_nint(nocbor_any_t any)
{
    return (any.header >> 5) == 1;
}

static inline bool nocbor_is_bstr(nocbor_any_t any)
{
    return (any.header >> 5) == 2;
}

static inline bool nocbor_is_tstr(nocbor_any_t any)
{
    return (any.header >> 5) == 3;
}

static inline bool nocbor_is_array(nocbor_any_t any)
{
    return (any.header >> 5) == 4;
}

static inline bool nocbor_is_map(nocbor_any_t any)
{
    return (any.header >> 5) == 5;
}

static inline bool nocbor_is_tag(nocbor_any_t any)
{
    return (any.header >> 5) == 6;
}

static inline bool nocbor_is_simple(nocbor_any_t any)
{
    return (any.header >> 5) == 7 && (any.header & 0x1F) <= 24;
}

static inline bool nocbor_is_false(nocbor_any_t any)
{
    return nocbor_is_simple(any) && any.value  == 20;
}

static inline bool nocbor_is_true(nocbor_any_t any)
{
    return nocbor_is_simple(any) && any.value  == 21;
}

static inline bool nocbor_is_bool(nocbor_any_t any)
{
    return nocbor_is_false(any) || nocbor_is_true(any);
}

static inline bool nocbor_is_null(nocbor_any_t any)
{
    return nocbor_is_simple(any) && any.value  == 22;
}

static inline bool nocbor_is_undefined(nocbor_any_t any)
{
    return nocbor_is_simple(any) && any.value  == 23;
}

/*
 * cbor parser primitives
 *
 * * nocbor_primitive_read_any(): read one cbor object
 */
static inline bool nocbor_primitive_read_any(nocbor_range_t *r, nocbor_any_t *dst, nocbor_error_t *e)
{
    const uint8_t *p = r->begin;

    if (p == r->end) goto need_data;

    uint8_t header = *p++;
    uint8_t mt = header >> 5;
    uint8_t ai = header & 0x1F;
    uint64_t value = ai;

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

    nocbor_range_t bytes = { NULL, NULL };
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
    *e = NOCBOR_OK;
    return true;
need_data:
    *e = NOCBOR_NEED_DATA;
    return false;
malformed:
    *e = NOCBOR_MALFORMED;
    return false;
not_supported:
    *e = NOCBOR_NOT_SUPPORTED;
    return false;
}

static inline bool nocbor_primitive_skip(nocbor_range_t *r, uint64_t count, nocbor_error_t *e)
{
    nocbor_range_t s = *r;
    while (count > 0) {
        nocbor_any_t any;
        if (!nocbor_primitive_read_any(&s, &any, e)) return false;
        if (nocbor_is_tag(any)) continue;
        count--;
        if (nocbor_is_array(any)) {
            uint64_t c = any.value;
            if (count + c < count) goto overflow;
            count += c;
            continue;
        } else if (nocbor_is_map(any)) {
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
    *e = NOCBOR_OVERFLOW;
    return false;
}

typedef struct nocbor_context
{
    nocbor_range_t r;
    uint64_t index;
    uint64_t count;
    nocbor_error_t error;
} nocbor_context_t;

static inline bool nocbor_fail(nocbor_context_t *ctx, nocbor_context_t saved)
{
    ctx->r = saved.r;
    ctx->index = saved.index;
    ctx->count = saved.count;
    return false;
}

static inline bool nocbor_is_end_of_context(nocbor_context_t c)
{
    return c.index == c.count;
}

static inline nocbor_context_t nocbor_toplevel(nocbor_range_t r)
{
    return (nocbor_context_t) {
        .r = r,
        .index = 0,
        .count = 1,
        .error = NOCBOR_OK
    };
}

static inline void nocbor_open(nocbor_context_t parent, nocbor_context_t *child, uint64_t count)
{
    child->r = parent.r;
    child->index = 0;
    child->count = count;
    child->error = NOCBOR_OK;
}

static inline bool nocbor_close(nocbor_context_t *parent, nocbor_context_t child)
{
    if (!nocbor_is_end_of_context(child)) {
        parent->error = NOCBOR_END_OF_CONTEXT;
        return false;
    }
    parent->r = child.r;
    return true;
}

static inline bool nocbor_read_any(nocbor_context_t *ctx, nocbor_any_t *dst)
{
    if (nocbor_is_end_of_context(*ctx)) {
        ctx->error = NOCBOR_END_OF_CONTEXT;
        return false;
    } else {
        if (nocbor_primitive_read_any(&ctx->r, dst, &ctx->error)) {
            ctx->index++;
            return true;
        } else {
            return false;
        }
    }
}

static inline bool nocbor_skip(nocbor_context_t *ctx, uint64_t count)
{
    if (ctx->count - ctx->index < count) {
        ctx->error = NOCBOR_END_OF_CONTEXT;
        return false;
    }
    if (!nocbor_primitive_skip(&ctx->r, count, &ctx->error)) return false;
    ctx->index += count;
    return true;
}

static inline bool nocbor_skip_all(nocbor_context_t *ctx)
{
    return nocbor_skip(ctx, ctx->count - ctx->index);
}

static inline bool nocbor_read_uint(nocbor_context_t *ctx, uint64_t *dst)
{
    nocbor_context_t saved = *ctx;
    nocbor_any_t any;
    if (!nocbor_read_any(ctx, &any)) goto err;
    if (!nocbor_is_uint(any)) goto mismatch;
    if (dst) *dst = any.value;
    return true;
mismatch:
    ctx->error = NOCBOR_NOT_MATCH;
err:
    return nocbor_fail(ctx, saved);
}

static inline bool nocbor_read_nint(nocbor_context_t *ctx, uint64_t *dst)
{
    nocbor_context_t saved = *ctx;
    nocbor_any_t any;
    if (!nocbor_read_any(ctx, &any)) goto err;
    if (!nocbor_is_nint(any)) goto mismatch;
    if (dst) *dst = any.value;
    return true;
mismatch:
    ctx->error = NOCBOR_NOT_MATCH;
err:
    return nocbor_fail(ctx, saved);
}

static inline bool nocbor_read_bstr(nocbor_context_t *ctx, nocbor_range_t *dst)
{
    nocbor_context_t saved = *ctx;
    nocbor_any_t any;
    if (!nocbor_read_any(ctx, &any)) goto err;
    if (!nocbor_is_bstr(any)) goto mismatch;
    if (dst) *dst = any.bytes;
    return true;
mismatch:
    ctx->error = NOCBOR_NOT_MATCH;
err:
    return nocbor_fail(ctx, saved);
}

static inline bool nocbor_read_tstr(nocbor_context_t *ctx, nocbor_range_t *dst)
{
    nocbor_context_t saved = *ctx;
    nocbor_any_t any;
    if (!nocbor_read_any(ctx, &any)) goto err;
    if (!nocbor_is_tstr(any)) goto mismatch;
    if (dst) *dst = any.bytes;
    return true;
mismatch:
    ctx->error = NOCBOR_NOT_MATCH;
err:
    return nocbor_fail(ctx, saved);
}

static inline bool nocbor_read_array(nocbor_context_t *ctx, nocbor_context_t *array)
{
    nocbor_context_t saved = *ctx;
    nocbor_any_t any;
    if (!nocbor_read_any(ctx, &any)) goto err;
    if (!nocbor_is_array(any)) goto mismatch;
    nocbor_open(*ctx, array, any.value);
    return true;
mismatch:
    ctx->error = NOCBOR_NOT_MATCH;
err:
    return nocbor_fail(ctx, saved);
}

static inline bool nocbor_read_map(nocbor_context_t *ctx, nocbor_context_t *map)
{
    nocbor_context_t saved = *ctx;
    nocbor_any_t any;
    if (!nocbor_read_any(ctx, &any)) goto err;
    if (!nocbor_is_map(any)) goto mismatch;
    if (any.value + any.value < any.value) {
        ctx->error = NOCBOR_OVERFLOW;
        goto err;
    }
    nocbor_open(*ctx, map, any.value + any.value);
    return true;
mismatch:
    ctx->error = NOCBOR_NOT_MATCH;
err:
    return nocbor_fail(ctx, saved);
}

static inline bool nocbor_read_null(nocbor_context_t *ctx)
{
    nocbor_context_t saved = *ctx;
    nocbor_any_t any;
    if (!nocbor_read_any(ctx, &any)) goto err;
    if (!nocbor_is_null(any)) goto mismatch;
    return true;
mismatch:
    ctx->error = NOCBOR_NOT_MATCH;
err:
    return nocbor_fail(ctx, saved);
}
