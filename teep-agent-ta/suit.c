#include "suit.h"

static bool label_match(const QCBORItem *item, uint64_t tag)
{
    if (item.uLabelTyple == QCBOR_TYPE_UINT64) {
        return item.label.uint64 == tag;
    } else {
        return false;
    }
}

struct array_reader {
    QCBORDecodeContext *dc;
    size_t count;
    size_t index;
    QCBORItem current;
    bool current_is_valid;
};

static bool map_start(QCBORDecodeContext *dc, struct array_reader *r)
{
    QCBORItem map;
    QCBORDecode_GetNext(&dc, &map);
    if (toplevel.uDataType != QCBOR_TYPE_MAP) {
        return false;
    }
    r->dc = dc;
    r->count = map.val.uCount;
    r->index = 0;
    r->current_is_valid = false;
    return true;
}

static QCBORItem *map_current(struct array_reader *r)
{
    if (r->current_is_valid) {
        return &r->current;
    } else {
        if (r->index < r->count) {
            QCBORDecode_GetNext(r->dc, &r->current);
            r->current_is_valid = true;
        } else {
            return NULL;
        }
    }
}

static bool map_next(struct array_reader *r)
{
    if (map_current(r)) {
        r->index++;
        r->current_is_valid = false;
    }
    return r->index < r->count;
}

static bool map_match(struct array_reader *r, uint64_t label)
{
    QCBORItem *item = map_current(r);
    if (item && item->uLabelTyple == QCBOR_TYPE_UINT64 && item->label.uint64 == label) {
        return true;
    } else {
        return false;
    }
}

static bool map_matchT(struct array_reader *r, uint64_t label, uint8_t type)
{
    return map_match(r, label) && map_current(r)->uDataType == type;
}

static void read_severable(struct array_reader *r, uint64_t label, struct suit_severed *p)
{
    p->has_value = false;
    if (map_matchT(&r, label, QCBOR_TYPE_BYTE_STRING)) {
        p->has_value = true;
        p->severed = false;
        p->body = map_current(r)->val.string;
        map_next(&r);
    }
}

static bool read_severed(struct array_reader *r, uint64_t label, struct suit_severed *p)
{
    if (map_matchT(&r, label, QCBOR_TYPE_ARRAY)) {
        if (p->has_value) {
            return false;
        }
        p->has_value = true;
        p->severed = true;
        size_t count = map_current(r)->val.uCount;
        if (count < 2) {
            return false;
        }
        QCBORItem item;
        QCBORDecode_GetNext(r->dc, &item);
        if (item.uDataType != QCBOR_TYPE_UINT64) {
            return false;
        }
        p->digest.algorith_id = item.val.uint64;
        QCBORDecode_GetNext(r->dc, &item);
        if (item.uDataType != QCBOR_TYPE_BYTE_STRING) {
            return false;
        }
        p->digest.bytes = item.val.string;
        for (size_t i = 2; i < count; i++) {
            QCBORDecode_GetNext(r->dc, &item);
        }
        map_next(&r);
    }
}

