///////////////////////////////////////////////////////////////////////////////
// \author (c) Marco Paland (info@paland.com)
//             2014-2018, PALANDesign Hannover, Germany
//
// \license The MIT License (MIT)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// \brief Tiny printf, sprintf and (v)snprintf implementation, optimized for speed on
//        embedded systems with a very limited resources. These routines are thread
//        safe and reentrant!
//        Use this instead of the bloated standard/newlib printf cause these use
//        malloc for printf (and may not be thread safe).
//
///////////////////////////////////////////////////////////////////////////////

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

// ntoa conversion buffer size, this must be big enough to hold
// one converted numeric number including padded zeros (dynamically created on stack)
// 32 byte is a good default
#define PRINTF_NTOA_BUFFER_SIZE    32U

// ftoa conversion buffer size, this must be big enough to hold
// one converted float number including padded zeros (dynamically created on stack)
// 32 byte is a good default
#define PRINTF_FTOA_BUFFER_SIZE    32U

// define this to support floating point (%f)
#define PRINTF_SUPPORT_FLOAT

// define this to support long long types (%llu or %p)
#define PRINTF_SUPPORT_LONG_LONG

// define this to support the ptrdiff_t type (%t)
// ptrdiff_t is normally defined in <stddef.h> as long or long long type
#define PRINTF_SUPPORT_PTRDIFF_T


///////////////////////////////////////////////////////////////////////////////

// internal flag definitions
#define FLAGS_ZEROPAD   (1U <<  0U)
#define FLAGS_LEFT      (1U <<  1U)
#define FLAGS_PLUS      (1U <<  2U)
#define FLAGS_SPACE     (1U <<  3U)
#define FLAGS_HASH      (1U <<  4U)
#define FLAGS_UPPERCASE (1U <<  5U)
#define FLAGS_CHAR      (1U <<  6U)
#define FLAGS_SHORT     (1U <<  7U)
#define FLAGS_LONG      (1U <<  8U)
#define FLAGS_LONG_LONG (1U <<  9U)
#define FLAGS_PRECISION (1U << 10U)

extern int putchar(char ch);
#define _putchar putchar

// output function type
typedef void (*out_fct_type)(char character, void* buffer, size_t idx, size_t maxlen);


// wrapper (used as buffer) for output function type
typedef struct {
  void  (*fct)(char character, void* arg);
  void* arg;
} out_fct_wrap_type;

/**
@brief This _out_buffer function called as internal buffer output

@param[in] character it is a char data type
@param[in] buffer string size of buffer
@param[in] idx bytes of size_t
@param[in] maxlen maximum lenghth of bytes in size_t
*/
static inline void _out_buffer(char character, void* buffer, size_t idx, size_t maxlen)
{
  if (idx < maxlen) {
    ((char*)buffer)[idx] = character;
  }
}

/**
@brief This _out_null function called as internal null output.

@param[in] character it is a char data type
@param[in] buffer string size of buffer
@param[in] idx bytes of size_t
@param[in] maxlen maximum lenghth of bytes in size_t

The typecasting for variable with void is to avoid unused variable warnings in some compilers.
*/
static inline void _out_null(char character, void* buffer, size_t idx, size_t maxlen)
{
  (void)character; (void)buffer; (void)idx; (void)maxlen;
}

/**
@brief The _out_char function is a internal putchar wrapper

@param[in] character it is a char data type
@param[in] buffer string size of buffer
@param[in] idx bytes of size_t
@param[in] maxlen maximum lenghth of bytes in size_t

It is a internal putchar function wrapper The typecasting for variable with void is to 
avoid unused variable warnings in some compilers. firstly it checks with the if condition the variable not equal to 
zero means that putchar function will call.
*/
static inline void _out_char(char character, void* buffer, size_t idx, size_t maxlen)
{
  (void)buffer; (void)idx; (void)maxlen;
  if (character) {
    _putchar(character);
  }
}

/**
@brief The _out_fct function is about internal output function wrapper

@param[in] character          it is a char data type
@param[in] buffer             string size of buffer
@param[in] idx                bytes of size_t
@param[in] maxlen             maximum lenghth of bytes in size_t

The _out_fct function fistly typecasting some variable to avoid compiler error is a 
output function wrapper and the buffer is the output fct pointer.
*/
static inline void _out_fct(char character, void* buffer, size_t idx, size_t maxlen)
{
  (void)idx; (void)maxlen;
  ((out_fct_wrap_type*)buffer)->fct(character, ((out_fct_wrap_type*)buffer)->arg);
}

