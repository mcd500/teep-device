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

static bool read_digest(nocbor_context_t *ctx, suit_digest_t *d)
{
    nocbor_context_t array;
    if (!nocbor_read_array(ctx, &array)) goto err;
    if (!nocbor_read_uint(&array, &d->algorithm_id)) goto err;
    if (!nocbor_read_bstr(&array, &d->bytes)) goto err;
    if (!nocbor_skip_all(&array)) goto err;
    if (!nocbor_close(ctx, array)) goto err;
    return true;
err:
    return false;
}

static bool read_key_and_severed(nocbor_context_t *ctx, uint64_t key, suit_severed_t *p)
{
    nocbor_context_t saved = *ctx;

    if (!match_key(ctx, key)) goto err;
    if (p->has_value) goto err;
    if (!read_digest(ctx, &p->digest)) goto err;
    p->has_value = true;
    p->severed = true;
    return true;
err:
    return nocbor_fail(ctx, saved);
}

static bool suit_parse_authentication_wrapper(suit_authentication_wrapper_t *ap, nocbor_range_t auth_wrapper)
{
    nocbor_context_t ctx = nocbor_toplevel(auth_wrapper);

    nocbor_context_t array;
    if (!nocbor_read_array(&ctx, &array)) return false;

    nocbor_range_t digest_bstr;
    if (!nocbor_read_bstr(&array, &digest_bstr)) return false;

    nocbor_context_t digest_ctx = nocbor_toplevel(digest_bstr);
    if (!read_digest(&digest_ctx, &ap->digest)) return false;

    while (!nocbor_is_end_of_context(array)) {
        nocbor_range_t auth_block;
        if (!nocbor_read_bstr(&array, &auth_block)) return false;
        // TODO: do auth
    }
    if (!nocbor_close(&ctx, array)) return false;

    return true;
}