static bool parse_manifest(struct suit_manifest *mp, UsefulBufC manifest)
{
    mp->binary = manifest;

    QCBORDecodeContext dc;
    QCBORDecode_Init(&dc, manifest, QCBOR_DECODE_MODE_NORMAL);

    struct array_reader r;
    if (!map_start(&dc, &r)) {
        return false;
    }

    // suit-manifest-version         => 1,
    if (!map_matchT(&r, SUIT_MANIFEST_VERSION, QCBOR_TYPE_UINT64)) {
        return false;
    }
    mp->version = map_current(&r)->val.uint64;
    if (mp->version != 1) {
        return false;
    }
    map_next(&r);

    // suit-manifest-sequence-number => uint,
    if (!map_matchT(&r, SUIT_MANIFEST_SEQUENCE_NUMBER, QCBOR_TYPE_UINT64)) {
        return false;
    }
    mp->sequence_number = map_current(&)->val.uint64;
    map_next(&r);

    // suit-common                   => bstr .cbor SUIT_Common,
    if (!map_matchT(&r, SUIT_COMMON, QCBOR_TYPE_BYTE_STRING)) {
        return false;
    }
    mp->common = map_current(r).val.string;
    map_next(&r);

    // ? suit-reference-uri          => tstr,
    mp->reference_uri = NULLUsefulBufC;
    if (map_matchT(&r, SUIT_REFERENCE_URI, QCBOR_TYPE_TEXT_STRING)) {
        mp->reference_uri = map_current(r).val.string;
        map_next(&r);
    }

    // SUIT_Severable_Manifest_Members,
    read_severable(&r, SUIT_DEPENDENCY_RESOLUTION, &mp->dependency_resolution);
    read_severable(&r, SUIT_PAYLOAD_FETCH, &mp->payload_fetch);
    read_severable(&r, SUIT_INSTALL, &mp->install);
    read_severable(&r, SUIT_TEXT, &mp->text);
    read_severable(&r, SUIT_COSWID, &mp->coswid);

    // SUIT_Severable_Members_Digests,
    if (!read_severed(&r, SUIT_DEPENDENCY_RESOLUTION, &mp->dependency_resolution)) {
        return false;
    }
    if (!read_severed(&r, SUIT_PAYLOAD_FETCH, &mp->payload_fetch)) {
        return false;
    }
    if (!read_severed(&r, SUIT_INSTALL, &mp->install)) {
        return false;
    }
    if (!read_severed(&r, SUIT_TEXT, &mp->text)) {
        return false;
    }
    if (!read_severed(&r, SUIT_COSWID, &mp->coswid)) {
        return false;
    }

    // SUIT_Unseverable_Members,
    mp->validate = NULLUsefulBufC;
    if (map_matchT(&r, SUIT_VALIDATE, QCBOR_TYPE_BYTE_STRING)) {
        mp->validate = map_current(r).val.string;
        map_next(&r);
    }
    mp->load = NULLUsefulBufC;
    if (map_matchT(&r, SUIT_LOAD, QCBOR_TYPE_BYTE_STRING)) {
        mp->load = map_current(r).val.string;
        map_next(&r);
    }
    mp->run = NULLUsefulBufC;
    if (map_matchT(&r, SUIT_RUN, QCBOR_TYPE_BYTE_STRING)) {
        mp->run = map_current(r).val.string;
        map_next(&r);
    }

    // * $$SUIT_Manifest_Extensions,
    while (map_next(&r)) {
    }

    if (QCBORDecode_Finish(&dc) != QCBOR_SUCCES) {
        return false;
    }
    return true;
}

static bool parse_envelope(struct suit_envelope *ep, UsefulBufC envelope)
{
    ep->binary = envelope;

    QCBORDecodeContext dc;
    QCBORDecode_Init(&dc, envelope, QCBOR_DECODE_MODE_NORMAL);

    struct array_reader r;
    if (!map_start(&dc, &r)) {
        return false;
    }

    // ? suit-delegation => bstr .cbor SUIT_Delegation
    ep->delegation = NULLUsefulBufC;
    if (map_matchT(&r, SUIT_DELEGATION, QCBOR_TYPE_BYTE_STRING)) {
        ep->delegation = map_current(r).val.string;
        map_next(&r);
    }

    // suit-authentication-wrapper => bstr .cbor SUIT_Authentication
    if (!map_matchT(&r, SUIT_AUTHENTICATION_WRAPPER, QCBOR_TYPE_BYTE_STRING)) {
        return false;
    }
    ep->authentication_wrapper = map_current(r).val.string;
    map_next(&r);

    // suit-manifest  => bstr .cbor SUIT_Manifest
    if (!map_matchT(&r, SUIT_MANIFEST, QCBOR_TYPE_BYTE_STRING)) {
        return false;
    }
    ep->manifest = map_current(r).val.string;
    map_next(&r);

    // following fields are only syntax checked.

    // SUIT_Severable_Manifest_Members = (
    //   ? suit-dependency-resolution => bstr .cbor SUIT_Command_Sequence,
    //   ? suit-payload-fetch => bstr .cbor SUIT_Command_Sequence,
    //   ? suit-install => bstr .cbor SUIT_Command_Sequence,
    //   ? suit-text => bstr .cbor SUIT_Text_Map,
    //   ? suit-coswid => bstr .cbor concise-software-identity,
    //   * $$SUIT_severable-members-extensions,
    // )
    if (map_matchT(&r, SUIT_DEPENDENCY_RESOLUTION, QCBOR_TYPE_BYTE_STRING)) {
        map_next(&r);
    }
    if (map_matchT(&r, SUIT_PAYLOAD_FETCH, QCBOR_TYPE_BYTE_STRING)) {
        map_next(&r);
    }
    if (map_matchT(&r, SUIT_INSTALL, QCBOR_TYPE_BYTE_STRING)) {
        map_next(&r);
    }
    if (map_matchT(&r, SUIT_TEXT, QCBOR_TYPE_BYTE_STRING)) {
        map_next(&r);
    }
    if (map_matchT(&r, SUIT_COSWID, QCBOR_TYPE_BYTE_STRING)) {
        map_next(&r);
    }
    //  * SUIT_Integrated_Payload,
    //  * SUIT_Integrated_Dependency,
    //  * $$SUIT_Envelope_Extensions,
    //  * (int => bstr)
    //

    // SUIT_Integrated_Payload = (suit-integrated-payload-key => bstr)
    // SUIT_Integrated_Dependency = (
    //     suit-integrated-payload-key => bstr .cbor SUIT_Envelope
    // )
    // suit-integrated-payload-key = nint / uint .ge 24
    do {
        QCBORItem *p = map_current(&r);
        if (p) {
            if (p->uLabelTyple == QCBOR_TYPE_INT64) {
                if (0 <= p->label.int64 && p->label.int64 < 24) {
                    return false;
                }
            }
            if (p->uLabelTyple == QCBOR_TYPE_UINT64) {
                if (p->label.uint64 < 24) {
                    return false;
                }
            }
            if (p->uDataType != QCBOR_TYPE_BYTE_STRING) {
                return false;
            }
        }
    } while (map_next(&r));

    if (QCBORDecode_Finish(&dc) != QCBOR_SUCCES) {
        return false;
    }
    return true;
}

