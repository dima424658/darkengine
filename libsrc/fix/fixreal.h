/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifndef __REAL_H
#define __REAL_H

#include <fix.h>
#include <altfix.h>
typedef fix real;

#define REAL_UNIT FIX_UNIT
#define REAL_UNIT_3 FIX_UNIT_3

#define REAL_MAX FIX_MAX
#define REAL_MIN FIX_MIN

#define REALANG_PI FIXANG_PI
#define REALANG_MAX 0xffff

#define INT_REAL_MAX real_int(REAL_MAX)
#define INT_REAL_MIN real_int(REAL_MIN)
#define real_safe_make(a, b) ((a)>REAL_MAX)
#define real_make fix_make
#define fix_to_real(_x) _x
#define real_to_fix(_x) _x

#define real_rint fix_rint
#define real_int fix_int

#define real_frac fix_frac

#define real_div_overflow(n,d) fix_div_overflow(n,d)
#define real_sign(n) fix_sign(n)
#define real_div_sign(n,d) fix_div_sign(n,d)

#define real_float fix_float
#define real_from_float fix_from_float
#define FLT_REAL_MAX real_float(REAL_MAX)
#define FLT_REAL_MIN real_float(REAL_MIN)
#define real_safe_from_float(f) (((f)>FLT_REAL_MAX)?REAL_MAX:(((f)<FLT_REAL_MIN)?REAL_MIN:fix_from_float(f)))

#define real_exp fix_exp
#define real_pow fix_pow
#define real_sqrt fix_sqrt

#define real_div_safe fix_div_safe
#define real_div fix_div
#define real_div_16_16_3 fix_div_16_16_3
#define real_div_3_16_3 fix_div_3_16_3

#define real_mul fix_mul
#define real_mul_3_3_3 fix_mul_3_3_3

#define real_abs fix_abs
#define real_sgn fix_sgn

#define real_safe_pyth_dist fix_safe_pyth_dist
#define real_fast_pyth_dist fix_fast_pyth_dist

#define realrad_to_realang fixrad_to_fixang
#define realang_to_realrad fixang_to_fixrad

#define realang fixang
#define real_fast_asin fix_fast_asin
#define real_sincos fix_sincos
#define real_sin fix_sin
#define real_cos fix_cos
#define real_fastsincos real_fastsincos
#define real_fastsin fix_fastsin
#define real_fastcos fix_fastcos
#define real_asin fix_asin
#define real_acos fix_acos
#define real_atan2 fix_atan2

#define __REAL_H
#endif
