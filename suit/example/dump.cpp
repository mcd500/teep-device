#include <cstdio>
#include "example-util.h"

static const char *command_name(enum suit_command command)
{
    switch (command) {
    case SUIT_CONDITION_VENDOR_IDENTIFIER:  return "cond_vendor_id";
    case SUIT_CONDITION_CLASS_IDENTIFIER:  return "cond_class_id";
    case SUIT_CONDITION_IMAGE_MATCH:  return "cond_image_match";
    case SUIT_CONDITION_USE_BEFORE:  return "cond_use_before";
    case SUIT_CONDITION_COMPONENT_OFFSET:  return "cond_component_offset";

    case SUIT_CONDITION_ABORT:  return "cond_abort";
    case SUIT_CONDITION_DEVICE_IDENTIFIER:  return "cond_device_id";
    case SUIT_CONDITION_IMAGE_NOT_MATCH:  return "cond_image_not_match";
    case SUIT_CONDITION_MINIMUM_BATTERY:  return "cond_minimum_bettery";
    case SUIT_CONDITION_UPDATE_AUTHORIZED:  return "cond_update_authorized";
    case SUIT_CONDITION_VERSION:  return "cond_version";

    case SUIT_DIRECTIVE_SET_COMPONENT_INDEX:  return "set_component_index";
    case SUIT_DIRECTIVE_SET_DEPENDENCY_INDEX:  return "set_dependency_index";
    case SUIT_DIRECTIVE_TRY_EACH:  return "try_each";
    case SUIT_DIRECTIVE_PROCESS_DEPENDENCY:  return "process_dependency";
    case SUIT_DIRECTIVE_SET_PARAMETERS:  return "set_parameters";
    case SUIT_DIRECTIVE_OVERRIDE_PARAMETERS:  return "override_parameters";
    case SUIT_DIRECTIVE_FETCH:  return "fetch";
    case SUIT_DIRECTIVE_COPY:  return "copy";
    case SUIT_DIRECTIVE_RUN:  return "run";

    case SUIT_DIRECTIVE_WAIT:  return "wait";
    case SUIT_DIRECTIVE_FETCH_URI_LIST:  return "fetch_uri_list";
    case SUIT_DIRECTIVE_SWAP:  return "swap";
    case SUIT_DIRECTIVE_RUN_SEQUENCE:  return "run_sequence";
    case SUIT_DIRECTIVE_GARBAGE_COLLECT:  return "gabage_collect";
    default:
        return NULL;
    }
}

static void print_command_sequence(nocbor_range_t command_sequence_bstr)
{
    if (nocbor_range_is_null(command_sequence_bstr)) {
        return;
    }
    suit_command_reader_t r;
    if (!suit_command_sequence_open(command_sequence_bstr, &r)) goto err;

    while (!suit_command_sequence_end_of_sequence(r)) {
        enum suit_command command;
        if (!suit_command_sequence_read_command(&r, &command)) goto err;
        const char *name = command_name(command);
        printf("  %s (%d)\n", name ? name : "unknown command", command);
        if (!suit_command_sequence_skip_param(&r)) goto err;
    }

    return;
err:
    printf("  parse error (command sequence)\n");
}

static void print_severable_command_sequence(suit_severable_t s)
{
    if (!s.has_value) {
        return;
    }
    if (s.severed) {
        // TODO
    } else {
        print_command_sequence(s.body);
    }
}


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

    nocbor_range envelope_bstr = {
        &*envelope_binary.begin(),
        &*envelope_binary.end()
    };

    printf("envelope:\n");
    hexdump(envelope_bstr);

    printf("envelope.delegation:\n");
    print_envelope_field(envelope_bstr, SUIT_DELEGATION);
    printf("envelope.authentication_wrapper:\n");
    print_envelope_field(envelope_bstr, SUIT_AUTHENTICATION_WRAPPER);

    suit_manifest_t manifest;
    if (suit_envelope_get_manifest(envelope_bstr, &manifest)) {
        printf("envelope.manifest:\n");
        hexdump(manifest.binary);
        printf("envelope.manifest.version: %" PRIu64 "\n", manifest.version);
        printf("envelope.manifest.sequence_number: %" PRIu64 "\n", manifest.sequence_number);
        printf("envelope.manifest.common:\n");
        hexdump(manifest.common_bstr);

        for (size_t i = 0; i < manifest.common.n_dependencies; i++) {
            printf("envelope.manifest.dependencies[%zu]: \n", i);
        }
        for (size_t i = 0; i < manifest.common.n_components; i++) {
            printf("envelope.manifest.components[%zu]: \n", i);
        }
        printf("envelope.manifest.command_sequence:\n");
        hexdump(manifest.common.command_sequence);
        print_command_sequence(manifest.common.command_sequence);

        printf("envelope.manifest.reference_uri:\n");
        hexdump(manifest.reference_uri);

        printf("envelope.manifest.dependency_resolution:\n");
        print_severable(manifest.dependency_resolution);
        print_severable_command_sequence(manifest.dependency_resolution);
        printf("envelope.manifest.payload_fetch:\n");
        print_severable(manifest.payload_fetch);
        print_severable_command_sequence(manifest.payload_fetch);
        printf("envelope.manifest.install:\n");
        print_severable(manifest.install);
        print_severable_command_sequence(manifest.install);
        printf("envelope.manifest.text:\n");
        print_severable(manifest.text);
        printf("envelope.manifest.coswid:\n");
        print_severable(manifest.coswid);

        printf("envelope.manifest.validate:\n");
        hexdump(manifest.validate);
        print_command_sequence(manifest.validate);
        printf("envelope.manifest.load:\n");
        hexdump(manifest.load);
        print_command_sequence(manifest.load);
        printf("envelope.manifest.run:\n");
        hexdump(manifest.run);
        print_command_sequence(manifest.run);
    }
    return 0;
}