/**
@brief The strlen function computes the length of the string str up to, but not including the terminating null character.

@param[in] str        This is the string whose length is to be found.

The strlen function fistly typecasting some variable to avoid compiler error declaring variable 

@return They typecastimg the return variable into unsigned int and 
retunr The length of the string (excluding the terminating 0)
*/
static inline unsigned int _strlen(const char* str)
{
  const char* s;
  for (s = str; *s; ++s);
  return (unsigned int)(s - str);
}

/**
@brief The _is_digit function is to internal test if char is a digit(0-9)

@param[in]   ch      This is the character to be checked.

@return Its return true if char is a digit
*/
static inline bool _is_digit(char ch)
{
  return (ch >= '0') && (ch <= '9');
}

/**
@brief The _atoi() is to converting the internal ASCII string into unsigned integer

@param[in]   str      string representation of an integral number.

@return After conversion it returns unsigned integer value.
*/
static unsigned int _atoi(const char** str)
{
  unsigned int i = 0U;
  while (_is_digit(**str)) {
    i = i * 10U + (unsigned int)(*((*str)++) - '0');
  }
  return i;
}

/**
@brief The ntoa format function is to convert the string into the defined format structure.

@param[in] out            type of out_fct_type
@param[in] buffer         string type of buffer
@param[in] idx            idx bytes of size_t
@param[in] maxlen         maximum lenghth of the size_t
@param[in] negative       boolean type
@param[in] base           an unsigned long data type
@param[in] prec           an unsigned integral data type 
@param[in] width          an unsigned integral data type 
@param[in] flags          an unsigned integral data type

The ntoa format converting the string type into this specified format it having while condition for pad leading zeros in the given values, using the flags and if else condition
handling the hash they checking the variable len with already defined base if not the len variable decrements using the same process remaining buf len are accumulated
Using the conditional statement pad spaces up to given width, And reversing the given string, append pad spaces up to given width are manipulated.

@return non integer value if success else error occur.
*/
static size_t _ntoa_format(out_fct_type out, char* buffer, size_t idx, size_t maxlen, char* buf, size_t len, bool negative, unsigned int base, unsigned int prec, unsigned int width, unsigned int flags)
{
  const size_t start_idx = idx;

  // pad leading zeros
  while (!(flags & FLAGS_LEFT) && (len < prec) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
    buf[len++] = '0';
  }
  while (!(flags & FLAGS_LEFT) && (flags & FLAGS_ZEROPAD) && (len < width) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
    buf[len++] = '0';
  }

  // handle hash
  if (flags & FLAGS_HASH) {
    if (!(flags & FLAGS_PRECISION) && len && ((len == prec) || (len == width))) {
      len--;
      if (len && (base == 16U)) {
        len--;
      }
    }
    if ((base == 16U) && !(flags & FLAGS_UPPERCASE) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = 'x';
    }
    else if ((base == 16U) && (flags & FLAGS_UPPERCASE) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = 'X';
    }
    else if ((base == 2U) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
      buf[len++] = 'b';
    }
    if (len < PRINTF_NTOA_BUFFER_SIZE) {
      buf[len++] = '0';
    }
  }

  // handle sign
  if (len && (len == width) && (negative || (flags & FLAGS_PLUS) || (flags & FLAGS_SPACE))) {
    len--;
  }
  if (len < PRINTF_NTOA_BUFFER_SIZE) {
    if (negative) {
      buf[len++] = '-';
    }
    else if (flags & FLAGS_PLUS) {
      buf[len++] = '+';  // ignore the space if the '+' exists
    }
    else if (flags & FLAGS_SPACE) {
      buf[len++] = ' ';
    }
  }

  // pad spaces up to given width
  if (!(flags & FLAGS_LEFT) && !(flags & FLAGS_ZEROPAD)) {
    for (size_t i = len; i < width; i++) {
      out(' ', buffer, idx++, maxlen);
    }
  }

  // reverse string
  for (size_t i = 0U; i < len; i++) {
    out(buf[len - i - 1U], buffer, idx++, maxlen);
  }

  // append pad spaces up to given width
  if (flags & FLAGS_LEFT) {
    while (idx - start_idx < width) {
      out(' ', buffer, idx++, maxlen);
    }
  }

  return idx;
}

