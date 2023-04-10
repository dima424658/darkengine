/* $Header: x:/prj/tech/libsrc/fix/RCS/fix.h 1.59 1997/04/24 13:59:25 TOML Exp $
 *
 * Code, prototypes and types for fixed-point routines.
 *
 */

#ifndef __FIX_H
#define __FIX_H

#include <types.h>

//////////////////////////////
//
// First some math functions that don't use fixes.
//

// Returns 0 if x < 0

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus

ushort long_sqrt (long x);
#ifdef __WATCOMC__
#pragma aux long_sqrt parm [eax] value [ax] modify [eax ebx ecx edx esi edi]
#endif

//////////////////////////////
//
// fix.c
//

long long_fast_pyth_dist (long a, long b);
long long_safe_pyth_dist (long a, long b);

//////////////////////////////
//
// Now for fixes themselves.
//

/* these functions operate on fixed-point numbers with one bit of sign, 15
   bits of integer, and 16 bits of fraction.  thus, a rational number a is
   represented as a 32-bit number as a*2^16. */

typedef long fix;
typedef fix fix16;

// define min and max
// We're avoiding 0x80000000 because that number doesn't
// have an associated positive number.

#define FIX_MAX ((fix) (0x7fffffff))
#define FIX_MIN ((fix) (0x80000001))

// A fixang (fixed-point angle) can be converted to radians by multiplying
// by 2 * PI and dividing by 2^16.
//
// 0x0000 -> 0
// 0x4000 -> PI/2
// 0x8000 -> PI
// 0xc000 -> 3PI/2
//

#define FIXANG_PI 0x8000
#define fix_2pi fix_make(6,18559) // that's 6 + 18559/65536 = 6.28319

typedef ushort fixang;

/* makes a fixed point number with integral part a and fractional part b. */
#define fix_make(a,b) ((fix) ((((long)(a))<<16)|(b)) )

#define FIX_UNIT fix_make(1,0)

/* lops off the fractional part of a fixed point number. */
#define fix_trunc(n) ((fix) ((n)&0xffff0000))

/* Does a floor */
#define fix_floor(n) ((fix) ((n)&0xffff0000))

/* Does a ceil */
#define fix_ceil(n) ((fix) (((n)+65535)&0xffff0000))

/* round a fix to the nearest integer, leaving in fix format. */
#define fix_round(n) ((fix) (((n)+32768)&0xffff0000))

/* returns the integral part of a fixed point number. */
#define fix_int(n) ((n)>>16)

// Absolute value and signum
#define fix_abs(n) (((n) < 0) ? -(n) : (n))
#define fix_sgn(n) (((n) < 0) ? -FIX_UNIT : (((n) == 0) ? 0 : FIX_UNIT))

/* converts the floor of n to an integer. */
#define fix_fint(n) ((n)>>16)

/* converts the ceiling of n to an integer. */
#define fix_cint(n) ((short) (((n)+0xffff)>>16))

/* returns the integral part of a fixed point number rounded up. */
// #define fix_rint(n) (fix_int (fix_round (n)))
// the following macro does it all explictly to avoid the spurious & in fix_round
#define fix_rint(n) ((fix) (((n)+0x8000)>>16))

/* returns the fractional part of a fixed point number. */
#define fix_frac(n) ((fix) ((n)&0xffff))


////////////////////////////////////////////////////////
//
//  Avoid divides by zero and divide overflows
//  for a shippable product.  This maybe should
//  go into fix.h
//
//
//  We want to know if   numer/denom  is going to overflow.
//  If we assume they're both ints, then  numer/denom
//  overflows if abs(numer/denom) >= MAX_INT, which only
//  happens if denom is 0.
//
//  If we assume they're both fixes, and we assume that all
//  the following math is integer math manipulating the bit
//  patterns affiliated with the fixed, then we overflow if
//    abs(numer/denom) >= fix_int(MAX_FIX)+1 = 0x8000 = FIX_UNIT/2
//
//  Let n = abs(numer), d = abs(denom).
//
//  We overflow if:
//    n/d >= MAX_FIX
//    n >= FIX_UNIT/2 * d
//    n/FIX_UNIT >= d/2
//    fix_int(n) >= d/2

#define fix_div_overflow(n,d)   (fix_int(fix_abs(n)) >= fix_abs(d)/2)
#define fix_sign(n)             ((n) < 0 ? -FIX_UNIT : (n) > 0 ? FIX_UNIT : 0)
#define fix_div_sign(n,d)       ((d) < 0 ? -fix_sign(n) : fix_sign(n))

