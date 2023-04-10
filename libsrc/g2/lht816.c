// $Header: x:/prj/tech/libsrc/g2/RCS/lht816.c 1.2 1997/05/16 09:53:21 KEVIN Exp $
#include <g2d.h>
#include <plyrast.h>
#include <plyparam.h>
#include <tmapd.h>

extern void light_il(fix i, fix di, int n);
extern void flat16_flat16_opaque_light_il(uchar *dst, uchar *src, int n, uchar *ltab);
extern void flat16_flat8_opaque_inner_loop(uchar *dst, uchar *src, int n);
extern g2il_func opaque_8to8_buffer_il;

// r->n = #pixels in this run
// n = - pixels remaining
static void opaque_inner_loop(g2s_raster *r, g2s_poly_params *p, int n)
{
   int offset = n - r->n; // current position - scanline end
   uchar *src = tmap_buffer_end+offset;
   uchar *tmp = tmap_buffer_end+2*offset;
   uchar *dst = r->p+2*offset;

   flat16_flat8_opaque_inner_loop(tmp, src, r->n);
   light_il(r->i, p->dix, r->n);
   flat16_flat16_opaque_light_il(tmp, tmp, r->n, grd_light_table);
   light_il(r->h, p->dhx, r->n);
   flat16_flat16_opaque_light_il(dst, tmp, r->n, g2d_haze_table);
}

void trans_lit_haze_8to16_inner_loop(g2s_raster *r, g2s_poly_params *p)
{
   g2s_raster r_o, r_t;
   int n;

   n = -r->n;
   if (n>=0)
      return;

   r_t.i = r->i;
   r_t.h = r->h;
   r_o.p = r->p + 2*(r->x+r->n); // point to end of scanline

   opaque_8to8_buffer_il(r, p);

   do {
      if (tmap_buffer_end[n]!=0) {
         r_o.i = r_t.i;
         r_o.h = r_t.h;
         r_o.n = 1;
         goto opaque_entry;
      }
transparent_entry:
      r_t.i += p->dix;
      r_t.h += p->dhx;
   } while (++n);
   return;

   do {
      if (tmap_buffer_end[n]==0) {
         opaque_inner_loop(&r_o, p, n);
         goto transparent_entry;
      }
      r_o.n++;
opaque_entry:
      r_t.i += p->dix;
      r_t.h += p->dhx;
   } while (++n);
   opaque_inner_loop(&r_o, p, n);
}

