#include <grtm.h>
#include <water.h>

int *grd_water_offset_table;
uchar **grd_water_clut_pointer;
int grd_water_reflection_delta;

#pragma off(unreferenced)
void gr_water_floor_umap(grs_bitmap *bm, int n, grs_vertex **vpl, grs_water *w)
{
   gr_lit_per_umap(bm, n, vpl);
//   int i;
//   fix y_min;
//   int y;
//   grs_tmap_info ti;
//
//   if ((bm->type != BMT_FLAT8)||(bm->flags&BMF_TRANS)||(bm->row!=1<<(bm->wlog)))
//      return;
//   bm->type = BMT_MONO;
//   y_min = vpl[0]->y;
//   for (i=1; i<n; i++)
//      if (vpl[i]->y<y_min)
//         y_min = vpl[i]->y;
//
//   y = fix_cint(y_min);
//   grd_water_reflection_delta = (w->horizon - y)*2*grd_bm.row;
//   grd_water_clut_pointer = w->clut_list+y;
//   grd_water_offset_table = w->offset_table;
//   ti.tmap_type=GRC_LIT_FLOOR;
//   ti.flags=TMF_FLOOR;
//   h_umap(bm,n,vpl,&ti);
//   bm->type = BMT_FLAT8;
}
#pragma on(unreferenced)

