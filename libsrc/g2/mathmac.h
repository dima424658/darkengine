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
static __inline long ulong_add_get_carry(ulong *a, ulong b)
{
   ulong old = *a;
   *a += b;
   return *a < old ? -1 : 0;
}

/* Deterministicaly multiplies by second operand.  Put smaller operand second
   for improved 486 performance.  Don't bother for Pentium. */
__inline fix smart_imul(fix a, long b)
{
   return (fix)(a * b);
}

#endif