void suit_processor_init(struct suit_processor *p, UsefulBufC root_envelope)
{
    memset(p, 0, sizeof *p);
    struct suit_envelope *ep = &p->envelope_buf[0];
    if (parse_envelope(ep, root_envelope)) {
        ep->used = true;
        p->root_envelope = ep;
    }
}

static bool run_common_command(struct suit_processor *p, UsefulBufC sequence)
{
    QCBORDecodeContext dc;
    QCBORDecode_Init(&dc, sequence, QCBOR_DECODE_MODE_NORMAL);

    struct array_reader r;
    if (!array_start(&dc, &r)) {
        return false;
    }

    for (;;) {
        if (!array_current(&r)) {
            break;
        }

        if (array_current(&r)->uDataType != QCBOR_TYPE_UINT64) {
            return false;
        }
        uint64_t command = array_current(&r)->val.uint64;

        if (!array_next(&r)) {
            return false;
        }

        switch (command) {
        // SUIT_Condition
        case SUIT_CONDITION_VENDOR_IDENTIFIER:
            {

            }
            break;


        // SUIT_Common_Commands
        case SUIT_DIRECTIVE_SET_COMPONENT_INDEX:
            {
                if (!read_index_arg(&r, &p->component_index)) {
                    return false;
                }
                clear_index_arg(&p->dependency_index);
            }
            break;
        case SUIT_DIRECTIVE_SET_DEPENDENCY_INDEX:
            {
                if (!read_index_arg(&r, &p->dependency_index)) {
                    return false;
                }
                clear_index_arg(&p->component_index);
            }
            break;
        case SUIT_DIRECTIVE_RUN_SEQUENCE:
            {
                // TODO
            }
            break;
        case SUIT_DIRECTIVE_TRY_EACH:
            {
                // TODO
            }
            break;
        case SUIT_DIRECTIVE_SET_PARAMETERS:
        case SUIT_DIRECTIVE_OVERRIDE_PARAMETERS:
            {
                bool override = command == SUIT_DIRECTIVE_OVERRIDE_PARAMETERS;
                if (!read_parameters(&r, p, override)) {
                    return false;
                }
            }
            break;
        default:
            return false;
        }

    }
}

static void run_common(struct suit_processor *p)
{
    UsefulBufC common = p->root_envelope->manifest.common;

    QCBORDecodeContext dc;
    QCBORDecode_Init(&dc, common, QCBOR_DECODE_MODE_NORMAL);

    struct array_reader r;
    if (!map_start(&dc, &r)) {
        return false;
    }

    if (map_matchT(&r, SUIT_DEPENDENCIES, QCBOR_TYPE_ARRAY)) {
        for (;TODO;) {
            get_or_allocate_dependency(p, ...);
        }
        map_next(&r);
    }

    if (map_matchT(&r, SUIT_COMPONENTS, QCBOR_TYPE_ARRAY)) {
        for (;TODO;) {
            get_or_allocate_component(p, ...);
        }
        map_next(&r);
    }

    if (map_matchT(&r, SUIT_COMPONENTS, QCBOR_TYPE_BYTE_STRING)) {
        // initialize dependencies and compnents with command
        if (!run_common_command(p, ...)) {
            return false;
        }
        map_next(&r);
    }

    while (map_next(&r)) {
    }

    if (QCBORDecode_Finish(&dc) != QCBOR_SUCCES) {
        return false;
    }
    return true;
}