/**
@brief The ntoa function is used for string into structure value.

@param[in] out        type of out_fct_type
@param[in] buffer     string type of buffer
@param[in] idx        idx bytes of size_t
@param[in] maxlen     maximum lenghth of the size_t
@param[in] negative   boolean type
@param[in] base       an unsigned long data type
@param[in] prec       an unsigned integral data type 
@param[in] width      an unsigned integral data type 
@param[in] flags      an unsigned integral data type

In the _ntoa_long function initialize the char buffer that value defined alread and check the for no hash value for zero and also checks with the same 
condtion for flags precision valid or not and too the digits specified in the formart matches with the string, some uppercase case constraints are included
to valid the string finally return the value with ntoa format.
*/
static size_t _ntoa_long(out_fct_type out, char* buffer, size_t idx, size_t maxlen, unsigned long value, bool negative, unsigned long base, unsigned int prec, unsigned int width, unsigned int flags)
{
  char buf[PRINTF_NTOA_BUFFER_SIZE];
  size_t len = 0U;

  // no hash for 0 values
  if (!value) {
    flags &= ~FLAGS_HASH;
  }

  // write if precision != 0 and value is != 0
  if (!(flags & FLAGS_PRECISION) || value) {
    do {
      const char digit = (char)(value % base);
      buf[len++] = digit < 10 ? '0' + digit : (flags & FLAGS_UPPERCASE ? 'A' : 'a') + digit - 10;
      value /= base;
    } while (value && (len < PRINTF_NTOA_BUFFER_SIZE));
  }

  return _ntoa_format(out, buffer, idx, maxlen, buf, len, negative, (unsigned int)base, prec, width, flags);
}


// internal itoa for 'long long' type
#if defined(PRINTF_SUPPORT_LONG_LONG)

/**
@brief The _ntoa_long_long function to convert string to a struct.

@param[in] out          type of out_fct_type
@param[in] buffer       string type of buffer
@param[in] idx          idx bytes of size_t
@param[in] maxlen       maximum lenghth of the size_t
@param[in] negative     boolean type
@param[in] base         an unsigned long data type
@param[in] prec         an unsigned integral data type 
@param[in] width        an unsigned integral data type 
@param[in] flags        an unsigned integral data type

This _ntoa_long_long function firstly initialize the variables and checks the if condition for no hash for zero values using the condition do while 
checking the buf lenghth, digits less than ten or not and flags uppercase is to check the string type finally its return the computed values into ntoa format.
*/
static size_t _ntoa_long_long(out_fct_type out, char* buffer, size_t idx, size_t maxlen, unsigned long long value, bool negative, unsigned long long base, unsigned int prec, unsigned int width, unsigned int flags)
{
  char buf[PRINTF_NTOA_BUFFER_SIZE];
  size_t len = 0U;

  // no hash for 0 values
  if (!value) {
    flags &= ~FLAGS_HASH;
  }

  // write if precision != 0 and value is != 0
  if (!(flags & FLAGS_PRECISION) || value) {
    do {
      const char digit = (char)(value % base);
      buf[len++] = digit < 10 ? '0' + digit : (flags & FLAGS_UPPERCASE ? 'A' : 'a') + digit - 10;
      value /= base;
    } while (value && (len < PRINTF_NTOA_BUFFER_SIZE));
  }

  return _ntoa_format(out, buffer, idx, maxlen, buf, len, negative, (unsigned int)base, prec, width, flags);
}
#endif  // PRINTF_SUPPORT_LONG_LONG


#if defined(PRINTF_SUPPORT_FLOAT)

