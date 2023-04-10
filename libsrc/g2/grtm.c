// $Header: x:/prj/tech/libsrc/g2/RCS/grtm.c 1.1 1997/05/16 09:48:55 KEVIN Exp $

#include <g2d.h>
#include <grtm.h>
#include <grspoint.h>
#include <g2tm.h>
#include <buffer.h>

static void vpl_to_ppl(grs_bitmap *bm, int n, grs_vertex **vpl, g2s_point **ppl)
{
   int i;
   float inv_u_scale, inv_v_scale;
   float inv_ltab_size = 1.0/grd_light_table_size;
   grs_point *pl = (grs_point *)(ppl + n);

   if (bm!=NULL) {
      inv_u_scale = 1.0/bm->w;
      inv_v_scale = 1.0/bm->h;
   }

   for (i=0; i<n; i++) {
      ppl[i] = (g2s_point *)(&pl[i]);
      pl[i].sx = vpl[i]->x;
      pl[i].sy = vpl[i]->y;
      pl[i].flags = 0;
      pl[i].i = fix_float(vpl[i]->i) * inv_ltab_size;
      if (bm!=NULL) {
         pl[i].u = fix_float(vpl[i]->u) * inv_u_scale;
         pl[i].v = fix_float(vpl[i]->v) * inv_v_scale;
         pl[i].w = fix_float(vpl[i]->w);
      }
   }
}

typedef int (map_func)(int n, g2s_point **ppl);
static int do_map(grs_bitmap *bm, int n, grs_vertex **vpl, map_func *f)
{
   g2s_point **ppl;
   int code;

   ppl = (g2s_point **)gr_alloc_temp(n * (4 + sizeof(grs_point)));
   vpl_to_ppl (bm, n, vpl, ppl);
   code = f (n, ppl);
   gr_free_temp (ppl);
   return code;
}

int gr_poly(int c, int n, grs_vertex **vpl)
{
   gr_set_fcolor (c);
   g2_poly_setup (NULL);
   return do_map(NULL, n, vpl, g2_map);
}

int gr_spoly(int c, int n, grs_vertex **vpl)
{
   gr_set_fcolor (c);
   g2_spoly_setup (NULL);
   return do_map(NULL, n, vpl, g2_map);
}

int gr_lin_map(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   g2_lin_map_setup (bm);
   return do_map(bm, n, vpl, g2_map);
}

int gr_lit_lin_map(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   g2_lit_lin_map_setup (bm);
   return do_map(bm, n, vpl, g2_map);
}

int gr_clut_lin_map(grs_bitmap *bm, int n, grs_vertex **vpl, uchar *clut)
{
   int code;

   if (gr_get_fill_type()==FILL_NORM) {    
      gr_set_fill_type(FILL_CLUT);         
      gr_set_fill_parm(clut);              
      code = gr_lin_map(bm, n, vpl);             
      gr_set_fill_type(FILL_NORM);         
   } else {                                
      code = gr_lin_map(bm, n, vpl);             
   }
   return code;
}

int gr_per_map(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   g2_per_map_setup(bm);
   return do_map(bm, n, vpl, g2_per_map);
}

int gr_lit_per_map(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   g2_lit_per_map_setup(bm);
   return do_map(bm, n, vpl, g2_per_map);
}

int gr_clut_per_map(grs_bitmap *bm, int n, grs_vertex **vpl, uchar *clut)
{
   int code;
   if (gr_get_fill_type()==FILL_NORM) {    
      gr_set_fill_type(FILL_CLUT);         
      gr_set_fill_parm(clut);              
      code = gr_per_map(bm, n, vpl);             
      gr_set_fill_type(FILL_NORM);         
   } else {                                
      code = gr_per_map(bm, n, vpl);             
   }
   return code;
}

void gr_upoly(int c, int n, grs_vertex **vpl)
{
   gr_set_fcolor (c);
   g2_upoly_setup (NULL);
   do_map(NULL, n, vpl, g2_umap);
}

void gr_uspoly(int c, int n, grs_vertex **vpl)
{
   gr_set_fcolor (c);
   g2_uspoly_setup (NULL);
   do_map(NULL, n, vpl, g2_umap);
}

void gr_lin_umap(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   g2_lin_umap_setup (bm);
   do_map(bm, n, vpl, g2_umap);
}

void gr_lit_lin_umap(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   g2_lit_lin_umap_setup (bm);
   do_map(bm, n, vpl, g2_umap);
}

void gr_clut_lin_umap(grs_bitmap *bm, int n, grs_vertex **vpl, uchar *clut)
{
   if (gr_get_fill_type()==FILL_NORM) {    
      gr_set_fill_type(FILL_CLUT);         
      gr_set_fill_parm(clut);              
      gr_lin_umap(bm, n, vpl);             
      gr_set_fill_type(FILL_NORM);         
   } else {                                
      gr_lin_umap(bm, n, vpl);             
   }                                       
}

void gr_per_umap(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   g2_per_umap_setup (bm);
   do_map(bm, n, vpl, g2_per_umap);
}

void gr_lit_per_umap(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   g2_lit_per_umap_setup (bm);
   do_map(bm, n, vpl, g2_per_umap);
}

void gr_clut_per_umap(grs_bitmap *bm, int n, grs_vertex **vpl, uchar *clut)
{
   if (gr_get_fill_type()==FILL_NORM) {    
      gr_set_fill_type(FILL_CLUT);         
      gr_set_fill_parm(clut);              
      gr_per_map(bm, n, vpl);             
      gr_set_fill_type(FILL_NORM);         
   } else {                                
      gr_per_map(bm, n, vpl);             
   }                                       
}
