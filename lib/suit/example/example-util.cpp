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

void hexdump(tcbor_range_t r)
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

void print_tstr(tcbor_range_t r)
{
    if (!r.begin) {
        printf("(NULL)");
        return;
    }
    for (const uint8_t *p = r.begin; p != r.end; p++) {
        printf("%c", *p);
    }
}

void print_bstr(tcbor_range_t r)
{
    if (!r.begin) {
        printf("(NULL)");
        return;
    }
    for (const uint8_t *p = r.begin; p != r.end; p++) {
        printf("%2.2X", *p);
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

void print_envelope_field(tcbor_range_t envelope_bstr, enum suit_cbor_label key)
{
    tcbor_range_t field;
    if (suit_envelope_get_field_by_key(envelope_bstr, key, &field)) {
        hexdump(field);
    }
}

void pirnt_object(const suit_object_t *target)
{
    if (target->is_component) {
        suit_component_t *c = target->component;
        printf("component: id=");
        print_bstr(c->id_cbor);
        printf("\n");
    } else {
        printf("dependency: id=");

    }
}
