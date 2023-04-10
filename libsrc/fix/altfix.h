/*
 * $Source: x:/prj/tech/libsrc/fix/RCS/altfix.h $
 * $Revision: 1.9 $
 * $Author: TOML $
 * $Date: 1996/10/14 10:44:58 $
 * 
 * 3 and 8 bit integer portion fixed point macros.
 *
 * This file is part of the 2d library.
 *
*/

#ifndef __ALTFIX_H
#define __ALTFIX_H
#include <fix.h>

/* This file contains inline functions and macros for */
/* fixed point numbers with nonstandard numbers of    */
/* bits of integer precision.                         */
/* These functions and macros are all of the form:    */
/* [operation]_[op1]_{[op2]_[op3]}_[result],          */
/* where the numbers indicate the number of bits of   */
/* integer in each fixed point number.                */
/* Thus fix_mul_div_8_16_3_16() takes an (8,24) fixed */
/* point number, multiplies it by a (16,16), divides  */
/* by a (3,29), and returns a (16,16).  Recall that   */
/* regular fixed point numbers from fix.h are (16,16) */

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus

#define FIX_UNIT_3 ((fix )0x20000000)

// i = 32+16-8-16 = 24
#ifdef __WATCOMC__
fix fix_mul_8_16_16 (fix a, fix b);
#pragma aux fix_mul_8_16_16 =   \
   "imul    edx"              \
   "shr     eax,24"           \
   "shl     edx,8"            \
   "or      eax,edx"          \
   parm [eax] [edx]           \
   modify [eax edx];
#else
__inline fix fix_mul_8_16_16 (fix a, fix b)
{
	__asm
	{
		mov		eax, a
		mov		edx, b
		imul	edx
		shr		eax, 24
		shl		edx, 8
		or		eax, edx
	}
}
#endif

// i = 32+3-3-3 = 29
#ifdef __WATCOMC__
fix fix_mul_3_3_3 (fix a, fix b);
#pragma aux fix_mul_3_3_3 =   \
   "imul    edx"              \
   "shr     eax,29"           \
   "shl     edx,3"            \
   "or      eax,edx"          \
   parm [eax] [edx]           \
   modify [eax edx];
#else
__inline fix fix_mul_3_3_3 (fix a, fix b)
{
	__asm
	{
		mov		eax, a
		mov		edx, b
		imul	edx	
		shr		eax, 29	
		shl		edx, 3	
		or		eax, edx	
	}
}
#endif

// i = 32+16-3-16 = 29
#define fix_mul_3_16_16 fix_mul_3_3_3

// i = 32+8-3-8 = 29
#define fix_mul_3_8_8 fix_mul_3_3_3

// i = 32+16-32-3 = 13
#ifdef __WATCOMC__
fix fix_mul_3_32_16 (fix a, fix b);
#pragma aux fix_mul_3_32_16 = \
   "imul    edx"              \
   "shr     eax,13"           \
   "shl     edx,19"           \
   "or      eax,edx"          \
   parm [eax] [edx]           \
   modify [eax edx];
#else
__inline fix fix_mul_3_32_16 (fix a, fix b)
{
	__asm
	{
		mov		eax, a
		mov		edx, b
		imul	edx
		shr		eax, 13
		shl		edx, 19
		or		eax, edx
	}
}
#endif

// i = 32+20-16-3 = 33(!)
#ifdef __WATCOMC__
fix fix_mul_3_16_20 (fix a, fix b);
#pragma aux fix_mul_3_16_20 = \
   "imul    edx"              \
   "sar     edx,1"            \
   "mov     eax,edx"          \
   parm [eax] [edx]           \
   modify [eax edx];
#else
__inline fix fix_mul_3_16_20 (fix a, fix b)
{
	__asm
	{
		mov		eax, a
		mov		edx, b
		imul	edx
		sar		edx, 1
		mov		eax, edx
	}
}
#endif

// i = 32+20-16-32 = 4
#ifdef __WATCOMC__
fix fix_mul_16_32_20 (fix a, fix b);
#pragma aux fix_mul_16_32_20 = \
   "imul    edx"              \
   "shr     eax,4"            \
   "shl     edx,28"           \
   "or      eax,edx"          \
   parm [eax] [edx]           \
   modify [eax edx];
#else
__inline fix fix_mul_16_32_20 (fix a, fix b)
{
	__asm
	{
		mov		eax, a
		mov		edx, b
		imul	edx
		shr		eax, 4
		shl		edx, 28
		or		eax, edx
	}
}
#endif

// i = 32+16-16-3 = 29
#ifdef __WATCOMC__
fix fix_div_16_16_3_fast (fix a, fix b);
#pragma aux fix_div_16_16_3_fast = \
   "mov     edx,eax"    \
   "sar     edx,3"      \
   "shl     eax,29"     \
   "idiv    ebx"        \
   parm     [eax] [ebx] \
   modify   [eax edx];
