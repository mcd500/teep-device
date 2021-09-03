#include <cstdio>
#include <iostream>
#include "example-util.h"

bool read_file(std::vector<uint8_t>& dst, const char *file)
{
    std::FILE *f = std::fopen(file, "rb");
    if (!f) {
        return false;
    }

    uint8_t buf[4096];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) {
        std::copy(&buf[0], &buf[n], std::back_inserter(dst));
    }
    std::fclose(f);
    return true;
}

void hexdump(nocbor_range_t r)
{
    if (!r.begin) {
        printf(" (NULL)\n");
        return;
    }
    for (const uint8_t *p = r.begin; p != r.end;) {
        for (int i = 0; i < 16 && p != r.end; i++, p++) {
            printf(" %2.2X", *p);
        }
        printf("\n");
    }
}

void print_severable(suit_severable_t s)
{
    if (!s.has_value) {
        printf(" (NULL)\n");
        return;
    }
    if (s.severed) {
        printf("  id=%" PRIu64 "\n", s.digest.algorithm_id);
        hexdump(s.digest.bytes);
    } else {
        hexdump(s.body);
    }
}

void print_digest(suit_digest_t d)
{
    printf("  algorithm=%" PRIu64 "\n", d.algorithm_id);
    hexdump(d.bytes);
}

void print_auth_wrapper(suit_authentication_wrapper w)
{
    print_digest(w.digest);
}

void print_envelope_field(nocbor_range_t envelope_bstr, enum suit_cbor_label key)
{
    nocbor_range_t field;
    if (suit_envelope_get_field_by_key(envelope_bstr, key, &field)) {
        hexdump(field);
    }
}
