#pragma once
#include "nocbor.h"

enum suit_cbor_label
{
    SUIT_DELEGATION = 1,
    SUIT_AUTHENTICATION_WRAPPER = 2,
    SUIT_MANIFEST = 3,

    SUIT_MANIFEST_VERSION = 1,
    SUIT_MANIFEST_SEQUENCE_NUMBER = 2,
    SUIT_COMMON = 3,
    SUIT_REFERENCE_URI = 4,
    SUIT_DEPENDENCY_RESOLUTION = 7,
    SUIT_PAYLOAD_FETCH = 8,
    SUIT_INSTALL = 9,
    SUIT_VALIDATE = 10,
    SUIT_LOAD = 11,
    SUIT_RUN = 12,
    SUIT_TEXT = 13,
    SUIT_COSWID = 14,

    SUIT_DEPENDENCIES = 1,
    SUIT_COMPONENTS = 2,
    SUIT_COMMON_SEQUENCE = 4,

    SUIT_DEPENDENCY_DIGEST = 1,
    SUIT_DEPENDENCY_PREFIX = 2,
};

enum suit_command
{
    SUIT_CONDITION_VENDOR_IDENTIFIER = 1,
    SUIT_CONDITION_CLASS_IDENTIFIER  = 2,
    SUIT_CONDITION_IMAGE_MATCH       = 3,
    SUIT_CONDITION_USE_BEFORE        = 4,
    SUIT_CONDITION_COMPONENT_OFFSET  = 5,

    SUIT_CONDITION_ABORT                    = 14,
    SUIT_CONDITION_DEVICE_IDENTIFIER        = 24,
    SUIT_CONDITION_IMAGE_NOT_MATCH          = 25,
    SUIT_CONDITION_MINIMUM_BATTERY          = 26,
    SUIT_CONDITION_UPDATE_AUTHORIZED        = 27,
    SUIT_CONDITION_VERSION                  = 28,

    SUIT_DIRECTIVE_SET_COMPONENT_INDEX      = 12,
    SUIT_DIRECTIVE_SET_DEPENDENCY_INDEX     = 13,
    SUIT_DIRECTIVE_TRY_EACH                 = 15,
    SUIT_DIRECTIVE_PROCESS_DEPENDENCY       = 18,
    SUIT_DIRECTIVE_SET_PARAMETERS           = 19,
    SUIT_DIRECTIVE_OVERRIDE_PARAMETERS      = 20,
    SUIT_DIRECTIVE_FETCH                    = 21,
    SUIT_DIRECTIVE_COPY                     = 22,
    SUIT_DIRECTIVE_RUN                      = 23,

    SUIT_DIRECTIVE_WAIT                     = 29,
    SUIT_DIRECTIVE_FETCH_URI_LIST           = 30,
    SUIT_DIRECTIVE_SWAP                     = 31,
    SUIT_DIRECTIVE_RUN_SEQUENCE             = 32,
    SUIT_DIRECTIVE_GARBAGE_COLLECT          = 33,

    // TODO: not here
    SUIT_WAIT_EVENT_AUTHORIZATION = 1,
    SUIT_WAIT_EVENT_POWER = 2,
    SUIT_WAIT_EVENT_NETWORK = 3,
    SUIT_WAIT_EVENT_OTHER_DEVICE_VERSION = 4,
    SUIT_WAIT_EVENT_TIME = 5,
    SUIT_WAIT_EVENT_TIME_OF_DAY = 6,
    SUIT_WAIT_EVENT_DAY_OF_WEEK = 7,
};

typedef enum suit_parameter_key
{
    SUIT_PARAMETER_VENDOR_IDENTIFIER = 1,
    SUIT_PARAMETER_CLASS_IDENTIFIER  = 2,
    SUIT_PARAMETER_IMAGE_DIGEST      = 3,
    SUIT_PARAMETER_USE_BEFORE        = 4,
    SUIT_PARAMETER_COMPONENT_OFFSET  = 5,

    SUIT_PARAMETER_STRICT_ORDER      = 12,
    SUIT_PARAMETER_SOFT_FAILURE      = 13,
    SUIT_PARAMETER_IMAGE_SIZE        = 14,

    SUIT_PARAMETER_ENCRYPTION_INFO   = 18,
    SUIT_PARAMETER_COMPRESSION_INFO  = 19,
    SUIT_PARAMETER_UNPACK_INFO       = 20,
    SUIT_PARAMETER_URI               = 21,
    SUIT_PARAMETER_SOURCE_COMPONENT  = 22,
    SUIT_PARAMETER_RUN_ARGS          = 23,

    SUIT_PARAMETER_DEVICE_IDENTIFIER = 24,
    SUIT_PARAMETER_MINIMUM_BATTERY   = 26,
    SUIT_PARAMETER_UPDATE_PRIORITY   = 27,
    SUIT_PARAMETER_VERSION           = 28,
    SUIT_PARAMETER_WAIT_INFO         = 29,
    SUIT_PARAMETER_URI_LIST          = 30,
} suit_parameter_key_t;

