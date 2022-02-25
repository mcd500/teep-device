/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (C) 2019 National Institute of Advanced Industrial Science
 *                           and Technology (AIST)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

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

enum suit_command_sequence
{
    SUIT_COMMAND_SEQUENCE_DEPENDENCY_RESOLUTION,
    SUIT_COMMAND_SEQUENCE_PAYLOAD_FETCH,
    SUIT_COMMAND_SEQUENCE_INSTALL,
    SUIT_COMMAND_SEQUENCE_VALIDATE,
    SUIT_COMMAND_SEQUENCE_LOAD,
    SUIT_COMMAND_SEQUENCE_RUN,
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

#ifdef __cplusplus
}
#endif
