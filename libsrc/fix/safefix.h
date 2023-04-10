// $Header: x:/prj/tech/libsrc/fix/RCS/safefix.h 1.1 1997/04/24 13:59:31 TOML Exp $

#ifndef __SAFEFIX_H
#define __SAFEFIX_H

// at the bottom is the stuff to make it happen automatically if you just flip
//   the USE_SAFE_FIXES define in your flags.mh or what have you
// or of course you can just call the safe versions yourself by hand
// or you can #define USE_SAFE_FIXES in the file you want to do safely prior
//   to including safefix.h

// note this is not all the crazy fix_mul and fix_div_safe stuff
// that is all seperate, mostly in fix.h, and so on

// currently we include fix.h, since there is no fixs.h, sadly
#include <fix.h>

fix safe_fix_from_float(float n);
fix safe_fix_mul (fix a, fix b);
fix safe_fix_div (fix a, fix b);

#ifdef WARN_ON
fix safe_fix_from_float_full(float n, char *file, int line);
fix safe_fix_mul_full (fix a, fix b, char *file, int line);
fix safe_fix_div_full(fix a, fix b, char *file, int line);
#else
#define safe_fix_from_float_full(n,file,line) safe_fix_from_float(n)
#define safe_fix_mul_full(a,b,file,line)      safe_fix_mul(a,b)
#define safe_fix_div_full(a,b,file,line)      safe_fix_div(a,b)
#endif


#ifdef USE_SAFE_FIXES

#ifdef fix_mul
#undef fix_mul
#endif

#ifdef WARN_ON
#define fix_mul(a,b) safe_fix_mul_full(a,b,__FILE__,__LINE__)
#else
#define fix_mul(a,b) safe_fix_mul(a,b)
#endif

#ifdef fix_div
#undef fix_div
#endif

#ifdef WARN_ON
#define fix_div(a,b) safe_fix_div_full(a,b,__FILE__,__LINE__)
#else
#define fix_div(a,b) safe_fix_div(a,b)
#endif

#ifdef fix_from_float
#undef fix_from_float
#endif

#ifdef WARN_ON
#define fix_from_float(n) safe_fix_from_float_full(n,__FILE__,__LINE__)
#else
#define fix_from_float(n) safe_fix_from_float(n)
#endif

#endif // USE_SAFE_FIXES

#endif // __SAFEFIX_H
