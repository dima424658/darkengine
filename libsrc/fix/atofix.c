/*
** atofix.c
**
** $Header: r:/prj/lib/src/fix/rcs/atofix.c 1.2 1993/11/11 13:50:29 rex Exp $
** $Log: atofix.c $
 * Revision 1.2  1993/11/11  13:50:29  rex
 * Fixed bug in atofix24
 * 
 * Revision 1.1  1993/11/11  13:30:53  rex
 * Initial revision
 * 
 * 
*/

#include <stdlib.h>
#include <stdio.h>

#include <fix.h>

void fixgetab(char *p, int fracshift, int *a, int *b, int *sign);

//	----------------------------------------------------------
//		CONVERSION ROUTINES
//	----------------------------------------------------------
//
//	atofix() converts an ascii string into a fixed-point number

fix atofix(char *p)
{
	int a,b,sign;

	fixgetab(p,16,&a,&b,&sign);
	return(sign*fix_make(a,b));
}

//	----------------------------------------------------------
//
//	atofix24() converts an ascii string into a fix24

fix24 atofix24(char *p)
{
	int a,b,sign;

	fixgetab(p,8,&a,&b,&sign);
	return(sign*fix24_make(a,b));
}

//	-----------------------------------------------------------
//		INTERNAL ROUTINES
//	-----------------------------------------------------------
//
//	fixgetab() gets integer and fractional part from ascii buffer

static void fixgetab(char *p, int fracshift, int *a, int *b, int *sign)
{
	int divis;

//	sign is +1 or -1

	*sign = 1;
	if (*p == '-')
		{
		*sign = -1;
		++p;
		}

//	Get integer portion

	*a = *b = 0;
	while ((*p >= '0') && (*p <= '9'))
		*a = (*a * 10) + (*p++ - '0');

//	If period, get fractional portion

	if (*p == '.')
		{
		++p;
		divis = 10;
		while ((*p >= '0') && (*p <= '9'))
			{
			*b += ((*p++ - '0') << fracshift) / divis;
			divis *= 10;
			if (divis > 655360)
				break;
			}
		}
}