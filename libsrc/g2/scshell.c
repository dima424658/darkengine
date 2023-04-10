// $Header: x:/prj/tech/libsrc/g2/RCS/scshell.c 1.5 1997/05/22 12:08:01 KEVIN Exp $
#include <plyparam.h>
#include <plyrast.h>
#include <trirast.h>
#include <tmapd.h>
#include <dbg.h>

#pragma off(unreferenced)
int gr_scale_warn(grs_bitmap *bm, int x, int y, int w, int h)
{
   Warning(("This bitmap type/fill type combination is unsupported.\n"));
   return CLIP_ALL;
}
#pragma on(unreferenced)

void scale_ushell(int x, int y, int w, int h, g2s_poly_params *p)
{
   g2s_raster r;
   grs_bitmap *bm = p->bm;

   if ((w == 0)||(h <= 0))
      return;

   if (w<0) {
      r.u = fix_make(bm->w-1,0xffff);
      r.n = -w;
      r.x = x - w;
   } else {
      r.u = fix_make(0, 0);
      r.x = x;
      r.n = w;
   }
   
   r.v = fix_make(0, 0);
   p->dux = fix_div(fix_make(bm->w,0), fix_make(w,0));
   p->dvx = 0;
   p->dvy = fix_div(fix_make(bm->h,0), fix_make(h,0));

   if (p->flags&PPF_MUNGE) {
      g2d_tmap_info.du_frac = p->dux<<16;
      g2d_tmap_info.dv_frac = 0;
      g2d_tmap_info.dsrc[1] = fix_int(p->dux);
   }

   r.p = p->p + y * p->canvas_row;
   goto uloop_start;

   do {
      r.v  += p->dvy;
      r.p  += p->canvas_row;
uloop_start:
      p->inner_loop (&r, p);
   } while (--h);
}

int scale_cshell(int x, int y, int w, int h, g2s_poly_params *p)
{
   g2s_raster r;
   int xf, yf;
   grs_bitmap *bm = p->bm;

   if ((w == 0)||(h <= 0))
      return CLIP_ALL;

   yf = y+h;
   if (w<0) {
      r.u = fix_make(bm->w-1,0xffff);
      xf = x;
      x = x + w;
   } else {
      r.u = fix_make(0, 0);
      xf = x + w;
   }
   
   if ((x>=grd_clip.right)||(xf<=grd_clip.left)||
       (y>=grd_clip.bot)||(yf<=grd_clip.top))
      return CLIP_ALL;

   r.v = fix_make(0, 0);
   p->dux = fix_div(fix_make(bm->w,0), fix_make(w,0));
   p->duy = p->dvx = 0;
   p->dvy = fix_div(fix_make(bm->h,0), fix_make(h,0));

   if (p->flags&PPF_MUNGE) {
      g2d_tmap_info.du_frac = p->dux<<16;
      g2d_tmap_info.dv_frac = 0;
      g2d_tmap_info.dsrc[1] = fix_int(p->dux);
   }

   if (x < grd_clip.left) {
      r.u += (grd_clip.left - x)*p->dux;
      x = grd_clip.left;
   }
   if (xf > grd_clip.right)
      xf = grd_clip.right;

   if (y < grd_clip.top) {
      r.v += (grd_clip.top - y)*p->dvy;
      y = grd_clip.top;
   }
   if (yf > grd_clip.bot)
      yf = grd_clip.bot;

   r.n = xf - x;
   h = yf - y;

   if ((h <= 0) || (r.n <= 0))
      return CLIP_ALL;

   r.x = x;
   r.p = p->p + y*p->canvas_row;
   goto loop_start;

   do {
      r.v += p->dvy;
      r.p += p->canvas_row;
loop_start:
      p->inner_loop (&r, p);
   } while (--h);
   return CLIP_NONE;
}
