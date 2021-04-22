#pragma once
#include <cinttypes>
#include <vector>
#include "nocbor.h"
#include "teesuit.h"

bool read_file(std::vector<std::uint8_t>& dst, const char *file);

void hexdump(nocbor_range_t r);
void print_severed(suit_severed_t s);
void print_digest(suit_digest_t d);
void print_auth_wrapper(suit_authentication_wrapper w);