static bool suit_parse_manifest(struct suit_manifest *mp, nocbor_range_t manifest)
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

    nocbor_range_t auth_wrapper;
    // suit-authentication-wrapper => bstr .cbor SUIT_Authentication
    if (!read_key_and_bstr(&map, SUIT_AUTHENTICATION_WRAPPER, &auth_wrapper)) {
        return false;
    }
    if (!suit_parse_authentication_wrapper(&ep->authentication_wrapper, auth_wrapper)) {
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

bool suit_processor_load_envelope(suit_processor_t *p, int index, nocbor_range_t envelope)
{
    if (index != p->n_envelope) return false; // XXX
    if (index == SUIT_ENVELOPE_MAX) return false;

    suit_envelope_t *ep = &p->envelope_buf[index];
    if (!suit_parse_envelope(ep, envelope)) return false;

    p->n_envelope++;
    return true;

}

suit_envelope_t *suit_processor_get_root_envelope(suit_processor_t *p)
{
    if (p->n_envelope == 0) return NULL;
    return &p->envelope_buf[0];
}

static bool push_program_counter(suit_runner_t *runner)
{
    if (runner->n_stack == SUIT_STACK_MAX) return false;

    runner->stack[runner->n_stack] = runner->program_counter;
    runner->n_stack++;

    return true;
}

static bool pop_program_counter(suit_runner_t *runner)
{
    if (runner->n_stack == 0) return false;

    runner->n_stack--;
    runner->program_counter = runner->stack[runner->n_stack];

    return true;
}

static bool load_label(suit_runner_t *runner, suit_envelope_t *ep, nocbor_range_t *command)
{
    switch (runner->label) {
    case SUIT_INSTALL:
        *command = ep->manifest.install.body; // TODO: severed
        return true;
    // TODO: other case
    default:
        return false;
    }
}

static bool call_label(suit_runner_t *runner, suit_envelope_t *ep, nocbor_range_t common_command)
{
    nocbor_range_t target_command;
    if (!load_label(runner, ep, &target_command)) return false;

    nocbor_context_t target_contxt = nocbor_toplevel(target_command);
    nocbor_context_t common_context = nocbor_toplevel(common_command);

    nocbor_context_t target_array;
    nocbor_context_t common_array;
    if (!nocbor_read_array(&target_contxt, &target_array)) return false;
    if (!nocbor_read_array(&common_context, &common_array)) return false;

    if (!push_program_counter(runner)) return false;
    runner->program_counter.envelope = ep;
    runner->program_counter.next = SUIT_NEXT_EXECUTE_COMMAND;
    runner->program_counter.command.common = common_array;
    runner->program_counter.command.target = target_array;

    return true;
}

static bool read_component(nocbor_context_t *ctx, suit_component_t *cp)
{
    // TODO
    return nocbor_skip(ctx, 1);
}

static bool read_dependency(nocbor_context_t *ctx, suit_dependency_t *dp)
{
    nocbor_context_t map;
    if (!nocbor_read_map(ctx, &map)) goto err;

    if (!match_key(&map, SUIT_DEPENDENCY_DIGEST)) goto err;
    if (!read_digest(&map, &dp->digest)) goto err;

    dp->component_id = (nocbor_range_t){ NULL, NULL };
    if (match_key(&map, SUIT_DEPENDENCY_PREFIX)) {
        // TODO
        if (!nocbor_skip(&map, 1)) goto err;
    }

    if (!nocbor_skip_all(&map)) goto err;
    if (!nocbor_close(ctx, map)) goto err;

    return true;
err:
    return false;
}

static bool digest_equal(suit_digest_t dx, suit_digest_t dy)
{
    if (dx.algorithm_id != dy.algorithm_id) return false;
    if (dx.bytes.end - dx.bytes.begin != dy.bytes.end - dy.bytes.begin) return false;
    if (memcmp(dx.bytes.begin, dy.bytes.begin, dx.bytes.end - dx.bytes.begin) != 0) return false;
    return true;
}

static suit_component_t *get_or_alloc_component(suit_runner_t *runner, const suit_component_t *cp)
{
    // TODO
    return &runner->component_buf[0];
}

static bool resolve_dependency(suit_runner_t *runner, suit_envelope_t *src, suit_digest_t digest, int dependency_index)
{
    if (runner->n_dependency >= SUIT_DEPENDENCY_MAX) return false;

    suit_processor_t *p = runner->processor;
    for (int i = 0; i < p->n_envelope; i++) {
        suit_envelope_t *e = &p->envelope_buf[i];
        suit_digest_t d = e->authentication_wrapper.digest;
        if (digest_equal(digest, d)) {
            suit_dependency_binder_t *p = &runner->dependency_map[runner->n_dependency];
            p->source = src;
            p->dependency_index = dependency_index;
            p->target = e;
            runner->n_dependency++;
            return true;
        }
    }
    return false;
}

static nocbor_range_t empty_array()
{
    static uint8_t array[] = {
        0x80
    };
    return (nocbor_range_t){ array, array + 1 };
}

static bool load_common(suit_runner_t *runner, suit_envelope_t *ep, nocbor_range_t *commands)
{
    if (!ep) goto err;

    nocbor_context_t ctx = nocbor_toplevel(ep->manifest.common);

    nocbor_context_t map;
    if (!nocbor_read_map(&ctx, &map)) goto err;

    if (match_key(&map, SUIT_DEPENDENCIES)) {
        nocbor_context_t array;
        if (!nocbor_read_array(&map, &array)) goto err;
        int dependency_index = 0;
        while (!nocbor_is_end_of_context(array)) {
            suit_dependency_t d;
            if (!read_dependency(&array, &d)) goto err;
            if (!resolve_dependency(runner, ep, d.digest, dependency_index)) goto err;
            dependency_index++;
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
    *commands = empty_array();
    if (match_key(&map, SUIT_COMMON_SEQUENCE)) {
        if (!nocbor_read_bstr(&map, commands)) goto err;
    }
    if (!nocbor_close(&ctx, map)) goto err;

    return true;
err:
    suit_runner_mark_error(runner);
    return false;
}

static bool call_dependency(suit_runner_t *runner, suit_envelope_t *envelope)
{
    nocbor_range_t common_commands;
    if (!load_common(runner, envelope, &common_commands)) return false;
    if (!call_label(runner, envelope, common_commands)) return false;
    return true;
}

void suit_runner_init(suit_runner_t *runner, suit_processor_t *p, suit_platform_t *platform, enum suit_cbor_label label)
{
    memset(runner, 0, sizeof *runner);
    runner->processor = p;
    runner->platform = platform;
    runner->label = label;
    runner->selected_component_bits = ~(uint64_t)0;
    runner->selected_dependency_bits = 0;
    suit_envelope_t *ep = suit_processor_get_root_envelope(runner->processor);
    runner->program_counter.envelope = ep;
    runner->program_counter.next = SUIT_NEXT_EXIT;
    if (!call_dependency(runner, ep)) {
        // TODO
        return;
    }
}

struct command_handler
{
    enum suit_command command;
    bool allow_common;
    bool (*handler)(suit_runner_t *runner, enum suit_command command, nocbor_context_t *ctx);
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

static bool condition_vendor_identifier(suit_runner_t *runner, enum suit_command command, nocbor_context_t *ctx)
{
    if (!read_rep_policy(ctx)) return false;
    printf("execute suit-condition-vendor-identifier\n");
    return true;
}

static bool condition_class_identifier(suit_runner_t *runner, enum suit_command command, nocbor_context_t *ctx)
{
    if (!read_rep_policy(ctx)) return false;
    printf("execute suit-condition-class-identifier\n");
    return true;
}

static bool condition_image_match(suit_runner_t *runner, enum suit_command command, nocbor_context_t *ctx)
{
    if (!read_rep_policy(ctx)) return false;
    printf("execute suit-condition-image-match\n");
    return true;
}

static bool directive_set_index(suit_runner_t *runner, enum suit_command command, nocbor_context_t *ctx)
{
    uint64_t selected = 0;

    bool flag;
    uint64_t index;
    nocbor_context_t array;
    if (nocbor_read_bool(ctx, &flag)) {
        if (flag) {
            selected = ~selected;
        }
    } else if (nocbor_read_uint(ctx, &index)) {
        if (index >= 64) return false;
        selected = 1 << index;
    } else if (nocbor_read_array(ctx, &array)) {
        while (!nocbor_is_end_of_context(array)) {
            if (!nocbor_read_uint(&array, &index)) {
                if (index >= 64) return false;
                // TODO: preserve order???
                selected |= 1 << index;
            }
        }
        if (!nocbor_close(ctx, array)) return false;
    } else {
        return false;
    }
    if (command == SUIT_DIRECTIVE_SET_COMPONENT_INDEX) {
        runner->selected_component_bits = selected;
    } else {
        runner->selected_dependency_bits = selected;
    }
    return true;
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

static bool directive_set_parameters(suit_runner_t *runner, enum suit_command command, nocbor_context_t *ctx)
{
    printf("execute suit-set-parameters\n");
    bool is_override = command == SUIT_DIRECTIVE_OVERRIDE_PARAMETERS;
    
    nocbor_context_t map;
    if (!nocbor_read_map(ctx, &map)) goto err;
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
    if (!nocbor_close(ctx, map)) goto err;

    return true;
err:
    return false;
}

static bool directive_process_dependency(suit_runner_t *runner, enum suit_command command, nocbor_context_t *ctx)
{
    if (!read_rep_policy(ctx)) return false;
    printf("execute suit-directive-process-dependency\n");

    suit_envelope_t *ep = runner->program_counter.envelope;
    uint64_t selected = runner->selected_dependency_bits;

    if (!push_program_counter(runner)) return false;
    runner->program_counter.envelope = ep;
    runner->program_counter.next = SUIT_NEXT_EXECUTE_DEPENDENCY;
    runner->program_counter.dependency.selected = selected;
    return true;
}

static bool directive_fetch(suit_runner_t *runner, enum suit_command command, nocbor_context_t *ctx)
{
    if (!read_rep_policy(ctx)) return false;
    printf("execute suit-directive-fetch\n");
    return runner->platform->callbacks->fetch(runner);
}


static const struct command_handler command_handlers[] = {
    { SUIT_CONDITION_VENDOR_IDENTIFIER, true, condition_vendor_identifier },
    { SUIT_CONDITION_CLASS_IDENTIFIER, true, condition_class_identifier },
    { SUIT_CONDITION_IMAGE_MATCH, true, condition_image_match },

    { SUIT_DIRECTIVE_SET_COMPONENT_INDEX, true, directive_set_index },
    { SUIT_DIRECTIVE_SET_DEPENDENCY_INDEX, true, directive_set_index },
    { SUIT_DIRECTIVE_SET_PARAMETERS, true, directive_set_parameters },
    { SUIT_DIRECTIVE_OVERRIDE_PARAMETERS, true, directive_set_parameters },

    { SUIT_DIRECTIVE_PROCESS_DEPENDENCY, false, directive_process_dependency },
    { SUIT_DIRECTIVE_FETCH, false, directive_fetch },

    { 0, false, NULL},
};

static const struct command_handler *
find_handler(uint64_t command, bool is_common)
{
    for (int i = 0;; i++) {
        const struct command_handler *h = &command_handlers[i];
        if (!h->handler) return NULL;
        if (h->command == command) {
            if (is_common && !h->allow_common) return NULL;
            return h;
        }
    }
    return NULL;
}

static void run_command_step(suit_runner_t *runner, bool *pop_pc)
{
    suit_continuation_t *pc = &runner->program_counter;
    nocbor_context_t *ctx;
    bool is_common;
    if (!nocbor_is_end_of_context(pc->command.common)) {
        ctx = &pc->command.common;
        is_common = true;
    } else if (!nocbor_is_end_of_context(pc->command.target)) {
        ctx = &pc->command.target;
        is_common = false;
    } else {
        *pop_pc = true;
        printf("end of command seq\n");
        return;
    }

    uint64_t command;

    if (!nocbor_read_uint(ctx, &command)) goto err;
    printf("command %lu\n", command);

    const struct command_handler *h = find_handler(command, is_common);
    if (!h) {
        printf("unknown command %lu\n", command);
        goto err;
    }
    if (!h->handler(runner, command, ctx)) goto err;

    return;
err:
    suit_runner_mark_error(runner);
}

static void run_try_step(suit_runner_t *runner, bool *pop_pc)
{
    // TODO
}

static void run_call_step(suit_runner_t *runner, bool *pop_pc)
{
    suit_continuation_t *pc = &runner->program_counter;
    suit_envelope_t *ep = runner->program_counter.envelope;
    int i;
    for (i = 0; i < runner->n_dependency; i++) {
        suit_dependency_binder_t *p = &runner->dependency_map[i];
        if (p->source == ep) {
            uint64_t bit = (uint64_t)1 << p->dependency_index;
            if (pc->dependency.selected & bit) {
                pc->dependency.selected &= ~bit;
                call_dependency(runner, p->target);
                return;
            }
        }
    }
    *pop_pc = true;
}

void suit_runner_run(suit_runner_t *runner)
{
    if (runner->has_error) return;

    for (;;) {
        if (runner->suspended) return;

        suit_continuation_t *pc = &runner->program_counter;

        bool pop_pc = false;
        if (pc->next == SUIT_NEXT_EXIT) {
            break;
        } else if (pc->next == SUIT_NEXT_EXECUTE_COMMAND) {
            run_command_step(runner, &pop_pc);
        } else if (pc->next == SUIT_NEXT_EXECUTE_TRY_ENTRY) {
            run_try_step(runner, &pop_pc);
        } else if (pc->next == SUIT_NEXT_EXECUTE_DEPENDENCY) {
            run_call_step(runner, &pop_pc);
        } else {
            goto err;
        }
        if (runner->has_error) {
            // TODO: unwind stack
            break;
        }
        if (pop_pc) {
            if (!pop_program_counter(runner)) goto err;
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
