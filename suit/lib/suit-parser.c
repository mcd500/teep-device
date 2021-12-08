#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "nocbor.h"
#include "teesuit.h"
#include "teelog.h"

static bool match_key(nocbor_context_t *ctx, uint64_t key)
{
    nocbor_context_t saved = *ctx;
    uint64_t k;
    if (!nocbor_read_uint(ctx, &k)) goto err;
    if (k != key) goto err;
    return true;
err:
    return nocbor_fail(ctx, saved);
}

static bool read_key_and_uint(nocbor_context_t *ctx, uint64_t key, uint64_t *dst)
{
    nocbor_context_t saved = *ctx;

    if (!match_key(ctx, key)) goto err;
    if (!nocbor_read_uint(ctx, dst)) goto err;
    return true;
err:
    return nocbor_fail(ctx, saved);
}

static bool read_key_and_bstr(nocbor_context_t *ctx, uint64_t key, nocbor_range_t *dst)
{
    nocbor_context_t saved = *ctx;

    if (!match_key(ctx, key)) goto err;
    if (!nocbor_read_bstr(ctx, dst)) goto err;
    return true;
err:
    return nocbor_fail(ctx, saved);
}


static bool read_digest(nocbor_context_t *ctx, suit_digest_t *d)
{
    nocbor_context_t array;
    if (!nocbor_read_array(ctx, &array)) goto err;
    if (!nocbor_read_nint(&array, &d->algorithm_id)) goto err;
    if (!nocbor_read_bstr(&array, &d->bytes)) goto err;
    if (!nocbor_skip_all(&array)) goto err;
    if (!nocbor_close(ctx, array)) goto err;
    return true;
err:
    return false;
}

static bool read_severed(nocbor_context_t *ctx, suit_severable_t *ret)
{
    if (nocbor_read_bstr(ctx, &ret->body)) {
        ret->has_value = true;
        ret->severed = false;
        return true;
    } else if (read_digest(ctx, &ret->digest)) {
        ret->has_value = true;
        ret->severed = true;
        return true;
    } else {
        return false;
    }
}

static bool read_component(nocbor_context_t *ctx, suit_component_t *cp)
{
    if (!nocbor_read_subobject(ctx, &cp->id_cbor)) {
        return false;
    }
    return true;
}

static bool read_dependency(nocbor_context_t *ctx, suit_dependency_t *dp)
{
    nocbor_context_t map;
    if (!nocbor_read_map(ctx, &map)) goto err;

    // TODO
    //if (!match_key(&map, SUIT_DEPENDENCY_DIGEST)) goto err;
    //if (!read_digest(&map, &dp->digest)) goto err;

    //dp->component_id = (nocbor_range_t){ NULL, NULL };
    //if (match_key(&map, SUIT_DEPENDENCY_PREFIX)) {
    //    // TODO
    //    if (!nocbor_skip(&map, 1)) goto err;
    //}

    if (!nocbor_skip_all(&map)) goto err;
    if (!nocbor_close(ctx, map)) goto err;

    return true;
err:
    return false;
}

static bool suit_parse_common(nocbor_range_t common_bstr, suit_common_t *ret)
{
    ret->n_dependencies = 0;
    ret->n_components = 0;
    ret->command_sequence = nocbor_range_null();

    nocbor_context_t ctx = nocbor_toplevel(common_bstr);

    nocbor_context_t map;
    if (!nocbor_read_map(&ctx, &map)) return false;

    bool dependencises_seen = false;
    bool components_seen = false;

    while (!nocbor_is_end_of_context(map)) {
        nocbor_any_t key;
        if (!nocbor_read_any(&map, &key)) return false;
        if (nocbor_is_tag(key)) return false;

        if (nocbor_is_uint(key)) {
            switch (key.value) {
            case SUIT_DEPENDENCIES: {
                if (dependencises_seen) return false;
                dependencises_seen = true;

                nocbor_context_t array;
                if (!nocbor_read_array(&map, &array)) return false;
                while (!nocbor_is_end_of_context(array)) {
                    if (ret->n_dependencies == SUIT_DEPENDENCY_MAX) return false;
                    if (!read_dependency(&array, &ret->dependencies[ret->n_dependencies])) return false;
                    ret->n_dependencies++;
                }
                if (!nocbor_close(&map, array)) return false;
                break;
            }
            case SUIT_COMPONENTS: {
                if (components_seen) return false;
                components_seen = true;

                nocbor_context_t array;
                if (!nocbor_read_array(&map, &array)) return false;
                while (!nocbor_is_end_of_context(array)) {
                    if (ret->n_components == SUIT_COMPONENT_MAX) return false;
                    if (!read_component(&array, &ret->components[ret->n_components])) return false;
                    ret->n_components++;
                }
                if (!nocbor_close(&map, array)) return false;
                break;
            }
            case SUIT_COMMON_SEQUENCE:
                if (!nocbor_range_is_null(ret->command_sequence)) return false;
                if (!nocbor_read_bstr(&map, &ret->command_sequence)) return false;
                break;
            default:
                return false;
            }
        } else {
            return false;
        }
    }
    if (!nocbor_close(&ctx, map)) return false;

    return true;
}

