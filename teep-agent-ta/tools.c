#include <string.h>

#ifdef PLAT_KEYSTONE
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
#endif
