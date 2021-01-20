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
@brief The _strlen() computes the length of the string str up to, but not including the terminating null character.

@param[in]    str string whose length is to be found.

The inline function strnlen is declares variable for loop till the condition attain and 
    while in the return they typecasting the value into unsigned int format. 
*/ 
static inline unsigned int _strlen(const char* str)
{
  const char* s;
  for (s = str; *s; ++s);
  return (unsigned int)(s - str);
}

/**
@brief The strcpy function is to copies the string pointed to, by src to dest.

@param[in] dst pointer to the destination array where the content is to be copied.
@param[in] src the string to be copied.

In the strcpy function firstly declares the variable for string copy and the while will do like, The d and s variables are pointers, the contents of d (*d) are copied to s (*s), one character.
d and s are both incremented (++). The assignment (copy) returns the character that was copied (to the while). The while continues until that character is zero (end of string in C).

@return It returns a pointer to the destination string dest.
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
@brief The strncpy function copies up to n characters from the string pointed to, by src to dest, In a case where the length of src is less than that of n, the remainder of dest will be padded with null bytes.

@param[in] dst pointer to the destination array
@param[in] src the string to be copied.
@param[in] n number of characters to be copied from source.

In strncpy fucntion firstly declaring the variable with size_t data type and two loops will run first loop is to run until the condition attain and assigen dest value into src value
second loop will run until variable the number of characters present in the source and returns the dest value.

@return It returns the final copy of the copied string.
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
@brief The strchr function searches for the first occurrence of the character c (an unsigned char) in the string pointed to by the argument str. And it just
return with another function  __builtin_strchr(s, c).

@param[in] *s string to be scanned.
@param[in]  c character to be searched in str.
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
@brief The puts function writes a string to stdout up to but not including the null character. A newline character is appended to the output.

@param[in] *s string to be written

The puts function having the ifdef  directive checks whether the identifier is currently defined Compiler may replace simple printf to puts declaring variable and calls the
ocall_print_wapper function and storing that value into sz 

@return Its return the sz with type size_t if success else return 0.
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
@brief The putchar function  is used to write a character, of unsigned char type, to stdout. This character is passed as the parameter to this method.

@param[in] c The value is internally converted to an unsigned char when written.

The putchar function is  having the ifdef  directive checks whether the identifier is currently defined Compiler may replace simple printf to putchar as well after that declaring variable and calls the
ocall_print_wapper function and storing that value into sz 

@return Its return the sz with type size_t if success else return 0
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
@brief The printf fucntion is sends formatted output to stdout. 

@param[in] fmt string that contains the text to be written to stdout.

The printf function is a system call function in with they included #ifdef(ENCLAVE_VERBOSE) directive allows for conditional compilation. The preprocessor determines if the provided macro exists before including the subsequent code in the compilation process.
along with variable argument list attributes are declared va_start function and va_end function in between they called vsnprinf function The va_start and end is nothing but allows a function with variable arguments which used the va_start macro to return. If va_end is not called before returning from the function, the result is undefined.

@return It return typecasted int of strlen(buf) else return 0.
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
