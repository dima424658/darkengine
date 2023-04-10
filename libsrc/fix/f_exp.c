/*
** f_exp.c
**
** fix_exp
**
** $Header: r:/prj/lib/src/fix/rcs/f_exp.c 1.1 1994/08/11 12:12:16 dfan Exp $
** $Log: f_exp.c $
 * Revision 1.1  1994/08/11  12:12:16  dfan
 * Initial revision
 * 
*/

#include <fix.h>
#include <trigtab.h>

//////////////////////////////
//
// returns e to the x
// does no range checking whatsoever
//
fix fix_exp (fix x)
{
	int int_part = fix_int (x);
	fix exp_int_part;
	int basex, fracx;
	fix loy, hiy;
	fix exp_frac_part;

   // If our exponent is so small that it goes off the small end of the table,
   // just return 0.

   if (int_part + INTEGER_EXP_OFFSET < 0) return 0;

   exp_int_part = expinttab[int_part + INTEGER_EXP_OFFSET];

	basex = fix_frac (x) >> 12;
	fracx = x & 0x0fff;

	loy = expfractab[basex];
	hiy = expfractab[basex+1];

	exp_frac_part = loy + (hiy - loy) * fracx / 0x1000;
	return (fix_mul (exp_int_part, exp_frac_part));
}
