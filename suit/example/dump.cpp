#include <cstdio>
#include "example-util.h"

static bool check_vendor_id(suit_runner_t *runner)
{
    printf("check vendor id\n");
    return true;
}

static void fetch_complete(suit_runner_t *runner, void *user)
{
    printf("finish fetch\n");
}

static bool fetch(suit_runner_t *runner)
{
    printf("start fetch\n");
    suit_runner_suspend(runner, fetch_complete);
    return true;
}

static suit_callbacks_t callbacks = {
    .check_vendor_id = check_vendor_id,
    .fetch = fetch,
};

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: dump envelope\n");
        return 1;
    }

    std::vector<uint8_t> envelope_binary;
    if (!read_file(envelope_binary, argv[1])) {
        perror(argv[1]);
        return 1;
    }

    nocbor_range r = {
        &*envelope_binary.begin(),
        &*envelope_binary.end()
    };

    suit_processor_t suit_processor;
    suit_processor_init(&suit_processor);

    if (!suit_processor_load_root_envelope(&suit_processor, r)) {
        printf("parse error\n");
        return 1;
    }

    struct suit_envelope *ep = &suit_processor.envelope_buf[0];

    printf("envelope:\n");
    hexdump(ep->binary);
    printf("envelope.delegation:\n");
    hexdump(ep->delegation);
    printf("envelope.authentication_wrapper:\n");
    print_auth_wrapper(ep->authentication_wrapper);

    printf("envelope.manifest:\n");
    hexdump(ep->manifest.binary);
    printf("envelope.manifest.version: %" PRIu64 "\n", ep->manifest.version);
    printf("envelope.manifest.sequence_number: %" PRIu64 "\n", ep->manifest.sequence_number);
    printf("envelope.manifest.common:\n");
    hexdump(ep->manifest.common);
    printf("envelope.manifest.reference_uri:\n");
    hexdump(ep->manifest.reference_uri);

    printf("envelope.manifest.dependency_resolution:\n");
    print_severed(ep->manifest.dependency_resolution);
    printf("envelope.manifest.payload_fetch:\n");
    print_severed(ep->manifest.payload_fetch);
    printf("envelope.manifest.install:\n");
    print_severed(ep->manifest.install);
    printf("envelope.manifest.text:\n");
    print_severed(ep->manifest.text);
    printf("envelope.manifest.coswid:\n");
    print_severed(ep->manifest.coswid);

    printf("envelope.manifest.validate:\n");
    hexdump(ep->manifest.validate);
    printf("envelope.manifest.load:\n");
    hexdump(ep->manifest.load);
    printf("envelope.manifest.run:\n");
    hexdump(ep->manifest.run);

    return 0;
}