bool suit_manifest_parse(nocbor_range_t manifest_bstr, suit_manifest_t *ret)
{
    ret->envelope_bstr = nocbor_range_null();
    ret->binary = manifest_bstr;
    ret->common_bstr = nocbor_range_null();
    ret->reference_uri = nocbor_range_null();
    ret->dependency_resolution.has_value = false;
    ret->payload_fetch.has_value = false;
    ret->install.has_value = false;
    ret->text.has_value = false;
    ret->coswid.has_value = false;
    ret->validate = nocbor_range_null();
    ret->load = nocbor_range_null();
    ret->run = nocbor_range_null();

    nocbor_context_t ctx = nocbor_toplevel(manifest_bstr);

    nocbor_context_t map;
    if (!nocbor_read_map(&ctx, &map)) return false;

    //
    // version and sequence-number must be first element.
    //
    if (!read_key_and_uint(&map, SUIT_MANIFEST_VERSION, &ret->version)) {
        return false;
    }
    if (ret->version != 1) {
        return false;
    }
    if (!read_key_and_uint(&map, SUIT_MANIFEST_SEQUENCE_NUMBER, &ret->sequence_number)) {
        return false;
    }

    while (!nocbor_is_end_of_context(map)) {
        nocbor_any_t key;
        if (!nocbor_read_any(&map, &key)) return false;
        if (nocbor_is_tag(key)) return false;

        if (nocbor_is_uint(key)) {
            switch (key.value) {
            case SUIT_MANIFEST_VERSION:
            case SUIT_MANIFEST_SEQUENCE_NUMBER:
                return false;
            case SUIT_COMMON:
                if (!nocbor_read_bstr(&map, &ret->common_bstr)) return false;
                if (!suit_parse_common(ret->common_bstr, &ret->common)) return false;
                break;
            case SUIT_REFERENCE_URI:
                if (!nocbor_read_bstr(&map, &ret->reference_uri)) return false;
                break;
            case SUIT_DEPENDENCY_RESOLUTION:
                if (!read_severed(&map, &ret->dependency_resolution)) return false;
                break;
            case SUIT_PAYLOAD_FETCH:
                if (!read_severed(&map, &ret->payload_fetch)) return false;
                break;
            case SUIT_INSTALL:
                if (!read_severed(&map, &ret->install)) return false;
                break;
            case SUIT_TEXT:
                if (!read_severed(&map, &ret->text)) return false;
                break;
            case SUIT_COSWID:
                if (!read_severed(&map, &ret->coswid)) return false;
                break;
            case SUIT_VALIDATE:
                if (!nocbor_read_bstr(&map, &ret->validate)) return false;
                break;
            case SUIT_LOAD:
                if (!nocbor_read_bstr(&map, &ret->load)) return false;
                break;
            case SUIT_RUN:
                if (!nocbor_read_bstr(&map, &ret->run)) return false;
                break;
            default:
                // skip unknown key value
                if (!nocbor_skip(&map, 1)) return false;
            }
        } else {
            // skip unknown key value
            if (!nocbor_skip(&map, 1)) return false;
        }
    }
    if (!nocbor_close(&ctx, map)) return false;

    if (nocbor_range_is_null(ret->common_bstr)) return false;

    return true;
}