#else
__inline fix fix_div_16_16_3_fast (fix a, fix b)
{
	__asm
	{
		mov		eax, a
		mov		ebx, b
		mov		edx, eax
		sar		edx, 3
		shl		eax, 29
		idiv	ebx
	}
}
#endif
// Jacobson, 2-7-96
// Here we added an additional subtraction to create the lower rail
// at 0x80000001. We're avoiding 0x80000000 because that number doesn't
// have an associated positive number.

// Jacobson, 2-27-96
// We're shifting edx by 2 correspond to a factor of two
// multiplication to prevent precision errors that would occur
// by shifting it 3 and then back one

#ifdef __WATCOMC__
fix fix_div_16_16_3_safe (fix a, fix b);
#pragma aux fix_div_16_16_3_safe = \
   "push    ebx"        \
   "mov     edx,eax"    \
   "sar     edx,2"      \
   "shl     eax,29"     \
   "test    ebx,ebx"    \
   "push    edx"        \
   "jns     ebx_ok"     \
   "neg     ebx"        \
"ebx_ok:"               \
   "test    edx,edx"    \
   "jns     edx_ok"     \
   "neg     edx"        \
"edx_ok:"               \
   "cmp     ebx,edx"    \
   "pop     edx"        \
   "pop     ebx"        \
   "ja      no_overflow"   \
   "sar     edx,31"     \
   "sar     ebx,31"     \
   "xor     edx,ebx"    \
   "mov     eax,0x7fffffffh" \
   "sub     eax,edx"    \
   "sub     eax,edx"    \
   "jmp     done"       \
"no_overflow:"          \
   "sar     edx,1"      \
   "idiv    ebx"        \
"done:"                 \
   parm     [eax] [ebx] \
   modify   [eax edx];
#else
__inline fix fix_div_16_16_3_safe (fix a, fix b)
{
	__asm
	{
		mov		eax, a
		mov		ebx, b
		push	ebx
		mov		edx, eax
		sar		edx, 2
		shl		eax, 29
		test	ebx, ebx
		push	edx
		jns		ebx_ok
		neg		ebx
	ebx_ok:
		test	edx,edx
		jns		edx_ok
		neg		edx
	edx_ok:
		cmp		ebx, edx
		pop		edx
		pop		ebx
		ja		no_overflow
		sar		edx, 31
		sar		ebx, 31
		xor		edx, ebx
		mov		eax, 0x7fffffff
		sub		eax, edx
		sub		eax, edx
		jmp		done
	no_overflow:
		sar		edx, 1
		idiv    ebx
	done:
	}
}
#endif

// i = 32+16-8-16 = 24
#ifdef __WATCOMC__
fix fix_div_16_8_16_fast (fix a, fix b);
#pragma aux fix_div_16_8_16_fast = \
   "mov     edx,eax"    \
   "sar     edx,8"      \
   "shl     eax,24"     \
   "idiv    ebx"        \
   parm     [eax] [ebx] \
   modify   [eax edx];
#else
__inline fix fix_div_16_8_16_fast (fix a, fix b)
{
	__asm
	{
		mov		eax, a
		mov		ebx, b
		mov		edx, eax
		sar		edx, 8
		shl		eax, 24
		idiv	ebx
	}
}
#endif

// Jacobson, 2-7-96
// Here we added an additional subtraction to create the lower rail
// at 0x80000001. We're avoiding 0x80000000 because that number doesn't
// have an associated positive number.

// Jacobson, 2-27-96
// We're shifting edx by 7 correspond to a factor of two
// multiplication to prevent precision errors that would occur
// by shifting it 8 and then back one

#ifdef __WATCOMC__
fix fix_div_16_8_16_safe (fix a, fix b);
#pragma aux fix_div_16_8_16_safe = \
   "push    ebx"        \
   "mov     edx,eax"    \
   "sar     edx,7"      \
   "shl     eax,24"     \
   "test    ebx,ebx"    \
   "push    edx"        \
   "jns     ebx_ok"     \
   "neg     ebx"        \
"ebx_ok:"               \
   "test    edx,edx"    \
   "jns     edx_ok"     \
   "neg     edx"        \
"edx_ok:"               \
   "cmp     ebx,edx"    \
   "pop     edx"        \
   "pop     ebx"        \
   "ja      no_overflow"   \
   "sar     edx,31"     \
   "sar     ebx,31"     \
   "xor     edx,ebx"    \
   "mov     eax,0x7fffffffh" \
   "sub     eax,edx"    \
   "sub     eax,edx"    \
   "jmp     done"       \
"no_overflow:"          \
   "sar     edx,1"      \
   "idiv    ebx"        \
"done:"                 \
   parm     [eax] [ebx] \
   modify   [eax edx];
