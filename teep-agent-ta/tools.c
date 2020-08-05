#include <stddef.h>
#include <stdarg.h>
#include <stdio.h> // vsnprintf, BUFSIZ
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

static inline unsigned int _strlen(const char* str)
{
  const char* s;
  for (s = str; *s; ++s);
  return (unsigned int)(s - str);
}

char *strcpy(char *dst, const char *src)
{
  char *d = dst;
  const char *s = src;
  while ((*d++ = *s++))
    ;
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n)
{
  size_t i;
  for (i = 0; i < n && src[i]; i++) {
    dst[i] = src[i];
  }
  for (; i < n; i++) {
    dst[i] = 0;
  }
  return dst;
}

char *strchr(const char *s, int c)
{
  return __builtin_strchr(s, c);
}

// Compiler may replace simple printf to puts and putchar
int puts(const char *s)
{
#ifdef ENCLAVE_VERBOSE
  size_t sz = ocall_print_string_wrapper(s);
  putchar('\n');
  return sz;
#else
  return 0;
#endif
}

int putchar(int c)
{
#ifdef ENCLAVE_VERBOSE
  char buf[2];
  buf[0] = (char)c; buf[1] = '\0';
  size_t sz = ocall_print_string_wrapper(buf);
  return sz;
#else
  return 0;
#endif
}

int printf(const char* fmt, ...)
{
#ifdef ENCLAVE_VERBOSE
  char buf[BUFSIZ] = { '\0' };
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(buf, BUFSIZ, fmt, ap);
  va_end(ap);
  ocall_print_string_wrapper(buf);

  return (int)_strlen(buf) + 1;
#else
  return 0;
#endif
}
