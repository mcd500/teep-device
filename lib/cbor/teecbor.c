#include "teecbor.h"

bool tcbor_primitive_read_any(tcbor_range_t *r, tcbor_any_t *dst, tcbor_error_t *e)
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
