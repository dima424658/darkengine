#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

// Small, dependency-free helpers for values originating in mission, resource,
// and script files.  Keep the calculation in 64 bits until the final range
// check; never rely on a wrapped 32-bit intermediate being detected later.
static inline int DarkCheckedAddU64(uint64_t a, uint64_t b, uint64_t *out)
{
   if (!out || a > UINT64_MAX - b)
      return 0;
   *out = a + b;
   return 1;
}

static inline int DarkCheckedMulU64(uint64_t a, uint64_t b, uint64_t *out)
{
   if (!out || (a != 0 && b > UINT64_MAX / a))
      return 0;
   *out = a * b;
   return 1;
}

static inline int DarkRangeWithinU64(uint64_t offset, uint64_t size,
                                    uint64_t limit)
{
   uint64_t end = 0;
   return DarkCheckedAddU64(offset,size,&end) && end <= limit;
}

static inline int DarkCheckedSizeT(uint64_t value, size_t *out)
{
   if (!out || value > (uint64_t)SIZE_MAX)
      return 0;
   *out = (size_t)value;
   return 1;
}

static inline int DarkBufferHasNull(const char *buffer, size_t size)
{
   return buffer && memchr(buffer,'\0',size) != NULL;
}

static inline int DarkFileSeek64(FILE *file, int64_t offset, int origin)
{
#ifdef _WIN32
   return _fseeki64(file,offset,origin);
#else
   return fseeko(file,(off_t)offset,origin);
#endif
}

static inline int64_t DarkFileTell64(FILE *file)
{
#ifdef _WIN32
   return _ftelli64(file);
#else
   return (int64_t)ftello(file);
#endif
}
