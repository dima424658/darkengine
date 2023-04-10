// $Header: x:/prj/tech/libsrc/g2/RCS/mathmac.h 1.4 1996/11/07 14:33:12 KEVIN Exp $

#ifndef __MATHMAC_H
#define __MATHMAC_H

#include <fix.h>

#define FIX_HALF ((fix )0x8000)

typedef struct {
   union {
      fix f;
      ulong ul;
   };
} fix_ulong;

/* returns -1 if there's a carry. */
#ifdef __WATCOMC__
long ulong_add_get_carry(ulong *a, ulong b);
#pragma aux ulong_add_get_carry = \
   "add  [edx],eax" \
   "sbb  eax,eax"   \
   parm [edx] [eax] \
   modify [eax];
#else
__inline long ulong_add_get_carry(ulong *a, ulong b)
{
	__asm
	{
		mov		edx, a
		mov		eax, b
		add		[edx], eax
		sbb		eax, eax
	}
}
#endif

/* Deterministicaly multiplies by second operand.  Put smaller operand second
   for improved 486 performance.  Don't bother for Pentium. */
#ifdef __WATCOMC__
fix smart_imul(fix a, long b);
#pragma aux smart_imul = \
   "imul edx"        \   
   parm [eax] [edx]  \
   modify [eax];
#else
__inline fix smart_imul(fix a, long b)
{
	__asm
	{
		mov		eax, a
		mov		edx, b
		imul	edx   
	}
}
#endif

#endif
