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
#include "example-util.h"

static bool check_vendor_id(suit_runner_t *runner, void *user, const suit_object_t *target, tcbor_range_t id)
{
    printf("check vendor id\n");
    pirnt_object(target);
    return true;
}

static bool store(suit_runner_t *runner, void *user, const suit_object_t *target, tcbor_range_t body)
{
    printf("store\n");
    pirnt_object(target);
    return true;
}

static bool fetch_and_store(suit_runner_t *runner, void *user, const suit_object_t *target, tcbor_range_t uri)
{
    printf("fetch_and_store\n");
    pirnt_object(target);
    printf("uri=");
    print_tstr(uri);
    printf("\n");
    return true;
}

static suit_callbacks_t callbacks = {
    .check_vendor_id = check_vendor_id,
    .store = store,
    .fetch_and_store = fetch_and_store,
};

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: run-install envelope...\n");
        return 1;
    }

    std::vector<std::vector<uint8_t>> envelope_binaries;
    for (int i = 1; i < argc; i++) {
        std::vector<uint8_t> bin;
        if (!read_file(bin, argv[i])) {
            perror(argv[i]);
            return 1;
        }
        envelope_binaries.push_back(std::move(bin));
    }
    
    suit_context_t suit_context;
    suit_context_init(&suit_context);

    for (auto i = envelope_binaries.begin(); i != envelope_binaries.end(); i++) {
        tcbor_range r = {
            &*i->begin(),
            &*i->end()
        };

        printf("== envelope ==\n");
        hexdump(r);
        printf("\n\n");

        if (!suit_context_add_envelope(&suit_context, r)) {
            printf("parse error\n");
            return 1;
        }
    }

    //struct suit_envelope *ep = &suit_context.envelope_buf[0];

    suit_runner_t runner;
    suit_runner_init(&runner, &suit_context, SUIT_COMMAND_SEQUENCE_INSTALL, &callbacks, NULL);

    while (true) {
        suit_runner_run(&runner);
        if (suit_runner_finished(&runner)) {
            break;
        } else if (suit_runner_suspended(&runner)) {
            printf("do HTTP here\n");
            suit_runner_resume(&runner, NULL);
        } else {
            printf("error\n");
            return 1;
        }
    }

    return 0;
}
