#pragma once
#include "nocbor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct suit_component suit_component_t;
typedef struct suit_envelope suit_envelope_t;
typedef struct suit_manifest suit_manifest_t;

typedef struct suit_dependency_binder
{
    suit_envelope_t *source;
    int dependency_index;
    suit_envelope_t *target;
} suit_dependency_binder_t;

enum suit_next_action
{
    SUIT_NEXT_EXECUTE_SEQUENCE,
    SUIT_NEXT_EXECUTE_COMMAND,
    SUIT_NEXT_EXECUTE_TRY_ENTRY,
    SUIT_NEXT_EXIT
};

typedef struct suit_continuation
{
    suit_manifest_t *manifest;
    enum suit_next_action next;
    union {
        struct {
            nocbor_context_t common;
            nocbor_context_t target;
        } sequence;
        struct {
            uint64_t command;
            nocbor_range_t param;
            bool is_component; // component or dependency
            bool selected_all;
            union {
                uint64_t index;  // selected all
                nocbor_context_t array; // selected array
            };
        } command;
        struct {
            nocbor_context_t try_entries;
        } try_entry;
        struct {
        } exit;
    };
} suit_continuation_t;

#ifdef __cplusplus
}
#endif
