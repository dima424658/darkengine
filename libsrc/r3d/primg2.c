// $Header: x:/prj/tech/libsrc/r3d/RCS/primg2.c 1.6 1998/02/19 16:13:26 KEVIN Exp $
#include <r3ds.h>
#include <g2.h>
#include <lgd3d.h>

#include <prim.h>
#include <primconv.h>
#include <primfunc.h>
#include <primcon_.h>
#include <primtab_.h>

static poly_setup_func poly_g2_setup_funcs[] =
{
   // 0
   g2_upoly_setup,           //      solid unlit
   g2_poly_setup,            //      solid unlit clipped
   g2_uspoly_setup,          //      solid gouraud
   g2_spoly_setup,           //      solid gouraud clipped
   g2_poly_setup_null,       //      solid rgb gouraud
   g2_poly_setup_null,       //      solid rgb gouraud clipped
   g2_poly_setup_null,       //      solid pal gouraud
   g2_poly_setup_null,       //      solid pal gouraud clipped

   // 4
   g2_poly_setup_null,       //      transluc unlit
   g2_poly_setup_null,       //      transluc unlit clipped
   g2_poly_setup_null,       //      transluc gouraud
   g2_poly_setup_null,       //      transluc gouraud clipped
   g2_poly_setup_null,       //      transluc rgb gouraud
   g2_poly_setup_null,       //      transluc rgb gouraud clipped
   g2_poly_setup_null,       //      transluc pal gouraud
   g2_poly_setup_null,       //      transluc pal gouraud clipped

   // 8
   g2_per_umap_setup,        //      texture unlit
   g2_per_map_setup,         //      texture unlit clipped
   g2_lit_per_umap_setup,    //      texture gouraud
   g2_lit_per_map_setup,     //      texture gouraud clipped
   g2_poly_setup_null,       //      texture rgb gouraud
   g2_poly_setup_null,       //      texture rgb gouraud clipped
   g2_poly_setup_null,       //      texture pal gouraud
   g2_poly_setup_null,       //      texture pal gouraud clipped

   // 16
   g2_lin_umap_setup,        //      linear unlit
   g2_lin_map_setup,         //      linear unlit clipped
   g2_lit_lin_umap_setup,    //      linear gouraud
   g2_lit_lin_map_setup,     //      linear gouraud clipped
   g2_poly_setup_null,       //      linear rgb gouraud
   g2_poly_setup_null,       //      linear rgb gouraud clipped
   g2_poly_setup_null,       //      linear pal gouraud
   g2_poly_setup_null        //      linear pal gouraud clipped
};


texture_manager *r3d_tmgr = NULL;

// just convert the points and call the poly
static int g2_convert(int n, r3s_phandle *vp)
{
   grs_point *p2[32];
   int result;
   uchar *hacked_bits;

   // This hack is done to avoid problems when a texture
   // needs to be used simultaneously by g2 and lgd3d
   if (r3d_poly_flags & R3_PL_TEXTURE_MASK) {
      hacked_bits = r3d_bm->bits;
      if ((r3d_tmgr!=NULL)&&(r3d_bm->flags & BMF_LOADED))
         r3d_tmgr->restore_bits(r3d_bm);
   }

   if (r3d_do_setup) {
      r3_2d_poly_setup_func(r3d_bm);
      r3d_do_setup = FALSE;
   }
   r3_phandle_to_vertex_list(n, vp, p2);
   result = g2_draw_poly_func(n, (g2s_point **)p2);

   if (r3d_poly_flags & R3_PL_TEXTURE_MASK)
      r3d_bm->bits = hacked_bits;

   return result;
}

static int do_g2_line(r3s_phandle p0, r3s_phandle p1)
{
   grs_vertex v0, v1;
//   gr_set_fcolor(r3d_color);
   v0.x = p0->grp.sx;
   v0.y = p0->grp.sy;
   v1.x = p1->grp.sx;
   v1.y = p1->grp.sy;
   return gr_line(&v0, &v1);
}

static int do_g2_point(r3s_phandle p)
{
   return gr_draw_point(grd_gc.fcolor, (g2s_point *)(&p->grp));
}


// use this to return to the default driver (g2)
void r3_use_g2(void)
{
   r3_setup_prim_funcs(poly_g2_setup_funcs, g2_convert, do_g2_line, do_g2_point);
   r3_set_prim();
}


