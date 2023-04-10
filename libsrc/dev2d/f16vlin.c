// $Header: x:/prj/tech/libsrc/dev2d/RCS/f16vlin.c 1.1 1998/04/02 11:47:14 KEVIN Exp $

#include <grd.h>
#include <f16il.h>

void flat16_uvline_fill(ushort c16, int x, int y, int y1)
{
   ushort *p;
   int i, row;

   if (y1<y)
      return;

   row = grd_bm.row;
   p = (ushort *)(grd_bm.bits + row*y + 2*x);
   for (i=y; i<=y1; i++) {
      *p = c16;
      p=(ushort *)(((uchar *)p) + row);
   }
}

void flat16_norm_uvline(int x, int y, int y1)
{
   ushort c = grd_gc.fcolor;
   flat16_uvline_fill(c, x, y, y1);
}

void flat16_clut_uvline(int x, int y, int y1)
{
   ushort c16 = grd_gc.fcolor;
   ushort *clut16 = (ushort *)grd_gc.fill_parm;

   c16 = clut16[c16&255] + clut16[256+(c16>>8)];
   flat16_uvline_fill(c16, x, y, y1);
}

void flat16_solid_uvline(int x, int y, int y1)
{
   ushort c = grd_gc.fill_parm;
   flat16_uvline_fill(c, x, y, y1);
}

void flat16_xor_uvline(int x, int y, int y1)
{
   ushort *p;
   ushort c16 = grd_gc.fcolor;
   int i, row;

   if (y1<y)
      return;

   row = grd_bm.row;
   p = (ushort *)(grd_bm.bits + row*y + 2*x);
   for (i=y; i<=y1; i++) {
      *p ^= c16;
      p=(ushort *)(((uchar *)p) + row);
   }

}

void flat16_tluc_uvline(int x, int y, int y1)
{
   ushort c16;
   ushort *clut16;

   c16 = grd_gc.fcolor;
   if (c16 <= 255) {
      clut16 = (ushort *)tluc8tab[c16];
      if (clut16!=NULL) {
         ushort *p;
         int i,row;

         row = grd_bm.row;
         p = (ushort *)(grd_bm.bits + row*y + 2*x);
         for (i=y; i<=y1; i++) {
            *p = clut16[(*p)&255] + clut16[256+((*p)>>8)];
            p=(ushort *)(((uchar *)p) + row);
         }
         return;
      }
   }
   flat16_uvline_fill(c16, x, y, y1);
}
