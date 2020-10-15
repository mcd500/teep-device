/*
 * Copyright (C) 2017 - 2019 National Institute of Advanced Industrial Science
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

#ifndef TEEP_MESSAGE_H
#define TEEP_MESSAGE_H

//int
//teep_message_wrap(const char *msg, int msg_len, char *resp, uint32_t *resp_len);

int
otrp_message_sign(const char *msg, int msg_len, char *resp, uint32_t *resp_len);

int
otrp_message_encrypt(const char *msg, int msg_len, char *resp, uint32_t *resp_len);

//int
//teep_message_unwrap(const char *msg, int msg_len, char *resp, uint32_t *resp_len);

int
otrp_message_verify(const char *msg, int msg_len, char *resp, uint32_t *resp_len);

int
otrp_message_decrypt(const char *msg, int msg_len, char *resp, uint32_t *resp_len);

int
teep_message_unwrap_ta_image(const char *msg, int msg_len, char *out, uint32_t *out_len);

int
teep_agent_message(int jose, const char *msg, int msg_len, char *out, uint32_t *out_len, char *ta_url_list, uint32_t *ta_url_list_len);

int
teep_agent_set_ta_list(const char *ta_list, int ta_list_len);

#endif /* TEEP_MESSAGE_H */
