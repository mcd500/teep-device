#include <stdio.h>
#include <string.h>
#include "nocbor.h"
#include "teesuit.h"

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

static bool read_key_and_severable(nocbor_context_t *ctx, uint64_t key, suit_severed_t *p)
{
    p->has_value = false;
    if (!read_key_and_bstr(ctx, key, &p->body)) return false;
    p->has_value = true;
    p->severed = false;
    return true;
}

static bool read_key_and_severed(nocbor_context_t *ctx, uint64_t key, suit_severed_t *p)
{
    nocbor_context_t saved = *ctx;

    if (!match_key(ctx, key)) goto err;
    if (p->has_value) goto err;
    nocbor_context_t array;
    if (!nocbor_read_array(ctx, &array)) goto err;
    if (!nocbor_read_uint(&array, &p->digest.algorithm_id)) goto err;
    if (!nocbor_read_bstr(&array, &p->digest.bytes)) goto err;
    if (!nocbor_skip_all(&array)) goto err;
    if (!nocbor_close(ctx, array)) goto err;
    p->has_value = true;
    p->severed = true;
    return true;
err:
    return nocbor_fail(ctx, saved);
}

bool suit_parse_manifest(struct suit_manifest *mp, nocbor_range_t manifest)
{
    mp->binary = manifest;

    nocbor_context_t ctx = nocbor_toplevel(manifest);

    nocbor_context_t map;
    if (!nocbor_read_map(&ctx, &map)) return false;

    // suit-manifest-version         => 1,
    if (!read_key_and_uint(&map, SUIT_MANIFEST_VERSION, &mp->version)) {
        return false;
    }
    if (mp->version != 1) {
        return false;
    }

    // suit-manifest-sequence-number => uint,
    if (!read_key_and_uint(&map, SUIT_MANIFEST_SEQUENCE_NUMBER, &mp->sequence_number)) {
        return false;
    }

    // suit-common                   => bstr .cbor SUIT_Common,
    if (!read_key_and_bstr(&map, SUIT_COMMON, &mp->common)) {
        return false;
    }

    // ? suit-reference-uri          => tstr,
    mp->reference_uri = (nocbor_range_t) { NULL, NULL };
    read_key_and_bstr(&map, SUIT_REFERENCE_URI, &mp->reference_uri);

    // SUIT_Severable_Manifest_Members,
    read_key_and_severable(&map, SUIT_DEPENDENCY_RESOLUTION, &mp->dependency_resolution);
    read_key_and_severable(&map, SUIT_PAYLOAD_FETCH, &mp->payload_fetch);
    read_key_and_severable(&map, SUIT_INSTALL, &mp->install);
    read_key_and_severable(&map, SUIT_TEXT, &mp->text);
    read_key_and_severable(&map, SUIT_COSWID, &mp->coswid);

    // SUIT_Severable_Members_Digests,
    read_key_and_severed(&map, SUIT_DEPENDENCY_RESOLUTION, &mp->dependency_resolution);
    read_key_and_severed(&map, SUIT_PAYLOAD_FETCH, &mp->payload_fetch);
    read_key_and_severed(&map, SUIT_INSTALL, &mp->install);
    read_key_and_severed(&map, SUIT_TEXT, &mp->text);
    read_key_and_severed(&map, SUIT_COSWID, &mp->coswid);

    // SUIT_Unseverable_Members,
    mp->validate = (nocbor_range_t) { NULL, NULL };
    read_key_and_bstr(&map, SUIT_VALIDATE, &mp->validate);
    mp->load = (nocbor_range_t) { NULL, NULL };
    read_key_and_bstr(&map, SUIT_LOAD, &mp->load);
    mp->run = (nocbor_range_t) { NULL, NULL };
    read_key_and_bstr(&map, SUIT_RUN, &mp->run);

    if (!nocbor_close(&ctx, map)) return false;

    return true;
}

