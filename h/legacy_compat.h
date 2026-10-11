#pragma once

#if !defined(_WIN32)
#include <alloca.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <limits.h>
#define link dark_posix_link
#include <unistd.h>
#undef link

#define _alloca alloca
#define _snprintf snprintf
#define _vsnprintf vsnprintf
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define _strcmpi strcasecmp
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define strcmpi strcasecmp
#define _O_RDONLY O_RDONLY
#define _O_RDWR O_RDWR
#define _O_CREAT O_CREAT
#define _O_TRUNC O_TRUNC
#define tell(fd) ((long)lseek((fd), 0, SEEK_CUR))
#define chsize(fd, size) ftruncate((fd), (off_t)(size))
#define _chsize(fd, size) ftruncate((fd), (off_t)(size))
#define _fileno fileno
#define _fstat fstat
#define _stat stat
#define _isnan isnan
#ifndef _MAX_PATH
#define _MAX_PATH PATH_MAX
#endif
#ifndef __int64
#define __int64 long long
#endif
#define __min(a, b) ((a) < (b) ? (a) : (b))
#define __max(a, b) ((a) > (b) ? (a) : (b))
#ifndef S_IREAD
#define S_IREAD S_IRUSR
#endif
#ifndef S_IWRITE
#define S_IWRITE S_IWUSR
#endif

static inline char *dark_strupr(char *text)
{
   char *p = text;
   while (*p) {
      *p = (char)toupper((unsigned char)*p);
      ++p;
   }
   return text;
}

static inline long dark_filelength(int fd)
{
   off_t position = lseek(fd, 0, SEEK_CUR);
   off_t length = lseek(fd, 0, SEEK_END);
   if (position >= 0)
      lseek(fd, position, SEEK_SET);
   return (long)length;
}

static inline char *dark_strlwr(char *text)
{
   char *p = text;
   while (*p) {
      *p = (char)tolower((unsigned char)*p);
      ++p;
   }
   return text;
}

static inline char *dark_strrev(char *text)
{
   size_t left = 0;
   size_t right = strlen(text);
   while (right > left) {
      char temp;
      --right;
      temp = text[left];
      text[left++] = text[right];
      text[right] = temp;
   }
   return text;
}

static inline char *dark_itoa(int value, char *text, int radix)
{
   static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
   unsigned int magnitude;
   int negative = value < 0 && radix == 10;
   char *p = text;
   if (radix < 2 || radix > 36) {
      *text = '\0';
      return text;
   }
   magnitude = negative ? 0u - (unsigned int)value : (unsigned int)value;
   do {
      *p++ = digits[magnitude % (unsigned int)radix];
      magnitude /= (unsigned int)radix;
   } while (magnitude);
   if (negative)
      *p++ = '-';
   *p = '\0';
   return dark_strrev(text);
}

static inline char *dark_ultoa(unsigned long value, char *text, int radix)
{
   static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
   char *p = text;
   if (radix < 2 || radix > 36) {
      *text = '\0';
      return text;
   }
   do {
      *p++ = digits[value % (unsigned long)radix];
      value /= (unsigned long)radix;
   } while (value);
   *p = '\0';
   return dark_strrev(text);
}

static inline char *dark_ltoa(long value, char *text, int radix)
{
   unsigned long magnitude;
   int negative = value < 0 && radix == 10;
   if (radix < 2 || radix > 36) {
      *text = '\0';
      return text;
   }
   magnitude = negative ? 0ul - (unsigned long)value : (unsigned long)value;
   dark_ultoa(magnitude, text, radix);
   if (negative) {
      size_t length = strlen(text);
      memmove(text + 1, text, length + 1);
      *text = '-';
   }
   return text;
}

static inline char *dark_gcvt(double value, int digits, char *text)
{
   snprintf(text, 64, "%.*g", digits, value);
   return text;
}

#define strupr dark_strupr
#define filelength dark_filelength
#define strlwr dark_strlwr
#define strrev dark_strrev
#define itoa dark_itoa
#define _itoa dark_itoa
#define ltoa dark_ltoa
#define _ltoa dark_ltoa
#define ultoa dark_ultoa
#define _ultoa dark_ultoa
#define gcvt dark_gcvt
#define _gcvt dark_gcvt
#endif
