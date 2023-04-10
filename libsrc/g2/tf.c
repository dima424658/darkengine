// $Header: x:/prj/tech/libsrc/g2/RCS/tf.c 1.4 1998/04/28 14:29:47 KEVIN Exp $
#include <trirast.h>
#include <plyshell.h>
#include <tmapd.h>

int g2_umap(int n, g2s_point **vpl)
{
   g2s_point *v0 = vpl[0];
   g2s_point *v1 = vpl[1];
   g2s_point *v2 = vpl[2];
   int i;

   g2_triangle_raster_sort(v0, v1, v2, g2d_ppp);
   for (i=3; i<n; i++) {
      v1 = vpl[i-1];
      v2 = vpl[i];
      g2_triangle_raster_sort(v0, v1, v2, g2d_ppp);
   }
   return CLIP_NONE;
}

void g2_trifan(g2s_poly_params *pp, int n, g2s_point **vpl)
{
   g2d_ppp = pp;
   g2_umap(n, vpl);
   g2d_ppp = &g2d_pp;
}
