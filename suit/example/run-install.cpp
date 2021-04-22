#include <cstdio>
#include "example-util.h"

static bool check_vendor_id(suit_runner_t *runner)
{
    printf("check vendor id\n");
    return true;
}

static bool fetch(suit_runner_t *runner)
{
    printf("fetch\n");
    return true;
}

static suit_callbacks_t callbacks = {
    .check_vendor_id = check_vendor_id,
    .fetch = fetch,
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
    
    suit_processor_t suit_processor;
    suit_processor_init(&suit_processor);

    int index = 0;
    for (auto i = envelope_binaries.begin(); i != envelope_binaries.end(); i++) {
        nocbor_range r = {
            &*i->begin(),
            &*i->end()
        };

        printf("== envelope ==\n");
        hexdump(r);
        printf("\n\n");

        if (!suit_processor_load_envelope(&suit_processor, index, r)) {
            printf("parse error\n");
            return 1;
        }
        index++;
    }

    struct suit_envelope *ep = &suit_processor.envelope_buf[0];

    suit_runner_t runner;
    suit_platform_t platform = {
        NULL,
        &callbacks
    };
    suit_runner_init(&runner, &suit_processor, &platform, SUIT_INSTALL);

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
