// $Header: x:/prj/tech/libsrc/dev2d/RCS/blend.c 1.5 1996/12/06 15:30:12 KEVIN Exp $
// Blending routines
#include <alpha.h>
#include <bitmap.h>
#include <dbg.h>

// returns (alpha)*c0 + (1-alpha)*c1 using format.  Not for real-time use!
uint gr_blend_color_vector(uint c0, uint c1, alpha_vector *alpha, int format)
{
   uint red, green, blue;
   uint red_mask, green_mask, blue_mask;
   uchar red_shift, green_shift, blue_shift;

   switch (format) {
   case BMF_RGB_555:
      blue_mask = 0x1f;
      green_mask = 0x1f;
      red_mask = 0x1f;
      blue_shift = 0;
      green_shift = 5;
      red_shift = 10;
      break;

   case BMF_RGB_565:
      blue_mask = 0x1f;
      green_mask = 0x3f;
      red_mask = 0x1f;
      blue_shift = 0;
      green_shift = 5;
      red_shift = 11;
      break;

   default:
      Warning(("gr_blend_color(): unsupported format %x\n", format));
      return 0;
   }

   blue = ((c0>>blue_shift) & blue_mask) * alpha->blue +
          ((c1>>blue_shift) & blue_mask) * (1.0 - alpha->blue);
   if (blue>blue_mask) blue = blue_mask;

   green = ((c0>>green_shift) & green_mask) * alpha->green +
           ((c1>>green_shift) & green_mask) * (1.0 - alpha->green);
   if (green>green_mask) green = green_mask;

   red = ((c0>>red_shift) & red_mask) * alpha->red +
         ((c1>>red_shift) & red_mask) * (1.0 - alpha->red);
   if (red>red_mask) red = red_mask;

   return (red<<red_shift)+(green<<green_shift)+(blue<<blue_shift);
}


// dst = src*(1-alpha) + color * alpha;
extern uint gr_blend_color_vector(uint c0, uint c1, alpha_vector *alpha, int format);
void gr_blend_pal16_vector(ushort *dst, ushort *src, uint color, alpha_vector *alpha, int format)
{
   int i;
   for (i=0; i<256; i++) {
      dst[i] = gr_blend_color_vector(color, src[i], alpha, format);
   }
}

void gr_blend_pal16(ushort *dst, ushort *src, uint color, alpha_real alpha, int format)
{
   alpha_vector alpha_vec;

   alpha_vec.red =
   alpha_vec.green =
   alpha_vec.blue = alpha;
   gr_blend_pal16_vector(dst, src, color, &alpha_vec, format);
}


