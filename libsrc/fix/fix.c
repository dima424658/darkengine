// $Header: x:/prj/tech/libsrc/fix/RCS/fix.c 1.28 1997/04/24 13:59:30 TOML Exp $

#include <stdlib.h>
#include <stdio.h>
#include <lg.h>
#include <fix.h>
#include <trigtab.h>

//////////////////////////////
//
// Returns the distance from (0,0) to (a,b)
//

fix fix_pyth_dist (fix a, fix b)
{  // should check for overflow!
   return fix_sqrt (fix_mul (a, a) + fix_mul (b, b));
}

//////////////////////////////
//
// Returns an approximation to the distance from (0,0) to (a,b)
//
fix fix_fast_pyth_dist (fix a, fix b)
{
   if (a < 0) a = -a;
   if (b < 0) b = -b;
   if (a > b) return (a + b/2);
         else return (b + a/2);
}


//////////////////////////////
//
// We can use the fix function because the difference in scale doesn't matter.
//

long long_fast_pyth_dist (long a, long b)
{
   return (fix_fast_pyth_dist (a, b));
}

//////////////////////////////
//
// This function is safer than the other fix_pyth_dist because we don't
// have to worry about overflow.
//
// Uses algorithm from METAFONT involving reflecting (a,b) through
// line from (0,0) to (a,b/2), which keeps a^2+b^2 invariant but
// greatly reduces b.  When b reaches 0, a is the distance.
//
// Knuth credits it to Moler & Morrison, IBM Journal of Research and
// Development 27 (1983).  Good for them.
//
#ifdef __WATCOMC__
#pragma off(unreferenced)
#endif
fix fix_safe_pyth_dist_info (fix a, fix b, char *file, int line)
{
   fix save_a=a, save_b=b;
   fix tmp;
         
   a = abs(a);
   b = abs(b);               // works fine since they're really longs
                             //   whoever wrote the above comment
                             //   apparently never heard of labs -- STB
   if (a < b)
   {
      tmp = a; a = b; b = tmp;
   }                                    // now 0 <= b <= a

   if (a > 0)
   {
      if (a > 0x2fffffff)
      {
         Warning (("Overflow in fix_safe_pyth_dist:"));
         // only print file, line if file!=NULL
         Assert (file==NULL, (" %s %d", file, line));
         Warning(("\nArguments were %f %f\n", save_a, save_b));
         return 0;
      }
      for (;;)
      {
         // This is a quick way of doing the reflection
         tmp = fix_div (b, a);
         tmp = fix_mul (tmp, tmp);
         if (tmp == 0) break;
         tmp = fix_div (tmp, tmp + fix_make(4,0));
         a += fix_mul (2*a, tmp);
         b = fix_mul (b, tmp);
      }
   }
   return a;
}
#ifdef __WATCOMC__
#pragma on(unreferenced)
#endif

fix fix_sqrt(fix x)
{
    return 0; // TODO
}

//////////////////////////////
//
// We can use the fix function because the difference in scale doesn't matter.
//

long long_safe_pyth_dist (long a, long b)
{
   return (fix_safe_pyth_dist (a, b));
}

//////////////////////////////
//
// Computes sin and cos of theta
//
void fix_sincos (fixang theta, fix *sin, fix *cos)
{
   uchar baseth, fracth;               // high and low bytes of th
   ushort lowsin, lowcos, hisin, hicos; // table lookups

   // divide the angle into high and low bytes
   // we will do a table lookup with the high byte and
   // interpolate with the low byte
   baseth = theta >> 8;
   fracth = theta & 0xff;
   
   // use the identity [cos x = sin (x + PI/2)] to look up
   // cosines in the sine table
   lowsin = sintab[baseth];
   hisin = sintab[baseth + 1];
   lowcos = sintab[baseth + 64];
   hicos = sintab[baseth + 65];
   
   // interpolate between low___ and hi___ according to fracth
   *sin = ((short) (lowsin + (short) (( ( (short) hisin - (short) lowsin ) * (uchar) fracth ) >> 8))) << 2;
   *cos = ((short) (lowcos + (short) (( ( (short) hicos - (short) lowcos ) * (uchar) fracth ) >> 8))) << 2;

   return;
}

//////////////////////////////
//
// Computes sin of theta
//
fix fix_sin (fixang theta)
{
   uchar baseth, fracth;
   ushort lowsin, hisin;

   baseth = theta >> 8;
   fracth = theta & 0xff;
   lowsin = sintab[baseth];
   hisin = sintab[baseth + 1];
   return ((short) (lowsin + (short) (( ( (short) hisin - (short) lowsin ) * (uchar) fracth ) >> 8))) << 2;
}