bool suit_parse_authentication_wrapper(nocbor_range_t authentication_wrapper_bstr, suit_authentication_wrapper_t *ret)
{
    nocbor_context_t ctx = nocbor_toplevel(authentication_wrapper_bstr);

    nocbor_context_t array;
    if (!nocbor_read_array(&ctx, &array)) return false;

    nocbor_range_t digest_bstr;
    if (!nocbor_read_bstr(&array, &digest_bstr)) return false;

    nocbor_context_t digest_ctx = nocbor_toplevel(digest_bstr);
    if (!read_digest(&digest_ctx, &ret->digest)) return false;

    while (!nocbor_is_end_of_context(array)) {
        nocbor_range_t auth_block;
        if (!nocbor_read_bstr(&array, &auth_block)) return false;
        // TODO: do auth
    }
    if (!nocbor_close(&ctx, array)) return false;

    return true;
}


static bool suit_envelope_get_field(
    nocbor_range_t envelope_bstr,
    bool use_tstr, enum suit_cbor_label key, nocbor_range_t key_tstr,
    nocbor_range_t *ret)
{
    if (!use_tstr) {
        switch (key) {
        case SUIT_DELEGATION:
        case SUIT_AUTHENTICATION_WRAPPER:
        case SUIT_MANIFEST:
        case SUIT_DEPENDENCY_RESOLUTION:
        case SUIT_PAYLOAD_FETCH:
        case SUIT_INSTALL:
        case SUIT_TEXT:
        case SUIT_COSWID:
            break;
        default:
            return false;
        }
    }

    nocbor_context_t ctx = nocbor_toplevel(envelope_bstr);
    
    uint64_t tag;
    if (!nocbor_read_tag(&ctx, &tag)) return false;
    if (tag != 107) return false;

    nocbor_context_t map;
    if (!nocbor_read_map(&ctx, &map)) return false;

    nocbor_range_t field = nocbor_range_null();
    
    while (!nocbor_is_end_of_context(map)) {
        nocbor_any_t k;
        if (!nocbor_read_any(&map, &k)) return false;
        if (nocbor_is_tag(k)) return false;

        if (nocbor_is_uint(k)) {
            if (!use_tstr && k.value == key) {
                if (!nocbor_read_bstr(&map, &field)) return false;
            } else {
                if (!nocbor_skip(&map, 1)) return false;
            }
        } else if (nocbor_is_tstr(k)) {
            if (use_tstr && nocbor_range_equal(k.bytes, key_tstr)) {
                if (!nocbor_read_bstr(&map, &field)) return false;
            } else {
                if (!nocbor_skip(&map, 1)) return false;
            }
        } else {
            // skip unknown key value
            if (!nocbor_skip(&map, 1)) return false;
        }
    }
    *ret = field;
    return true;
}

bool suit_envelope_get_field_by_name(nocbor_range_t envelope_bstr, nocbor_range_t key_tstr, nocbor_range_t *ret)
{
    return suit_envelope_get_field(envelope_bstr, true, SUIT_DELEGATION, key_tstr, ret);
}

bool suit_envelope_get_field_by_key(nocbor_range_t envelope_bstr, enum suit_cbor_label key, nocbor_range_t *ret)
{
    return suit_envelope_get_field(envelope_bstr, false, key, nocbor_range_null(), ret);
}

bool suit_envelope_get_manifest(nocbor_range_t envelope_bstr, suit_manifest_t *ret)
{
    nocbor_range_t manifest_bstr;

    if (!suit_envelope_get_field_by_key(envelope_bstr, SUIT_MANIFEST, &manifest_bstr)) return false;
    if (!suit_manifest_parse(manifest_bstr, ret)) return false;

    ret->envelope_bstr = envelope_bstr;
    return true;
}

