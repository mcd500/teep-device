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

#include <string.h>

#if defined(PLAT_KEYSTONE) || defined(PLAT_SGX)
/**
 * strcpy() - Copies the string from source to destination.
 *
 * This function iniializes the destination "dst" variable to copy the string
 * using the while_loop.
 * 
 * @param dst	A pointer to the destination array where the content is to be 
 *		copied.
 * @param src	The string to be copied.
 *
 * @return	It returns a pointer to the destination string dest.
 */
char *strncpy(char *dst, const char *src, size_t n)
{
  char *d = dst;
  const char *s = src;
  size_t i = 0;
  while (i++ != n && (*d++ = *s++))
    ;
  *(dst + n) = '\0';
  return dst;
}
#endif

#if defined(PLAT_KEYSTONE)
char *strstr(const char *x, const char *y)
{
  if (*y == 0) return (char *)x;
  for (; *x; x++) {
    const char *p = x;
    const char *q = y;
    for (; *p && *q; p++, q++) {
      if (*p != *q) break;
    }
    if (*q == 0) return (char *)x;
    if (*p == 0) return NULL;
  }
  return NULL;
}
#endif