/**
@brief The _ftoa function that converts a given floating-point number or a double to a string. Use of standard library functions for direct conversion is not allowed

@param[in] out        type of out_fct_type
@param[in] buffer     string type of buffer
@param[in] idx        it an unsigned integral data type 
@param[in] maxlen     length of the string
@param[in] value      it an double data type
@param[in] prec       an unsigned integral data type 
@param[in] width      an unsigned integral data type 
@param[in] flags      an unsigned integral data type

The ftoa function used for conversion float point into string firstly initialize the varibles and test case is added to check value are negative or not
set up the default precision  to 6, if it not set explicitly its nothing but format specifier, And afterlimit precision to nine, cause a prec greater than or equal ten can lead to overflow errors.
Initialize some variable for precision roll-over, round up also added if it required to round up the value, For very large numbers switch back to native sprintf for exponentials.
Some fractional part adding some extra zeros, adding decimal for these also some condition id defined, Using the while loop condtion pad leading zeros concept also is there,
using for loop they reverse the string if requires and append pad spaces up to given width.

@return non integer value if success else error occur
*/
static size_t _ftoa(out_fct_type out, char* buffer, size_t idx, size_t maxlen, double value, unsigned int prec, unsigned int width, unsigned int flags)
{
  const size_t start_idx = idx;

  char buf[PRINTF_FTOA_BUFFER_SIZE];
  size_t len  = 0U;
  double diff = 0.0;

  // if input is larger than thres_max, revert to exponential
  const double thres_max = (double)0x7FFFFFFF;

  // powers of 10
  static const double pow10[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };

  // test for negative
  bool negative = false;
  if (value < 0) {
    negative = true;
    value = 0 - value;
  }

  // set default precision to 6, if not set explicitly
  if (!(flags & FLAGS_PRECISION)) {
    prec = 6U;
  }
  // limit precision to 9, cause a prec >= 10 can lead to overflow errors
  while ((len < PRINTF_FTOA_BUFFER_SIZE) && (prec > 9U)) {
    buf[len++] = '0';
    prec--;
  }

  int whole = (int)value;
  double tmp = (value - whole) * pow10[prec];
  unsigned long frac = (unsigned long)tmp;
  diff = tmp - frac;

  if (diff > 0.5) {
    ++frac;
    // handle rollover, e.g. case 0.99 with prec 1 is 1.0
    if (frac >= pow10[prec]) {
      frac = 0;
      ++whole;
    }
  }
  else if ((diff == 0.5) && ((frac == 0U) || (frac & 1U))) {
    // if halfway, round up if odd, OR if last digit is 0
    ++frac;
  }

  // TBD: for very large numbers switch back to native sprintf for exponentials. Anyone want to write code to replace this?
  // Normal printf behavior is to print EVERY whole number digit which can be 100s of characters overflowing your buffers == bad
  if (value > thres_max) {
    return 0U;
  }

  if (prec == 0U) {
    diff = value - (double)whole;
    if (diff > 0.5) {
      // greater than 0.5, round up, e.g. 1.6 -> 2
      ++whole;
    }
    else if ((diff == 0.5) && (whole & 1)) {
      // exactly 0.5 and ODD, then round up
      // 1.5 -> 2, but 2.5 -> 2
      ++whole;
    }
  }
  else {
    unsigned int count = prec;
    // now do fractional part, as an unsigned number
    while (len < PRINTF_FTOA_BUFFER_SIZE) {
      --count;
      buf[len++] = (char)(48U + (frac % 10U));
      if (!(frac /= 10U)) {
        break;
      }
    }
    // add extra 0s
    while ((len < PRINTF_FTOA_BUFFER_SIZE) && (count-- > 0U)) {
      buf[len++] = '0';
    }
    if (len < PRINTF_FTOA_BUFFER_SIZE) {
      // add decimal
      buf[len++] = '.';
    }
  }

  // do whole part, number is reversed
  while (len < PRINTF_FTOA_BUFFER_SIZE) {
    buf[len++] = (char)(48 + (whole % 10));
    if (!(whole /= 10)) {
      break;
    }
  }

  // pad leading zeros
  while (!(flags & FLAGS_LEFT) && (flags & FLAGS_ZEROPAD) && (len < width) && (len < PRINTF_FTOA_BUFFER_SIZE)) {
    buf[len++] = '0';
  }

  // handle sign
  if ((len == width) && (negative || (flags & FLAGS_PLUS) || (flags & FLAGS_SPACE))) {
    len--;
  }
  if (len < PRINTF_FTOA_BUFFER_SIZE) {
    if (negative) {
      buf[len++] = '-';
    }
    else if (flags & FLAGS_PLUS) {
      buf[len++] = '+';  // ignore the space if the '+' exists
    }
    else if (flags & FLAGS_SPACE) {
      buf[len++] = ' ';
    }
  }

  // pad spaces up to given width
  if (!(flags & FLAGS_LEFT) && !(flags & FLAGS_ZEROPAD)) {
    for (size_t i = len; i < width; i++) {
      out(' ', buffer, idx++, maxlen);
    }
  }

  // reverse string
  for (size_t i = 0U; i < len; i++) {
    out(buf[len - i - 1U], buffer, idx++, maxlen);
  }

  // append pad spaces up to given width
  if (flags & FLAGS_LEFT) {
    while (idx - start_idx < width) {
      out(' ', buffer, idx++, maxlen);
    }
  }

  return idx;
}
#endif  // PRINTF_SUPPORT_FLOAT