// fixrad_to_fixang converts a fixed-point in radians to a fixang
// fixang_to_fixrad converts a fixang to a fixed point radians
// degrees_to_fixang converts an integer number of degrees to a fixang
// fixang_to_degrees converts a fixang to an integer number of degrees

#define fixrad_to_fixang(fixradian) (fix_frac(fix_div((fixradian),fix_2pi)))
#define fixang_to_fixrad(ang) fix_div(fix_mul(ang,fix_2pi),0x10000)
#define degrees_to_fixang(d)    ((fixang)(((d)*FIXANG_PI)/180))
#define fixang_to_degrees(ang)  (((long)(ang)*180)/FIXANG_PI)

// turns a fixed point into a float.
#define fix_float(n) ( ((float)(n)) / ((float)(65536L)) )

// makes a fixed point from a float.
#define fix_from_float(n) ((fix)(65536.0*(n)))

#define fix_mul fast_fix_mul

extern const fixang asintab[128+1];
#define fix_fast_asin(fixx) (asintab[((fixx)>>10)+0x40])

/* multiply 2 fixed point numbers, return the result.  first calculate the
   64-bit product edx:eax.  since the original numbers are a*2^16 and b*2^16,
   their product is a*b*2^32, and edx is the integral part and eax is the
   fractional part.  the bottom 16 bits or edx and the top 16 bits of eax are
   the 32-bit fixed-point product. */
#ifdef __WATCOMC__
fix quick_fix_mul (fix a, fix b);
#pragma aux quick_fix_mul =\
   "imul    edx"     \
   "shr     eax,16"  \
   "shl     edx,16"  \
   "or      eax,edx" \
   parm [eax] [edx]  \
   modify [eax edx];
#else
__inline fix quick_fix_mul(fix a, fix b)
{
	#pragma warning(disable : 4035)	// disables no return value warning
	__asm
	{
		mov		eax, a
		mov		edx, b
		imul	edx
		shr		eax, 16
		shl		edx, 16
		or		eax, edx
	}
}
#endif

// well, this turns out to be 2.2 or so times faster than the above on a 486
// and about 1.4 times slower on a Pentium.
// i love intel, sadly for now we probably should stay with 486 based optimizations
// so im defaulting to this one for now....
#ifdef __WATCOMC__
fix fast_fix_mul (fix a, fix b);
#pragma aux fast_fix_mul =\
   "imul    edx"     \
   "shrd    eax,edx,16" \
   parm [eax] [edx]  \
   modify [eax edx];
#else
__inline fix fast_fix_mul (fix a, fix b)
{
	#pragma warning(disable : 4035)	// disables no return value warning
	__asm
	{
		mov		eax, a
		mov		edx, b
		imul	edx
		shrd	eax, edx, 16
	}
}
#endif

#ifdef __WATCOMC__
fix fast_fix_mul_int (fix a, fix b);
#pragma aux fast_fix_mul_int =\
   "imul    edx"     \
   "mov     eax,edx" \
   parm [eax] [edx]  \
   modify [eax edx];
#else
__inline fix fast_fix_mul_int(fix a, fix b)
{
	#pragma warning(disable : 4035)	// disables no return value warning
	__asm
	{
		mov		eax, a
		mov		edx, b
		imul	edx
		mov		eax, edx
	}
}
#endif

extern char mul_test;

// Jacobson, 2-27-96
// Made the test more robust. We now catch cases like the result
// being 0x80000000FFFFFFFF which weren't caught before. We also
// rail to the top or bottom rails.

fix safe_fix_mul (fix a, fix b);
#ifdef __WATCOMC__
fix safe_fix_mul_asm (fix a, fix b);
#pragma aux safe_fix_mul_asm =\
   "mov     mul_test,0"\
   "imul    edx"       \
   "shr     eax,16"    \
   "ror     edx,15"    \
   "mov     ebx,edx"   \
   "inc     ebx"       \
   "and     ebx,0x0001FFFF" \
   "cmp     ebx,1"     \
   "ja      overflow"  \
   "ror     edx,1"     \
   "mov     dx,0"      \
   "or      eax,edx"   \
   "jmp     done"      \
"overflow:"            \
   "rol     edx,15"    \
   "sar     edx,31"    \
   "mov     eax,0x7fffffff" \
   "sub     eax,edx"   \
   "sub     eax,edx"   \
   "mov     mul_test,1" \
"done:"                \
   parm [eax] [edx]    \
   modify [eax ebx edx];
