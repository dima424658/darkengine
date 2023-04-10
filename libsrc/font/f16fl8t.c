#include <dev2d.h>

static void il(uchar *dst, uchar *src, int n)
{
   ushort *d16 = (ushort *)dst;
   uint check = grd_light_table_size-1;
   uint fclow = grd_gc.fcolor&0xff;
   uint fchigh = 256 + ((grd_gc.fcolor>>8)&0xff);
   int i;

   for (i=0; i<n; i++) {
      uint l = src[i];
      ushort c16;
      ushort *clut;

      if (l-1>=check)  // i.e., 0<l<grd_light_table_size
         continue;

      clut = (ushort *)(grd_light_table+(l<<10));
      c16 = clut[fclow] + clut[fchigh];
      clut = (ushort *)(grd_light_table+((check-l)<<10));
      d16[i] = c16 + clut[dst[2*i]] + clut[256+dst[2*i+1]];
   }
}

void flat16_flat8_tluc16_ubitmap(grs_bitmap *bm, int x, int y)
{
   uchar *src, *dst;
   int w = bm->w;
   int h = bm->h;

   src = bm->bits;
   dst = grd_bm.bits + grd_bm.row*y + 2*x;

   while (h--) {
      il(dst, src, w);
      src += bm->row;
      dst += grd_bm.row;
   }
}


