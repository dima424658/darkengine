// $Header: x:/prj/tech/libsrc/g2/RCS/genf16s.c 1.1 1996/09/16 11:21:19 KEVIN Exp $

#include <tmapd.h>
#include <ilfunc.h>
#include <scale.h>

extern void gen_flat16_il(int x, int xf, fix u, fix v);
void gen_flat16_uscale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_tmap_info *ti;

   ti = &g2d_tmap_info;
   ti->bm = bm;
   ti->il_func = gen_flat16_il;
   ti->pix_func = gd_upix16_expose(0, 0, 0);
   ti->y = y;
   uscale_shell(x, y, w, h);
}

int gen_flat16_scale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_tmap_info *ti;

   ti = &g2d_tmap_info;
   ti->bm = bm;
   ti->il_func = gen_flat16_il;
   ti->pix_func = gd_upix16_expose(0, 0, 0);
   if (y<grd_clip.top)
      ti->y = grd_clip.top;
   else
      ti->y = y;
   return scale_shell(x, y, w, h);
}


