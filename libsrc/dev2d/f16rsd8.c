// $Header: x:/prj/tech/libsrc/dev2d/RCS/f16rsd8.c 1.3 1997/02/17 14:49:11 KEVIN Exp $
// Routines for drawing rsd bitmaps into a flat16 canvas.

#include <grd.h>
#include <pal16.h>
#include <f16il.h>

void flat16_flat8_memset(uchar c, ushort *dst, int n)
{
   ushort c16 = ((ushort *)pixpal)[c];
   flat16_memset(c16, dst, n);
}

void flat16_flat8_tluc_memset(uchar c, ushort *dst, int n)
{
   ushort *clut = (ushort *)tluc8tab[c];
   if (clut!=NULL)
      flat16_flat16_opaque_clut_il(dst, dst, n, clut);
   else
      flat16_flat8_memset(c, dst, n);
}

void flat16_flat8_opaque_tluc_inner_loop(ushort *dst, uchar *src, int n)
{
   int i;
   ushort *pal = (ushort *)pixpal;

   for (i=0; i<n; i++) {
      uchar c = src[i];
      ushort *clut = (ushort *)tluc8tab[c];
      if (clut!=NULL) {
         ushort c16 = dst[i];
         dst[i] = clut[c16&0xff] + clut[(c16>>8)+256];
      } else {
         dst[i] = pal[c];
      }
   }
}

typedef void (*memset_fcn)(uchar c, ushort *dst, int count);
typedef void (*memcpy_fcn)(ushort *dst, uchar *src, int count);
extern void flat16_rsd8_blit_init (memset_fcn memset, memcpy_fcn memcpy);
extern void flat16_rsd8_blit (uchar *rsd_src, uchar *dst, int grd_bm_row, int bm_w);

void flat16_rsd8_ubitmap(grs_bitmap *bm, short x, short y)
{
   uchar *p_dst;

   flat16_rsd8_blit_init(flat16_flat8_memset, flat16_flat8_opaque_inner_loop);
   pixpal = (void *)grd_pal16_list[bm->align];
   p_dst = grd_bm.bits + grd_bm.row*y + 2*x;
   flat16_rsd8_blit(bm->bits, p_dst, grd_bm.row, bm->w);
}
          
void flat16_rsd8_blend_ubitmap(grs_bitmap *bm, short x, short y)
{
   uchar *p_dst;

   flat16_rsd8_blit_init(flat16_flat8_tluc_memset, flat16_flat8_opaque_tluc_inner_loop);
   pixpal = (void *)grd_pal16_list[bm->align];
   p_dst = grd_bm.bits + grd_bm.row*y + 2*x;
   flat16_rsd8_blit(bm->bits, p_dst, grd_bm.row, bm->w);
}
          
