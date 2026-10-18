/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/f16pix.c $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1997/04/01 14:52:05 $
 * 
 * Routines for drawing pixels into a flat 8 canvas.
 *
 * This file is part of the dev2d library.
 *
 * $Log: f16pix.c $
 * Revision 1.2  1997/04/01  14:52:05  KEVIN
 * Added gpix16 funcs.
 * 
 * Revision 1.1  1996/04/10  15:44:16  KEVIN
 * Initial revision
 * 
 */

#include <pixftype.h>
#include <grd.h>
#include <grdev.h>
extern void (*flat16_upix8_func[])();
extern void (*flat16_upix16_func[])();

static int component_shift(ulong mask)
{
   int shift = 0;
   if (!mask)
      return 0;
   while ((mask & 1) == 0)
   {
      mask >>= 1;
      ++shift;
   }
   return shift;
}

static int component_bits(ulong mask)
{
   int bits = 0;
   mask >>= component_shift(mask);
   while (mask & 1)
   {
      mask >>= 1;
      ++bits;
   }
   return bits;
}

static int component_5(ushort pixel, ulong mask)
{
   int bits;
   ulong value;
   if (!mask)
      return 0;
   bits = component_bits(mask);
   value = ((ulong)pixel & mask) >> component_shift(mask);
   if (bits > 5)
      value >>= bits - 5;
   else if (bits < 5)
   {
      ulong maximum = (1UL << bits) - 1;
      value = maximum ? (value * 31 + maximum / 2) / maximum : 0;
   }
   return (int)(value & 0x1f);
}

int flat16_ugpix8(int x, int y)
{
   grs_rgb_bitmask bitmask;
   ushort pixel;
   ushort *p;
   int index;

   if (!grd_bm.bits || !grd_ipal || x < 0 || y < 0 ||
       x >= grd_bm.w || y >= grd_bm.h)
      return 0;

   p = (ushort *)(grd_bm.bits + 2*x + y*grd_bm.row);
   pixel = *p;

   // grd_ipal has 32,768 entries in red-low RGB555 order.  The screen
   // canvas is normally RGB565 (and can also be BGR), so the raw 16-bit
   // screen pixel is neither a valid nor necessarily in-range ipal index.
   gr_get_screen_rgb_bitmask(&bitmask);
   index = component_5(pixel, bitmask.red)
         | (component_5(pixel, bitmask.green) << 5)
         | (component_5(pixel, bitmask.blue) << 10);
   return grd_ipal[index];
}

gdgpix_func *flat16_ugpix8_expose(int x, int y)
{
   return flat16_ugpix8;
}

int flat16_ugpix16(int x, int y)
{
   return *((ushort *)(grd_bm.bits + 2*x + y*grd_bm.row));
}

gdgpix_func *flat16_ugpix16_expose(int x, int y)
{
   return flat16_ugpix16;
}

gdupix_func *flat16_upix8_expose(int c, int x, int y)
{
   return (gdupix_func *)flat16_upix8_func[grd_gc.fill_type];
}

gdupix_func *flat16_upix16_expose(int c, int x, int y)
{
   return (gdupix_func *)flat16_upix16_func[grd_gc.fill_type];
}

void flat16_upix8(int c, int x, int y)
{
   (flat16_upix8_expose(c, x, y))(c, x, y);
}

void flat16_upix16(int c, int x, int y)
{
   (flat16_upix16_expose(c, x, y))(c, x, y);
}

