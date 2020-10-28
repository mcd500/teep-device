#pragma once

int http_get(const char *uri, void *out, size_t *out_len);
int http_post(const char *uri, const void *in, size_t in_len, void *out, size_t *out_len);