#else
__inline fix safe_fix_mul_asm (fix a, fix b)
{
	#pragma warning(disable : 4035)	// disables no return value warning
	__asm
	{
		mov		eax, a
		mov		edx, b
		mov		mul_test, 0
		imul		edx
		shr		eax, 16
		ror		edx, 15
		mov		ebx, edx
		inc		ebx
		and		ebx, 0x0001FFFF
		cmp		ebx, 1
		ja		overflow
		ror		edx, 1
		mov		dx, 0
		or		eax, edx
		jmp		done
	overflow:
		rol		edx, 15
		sar		edx, 31
		mov		eax, 0x7fffffff
		sub		eax, edx
		sub		eax, edx
		mov		mul_test, 1
	done:
	}
}
#endif

/* divide 2 fixed point numbers, return the result.  we want the result to be
   a/b*2^16 which is also a*2^32/b*2^16.  since a*2^16 is 32 bits, a*2^32 is
   48 bits, and has to span edx:eax.  the top 16 bits of a*2^16 get put in
   edx and the bottom 16 get shift up in eax.  the quotient is obtained by
   dividing by b*2^16. */
#ifdef __WATCOMC__
fix fix_div_fast (fix a, fix b);
#pragma aux fix_div_fast =\
   "mov     edx,eax" \
   "sar     edx,16"  \
   "shl     eax,16"  \
   "idiv    ebx"     \
   parm [eax] [ebx]  \
   modify [eax edx];
#else
__inline fix fix_div_fast (fix a, fix b)
{
	#pragma warning(disable : 4035)	// disables no return value warning
	__asm
	{
		mov		eax, a
		mov		ebx, b
		mov		edx, eax
		sar		edx, 16
		shl		eax, 16
		idiv	ebx
	}
}
#endif

extern fix fix_div_safe (fix a, fix b);

/* Multiply two fixed-point numbers and divide by a third.
    Maintains the 64-bit intermediate result. */
#ifdef __WATCOMC__
fix fix_mul_div_fast (fix m0, fix m1, fix d);
#pragma aux fix_mul_div_fast =\
   "imul    edx"           \
   "idiv    ebx"           \
   parm [eax] [edx] [ebx]  \
   modify [eax edx];
#else
__inline fix fix_mul_div_fast (fix m0, fix m1, fix d)
{
	#pragma warning(disable : 4035)	// disables no return value warning
	__asm
	{
		mov		eax, m0
		mov		edx, m1
		mov		ebx, d
		imul	edx
		idiv	ebx
	}
}
#endif

extern fix fix_mul_div_safe (fix m0, fix m1, fix d);


#ifndef WIN32
#define fix_div fix_div_fast
#define fix_mul_div fix_mul_div_fast
#else
#define fix_div fix_div_safe
#define fix_mul_div fix_mul_div_safe
#endif

// Returns sqrt (a^2 + b^2)
fix fix_pyth_dist (fix a, fix b);

// Returns approximately sqrt (a^2 + b^2)
// Is never off by more than 12% (it's worst at 45 deg)
fix fix_fast_pyth_dist (fix a, fix b);

// pyth_dist with less fear of overflow.  Either number
// can be up to 0x2fffffff.
fix fix_safe_pyth_dist_info (fix a, fix b, char *file, int line);

#ifdef WARN_ON
#define fix_safe_pyth_dist(a,b) fix_safe_pyth_dist_info(a, b, __FILE__, __LINE__)
#else
#define fix_safe_pyth_dist(a,b) fix_safe_pyth_dist_info(a, b, NULL, 0)
#endif

// Returns 0 if x < 0
fix fix_sqrt (fix x);
#ifdef __WATCOMC__
#pragma aux fix_sqrt parm [eax] value [eax] modify [eax ebx ecx edx esi edi]
#endif

// Returns 0 if x < 0
fix fix_sloppy_sqrt (fix x);
#ifdef __WATCOMC__
#pragma aux fix_sloppy_sqrt parm [eax] value [eax] modify [eax ebx ecx edx esi edi]
#endif

// Computes sin and cos of theta
void fix_sincos (fixang theta, fix *sin, fix *cos);

fix fix_sin (fixang theta);

fix fix_cos (fixang theta);

// Computes sin and cos of theta
// Faster than fix_sincos() but not as accurate (does not interpolate)
void fix_fastsincos (fixang theta, fix *sin, fix *cos);

fix fix_fastsin (fixang theta);

fix fix_fastcos (fixang theta);

// Computes the arcsin of x
fixang fix_asin (fix x);

// Computes the arccos of x
fixang fix_acos (fix x);

// Computes the atan of y/x, in the correct quadrant and everything
fixang fix_atan2 (fix y, fix x);


