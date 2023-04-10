// $Header: x:/prj/tech/libsrc/fix/RCS/safefix.c 1.1 1997/04/24 13:59:58 TOML Exp $

#include <lg.h>
#include <fix.h>
#ifdef WARN_ON
#include <string.h>
#endif

// note:
//  we dont include safefix.h
//  this is so we always get the real fix_div, fix_mul, and so on
//  we want that, so we know whether we are calling
//    fix_div_fast or fix_div_safe, for instance

// i suppose we could make all the _full versions in a warn_on
// and then in the .h #def them from the other versions

//////////////////
// shorten long names
#ifdef WARN_ON
static const char * short_name(const char * p)
{
   // Trim out any path from the file name
   const char * pShortName;
   if ((pShortName = strrchr(p, '\\')) != 0)
      pShortName++;
   else
      pShortName = p;
   return pShortName;
}
#endif


//////////////////
// safe_fix_from_float
// test for overflow and warn and clamp

fix safe_fix_from_float(float n)
{
   if ((n<fix_int(FIX_MAX)&&(n>fix_int(FIX_MIN))))
      return fix_from_float(n);
   Warning(("Overflow in safe_fix_from_float() on %g!\n",n));
   return (n>0) ? FIX_MAX : FIX_MIN;
}

#ifdef WARN_ON
fix safe_fix_from_float_full(float n, char *file, int line)
{
   if ((n<fix_int(FIX_MAX)&&(n>fix_int(FIX_MIN))))
      return fix_from_float(n);
   Warning(("Overflow in safe_fix_from_float() on %g, %s line %d!\n",n,short_name(file),line));
   return (n>0) ? FIX_MAX : FIX_MIN;
}
#endif

// kevins version doesnt work under MSVC at all
// fix from float simply goes ahead and generates the truncated value
// perhaps it does work under WATCOM, so we could ifdef it, since his is faster than mine
#ifdef KEVINS_VERSION
// this is what fist returns when float is out of range...
#define INT_FROM_FLOAT_OVERFLOW 0x80000000

fix safe_fix_from_float(float n)
{
   fix retval;   
   retval = fix_from_float(n);
   if (retval!=INT_FROM_FLOAT_OVERFLOW)
      return retval;
   Warning(("Overflow in safe_fix_from_float() on %g!\n",n));
   return (n>0) ? FIX_MAX : FIX_MIN;
}

#ifdef WARN_ON
fix safe_fix_from_float_full(float n, char *file, int line)
{
   fix retval;
   retval = fix_from_float(n);
   if (retval!=INT_FROM_FLOAT_OVERFLOW)
      return retval;
   Warning(("Overflow: safe_fix_from_float(%g)- %s line %d!\n",n,short_name(file),line));
   return (n>0) ? FIX_MAX : FIX_MIN;
}
#endif
#endif

//////////////////////////////
// safe_fix_mul

char mul_test;

fix safe_fix_mul (fix a, fix b)
{
   fix ret = safe_fix_mul_asm (a, b);
   if (mul_test)
      Warning (("Overflow in fix_mul of %x by %x!\n",a,b));
   return ret;
}

#ifdef WARN_ON
fix safe_fix_mul_full (fix a, fix b, char *file, int line)
{
   fix ret = safe_fix_mul_asm (a, b);
   if (mul_test)
      Warning (("Overflow in fix_mul of %x by %x, %s line %d!\n",a,b,short_name(file),line));
   return ret;
}
#endif

//////////////////////////////
// safe_fix_div

fix safe_fix_div(fix a, fix b)
{
   if (fix_div_overflow(a,b))
      Warning(("Div of %x by %x about to overflow!\n",a,b));
   return fix_div(a,b);
}

#ifdef WARN_ON
fix safe_fix_div_full(fix a, fix b, char *file, int line)
{
   if (fix_div_overflow(a,b))
      Warning(("Div of %x by %x about to overflow!, %s line %d\n",a,b,short_name(file),line));
   return fix_div(a,b);
}
#endif
