// $Header: x:/prj/tech/libsrc/g2/RCS/cc816.c 1.3 1997/05/05 11:02:07 KEVIN Exp $
#include <g2d.h>
#include <plyrast.h>
#include <plyparam.h>
#include <scshell.h>
#include <tmapd.h>

extern void flat16_flat16_opaque_clut_il(uchar *dst, uchar *src, int n, uchar *clut);
extern void flat16_flat8_opaque_inner_loop(uchar *dst, uchar *src, int n);

static uchar *clut0, *clut1;

// r->n = #pixels in this run
// n = - pixels remaining
static void opaque_inner_loop(g2s_raster *r, g2s_poly_params *p, int n)
{
   int offset = n - r->n; // current position - scanline end
   uchar *src = tmap_buffer_end+offset;
   uchar *tmp = tmap_buffer_end+2*offset;
   uchar *dst = r->p+2*offset;

   flat16_flat8_opaque_inner_loop(tmp, src, r->n);
   flat16_flat16_opaque_clut_il(tmp, tmp, r->n, clut0);
   flat16_flat16_opaque_clut_il(dst, tmp, r->n, clut1);
}

static void scale_il(g2s_raster *r, g2s_poly_params *p)
{
   uchar *p_src = p->bm->bits + fix_int(r->v)*p->bm->row;
   int n = -r->n;
   fix u = r->u;
   fix du = p->dux;
   
   do {
      tmap_buffer_end[n] = p_src[fix_int(u)];
      u += du;
   } while (++n);
}
   

static void trans_inner_loop_shell(g2s_raster *r, g2s_poly_params *p)
{
   g2s_raster r_o;
   int n;

   n = -r->n;
   if (n>=0)
      return;

   r_o.p = r->p + 2*(r->x+r->n); // point to end of scanline

   scale_il(r, p);

   do {
      if (tmap_buffer_end[n]!=0) {
         r_o.n = 1;
         goto opaque_entry;
      }
transparent_entry: ;
   } while (++n);
   return;

   do {
      if (tmap_buffer_end[n]==0) {
         opaque_inner_loop(&r_o, p, n);
         goto transparent_entry;
      }
      r_o.n++;
opaque_entry: ;
   } while (++n);
   opaque_inner_loop(&r_o, p, n);
}

static void (*shell_func)(int x, int y, int w, int h, g2s_poly_params *p);

static void cc_scale_shell(grs_bitmap *bm, int x, int y, int w, int h)
{
   g2s_poly_params p;

   p.bm = bm;
   p.p = grd_bm.bits;
   p.canvas_row = grd_bm.row;
   p.inner_loop = trans_inner_loop_shell;
   shell_func(x, y, w, h, &p);
}

int clut_clut_8to16_scale(grs_bitmap *bm, int x, int y, int w, int h, uchar *c0, uchar *c1)
{
   clut0 = c0;
   clut1 = c1;
   shell_func = scale_cshell;
   cc_scale_shell(bm, x, y, w, h);
   return CLIP_NONE;
}

void clut_clut_8to16_uscale(grs_bitmap *bm, int x, int y, int w, int h, uchar *c0, uchar *c1)
{
   clut0 = c0;
   clut1 = c1;
   shell_func = scale_ushell;
   cc_scale_shell(bm, x, y, w, h);
}



