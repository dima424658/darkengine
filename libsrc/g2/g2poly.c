/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/g2poly.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:29:03 $
 *
 * 2D polygon clipping routines.
 *
 */

#include <buffer.h>
#include <grply.h>
#include <g2clip.h>

static int do_poly(int c, int n, grs_vertex **vpl, int clip_type, g2up_func *f)
{
   grs_vertex **cvpl=NULL;
   int m, code;

   m=g2_clip_polygon(n, clip_type, vpl, &cvpl);
   if (m<3) {
      code = CLIP_ALL;
   } else {
      f(c, m, cvpl);
      code = CLIP_NONE;
   }
   gr_free_temp(cvpl);
   return code;
}

int gr_poly(int c, int n, grs_vertex **vpl)
{
   return do_poly(c, n, vpl, G2C_CLIP_NONE,
      gr_upoly_expose(c, n, vpl));
}

int gr_spoly(int c, int n, grs_vertex **vpl)
{
   return do_poly(c, n, vpl, G2C_CLIP_I,
      gr_uspoly_expose(c, n, vpl));
}

int gr_cpoly(int c, int n, grs_vertex **vpl)
{
   return do_poly(c, n, vpl, G2C_CLIP_RGB,
      gr_ucpoly_expose(c, n, vpl));
}