#else
__inline fix fix_div_16_8_16_safe (fix a, fix b)
{
	__asm
	{
		mov		eax, a
		mov		ebx, b
		push	ebx
		mov		edx, eax
		sar		edx, 7
		shl		eax, 24
		test	ebx, ebx
		push	edx
		jns		ebx_ok
		neg		ebx
	ebx_ok:
		test	edx, edx
		jns		edx_ok
		neg		edx
	edx_ok:
		cmp		ebx, edx
		pop		edx
		pop		ebx
		ja		no_overflow
		sar		edx, 31
		sar		ebx, 31
		xor		edx, ebx
		mov		eax, 0x7fffffff
		sub		eax, edx
		sub		eax, edx
		jmp		done
	no_overflow:
		sar		edx, 1
		idiv	ebx
	done:
	}
}
#endif

// i = 32+16-32-3 = 13
#ifdef __WATCOMC__
fix fix_div_16_3_32_fast (fix a, fix b);
#pragma aux fix_div_16_3_32_fast = \
   "mov     edx,eax"    \
   "sar     edx,19"     \
   "shl     eax,13"     \
   "idiv    ebx"        \
   parm     [eax] [ebx] \
   modify   [eax edx];
#else
__inline fix fix_div_16_3_32_fast (fix a, fix b)
{
	__asm
	{
		mov		eax, a
		mov		ebx, b
		mov		edx,eax
		sar		edx,19
		shl		eax,13
		idiv	ebx
	}
}
#endif

// Jacobson, 2-7-96
// Here we added an additional subtraction to create the lower rail
// at 0x80000001. We're avoiding 0x80000000 because that number doesn't
// have an associated positive number.

#ifdef __WATCOMC__
fix fix_div_16_3_32_safe (fix a, fix b);
#pragma aux fix_div_16_3_32_safe = \
   "push    ebx"        \
   "mov     edx,eax"    \
   "sar     edx,18"     \
   "shl     eax,13"     \
   "test    ebx,ebx"    \
   "push    edx"        \
   "jns     ebx_ok"     \
   "neg     ebx"        \
"ebx_ok:"               \
   "test    edx,edx"    \
   "jns     edx_ok"     \
   "neg     edx"        \
"edx_ok:"               \
   "cmp     ebx,edx"    \
   "pop     edx"        \
   "pop     ebx"        \
   "ja      no_overflow"   \
   "sar     edx,31"     \
   "sar     ebx,31"     \
   "xor     edx,ebx"    \
   "mov     eax,0x7fffffffh" \
   "sub     eax,edx"    \
   "sub     eax,edx"    \
   "jmp     done"       \
"no_overflow:"          \
   "sar     edx,1"	\
   "idiv    ebx"        \
"done:"                 \
   parm     [eax] [ebx] \
   modify   [eax edx];
#else
__inline fix fix_div_16_3_32_safe (fix a, fix b)
{
	__asm
	{
		mov		eax, a
		mov		ebx, b
		push	ebx
		mov		edx, eax
		sar		edx, 18
		shl		eax, 13
		test	ebx, ebx
		push	edx
		jns		ebx_ok
		neg		ebx
	ebx_ok:
		test	edx, edx
		jns		edx_ok
		neg		edx
	edx_ok:
		cmp		ebx, edx
		pop		edx
		pop		ebx
		ja		no_overflow
		sar		edx, 31
		sar		ebx, 31
		xor		edx, ebx
		mov		eax, 0x7fffffff
		sub		eax, edx
		sub		eax, edx
		jmp		done
	no_overflow:
		sar		edx, 1
		idiv	ebx
	done:
	}
}
#endif

#ifndef WIN32
#define fix_div_16_16_3 fix_div_16_16_3_fast
#define fix_div_16_3_32 fix_div_16_3_32_fast
#define fix_div_16_8_16 fix_div_16_8_16_fast
#endif

#ifdef WIN32
#define fix_div_16_16_3 fix_div_16_16_3_safe
#define fix_div_16_3_32 fix_div_16_3_32_safe
#define fix_div_16_8_16 fix_div_16_8_16_safe
#endif

// i = 32+3-8-3 = 24
#define fix_div_3_8_3 fix_div_16_8_16
// i = 32+8-8-8 = 24
#define fix_div_8_8_8 fix_div_16_8_16

// i = 32+16-3-16 = 29
#define fix_div_16_3_16 fix_div_16_16_3

// i = 32+3-3-16 = 16
#define fix_div_3_3_16 fix_div
// i = 32+3-16-3 = 16
#define fix_div_3_16_3 fix_div

#define fix_mul_div_3_16_16_3 fix_mul_div
#define fix_mul_div_3_16_3_16 fix_mul_div
#define fix_mul_div_3_8_8_3 fix_mul_div

#define fix_sar(a,b) ((a)>>(b))

#define fix_3_12(a) ((a)>>9)
#define fix_12_16(a) ((a)>>4)
#define fix_3_16(a) ((a)>>13)
#define fix_3_8(a) ((a)>>5)

#ifdef __cplusplus
}
#endif //cplusplus

#endif