typedef enum suit_parameter_type
{
    SUIT_PARAMETER_TYPE_RFC4122_UUID,
    SUIT_PARAMETER_TYPE_CBOR_PEN,
    SUIT_PARAMETER_TYPE_SUIT_DIGEST,
    SUIT_PARAMETER_TYPE_UINT,
    SUIT_PARAMETER_TYPE_ENCRYPTION_INFO,
    SUIT_PARAMETER_TYPE_COMPRESSION_INFO,
    SUIT_PARAMETER_TYPE_UNPACK_INFO,
    SUIT_PARAMETER_TYPE_TSTR,
    SUIT_PARAMETER_TYPE_BSTR,
    SUIT_PARAMETER_TYPE_VERSION_MATCH,
    SUIT_PARAMETER_TYPE_BOOL,
} suit_parameter_type_t;

typedef struct suit_component suit_component_t;

typedef struct suit_binder
{
    suit_component_t *component;
    suit_parameter_key_t key;
    nocbor_any_t value;
} suit_binder_t;

typedef struct suit_digest
{
    uint64_t algorithm_id;
    nocbor_range_t bytes;
} suit_digest_t;

typedef struct suit_severed
{
    bool has_value;
    bool severed;
    union {
        suit_digest_t digest; // severed
        nocbor_range_t body;  // not severed
    };
} suit_severed_t;

struct suit_manifest
{
	nocbor_range_t binary;

    uint64_t version;
    uint64_t sequence_number;
    nocbor_range_t common;
    nocbor_range_t reference_uri;
    suit_severed_t dependency_resolution;
    suit_severed_t payload_fetch;
    suit_severed_t install;
    suit_severed_t text;
    suit_severed_t coswid;
    nocbor_range_t validate;
    nocbor_range_t load;
    nocbor_range_t run;
};

typedef struct suit_envelope
{
    nocbor_range_t binary;

    nocbor_range_t delegation;
    nocbor_range_t authentication_wrapper;
    struct suit_manifest manifest;
} suit_envelope_t;

typedef struct suit_component
{
    nocbor_range_t id;
} suit_component_t;

typedef struct suit_dependency
{
    nocbor_range_t digest;
    nocbor_range_t component_id;
} suit_dependency_t;

#define SUIT_ENVELOPE_MAX 8
#define SUIT_COMPONENT_MAX 32
#define SUIT_DEPENDENCY_MAX 32
#define SUIT_BINDER_MAX 32
#define SUIT_STACK_MAX 8

typedef struct suit_processor
{
    int n_envelope;
    struct suit_envelope envelope_buf[SUIT_ENVELOPE_MAX];
} suit_processor_t;

typedef struct suit_platform suit_platform_t;

typedef struct suit_continuation
{
    bool is_try_each;
    bool is_common;
    nocbor_context_t context;
} suit_continuation_t;

typedef struct suit_runner suit_runner_t;

struct suit_runner
{
    suit_processor_t *processor;
    suit_platform_t *platform;

    int n_component;
    suit_component_t component_buf[SUIT_COMPONENT_MAX];
    int n_dependency;
    suit_dependency_t dependency_buf[SUIT_DEPENDENCY_MAX];

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

typedef struct suit_callbacks
{
    bool (*check_vendor_id)(suit_runner_t *runner);
    bool (*fetch)(suit_runner_t *runner);
} suit_callbacks_t;

struct suit_platform
{
    void *user;
    suit_callbacks_t *callbacks;
};

void suit_processor_init(suit_processor_t *p);
bool suit_processor_load_root_envelope(suit_processor_t *p, nocbor_range_t envelope);

// prepare runner for executing SUIT command sequence.
void suit_runner_init(suit_runner_t *r, suit_processor_t *p, suit_platform_t *platform, nocbor_range_t commands);

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

bool suit_runner_get_parameter(suit_runner_t *r, uint64_t key, nocbor_any_t *any);