static bool suit_processor_run_1(struct suit_processor *p, enum suit_result *sr);

enum suit_result suit_processor_run(struct suit_processor *p)
{
    enum suit_result sr;

    while (suit_processor_run_1(p, &sr))
        ;
    
    return sr;
}

void suit_processor_get_request(const struct suit_processor *p, struct suit_platform_request *ret)
{

}

static uint64_t step_bit(enum suit_step step)
{
    return 1UL << step;
}

struct step_handler {
    enum suit_step step;
    bool (*handler)(struct suit_processor *p, enum suit_result *sr, enum suit_step step, bool parse_only);
};

static bool handle_manifest_version(struct suit_processor *p, enum suit_result *sr, enum suit_step step, bool parse_only);
static bool handle_manifest_sequence_number(struct suit_processor *p, enum suit_result *sr, enum suit_step step, bool parse_only);
static bool handle_command_sequence(struct suit_processor *p, enum suit_result *sr, enum suit_step step, bool parse_only);
static bool handle_reference_uri(struct suit_processor *p, enum suit_result *sr, enum suit_step step, bool parse_only);
static bool handle_text(struct suit_processor *p, enum suit_result *sr, enum suit_step step, bool parse_only);
static bool handle_coswid(struct suit_processor *p, enum suit_result *sr, enum suit_step step, bool parse_only);

static struct step_handler step_handlers = {
    { suit_manifest_version, handle_manifest_version },
    { suit_manifest_sequence_number, handle_manifest_sequence_number },
    { suit_common, handle_command_sequence },
    { suit_reference_uri, handle_reference_uri },
    { suit_dependency_resolution, handle_command_sequence },
    { suit_payload_fetch, handle_command_sequence },
    { suit_install, handle_command_sequence },
    { suit_validate, handle_command_sequence },
    { suit_load, handle_command_sequence },
    { suit_run, handle_command_sequence },
    { suit_text, handle_text },
    { suit_coswid, handle_coswid },
    { 0, 0 },
};

static bool suit_processor_run_1(struct suit_processor *p, enum suit_result *sr)
{
    for (int i = 0; step_handlers[i].handler; i++) {
        enum suit_step step = step_handlers[i].step;
        if (!(p->steps_done & step_bit(step))) {
            bool requested = p->steps_requested & step_bit(step);
            bool parse_only = !requested;
            return step_handlers[i].handler(p, sr, step, parse_only);
        }
    }
}

static bool handle_fail(enum suit_result *sr)
{
    *sr = SUIT_ERROR;
    return false;
}

static bool handle_assert(bool cond, enum suit_result *sr)
{
    if (!cond) return handle_fail(sr);
    return true;
}

static bool next_toplevel(struct suit_processor *p, enum suit_result *sr, QCBORItem *item)
{
    QCBORDecode_GetNext(&p->parser.manifest, item);
    if (!handle_assert(item.uNestingLevel == p->parser.toplevel.uNestingLevel + 1)) return false;
    return true;
}

static bool handle_manifest_version(struct suit_processor *p, enum suit_result *sr, enum suit_step step, bool parse_only)
{
    QCBORDecode_GetNext(&p->parser.manifest, &p->parser.toplevel);
    if (!handle_assert(p->parser.toplevel.uDataType != QCBOR_TYPE_MAP, sr)) return false;

    QCBORItem item;
    if (!next_toplevel(p, sr, &item)) return false;


}

static bool handle_manifest_sequence_number(struct suit_processor *p, enum suit_result *sr, enum suit_step step, bool parse_only)
{

}

static bool handle_command_sequence(struct suit_processor *p, enum suit_result *sr, enum suit_step step, bool parse_only)
{

}

static bool handle_reference_uri(struct suit_processor *p, enum suit_result *sr, enum suit_step step, bool parse_only)
{

}

static bool handle_text(struct suit_processor *p, enum suit_result *sr, enum suit_step step, bool parse_only)
{

}

static bool handle_coswid(struct suit_processor *p, enum suit_result *sr, enum suit_step step, bool parse_only)
{

}
