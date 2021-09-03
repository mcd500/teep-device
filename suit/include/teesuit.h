#pragma once
#include "nocbor.h"

#include "teesuit-constant.h"
#include "teesuit-internal.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SUIT AST types
 */

typedef struct suit_digest
{
    uint64_t algorithm_id;
    nocbor_range_t bytes;
} suit_digest_t;

typedef struct suit_severable
{
    bool has_value;
    bool severed;
    union {
        suit_digest_t digest; // severed
        nocbor_range_t body;  // not severed
    };
} suit_severable_t;

#define SUIT_COMONENT_ID_LEN 8

typedef struct suit_component
{
    size_t id_len;
    nocbor_range_t id[SUIT_COMONENT_ID_LEN];
} suit_component_t;

typedef struct suit_dependency
{
    suit_digest_t digest;
    size_t prefix_len;
    nocbor_range_t prefix[SUIT_COMONENT_ID_LEN];
} suit_dependency_t;

#define SUIT_COMPONENT_MAX 8   // max # of component in single manifest
#define SUIT_DEPENDENCY_MAX 8  // max # of dependencies in single manifest

typedef struct suit_common
{
    size_t n_dependencies;
    suit_dependency_t dependencies[SUIT_DEPENDENCY_MAX];
    size_t n_components;
    suit_component_t components[SUIT_COMPONENT_MAX];
    nocbor_range_t command_sequence;
} suit_common_t;

typedef struct suit_manifest
{
    nocbor_range_t envelope_bstr;
	nocbor_range_t binary;

    uint64_t version;
    uint64_t sequence_number;
    nocbor_range_t common_bstr;
    suit_common_t common;
    nocbor_range_t reference_uri;
    suit_severable_t dependency_resolution;
    suit_severable_t payload_fetch;
    suit_severable_t install;
    suit_severable_t text;
    suit_severable_t coswid;
    nocbor_range_t validate;
    nocbor_range_t load;
    nocbor_range_t run;
} suit_manifest_t;

bool suit_manifest_parse(nocbor_range_t manifest_bstr, suit_manifest_t *ret);

typedef struct suit_authentication_wrapper
{
    suit_digest_t digest;
} suit_authentication_wrapper_t;

bool suit_parse_authentication_wrapper(nocbor_range_t authentication_wrapper_bstr, suit_authentication_wrapper_t *ret);

typedef struct suit_envelope
{
    nocbor_range_t binary;
    nocbor_range_t delegation;
    nocbor_range_t authentication_wrapper;
    nocbor_range_t manifest;
    nocbor_range_t dependency_resolution;
    nocbor_range_t payload_fetch;
    nocbor_range_t install;
    nocbor_range_t text;
    nocbor_range_t coswid;
} suit_envelope_t;

bool suit_envelope_get_field_by_key(nocbor_range_t envelope_bstr, enum suit_cbor_label key, nocbor_range_t *ret);
bool suit_envelope_get_field_by_name(nocbor_range_t envelope_bstr, nocbor_range_t key_tstr, nocbor_range_t *ret);

//bool suit_parse_envelope(nocbor_range_t envelope_bstr, suit_envelope_t *ret);

bool suit_envelope_get_manifest(nocbor_range_t envelope_bstr, suit_manifest_t *ret);

//bool suit_envelope_get_payload(const suit_envelope_t *envelope, nocbor_range_t key_tstr, nocbor_range_t *ret);
bool suit_envelope_get_severed(const suit_severable_t *severable, nocbor_range_t severed, nocbor_range_t *ret);

typedef struct suit_command_reader
{
    nocbor_context_t ctx;
} suit_command_reader_t;

bool suit_command_sequence_open(nocbor_range_t command_sequence_bstr, suit_command_reader_t *ret);
bool suit_command_sequence_read_command(suit_command_reader_t *r, enum suit_command *ret);

bool suit_command_sequence_read_reppolicy(suit_command_reader_t *r /* TODO ret */);
bool suit_command_sequence_skip_param(suit_command_reader_t *r);

bool suit_command_sequence_end_of_sequence(suit_command_reader_t r);

/*
 * SUIT global context
 *
 */
#define SUIT_MANIFEST_MAX 8

typedef struct suit_context
{
    //int n_envelope;
    //struct suit_envelope envelope_buf[SUIT_ENVELOPE_MAX];
    int n_manifest;
    suit_manifest_t manifests[SUIT_MANIFEST_MAX];
} suit_context_t;

void suit_context_init(suit_context_t *p);
bool suit_context_add_envelope(suit_context_t *p, nocbor_range_t envelope_bstr);
suit_manifest_t *suit_context_get_root_manifest(suit_context_t *p);

/*
 * SUIT command runner context
 *
 */
typedef struct suit_runner suit_runner_t;

/* call back functions user supplies */
typedef struct suit_callbacks
{
    bool (*check_vendor_id)(suit_runner_t *runner, void *user);
    bool (*fetch)(suit_runner_t *runner, void *user);
} suit_callbacks_t;


#define SUIT_BINDER_MAX 32
#define SUIT_STACK_MAX 8

struct suit_runner
{
    suit_context_t *context;
    const suit_callbacks_t *callbacks;
    void *user;
    enum suit_command_sequence sequence;

    int n_component;
    suit_component_t component_buf[SUIT_COMPONENT_MAX];
    int n_dependency;
    suit_dependency_binder_t dependency_map[SUIT_DEPENDENCY_MAX];

    uint64_t selected_component_bits;
    uint64_t selected_dependency_bits;

    // TODO: use component indexed structure
    int n_binder;
    suit_binder_t bindings[SUIT_BINDER_MAX];

    suit_continuation_t program_counter;
    int n_stack;
    suit_continuation_t stack[SUIT_STACK_MAX];

    // TODO: more error detail
    bool has_error;

    bool suspended;
    void (*on_resume)(suit_runner_t *runner, void *user);
};

void suit_runner_init(suit_runner_t *runner, suit_context_t *context, enum suit_command_sequence sequence, const suit_callbacks_t *callbacks, void *user);

// run some SUIT commands in suit runner
// return when all command has executed successfully,
//   `suit_runner_suspend()` is called or error
void suit_runner_run(suit_runner_t *r);

// check why `suit_runner_run()` retruned
bool suit_runner_finished(suit_runner_t *r);
bool suit_runner_has_error(suit_runner_t *r);
bool suit_runner_suspended(suit_runner_t *r);


void suit_runner_mark_error(suit_runner_t *r);
bool suit_runner_get_error(const suit_runner_t *r, void *error_enum_todo);

void suit_runner_suspend(suit_runner_t *r, void (*on_resume)(suit_runner_t *, void *));
void suit_runner_resume(suit_runner_t *r, void *user);

bool suit_runner_get_parameter(suit_runner_t *r, uint64_t key, nocbor_any_t *any);

#ifdef __cplusplus
}
#endif
