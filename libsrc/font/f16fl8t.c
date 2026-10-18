#include <dev2d.h>

#define AA_FONT_COVERAGE_MAX 15

static ushort blend_pixel(ushort background, ushort foreground, uint alpha,
                          uint alpha_max, int format)
{
   uint inverse = alpha_max - alpha;
   uint r, g, b;

   if (format == BMF_RGB_565)
   {
      r = ((((background >> 11) & 0x1f) * inverse) +
           (((foreground >> 11) & 0x1f) * alpha) + alpha_max / 2) /
          alpha_max;
      g = ((((background >> 5) & 0x3f) * inverse) +
           (((foreground >> 5) & 0x3f) * alpha) + alpha_max / 2) /
          alpha_max;
      b = (((background & 0x1f) * inverse) +
           ((foreground & 0x1f) * alpha) + alpha_max / 2) / alpha_max;
      return (ushort)((r << 11) | (g << 5) | b);
   }

   r = ((((background >> 10) & 0x1f) * inverse) +
        (((foreground >> 10) & 0x1f) * alpha) + alpha_max / 2) /
       alpha_max;
   g = ((((background >> 5) & 0x1f) * inverse) +
        (((foreground >> 5) & 0x1f) * alpha) + alpha_max / 2) /
       alpha_max;
   b = (((background & 0x1f) * inverse) +
        ((foreground & 0x1f) * alpha) + alpha_max / 2) / alpha_max;
   return (ushort)((r << 10) | (g << 5) | b);
}

static int destination_format(void)
{
   grs_rgb_bitmask masks;

   if (gr_test_bitmap_format(&grd_bm, BMF_RGB_555))
      return BMF_RGB_555;
   if (gr_test_bitmap_format(&grd_bm, BMF_RGB_565))
      return BMF_RGB_565;

   // Dark's screen canvases historically have no RGB format tag. Query the
   // active display masks rather than treating an untagged 565 canvas as 555.
   gr_get_screen_rgb_bitmask(&masks);
   return masks.green == 0x07e0 ? BMF_RGB_565 : BMF_RGB_555;
}

static void il(uchar *dst, uchar *src, int n, int format)
{
   ushort *d16 = (ushort *)dst;
   int i;

   for (i=0; i<n; i++) {
      uint coverage = src[i];

      if (!coverage)
         continue;
      if (coverage > AA_FONT_COVERAGE_MAX)
         coverage = AA_FONT_COVERAGE_MAX;

      // Antialiased FON resources store fixed four-bit coverage: zero is
      // transparent and fifteen is a solid glyph pixel. The renderer light
      // table has an unrelated, mutable size (normally fourteen here), so it
      // cannot define font opacity; doing so discarded every solid pixel and
      // left only a colored fringe. Blend the coverage directly in the
      // destination canvas format instead.
      d16[i] = blend_pixel(d16[i], (ushort)grd_gc.fcolor, coverage,
                           AA_FONT_COVERAGE_MAX, format);
   }
}

void flat16_flat8_tluc16_ubitmap(grs_bitmap *bm, int x, int y)
{
   uchar *src, *dst;
   int w = bm->w;
   int h = bm->h;
   int format = destination_format();

   src = bm->bits;
   dst = grd_bm.bits + grd_bm.row*y + 2*x;

   while (h--) {
      il(dst, src, w, format);
      src += bm->row;
      dst += grd_bm.row;
   }
}


