// $Header: x:/prj/tech/libsrc/dev2d/RCS/f16hlin.c 1.6 1997/02/17 14:49:07 KEVIN Exp $

#include <grd.h>
#include <f16il.h>

void flat16_uhline_fill(ushort c16, int x, int y, int x1)
{
   ushort *p;

   if (x1<x)
      return;

   p = (ushort *)(grd_bm.bits + grd_bm.row*y + 2*x);
   flat16_memset(c16, p, x1 - x + 1);
}

void flat16_norm_uhline(int x, int y, int x1)
{
   ushort c = grd_gc.fcolor;
   flat16_uhline_fill(c, x, y, x1);
}

void flat16_clut_uhline(int x, int y, int x1)
{
   ushort c16 = grd_gc.fcolor;
   ushort *clut16 = (ushort *)grd_gc.fill_parm;

   c16 = clut16[c16&255] + clut16[256+(c16>>8)];
   flat16_uhline_fill(c16, x, y, x1);
}

void flat16_solid_uhline(int x, int y, int x1)
{
   ushort c = grd_gc.fill_parm;
   flat16_uhline_fill(c, x, y, x1);
}

void flat16_xor_uhline(int x, int y, int x1)
{
   ushort *p, *p_last;
   ushort c16 = grd_gc.fcolor;

   p = (ushort *)(grd_bm.bits + grd_bm.row*y);
   p_last = p + x1;
   for (p+=x; p<=p_last; p++)
      *p = *p^c16;
}

void flat16_tluc_uhline(int x, int y, int x1)
{
   ushort c16;
   ushort *clut16;

   c16 = grd_gc.fcolor;
   if (c16 <= 255) {
      clut16 = (ushort *)tluc8tab[c16];
      if (clut16!=NULL) {
         ushort *p;
         p = (ushort *)(grd_bm.bits + grd_bm.row*y + 2*x);
         flat16_flat16_opaque_clut_il(p, p, x1 - x + 1, clut16);
         return;
      }
   }
   flat16_uhline_fill(c16, x, y, x1);
}