//////////////////////////////
//
// Computes cos of theta
//
fix fix_cos (fixang theta)
{
   uchar baseth, fracth;
   ushort lowcos, hicos;

   baseth = theta >> 8;
   fracth = theta & 0xff;
   lowcos = sintab[baseth + 64];
   hicos = sintab[baseth + 65];
   return ((short) (lowcos + (short) (( ( (short) hicos - (short) lowcos ) * (uchar) fracth ) >> 8))) << 2;
}

//////////////////////////////
//
// Computes sin and cos of theta
// Faster than fix_sincos() but not as accurate (does not interpolate)
//
void fix_fastsincos (fixang theta, fix *sin, fix *cos)
{
   // use the identity [cos x = sin (x + PI/2)] to look up
   // cosines in the sine table
   *sin = (((short) (sintab[theta >> 8])) << 2);
   *cos = (((short) (sintab[(theta >> 8) + 64])) << 2);

   return;
}

//////////////////////////////
//
// Fast sin of theta
//
fix fix_fastsin (fixang theta)
{
   return (((short) (sintab[theta >> 8])) << 2);
}

//////////////////////////////
//
// Fast cos of theta
//
fix fix_fastcos (fixang theta)
{
   return (((short) (sintab[(theta >> 8) + 64])) << 2);
}


//////////////////////////////
//
// Computes the arcsin of x
// Assumes -1 <= x <= 1
// Returns 0xc000..0x4000 (-PI/2..PI/2)
//
fixang fix_asin (fix x)
{
   uchar basex, fracx;                        // high and low bytes of x
   fixang lowy, hiy;                     // table lookups

   // divide x into high and low bytes
   // lookup with the high byte, interpolate with the low
   // We shift basex around to make it continuous; see trigtab.h

   basex = ((x >> 2) >> 8) + 0x40;
   fracx = (x >> 2) & 0xff;

   lowy = asintab[basex];
   hiy = asintab[basex+1];

   // interpolate between lowy and hiy according to fracx
   return (lowy + (short) (( ( (short) hiy - (short) lowy ) * (uchar) fracx ) >> 8));
}



//////////////////////////////
//
// Computes the arccos of x
// Returns 0x0000..0x8000 (0..PI)
//
fixang fix_acos (fix x)
{
   uchar basex, fracx;
   ushort lowy, hiy;
   fixang asin_answer;

   // acos(x) = PI/2 - asin(x)

   basex = ((x >> 2) >> 8) + 0x40;
   fracx = (x >> 2) & 0xff;

   lowy = asintab[basex];
   hiy = asintab[basex+1];

   asin_answer = (lowy + (short) (( ( (short) hiy - (short) lowy ) * (uchar) fracx ) >> 8));
   return (0x4000 - asin_answer);
}

//////////////////////////////
//
// Computes the atan of y/x, in the correct quadrant and everything
//
fixang fix_atan2 (fix y, fix x)
{
   fix hyp;                              // hypotenuse
   fix s, c;                           // sine, cosine
   fixang th;                           // our answer


   // Get special cases out of the way so we don't have to deal
   // with things like making sure 1 gets converted to 0x7fff and
   // not 0x8000.  Note that we grab the y = x = 0 case here
   if (y == 0)
   {
      if (x >= 0) return 0x0000;
      else return 0x8000;
   }
   else if (x == 0)
   {
      if (y >= 0) return 0x4000;
      else return 0xc000;
   }


   if ((hyp = fix_safe_pyth_dist (x, y)) == 0)
   {
//      printf ("hey, dist was 0\n");
      Warning (("hey, dist was 0\n"));
      return 0;
   }

   // Use fix_asin or fix_acos depending on where we are.  We don't want to use
   // fix_asin if the sin is close to 1 or -1
   s = fix_div (y, hyp);
   if ((ulong) s < 0x00004000 || (ulong) s > 0xffffc000)
   {                                    // range is good, use asin
      th = fix_asin (s);
      if (x < 0)
      {
         if (th < 0x4000) th = 0x8000 - th;
         else th = ~th + 0x8000;         // that is, 0xffff - th + 0x8000
      }
   }
   else
   {                                    // use acos instead
      c = fix_div (x, hyp);
      th = fix_acos (c);
      if (y < 0)
      {
         th = ~th;                     // that is, 0xffff - th
      }
   }

// The above (x < 0) and (y < 0) conditionals should take care of placing us in
// the correct quadrant, so we shouldn't need the code below.  Additionally,
// the code below can cause rounding errors when (th & 0x3fff == 0).  So let's
// try omitting it.

#ifdef NO_NEED
   // set high bits based on what quadrant we are in
   th &= 0x3fff;
   th |= (y > 0 ? ( x > 0 ? 0x0000 : 0x4000)
                : ( x > 0 ? 0xc000 : 0x8000));
#endif

   return th;
}

fix fix_div_safe(fix a, fix b)
{
    return 0; // TODO
}

fix fix_mul_div_safe(fix m0, fix m1, fix d)
{
    return 0; // TODO
}