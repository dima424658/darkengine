// $Header: x:/prj/tech/libsrc/g2/RCS/fl8fl8s.c 1.2 1997/05/16 09:53:17 KEVIN Exp $

#include <scshell.h>
#include <trirast.h>

extern g2il_func opaque_nowrap_8to8_il;
extern g2il_func trans_nowrap_8to8_il;
extern g2il_func opaque_clut_nowrap_8to8_il;
extern g2il_func trans_clut_nowrap_8to8_il;
extern void opaque_clut_nowrap_8to8_il_init(ulong clut);
extern void trans_clut_nowrap_8to8_il_init(ulong clut);

static void setup(grs_bitmap *bm, g2s_poly_params *p)
{
   p->bm = bm;
   p->p = grd_bm.bits;
   p->canvas_row = grd_bm.row;
   p->flags = PPF_MUNGE;
}

void opaque_8to8_uscale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_poly_params p;

   setup(bm, &p);
   p.inner_loop = opaque_nowrap_8to8_il;
   scale_ushell(x, y, w, h, &p);
}

int opaque_8to8_scale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_poly_params p;

   setup(bm, &p);
   p.inner_loop = opaque_nowrap_8to8_il;
   return scale_cshell(x, y, w, h, &p);
}

void trans_8to8_uscale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_poly_params p;

   setup(bm, &p);
   p.inner_loop = trans_nowrap_8to8_il;
   scale_ushell(x, y, w, h, &p);
}

int trans_8to8_scale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_poly_params p;

   setup(bm, &p);
   p.inner_loop = trans_nowrap_8to8_il;
   return scale_cshell(x, y, w, h, &p);
}

void opaque_clut_8to8_uscale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_poly_params p;

   setup(bm, &p);
   p.inner_loop = opaque_clut_nowrap_8to8_il;
   opaque_clut_nowrap_8to8_il_init(grd_gc.fill_parm);
   scale_ushell(x, y, w, h, &p);
}

int opaque_clut_8to8_scale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_poly_params p;

   setup(bm, &p);
   p.inner_loop = opaque_clut_nowrap_8to8_il;
   opaque_clut_nowrap_8to8_il_init(grd_gc.fill_parm);
   return scale_cshell(x, y, w, h, &p);
}

void trans_clut_8to8_uscale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_poly_params p;

   setup(bm, &p);
   p.inner_loop = trans_clut_nowrap_8to8_il;
   trans_clut_nowrap_8to8_il_init(grd_gc.fill_parm);
   scale_ushell(x, y, w, h, &p);
}

int trans_clut_8to8_scale(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_poly_params p;

   setup(bm, &p);
   p.inner_loop = trans_clut_nowrap_8to8_il;
   trans_clut_nowrap_8to8_il_init(grd_gc.fill_parm);
   return scale_cshell(x, y, w, h, &p);
}