/**
@brief The _vsnprintf function write formatted output to a character array, up to a maximum number of characters (varargs) and evaluation of format specifiers are happening in this function.

@param[in] out          type of out_fct_type
@param[in] buffer       pointer to the buffer where you want to function to store the formatted string
@param[in] maxlen       maximum number of characters to store in the buffer
@param[in] format       string that specifies the format of the output.
@param[in] va           variable-argument list of the additional argument

The _vsnprintf fucntion firstly initialize the varibles of format specifers like flags, width, precsion
in this they evalucating all the specifiers invidually.firlst checks the buffer equal to zero for null out function.
after that flags evaluation will start using the switch case,
then width field evalucation take process using if conditions,after using flag precision filed  its checking the 
precision field finally using switch  its evaluate length field, after that defined PRINTF_SUPPORT_PTRDIFF_T depends 
on the case it case statement will executes and end evaluate starts for specifer with switch case and setting
the base with unsigned base, its convert the interger format of flags precision its having some if else condition to 
cheack post padding, pre paddings and string output finally return written characters without terminating.

@return Its return the typecasted int of idx if success otherwise error occured.
*/
// internal vsnprintf
static int _vsnprintf(out_fct_type out, char* buffer, const size_t maxlen, const char* format, va_list va)
{
  unsigned int flags, width, precision, n;
  size_t idx = 0U;

  if (!buffer) {
    // use null output function
    out = _out_null;
  }

  while (*format)
  {
    // format specifier?  %[flags][width][.precision][length]
    if (*format != '%') {
      // no
      out(*format, buffer, idx++, maxlen);
      format++;
      continue;
    }
    else {
      // yes, evaluate it
      format++;
    }

    // evaluate flags
    flags = 0U;
    do {
      switch (*format) {
        case '0': flags |= FLAGS_ZEROPAD; format++; n = 1U; break;
        case '-': flags |= FLAGS_LEFT;    format++; n = 1U; break;
        case '+': flags |= FLAGS_PLUS;    format++; n = 1U; break;
        case ' ': flags |= FLAGS_SPACE;   format++; n = 1U; break;
        case '#': flags |= FLAGS_HASH;    format++; n = 1U; break;
        default :                                   n = 0U; break;
      }
    } while (n);

    // evaluate width field
    width = 0U;
    if (_is_digit(*format)) {
      width = _atoi(&format);
    }
    else if (*format == '*') {
      const int w = va_arg(va, int);
      if (w < 0) {
        flags |= FLAGS_LEFT;    // reverse padding
        width = (unsigned int)-w;
      }
      else {
        width = (unsigned int)w;
      }
      format++;
    }

    // evaluate precision field
    precision = 0U;
    if (*format == '.') {
      flags |= FLAGS_PRECISION;
      format++;
      if (_is_digit(*format)) {
        precision = _atoi(&format);
      }
      else if (*format == '*') {
        const int prec = (int)va_arg(va, int);
        precision = prec > 0 ? (unsigned int)prec : 0U;
        format++;
      }
    }

    // evaluate length field
    switch (*format) {
      case 'l' :
        flags |= FLAGS_LONG;
        format++;
        if (*format == 'l') {
          flags |= FLAGS_LONG_LONG;
          format++;
        }
        break;
      case 'h' :
        flags |= FLAGS_SHORT;
        format++;
        if (*format == 'h') {
          flags |= FLAGS_CHAR;
          format++;
        }
        break;
#if defined(PRINTF_SUPPORT_PTRDIFF_T)
      case 't' :
        flags |= (sizeof(ptrdiff_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;
#endif
      case 'j' :
        flags |= (sizeof(intmax_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;
      case 'z' :
        flags |= (sizeof(size_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;
      default :
        break;
    }

    // evaluate specifier
    switch (*format) {
      case 'd' :
      case 'i' :
      case 'u' :
      case 'x' :
      case 'X' :
      case 'o' :
      case 'b' : {
        // set the base
        unsigned int base;
        if (*format == 'x' || *format == 'X') {
          base = 16U;
        }
        else if (*format == 'o') {
          base =  8U;
        }
        else if (*format == 'b') {
          base =  2U;
        }
        else {
          base = 10U;
          flags &= ~FLAGS_HASH;   // no hash for dec format
        }
        // uppercase
        if (*format == 'X') {
          flags |= FLAGS_UPPERCASE;
        }

        // no plus or space flag for u, x, X, o, b
        if ((*format != 'i') && (*format != 'd')) {
          flags &= ~(FLAGS_PLUS | FLAGS_SPACE);
        }

        // ignore '0' flag when precision is given
        if (flags & FLAGS_PRECISION) {
          flags &= ~FLAGS_ZEROPAD;
        }

        // convert the integer
        if ((*format == 'i') || (*format == 'd')) {
          // signed
          if (flags & FLAGS_LONG_LONG) {
#if defined(PRINTF_SUPPORT_LONG_LONG)
            const long long value = va_arg(va, long long);
            idx = _ntoa_long_long(out, buffer, idx, maxlen, (unsigned long long)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
#endif
          }
          else if (flags & FLAGS_LONG) {
            const long value = va_arg(va, long);
            idx = _ntoa_long(out, buffer, idx, maxlen, (unsigned long)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
          }
          else {
            const int value = (flags & FLAGS_CHAR) ? (char)va_arg(va, int) : (flags & FLAGS_SHORT) ? (short int)va_arg(va, int) : va_arg(va, int);
            idx = _ntoa_long(out, buffer, idx, maxlen, (unsigned int)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
          }
        }
        else {
          // unsigned
          if (flags & FLAGS_LONG_LONG) {
#if defined(PRINTF_SUPPORT_LONG_LONG)
            idx = _ntoa_long_long(out, buffer, idx, maxlen, va_arg(va, unsigned long long), false, base, precision, width, flags);
#endif
          }
          else if (flags & FLAGS_LONG) {
            idx = _ntoa_long(out, buffer, idx, maxlen, va_arg(va, unsigned long), false, base, precision, width, flags);
          }
          else {
            const unsigned int value = (flags & FLAGS_CHAR) ? (unsigned char)va_arg(va, unsigned int) : (flags & FLAGS_SHORT) ? (unsigned short int)va_arg(va, unsigned int) : va_arg(va, unsigned int);
            idx = _ntoa_long(out, buffer, idx, maxlen, value, false, base, precision, width, flags);
          }
        }
        format++;
        break;
      }
#if defined(PRINTF_SUPPORT_FLOAT)
      case 'f' :
      case 'F' :
        idx = _ftoa(out, buffer, idx, maxlen, va_arg(va, double), precision, width, flags);
        format++;
        break;
#endif  // PRINTF_SUPPORT_FLOAT
      case 'c' : {
        unsigned int l = 1U;
        // pre padding
        if (!(flags & FLAGS_LEFT)) {
          while (l++ < width) {
            out(' ', buffer, idx++, maxlen);
          }
        }
        // char output
        out((char)va_arg(va, int), buffer, idx++, maxlen);
        // post padding
        if (flags & FLAGS_LEFT) {
          while (l++ < width) {
            out(' ', buffer, idx++, maxlen);
          }
        }
        format++;
        break;
      }

      case 's' : {
        char* p = va_arg(va, char*);
        unsigned int l = _strlen(p);
        // pre padding
        if (flags & FLAGS_PRECISION) {
          l = (l < precision ? l : precision);
        }
        if (!(flags & FLAGS_LEFT)) {
          while (l++ < width) {
            out(' ', buffer, idx++, maxlen);
          }
        }
        // string output
        while ((*p != 0) && (!(flags & FLAGS_PRECISION) || precision--)) {
          out(*(p++), buffer, idx++, maxlen);
        }
        // post padding
        if (flags & FLAGS_LEFT) {
          while (l++ < width) {
            out(' ', buffer, idx++, maxlen);
          }
        }
        format++;
        break;
      }

      case 'p' : {
        width = sizeof(void*) * 2U;
        flags |= FLAGS_ZEROPAD | FLAGS_UPPERCASE;
#if defined(PRINTF_SUPPORT_LONG_LONG)
        const bool is_ll = sizeof(uintptr_t) == sizeof(long long);
        if (is_ll) {
          idx = _ntoa_long_long(out, buffer, idx, maxlen, (uintptr_t)va_arg(va, void*), false, 16U, precision, width, flags);
        }
        else {
#endif
          idx = _ntoa_long(out, buffer, idx, maxlen, (unsigned long)((uintptr_t)va_arg(va, void*)), false, 16U, precision, width, flags);
#if defined(PRINTF_SUPPORT_LONG_LONG)
        }
#endif
        format++;
        break;
      }

      case '%' :
        out('%', buffer, idx++, maxlen);
        format++;
        break;

      default :
        out(*format, buffer, idx++, maxlen);
        format++;
        break;
    }
  }

  // termination
  out((char)0, buffer, idx < maxlen ? idx : maxlen - 1U, maxlen);

  // return written chars without terminating \0
  return (int)idx;
}

// This will issue one ocall for each charactor.
#if 0

/**
@brief The printf function is a system call function used inside if 
condition whether the value is zero means it will execute and also here used varibale arguments
inside vsnprintf function the function will do composes a string with the same text that would be 
printed if format was used on printf, but using the elements in the variable argument list identified
by arg instead of additional function arguments and storing the resulting content as a C string in the
buffer pointed by argument.
*/
int printf(const char* format, ...)
{
  va_list va;
  va_start(va, format);
  char buffer[1];
  const int ret = _vsnprintf(_out_char, buffer, (size_t)-1, format, va);
  va_end(va);
  return ret;
}
#endif

/**
@brief The sprintf function sends formatted output to a string pointed to by the argument buffer.  

@param[in] buffer  pointer to an array of char elements resulting string will store.
@param[in] format tring that contains the text to be written to buffer.

The sprintf function using the va_start() and va_end() its ac libraty macros before using the va_arg va_start function will call and
in the int ret variable taking the value of the function called _vsnprintf function after the va_end macro its return the value.

@returns Its returns the ret value as an integer type. 
*/
int sprintf(char* buffer, const char* format, ...)
{
  va_list va;
  va_start(va, format);
  const int ret = _vsnprintf(_out_buffer, buffer, (size_t)-1, format, va);
  va_end(va);
  return ret;
}

/**
@brief The snprintf() places the generated output into the character array pointed to by buf, instead of writing it to a file

@param[in] buffer       pointer to buffer where you want to function to store the formatted string. 
@param[in] count        maximum number of characters to store in the buffer.
@param[in] format       string that specifies the format of the output. 

The snprintf function using the va_start() and va_end() its ac libraty macros before using the va_arg va_start function will call and
in the int ret variable taking the value of the function called _vsnprintf function after the va_end macro its return the value.

@returns Its returns the ret value as an integer type. 
*/
int snprintf(char* buffer, size_t count, const char* format, ...)
{
  va_list va;
  va_start(va, format);
  const int ret = _vsnprintf(_out_buffer, buffer, count, format, va);
  va_end(va);
  return ret;
}

/**
@brief The vsnprintf() just returns with another function called _vsnprintf() with some arguments.

@param[in] buffer       pointer to the buffer where you want to function to store the formatted string. 
@param[in] count        maximum number of characters to store in the buffer.
@param[in] format       string that specifies the format of the output.
*/
int vsnprintf(char* buffer, size_t count, const char* format, va_list va)
{
  return _vsnprintf(_out_buffer, buffer, count, format, va);
}

/**
@brief The fctprintf function is using the libary macros of variable aruguments like vastart and vaend 

@param[in] out        An output function which takes one character and an argument pointer.
@param[in] arg        An argument pointer for user data passed to output function.
@param[in] format     A string that specifies the format of the output. 

The fctprintf function using the va_start() and va_end() its ac library macros before using the va_arg va_start function will call and
in the int ret variable taking the value of the function called _vsnprintf function after the va_end macro its return the value.

@return The number of characters that are sent to the output function, not counting the terminating null character
*/
int fctprintf(void (*out)(char character, void* arg), void* arg, const char* format, ...)
{
  va_list va;
  va_start(va, format);
  const out_fct_wrap_type out_fct_wrap = { out, arg };
  const int ret = _vsnprintf(_out_fct, (char*)&out_fct_wrap, (size_t)-1, format, va);
  va_end(va);
  return ret;
}
