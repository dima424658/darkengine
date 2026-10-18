/* Portable operations for the Dark Engine's alternate fixed-point formats. */

#ifndef __ALTFIX_H
#define __ALTFIX_H

#include <stdint.h>
#include <fix.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FIX_UNIT_3 ((fix)0x20000000)

static __inline fix altfix_mul_shift(fix a, fix b, unsigned shift)
{
   return (fix)(((int64_t)a * (int64_t)b) >> shift);
}

static __inline fix altfix_div_shift(fix a, fix b, unsigned shift)
{
   return (fix)(((int64_t)a * ((int64_t)1 << shift)) / b);
}

static __inline fix altfix_div_shift_safe(fix a, fix b, unsigned shift)
{
   int64_t result;
   if (b == 0)
      return a < 0 ? FIX_MIN : FIX_MAX;
   result = ((int64_t)a * ((int64_t)1 << shift)) / b;
   if (result > FIX_MAX)
      return FIX_MAX;
   if (result < FIX_MIN)
      return FIX_MIN;
   return (fix)result;
}

static __inline fix fix_mul_8_16_16(fix a, fix b)
{
   return altfix_mul_shift(a, b, 24);
}

static __inline fix fix_mul_3_3_3(fix a, fix b)
{
   return altfix_mul_shift(a, b, 29);
}

#define fix_mul_3_16_16 fix_mul_3_3_3
#define fix_mul_3_8_8 fix_mul_3_3_3

static __inline fix fix_mul_3_32_16(fix a, fix b)
{
   return altfix_mul_shift(a, b, 13);
}

static __inline fix fix_mul_3_16_20(fix a, fix b)
{
   return altfix_mul_shift(a, b, 33);
}

static __inline fix fix_mul_16_32_20(fix a, fix b)
{
   return altfix_mul_shift(a, b, 4);
}

static __inline fix fix_div_16_16_3_fast(fix a, fix b)
{
   return altfix_div_shift(a, b, 29);
}

static __inline fix fix_div_16_16_3_safe(fix a, fix b)
{
   return altfix_div_shift_safe(a, b, 29);
}

static __inline fix fix_div_16_8_16_fast(fix a, fix b)
{
   return altfix_div_shift(a, b, 24);
}

static __inline fix fix_div_16_8_16_safe(fix a, fix b)
{
   return altfix_div_shift_safe(a, b, 24);
}

static __inline fix fix_div_16_3_32_fast(fix a, fix b)
{
   return altfix_div_shift(a, b, 13);
}

static __inline fix fix_div_16_3_32_safe(fix a, fix b)
{
   return altfix_div_shift_safe(a, b, 13);
}

#ifdef _WIN32
#define fix_div_16_16_3 fix_div_16_16_3_safe
#define fix_div_16_3_32 fix_div_16_3_32_safe
#define fix_div_16_8_16 fix_div_16_8_16_safe
#else
#define fix_div_16_16_3 fix_div_16_16_3_fast
#define fix_div_16_3_32 fix_div_16_3_32_fast
#define fix_div_16_8_16 fix_div_16_8_16_fast
#endif

#define fix_div_3_8_3 fix_div_16_8_16
#define fix_div_8_8_8 fix_div_16_8_16
#define fix_div_16_3_16 fix_div_16_16_3
#define fix_div_3_3_16 fix_div
#define fix_div_3_16_3 fix_div

#define fix_mul_div_3_16_16_3 fix_mul_div
#define fix_mul_div_3_16_3_16 fix_mul_div
#define fix_mul_div_3_8_8_3 fix_mul_div

#define fix_sar(a,b) ((a) >> (b))
#define fix_3_12(a) ((a) >> 9)
#define fix_12_16(a) ((a) >> 4)
#define fix_3_16(a) ((a) >> 13)
#define fix_3_8(a) ((a) >> 5)

#ifdef __cplusplus
}
#endif

#endif
