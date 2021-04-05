#pragma once
#include "qcbor_ext.h"

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

    SUIT_WAIT_EVENT_AUTHORIZATION = 1,
    SUIT_WAIT_EVENT_POWER = 2,
    SUIT_WAIT_EVENT_NETWORK = 3,
    SUIT_WAIT_EVENT_OTHER_DEVICE_VERSION = 4,
    SUIT_WAIT_EVENT_TIME = 5,
    SUIT_WAIT_EVENT_TIME_OF_DAY = 6,
    SUIT_WAIT_EVENT_DAY_OF_WEEK = 7,
};

enum suit_parameter_key
{
    suit_parameter_vendor_identifier = 1,
    suit_parameter_class_identifier  = 2,
    suit_parameter_image_digest      = 3,
    suit_parameter_use_before        = 4,
    suit_parameter_component_offset  = 5,

    suit_parameter_strict_order      = 12,
    suit_parameter_soft_failure      = 13,
    suit_parameter_image_size        = 14,

    suit_parameter_encryption_info   = 18,
    suit_parameter_compression_info  = 19,
    suit_parameter_unpack_info       = 20,
    suit_parameter_uri               = 21,
    suit_parameter_source_component  = 22,
    suit_parameter_run_args          = 23,

    suit_parameter_device_identifier = 24,
    suit_parameter_minimum_battery   = 26,
    suit_parameter_update_priority   = 27,
    suit_parameter_version           = 28,
    suit_parameter_wait_info         = 29,
    suit_parameter_uri_list          = 30,
};

struct suit_parameters
{
    // bit map of enum suit_parameter_key
    // set   => parameter has value
    // unset => parameter value is not set yet
    uint64_t keys;

	UsefulBufC image_digest;
	uint64_t image_size;
	UsefulBufC uri;
};

enum suit_step
{
    suit_manifest_version = 1,
    suit_manifest_sequence_number = 2,
    suit_common = 3,
    suit_reference_uri = 4,
    suit_dependency_resolution = 7,
    suit_payload_fetch = 8,,
    suit_install = 9,
    suit_validate = 10,
    suit_load = 11,
    suit_run = 12,
    suit_text = 13,
    suit_coswid = 14,
};

#define SUIT_ENVELOPE_MAX 8
#define SUIT_COMPONENT_MAX 32
#define SUIT_DEPENDENCY_MAX 32

struct suit_digest
{
    uint64_t algorithm_id;
    UsefulBufC bytes;
};

struct suit_severed
{
    bool has_value;
    bool severed;
    union {
        UsefulBufC digest;
        UsefulBufC body;
    };
};

struct suit_manifest
{
	UsefulBufC binary;

    uint64_t version;
    uint64_t sequence_number;
    UsefulBufC common;
    UsefulBufC reference_uri;
    struct suit_severed dependency_resolution;
    struct suit_severed payload_fetch;
    struct suit_severed install;
    struct suit_severed text;
    struct suit_severed coswid;
    UsefulBufC validate;
    UsefulBufC load;
    UsefulBufC run;
};

struct suit_envelope
{
    UsefulBufC binary;

    UsefulBufC delegation;
    UsefulBufC authentication_wrapper;
    struct suit_manifest manifest;

    bool used;
};

struct suit_component
{
    UsefulBufC id;
	struct suit_parameters parameters;
};

struct suit_dependency
{
    UsefulBufC digest;
    UsefulBufC component_id;
	struct suit_parameters parameters; // ???
};



struct suit_processor
{
    struct suit_envelope envelope_buf[SUIT_ENVELOPE_MAX];
    struct suit_component component_buf[SUIT_COMPONENT_MAX];
    struct suit_dependency dependency_buf[SUIT_DEPENDENCY_MAX];

    struct suit_envelope *root_envelope;
};

typedef enum suit_result
{
	SUIT_RESULT_DONE,
	SUIT_RESULT_PLATFORM_REQUEST,
	SUIT_RESULT_ERROR
} suit_result_t;

struct suit_platform_request
{

};

suit_result_t suit_processor_init(struct suit_processor *p, UsefulBufC root_envelope);

enum suit_result suit_processor_run(struct suit_processor *p);
void suit_processor_get_request(const struct suit_processor *p, struct suit_platform_request *ret);