#if 0
bool suit_parse_envelope(nocbor_range_t envelope_bstr, suit_envelope_t *ret);
{
    ret->binary = envelope_bstr;
    ret->delegation = nocbor_range_null();
    ret->authentication_wrapper = nocbor_range_null();
    ret->manifest = nocbor_range_null();
    ret->dependency_resolution = nocbor_range_null();
    ret->payload_fetch = nocbor_range_null();
    ret->install = nocbor_range_null();
    ret->text = nocbor_range_null();
    ret->delegation = nocbor_range_null();
    ret->coswid = nocbor_range_null();

    nocbor_context_t ctx = nocbor_toplevel(envelope);

    uint64_t tag;
    if (!nocbor_read_tag(&ctx, &tag)) return false;
    if (tag != 107) return false;

    nocbor_context_t map;
    if (!nocbor_read_map(&ctx, &map)) return false;

    while (!nocbor_is_end_of_context(map)) {
        nocbor_any_t key;
        if (!nocbor_read_any(&map, &key)) return false;
        if (nocbor_is_tag(key)) return false;

        if (nocbor_is_uint(key)) {
            switch (key.value) {
            case SUIT_DELEGATION:
                if (!nocbor_read_bstr(&map, &ret->delegation)) return false;
                break;
            case SUIT_AUTHENTICATION_WRAPPER:
                if (!nocbor_read_bstr(&map, &ret->authentication_wrapper)) return false;
                break;
            case SUIT_MANIFEST:
                if (!nocbor_read_bstr(&map, &ret->manifest)) return false;
                break;
            case SUIT_DEPENDENCY_RESOLUTION:
                if (!nocbor_read_bstr(&map, &ret->dependency_resolution)) return false;
                break;
            case SUIT_PAYLOAD_FETCH:
                if (!nocbor_read_bstr(&map, &ret->payload_fetch)) return false;
                break;
            case SUIT_INSTALL:
                if (!nocbor_read_bstr(&map, &ret->install)) return false;
                break;
            case SUIT_TEXT:
                if (!nocbor_read_bstr(&map, &ret->text)) return false;
                break;
            case SUIT_DELEGATION:
                if (!nocbor_read_bstr(&map, &ret->delegation)) return false;
                break;
            case SUIT_COSWID:
                if (!nocbor_read_bstr(&map, &ret->coswid)) return false;
                break;
            default:
                // skip unknown key value
                if (!nocbor_skip(&map, 1)) return false;
                break;
            }
        } else if (nocbor_is_tstr(key)) {
            // in here syntax check only
            nocbor_range_t payload;
            if (!nocbor_read_bstr(&map, &payload)) return false;
        } else {
            // skip unknown key value
            if (!nocbor_skip(&map, 1)) return false;
        }
    }
    if (!nocbor_close(&ctx, map)) return false;

    if (nocbor_range_is_null(ret->authentication_wrapper)) return false;
    if (nocbor_range_is_null(ret->manifest)) return false;

    return true;
}
#endif

bool suit_envelope_get_severed(const suit_severable_t *severable, nocbor_range_t severed, nocbor_range_t *ret)
{
    if (!severable->has_value) {
        *ret = nocbor_range_null();
        return true;
    } else if (severable->severed) {
        // TODO: check digest
        *ret = severed;
        return true;
    } else {
        *ret = severable->body;
        return true;
    }
}

bool suit_command_sequence_open(nocbor_range_t command_sequence_bstr, suit_command_reader_t *ret)
{
    nocbor_context_t ctx = nocbor_toplevel(command_sequence_bstr);
    nocbor_context_t array;

    if (!nocbor_read_array(&ctx, &array)) return false;

    ret->ctx = array;
    return true;
}

bool suit_command_sequence_read_command(suit_command_reader_t *r, enum suit_command *ret)
{
    uint64_t command;
    if (!nocbor_read_uint(&r->ctx, &command)) return false;

    if (command > INT_MAX) return false;
    *ret = command;

    return true;
}

bool suit_command_sequence_read_reppolicy(suit_command_reader_t *r /* TODO ret */)
{
    uint64_t policy;
    if (!nocbor_read_uint(&r->ctx, &policy)) return false;

    // TODO

    return true;

}

bool suit_command_sequence_skip_param(suit_command_reader_t *r)
{
    return nocbor_skip(&r->ctx, 1);
}

bool suit_command_sequence_end_of_sequence(suit_command_reader_t r)
{
    return nocbor_is_end_of_context(r.ctx);
}
