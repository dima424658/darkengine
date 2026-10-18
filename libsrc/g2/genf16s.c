// $Header: x:/prj/tech/libsrc/g2/RCS/genf16s.c 1.1 1996/09/16 11:21:19 KEVIN Exp $

#include <scshell.h>

extern g2il_func gen_flat16_il;
void gen_flat16_uscale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_poly_params p;

   p.bm = bm;
   p.y = 0;
   p.dy = 1;
   p.flags = 0;
   p.pix_func = gd_upix16_expose(0, 0, 0);
   p.inner_loop = gen_flat16_il;

   scale_ushell(x, y, w, h, &p);
}

int gen_flat16_scale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_poly_params p;

   p.bm = bm;
   p.y = 0;
   p.dy = 1;
   p.flags = 0;
   p.pix_func = gd_upix16_expose(0, 0, 0);
   p.inner_loop = gen_flat16_il;

   return scale_cshell(x, y, w, h, &p);
}


