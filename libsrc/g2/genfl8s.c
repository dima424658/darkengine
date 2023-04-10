// $Header: x:/prj/tech/libsrc/g2/RCS/genfl8s.c 1.2 1997/05/16 09:52:58 KEVIN Exp $

#include <scshell.h>
extern g2il_func gen_flat8_il;

void gen_flat8_uscale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_poly_params p;

   p.bm = bm;
   p.y = 0;
   p.dy = 1;
   p.pix_func = gd_upix8_expose(0, 0, 0);
   p.inner_loop = gen_flat8_il;
   pixpal = (void *)(grd_pal16_list[bm->align]);

   scale_ushell(x, y, w, h, &p);
}

int gen_flat8_scale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_poly_params p;

   p.bm = bm;
   p.y = 0;
   p.dy = 1;
   p.pix_func = gd_upix8_expose(0, 0, 0);
   p.inner_loop = gen_flat8_il;
   pixpal = (void *)(grd_pal16_list[bm->align]);

   return scale_cshell(x, y, w, h, &p);
}


