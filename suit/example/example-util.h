#pragma once
#include <cinttypes>
#include <vector>
#include "nocbor.h"
#include "teesuit.h"

bool read_file(std::vector<std::uint8_t>& dst, const char *file);

void hexdump(nocbor_range_t r);

void print_tstr(nocbor_range_t r);
void print_bstr(nocbor_range_t r);

void print_severable(suit_severable_t s);
void print_digest(suit_digest_t d);
void print_auth_wrapper(suit_authentication_wrapper w);

void print_envelope_field(nocbor_range_t envelope_bstr, enum suit_cbor_label key);

void pirnt_object(const suit_object_t *target);
