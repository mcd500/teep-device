#include <stddef.h>
#include <stdarg.h>
#include <stdio.h> // vsnprintf, BUFSIZ
#include <stdlib.h>
#include <string.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#ifdef PLAT_OPTEE
#include <time.h>
#include <libwebsockets.h>
#endif

/**
 * _strlen() - Computes the length of the string.
 *
 * This function used for_loop to parse the string
 * and return length of the string as an unsigned interger format.
 * 
 * @param str    string whose length is to be found. 
 */ 
static inline unsigned int _strlen(const char* str)
{
  const char* s;
  for (s = str; *s; ++s);
  return (unsigned int)(s - str);
}

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
char *strcpy(char *dst, const char *src)
{
  char *d = dst;
  const char *s = src;
  while ((*d++ = *s++))
    ;
  return dst;
}

/**
 * strncpy() - Copies up to n characters from the string pointed from source to 
 * destination.
 * 
 * This fucntion begins with a loop 
 * and assigns destination dst value to source src value. It runs an 
 *  another loop that returns the destination dest value.
 * 
 * @param dst	A pointer to the destination array
 * @param src	The string to be copied.
 * @param n	The number of characters to be copied from source.
 *
 * @return	It returns the final copy of the copied string.
 */
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

char *strncat(char *dest, const char *src, size_t n)
{
  size_t dest_len = strlen(dest);
  size_t i;

  for (i = 0 ; i < n && src[i] != '\0' ; i++)
    dest[dest_len + i] = src[i];
  dest[dest_len + i] = '\0';

  return dest;
}

char *strdup(const char *s)
{
  size_t len = strlen(s);
  char *p = malloc(len + 1);
  if (p) {
    memcpy(p, s, len + 1);
  }
  return p;
}

/**
 * strchr() - Searches for the first occurrence of the character c 
 * (an unsigned char) in the string pointed to by the argument str. 
 * And it just returns with an another function called  __builtin_strchr(s, c).
 * 
 * @param s	The string to be scanned.
 * @param c	The character to be searched in str format.
 */
char *strchr(const char *s, int c)
{
  return __builtin_strchr(s, c);
}

char *strrchr(const char *s, int c)
{
  const char *p = NULL;
  while (*s) {
    if (*s == c) {
      p = s;
    }
    s++;
  }
  return (char *)p;
}

#ifdef KEYSTONE
/**
 * puts() - Writes a string to stdout, excluding the null 
 * character. A newline character is appended to the output.
 *
 * This function has the ifdef  directive to check  whether the identifier 
 * is currently defined Compiler may replace simple printf to puts declaring 
 * variable and calls the ocall_print_wapper function and storing 
 * that value into sz.
 * 
 * @param s	The string to be written
 * 
 * @return	Its return the sz with type size_t if success else return 0.
 */
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

/**
 * putchar() - It is used to write a character of an unsigned char type, to 
 * stdout. This character is passed as the parameter to this method.
 * 
 * This function is having the ifdef directive that checks whether the 
 * identifier is currently defined or not. Compiler may replace simple printf to 
 * putchar and invokes the ocall_print_wapper function and storing that value into sz 
 *
 * @param c	The value is internally converted to an unsigned char when written.
 *
 * @return     Its return the sz with type size_t if success else return 0
 */
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

/**
 * printf() - It sends the formatted output to stdout. 
 *  
 * This function is a system call function. It has
 * #ifdef(ENCLAVE_VERBOSE) directive  that allows for conditional compilations. The 
 * preprocessor determines if the provided macro exists or not before including the 
 * subsequent code in the compilation process, along with variable argument 
 * and list attributes like va_start function and va_end are declared.
 *
 * @param fmt	A string that contains the text to be written to stdout.
 *
 * @return	It returns the typecasted int of strlen(buf) else return 0.
 */
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
#endif

int atoi(const char *s)
{
  int n = 0;
  while ('0' <= *s && *s <= '9') {
    n = n * 10 + (*s - '0');
    s++;
  }
  return n;
}

time_t time(time_t *tloc)
{
	TEE_Time t;

	TEE_GetREETime(&t);

	if (tloc)
		*tloc = t.seconds;

	return t.seconds;
}


int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	TEE_Time t;

	(void)tz;

	TEE_GetREETime(&t);

	tv->tv_sec = t.seconds;
	tv->tv_usec = t.millis * 1000;

	return 0;
}
