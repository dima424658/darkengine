// $Header: x:/prj/tech/libsrc/dev2d/RCS/clut16.c 1.4 1996/12/06 14:29:56 KEVIN Exp $
// Routines to construct 16 bit cluts.

#include <dbg.h>
#include <bitmap.h>
#include <alpha.h>

#define b_mask  0x1f
#define gl_mask 0xe0
//      gh_mask variable
//      r_mask  variable

#define b_max  0x1f
#define gl_max 0x7
//      gh_max variable
#define r_max  0x1f

//      b_shift  0
#define g_shift  5
#define gh_shift 3
//      r_shift  variable

void gr_init_clut16_vector(ushort *clut16, int color, alpha_vector *opacity, int format)
{
   int i;
   alpha_real r0, gl0, gh0, b0;
   alpha_vector alpha;

   int gh_mask, r_mask, gh_max;
   uchar r_shift;

   switch (format) {
   case BMF_RGB_565:
      gh_mask = 0x7;
      r_mask = 0xf8;

      gh_max = 0x38;

      r_shift = 11;

      break;

   case BMF_RGB_555:
      gh_mask = 0x3;
      r_mask = 0x7c;

      gh_max = 0x18;

      r_shift = 10;

      break;

   default:
      Warning(("gr_init_clut16(): unsupported format\n"));
      return;
   }

   for (i = 0; i<3; i++) {
      alpha_real a = 1.0 - opacity->v.el[i];
      if (a > 1.0) {
         Warning(("gr_init_clut16(): alpha out of range.\n"));
         a = 1.0;
      } else if (a < 0.0) {
         Warning(("gr_init_clut16(): alpha out of range.\n"));
         a = 0.0;
      }
      alpha.v.el[i] = a;
   }


   // red bits
   b0  =  (color& b_mask)               *opacity->blue;
   gl0 = ((color&gl_mask)>>g_shift)     *opacity->green;
   gh0 = ((color&(gh_mask<<8))>>g_shift)*opacity->green;
   r0  = ((color&( r_mask<<8))>>r_shift)*opacity->red;
   for (i=0; i<256; i++) {
      int r, gl, gh, b;

      b = b0 + (i&b_mask)*alpha.blue;
      if (b>b_max) b = b_max;
      gl = gl0 + ((i&gl_mask)>>g_shift)*alpha.green;
      if (gl>gl_max) gl = gl_max;
      clut16[i] = (gl<<g_shift) + b;

      gh = gh0 + ((i&gh_mask)<<gh_shift)*alpha.green;
      if (gh>gh_max) gh = gh_max;
      r = r0 + ((i&r_mask)>>(r_shift-8))*alpha.red;
      if (r>r_max) r = r_max;
      clut16[256+i] = (r<<r_shift) + (gh<<g_shift);
   }
}

void gr_init_clut16(ushort *clut16, int color, alpha_real opacity, int format)
{
   alpha_vector opacity_vector;
   opacity_vector.red = 
   opacity_vector.green = 
   opacity_vector.blue = opacity;
   gr_init_clut16_vector(clut16, color, &opacity_vector, format);
}

void gr_init_clut565(ushort *clut16, int color, alpha_real opacity)
{
   gr_init_clut16(clut16, color, opacity, BMF_RGB_565);
}

void gr_init_clut555(ushort *clut16, int color, alpha_real opacity)
{
   gr_init_clut16(clut16, color, opacity, BMF_RGB_555);
}
