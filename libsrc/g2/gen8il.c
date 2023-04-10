// $Header: x:/prj/tech/libsrc/g2/RCS/gen8il.c 1.1 1997/05/16 09:52:41 KEVIN Exp $
// Generic flat8 source inner loops.

#include <g2d.h>
#include <plyparam.h>

void gen_flat8_il(g2s_raster *r, g2s_poly_params *tp)
{
   int x,y,n;
   fix u, v, du, dv;
   int row;
   uchar *bits;
   gdupix_func *pix8_func;
   uint offset_max;
   uchar opaque;

   n = r->n;
   u = r->u;
   v = r->v;
   x = r->x;
   y = r->y;

   bits = tp->bm->bits;
   row = tp->bm->row;
   offset_max = (uint )(tp->bm->h * row);
   du = tp->dux;
   dv = tp->dvx;
   pix8_func = tp->pix_func;
   opaque = (tp->bm->flags&BMF_TRANS) ? 0:1;

   do {
      uint offset = (uint )(fix_int(v)*row + fix_int(u));
      uchar c;
      if (offset >= offset_max) offset = 0;
      c=bits[offset];
      if (opaque|c)
         pix8_func(c, x, y);
      u += du;
      v += dv;
      x++;
   } while (--n);
}

void gen_flat8_lit_il(g2s_raster *r, g2s_poly_params *tp)
{
   int x,y,n;
   fix i, u, v, di, du, dv, err, err_mask;
   int row;
   uchar *bits;
   gdupix_func *pix_func;
   gdlpix_func *lpix8_func;
   uint offset_max;
   uchar opaque;

   n = r->n;
   u = r->u;
   v = r->v;
   i = r->i;
   x = r->x;
   y = r->y;

   bits = tp->bm->bits;
   row = tp->bm->row;
   offset_max = (uint )(tp->bm->h * row);
   du = tp->dux;
   dv = tp->dvx;
   di = tp->dix;
   err = 0;
   err_mask = (g2d_error_dither) ? (FIX_UNIT-1) : 0;
   pix_func = tp->pix_func;
   lpix8_func = tp->lpix_func;
   opaque = (tp->bm->flags&BMF_TRANS) ? 0:1;

   do {
      uint offset = (uint )(fix_int(v)*row + fix_int(u));
      uchar c;
      if (offset >= offset_max) offset = 0;
      c=bits[offset];
      if (opaque|c)
         pix_func(lpix8_func(i+err, c), x, y);
      u += du;
      v += dv;
      i += di;
      err = (err+i)&err_mask;
      x++;
   } while (--n);
}

void gen_flat8_il_wrap(g2s_raster *r, g2s_poly_params *tp)
{
   int x,y,n;
   fix u, v, du, dv;
   int row, v_shift;
   uint u_mask, v_mask;
   uchar *bits;
   gdupix_func *pix8_func;
   uchar opaque;

   n = r->n;
   u = r->u;
   v = r->v;
   x = r->x;
   y = r->y;

   bits = tp->bm->bits;
   row = tp->bm->row;
   u_mask = (uint )tp->bm->w-1;
   v_mask = (uint )fix_make(tp->bm->h-1, 0);
   v_shift = 16 - tp->bm->wlog;
   du = tp->dux;
   dv = tp->dvx;
   pix8_func = tp->pix_func;
   opaque = (tp->bm->flags&BMF_TRANS) ? 0:1;

   do {
      uint offset = (uint )(((v&v_mask)>>v_shift) +
                            (fix_int(u)&u_mask));
      uchar c=bits[offset];
      if (opaque|c)
         pix8_func(c, x, y);
      u += du;
      v += dv;
      x++;
   } while (--n);
}

void gen_flat8_lit_il_wrap(g2s_raster *r, g2s_poly_params *tp)
{
   int x,y,n;
   fix i, u, v, di, du, dv, err, err_mask;
   int row, v_shift;
   uint u_mask, v_mask;
   uchar *bits;
   gdupix_func *pix_func;
   gdlpix_func *lpix8_func;
   uchar opaque;

   n = r->n;
   u = r->u;
   v = r->v;
   i = r->i;
   x = r->x;
   y = r->y;

   bits = tp->bm->bits;
   row = tp->bm->row;
   u_mask = (uint )tp->bm->w-1;
   v_mask = (uint )fix_make(tp->bm->h-1, 0);
   v_shift = 16 - tp->bm->wlog;
   du = tp->dux;
   dv = tp->dvx;
   di = tp->dix;
   err = 0;
   err_mask = (g2d_error_dither) ? (FIX_UNIT-1) : 0;
   lpix8_func = tp->lpix_func;
   pix_func = tp->pix_func;
   opaque = (tp->bm->flags&BMF_TRANS) ? 0:1;

   do {
      uint offset = (uint )(((v&v_mask)>>v_shift) +
                            (fix_int(u)&u_mask));
      uchar c=bits[offset];
      if (opaque|c)
         pix_func(lpix8_func(i+err, c), x, y);
      u += du;
      v += dv;
      i += di;
      err = (err + i)&err_mask;
      x++;
   } while (--n);
}