bool suit_parse_envelope(struct suit_envelope *ep, nocbor_range_t envelope)
{
    ep->binary = envelope;

    nocbor_context_t ctx = nocbor_toplevel(envelope);

    nocbor_context_t map;
    if (!nocbor_read_map(&ctx, &map)) return false;

    ep->delegation = (nocbor_range_t) { NULL, NULL };
    // ? suit-delegation => bstr .cbor SUIT_Delegation
    read_key_and_bstr(&map, SUIT_DELEGATION, &ep->delegation);

    // suit-authentication-wrapper => bstr .cbor SUIT_Authentication
    if (!read_key_and_bstr(&map, SUIT_AUTHENTICATION_WRAPPER, &ep->authentication_wrapper)) {
        return false;
    }

    nocbor_range_t manifest;
    // suit-manifest  => bstr .cbor SUIT_Manifest
    if (!read_key_and_bstr(&map, SUIT_MANIFEST, &manifest)) {
        return false;
    }
    if (!suit_parse_manifest(&ep->manifest, manifest)) {
        return false;
    }

    // following fields are only syntax checked.

    // SUIT_Severable_Manifest_Members = (
    //   ? suit-dependency-resolution => bstr .cbor SUIT_Command_Sequence,
    //   ? suit-payload-fetch => bstr .cbor SUIT_Command_Sequence,
    //   ? suit-install => bstr .cbor SUIT_Command_Sequence,
    //   ? suit-text => bstr .cbor SUIT_Text_Map,
    //   ? suit-coswid => bstr .cbor concise-software-identity,
    //   * $$SUIT_severable-members-extensions,
    // )
    read_key_and_bstr(&map, SUIT_DEPENDENCY_RESOLUTION, NULL);
    read_key_and_bstr(&map, SUIT_PAYLOAD_FETCH, NULL);
    read_key_and_bstr(&map, SUIT_INSTALL, NULL);
    read_key_and_bstr(&map, SUIT_TEXT, NULL);
    read_key_and_bstr(&map, SUIT_DELEGATION, NULL);
    read_key_and_bstr(&map, SUIT_COSWID, NULL);

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

    // TODO

    return true;
}

void suit_processor_init(suit_processor_t *p)
{
    memset(p, 0, sizeof *p);
}

bool suit_processor_load_root_envelope(suit_processor_t *p, nocbor_range_t envelope)
{
    if (p->n_envelope) return false;

    suit_envelope_t *ep = &p->envelope_buf[0];
    if (!suit_parse_envelope(ep, envelope)) return false;

    p->n_envelope++;
    return true;
}

suit_envelope_t *suit_processor_get_root_envelope(suit_processor_t *p)
{
    if (p->n_envelope == 0) return NULL;
    return &p->envelope_buf[0];
}

static bool load_program_counter(suit_runner_t *runner, nocbor_range_t commands)
{
    nocbor_context_t ctx = nocbor_toplevel(commands);

    nocbor_context_t array;
    if (!nocbor_read_array(&ctx, &array)) goto err;
    runner->program_counter.is_try_each = false;
    runner->program_counter.is_try_each = false; // TODO
    runner->program_counter.context = array;
    return true;
err:
    suit_runner_mark_error(runner);
    return false;
}

static bool call_commands(suit_runner_t *runner, nocbor_range_t commands)
{
    if (runner->n_stack == SUIT_STACK_MAX) goto err;

    runner->stack[runner->n_stack] = runner->program_counter;
    runner->n_stack++;
    return load_program_counter(runner, commands);
err:
    suit_runner_mark_error(runner);
    return false;
}

static bool read_component(nocbor_context_t *ctx, suit_component_t *cp)
{
    // TODO
    return true;
}

static bool read_dependency(nocbor_context_t *ctx, suit_dependency_t *dp)
{
    // TODO
    return true;
}

static suit_component_t *get_or_alloc_component(suit_runner_t *runner, const suit_component_t *cp)
{
    // TODO
    return NULL;
}

static suit_dependency_t *get_or_alloc_dependency(suit_runner_t *runner, const suit_dependency_t *dp)
{
    // TODO
    return NULL;
}

static void select_all_component(suit_runner_t *runner)
{
    // TODO
}