//	Converts string into fixed-point
fix atofix(char *p);

// Puts a decimal representation of x into str
char *fix_sprint (char *str, fix x);
char *fix_sprint_hex (char *str, fix x);

/* fixpoint x ^ y */
extern fix fix_pow (fix x, fix y);

//////////////////////////////
//
// f_exp.c

// Computes e to the x
//
fix fix_exp (fix x);

//////////////////////////////
//
// fix24 - 24 bits integer, 8 bits fraction
//
// fix24.c

typedef long fix24;

#define fix24_make(a,b) ((fix24) ((((long)(a))<<8)|(b)))
#define fix24_trunc(n) ((fix24) ((n)&0xffffff00))
#define fix24_round(n) ((fix24) (((n)+128)&0xffffff00))
#define fix24_int(n) ((n)>>8)
#define fix24_rint(n) (((n)+128)>>8)
#define fix24_frac(n) ((fix24) ((n)&0xff))
#define fix24_float(n) ((float)(fix24_int(n)) + (float)(fix24_frac(n))/256.0)
#define fix24_from_float(n) (fix24_make((short)(floor(n)), (ushort)((n-floor(n))*256.0)))

#define fix24_from_fix16(n) ((fix24) ((n)>>8))
#define fix16_from_fix24(n) ((fix24) ((n)<<8))

#define fix16_to_fix24(n) (fix24_from_fix16(n))
#define fix24_to_fix16(n) (fix16_from_fix24(n))

#ifdef __WATCOMC__
fix24 fix24_mul (fix24 a, fix24 b);
#pragma aux fix24_mul =\
   "imul    edx"     \
   "shr     eax,8"  \
   "shl     edx,24"  \
   "or      eax,edx" \
   parm [eax] [edx]  \
   modify [eax edx];
#else
__inline fix24 fix24_mul (fix24 a, fix24 b)
{
	#pragma warning(disable : 4035)	// disables no return value warning
	__asm
	{
		mov		eax, a
		mov		edx, b
		imul	edx
		shr		eax, 8
		shl		edx, 24
		or		eax, edx
	}
}
#endif

#ifdef __WATCOMC__
fix24 fix24_div (fix24 a, fix24 b);
#pragma aux fix24_div =\
   "mov     edx,eax" \
   "sar     edx,24"  \
   "shl     eax,8"  \
   "idiv    ebx"     \
   parm [eax] [ebx]  \
   modify [eax edx];
#else
__inline fix24 fix24_div (fix24 a, fix24 b)
{
	#pragma warning(disable : 4035)	// disables no return value warning
	__asm
	{
		mov		eax, a
		mov		ebx, b
		mov		edx, eax
		sar		edx, 24
		shl		eax, 8
		idiv	ebx
	}
}
#endif

#ifdef __WATCOMC__
fix24 fix24_mul_div (fix24 m0, fix24 m1, fix24 d);
#pragma aux fix24_mul_div =\
   "imul    edx"     \
   "idiv    ebx"     \
   parm [eax] [edx] [ebx]  \
   modify [eax edx];
#else
__inline fix24 fix24_mul_div (fix24 m0, fix24 m1, fix24 d)
{
	#pragma warning(disable : 4035)	// disables no return value warning
	__asm
	{
		mov		eax, m0
		mov		edx, m1
		mov		ebx, d
		imul	edx
		idiv	ebx
	}
}
#endif

fix24 fix24_pyth_dist (fix24 a, fix24 b);
fix24 fix24_fast_pyth_dist (fix24 a, fix24 b);
fix24 fix24_safe_pyth_dist (fix24 a, fix24 b);

fix24 fix24_sqrt (fix24 x);
#ifdef __WATCOMC__
#pragma aux fix24_sqrt parm [eax] value [eax] modify [eax ebx ecx edx esi edi]
#endif

void fix24_sincos (fixang theta, fix24 *sin, fix24 *cos);
fix24 fix24_sin (fixang theta);
fix24 fix24_cos (fixang theta);
void fix24_fastsincos (fixang theta, fix24 *sin, fix24 *cos);
fix24 fix24_fastsin (fixang theta);
fix24 fix24_fastcos (fixang theta);
fixang fix24_asin (fix24 x);
fixang fix24_acos (fix24 x);
fixang fix24_atan2 (fix24 y, fix24 x);
fix24 atofix24(char *p);
char *fix24_sprint (char *str, fix24 x);
char *fix24_sprint_hex (char *str, fix24 x);

#ifdef __cplusplus
}
#endif //cplusplus

#endif /* !__fix24_H */