static bool load_common(suit_runner_t *runner)
{
    suit_envelope_t *ep = suit_processor_get_root_envelope(runner->processor);
    if (!ep) goto err;

    nocbor_context_t ctx = nocbor_toplevel(ep->manifest.common);

    nocbor_context_t map;
    if (!nocbor_read_map(&ctx, &map)) goto err;

    if (match_key(&map, SUIT_DEPENDENCIES)) {
        nocbor_context_t array;
        if (!nocbor_read_array(&map, &array)) goto err;
        while (!nocbor_is_end_of_context(array)) {
            suit_dependency_t d;
            if (!read_dependency(&array, &d)) goto err;
            if (!get_or_alloc_dependency(runner, &d)) goto err;
        }
        if (!nocbor_close(&map, array)) goto err;
    }
    if (match_key(&map, SUIT_COMPONENTS)) {
        nocbor_range_t array_bstr;
        if (nocbor_read_bstr(&map, &array_bstr)) {
            // TODO
        } else {
        nocbor_context_t array;
        if (!nocbor_read_array(&map, &array)) goto err;
        while (!nocbor_is_end_of_context(array)) {
            suit_component_t c;
            if (!read_component(&array, &c)) goto err;
            if (!get_or_alloc_component(runner, &c)) goto err;
        }
        if (!nocbor_close(&map, array)) goto err;
        }
    }
    if (match_key(&map, SUIT_COMMON_SEQUENCE)) {
        nocbor_range_t commands;
        if (!nocbor_read_bstr(&map, &commands)) goto err;
        if (!call_commands(runner, commands)) goto err;
    }
    if (!nocbor_close(&ctx, map)) goto err;

    return true;
err:
    suit_runner_mark_error(runner);
    return false;
}

void suit_runner_init(suit_runner_t *runner, suit_processor_t *p, suit_platform_t *platform, nocbor_range_t commands)
{
    memset(runner, 0, sizeof *runner);
    runner->processor = p;
    runner->platform = platform;
    if (!load_program_counter(runner, commands)) {
        return;
    }
    load_common(runner);
    select_all_component(runner);
}

struct command_handler
{
    enum suit_command command;
    bool allow_common;
    bool (*handler)(suit_runner_t *runner, enum suit_command command);
};

static bool read_rep_policy(nocbor_context_t *ctx)
{
    uint64_t policy;
    if (nocbor_read_uint(ctx, &policy)) {
        return true;
    } else if (nocbor_read_null(ctx)) {
        return true;
    } else {
        return false;
    }
}

static bool condition_vendor_identifier(suit_runner_t *runner, enum suit_command command)
{
    suit_continuation_t *pc = &runner->program_counter;
    if (!read_rep_policy(&pc->context)) return false;
    printf("execute suit-condition-vendor-identifier\n");
    return true;
}

static bool condition_class_identifier(suit_runner_t *runner, enum suit_command command)
{
    suit_continuation_t *pc = &runner->program_counter;
    if (!read_rep_policy(&pc->context)) return false;
    printf("execute suit-condition-class-identifier\n");
    return true;
}

static bool condition_image_match(suit_runner_t *runner, enum suit_command command)
{
    suit_continuation_t *pc = &runner->program_counter;
    if (!read_rep_policy(&pc->context)) return false;
    printf("execute suit-condition-image-match\n");
    return true;
}

static bool directive_set_index(suit_runner_t *runner, enum suit_command command)
{
    suit_continuation_t *pc = &runner->program_counter;
    // TODO
    return false;
}

static suit_binder_t *lookup_binder(suit_runner_t *runner, suit_component_t *component, uint64_t key)
{
    for (int i = 0; i < runner->n_binder; i++) {
        suit_binder_t *binder = &runner->bindings[i];
        if (binder->component == component && binder->key == key) {
            return binder;
        }
    }
    return NULL;
}

static suit_binder_t *alloc_binder(suit_runner_t *runner, suit_component_t *component, uint64_t key)
{
    if (runner->n_binder == SUIT_BINDER_MAX) return NULL;
    suit_binder_t *binder = &runner->bindings[runner->n_binder++];
    binder->component = component;
    binder->key = key;
    return binder;
}

static bool set_parameter(suit_runner_t *runner, uint64_t key, nocbor_any_t value, bool override)
{
    suit_component_t *component = NULL; // TODO: use component index
    suit_binder_t *binder = lookup_binder(runner, component, key);
    if (binder && !override) return true;
    if (!binder) {
        binder = alloc_binder(runner, component, key);
        if (!binder) return false;
    }
    binder->value = value;
    return true;
}

bool suit_runner_get_parameter(suit_runner_t *runner, uint64_t key, nocbor_any_t *any)
{
    suit_component_t *component = NULL; // TODO: use component index
    suit_binder_t *binder = lookup_binder(runner, component, key);
    if (!binder) {
        return false;
    }
    *any = binder->value;
    return true;
}

static bool directive_set_parameters(suit_runner_t *runner, enum suit_command command)
{
    bool is_override = command == SUIT_DIRECTIVE_OVERRIDE_PARAMETERS;
    suit_continuation_t *pc = &runner->program_counter;
    
    nocbor_context_t map;
    if (!nocbor_read_map(&pc->context, &map)) goto err;
    while (!nocbor_is_end_of_context(map)) {
        uint64_t key;
        nocbor_any_t value;
        if (!nocbor_read_uint(&map, &key)) goto err;
        // TODO: cbor-pen tag
        if (!nocbor_read_any(&map, &value)) goto err;

        if (!(nocbor_is_uint(value)
            || nocbor_is_nint(value)
            || nocbor_is_bool(value)
            || nocbor_is_bstr(value)
            || nocbor_is_tstr(value))) goto err;

        if (!set_parameter(runner, key, value, is_override)) goto err;
    }
    if (!nocbor_close(&pc->context, map)) goto err;

    return true;
err:
    return false;
}

static bool directive_fetch(suit_runner_t *runner, enum suit_command command)
{
    suit_continuation_t *pc = &runner->program_counter;
    if (!read_rep_policy(&pc->context)) return false;
    printf("execute suit-directive-fetch\n");
    return runner->platform->callbacks->fetch(runner);
}


static struct command_handler command_handlers[] = {
    { SUIT_CONDITION_VENDOR_IDENTIFIER, true, condition_vendor_identifier },
    { SUIT_CONDITION_CLASS_IDENTIFIER, true, condition_class_identifier },
    { SUIT_CONDITION_IMAGE_MATCH, true, condition_image_match },

    { SUIT_DIRECTIVE_SET_COMPONENT_INDEX, true, directive_set_index },
    { SUIT_DIRECTIVE_SET_PARAMETERS, true, directive_set_parameters },
    { SUIT_DIRECTIVE_OVERRIDE_PARAMETERS, true, directive_set_parameters },

    { SUIT_DIRECTIVE_FETCH, false, directive_fetch },

    { 0, false, NULL},
};

void suit_runner_run(suit_runner_t *runner)
{
    if (runner->has_error) return;

    for (;;) {
        if (runner->suspended) return;

        suit_continuation_t *pc = &runner->program_counter;

        if (nocbor_is_end_of_context(pc->context)) {
            if (runner->n_stack == 0) {
                break;
            } else {
                runner->n_stack--;
                runner->program_counter = runner->stack[runner->n_stack];
                // TODO: check is_try_each
            }
        } else if (pc->is_try_each) {
            // TODO
        } else {
            uint64_t command;

            if (!nocbor_read_uint(&pc->context, &command)) goto err;

            struct command_handler *h = NULL;
            for (int i = 0; command_handlers[i].handler; i++) {
                if (command_handlers[i].command == command) {
                    h = &command_handlers[i];
                    break;
                }
            }
            if (!h) {
                printf("unknown command %lu\n", command);
                goto err;
            }
            if (pc->is_common && !h->allow_common) goto err;
            if (!h->handler(runner, command)) goto err;
        }
        if (runner->has_error) {
            // TODO: unwind stack
            break;
        }
    }
    return;
err:
    suit_runner_mark_error(runner); // syntax error
    return;
}

bool suit_runner_finished(suit_runner_t *runner)
{
    return !runner->has_error && !runner->suspended;
}

bool suit_runner_has_error(suit_runner_t *runner)
{
    return runner->has_error;
}

bool suit_runner_suspended(suit_runner_t *runner)
{
    return !runner->has_error && runner->suspended;
}


void suit_runner_mark_error(suit_runner_t *runner)
{
    runner->has_error = true;
}

bool suit_runner_get_error(const suit_runner_t *runner, void *error_enum_todo)
{
    return runner->has_error;
}

void suit_runner_suspend(suit_runner_t *runner, void (*on_resume)(suit_runner_t *, void *))
{
    runner->suspended = true;
    runner->on_resume = on_resume;
}

void suit_runner_resume(suit_runner_t *runner, void *user)
{
    if (runner->has_error) {
        return;
    }
    if (runner->suspended) {
        // allow on_resume to call suspend
        runner->suspended = false;
        void (*f)(suit_runner_t *, void *) = runner->on_resume;
        runner->on_resume = NULL;
        if (f) {
            f(runner, user);
        }
    }
}
