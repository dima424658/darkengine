#include <g2tm.h>
#include <tmapd.h>

extern void gen_flat8_ulmap_setup(grs_bitmap *bm,
                                  void (*caller)(grs_bitmap *));

void opaque_8to8_setup(grs_bitmap *bm, void (*caller)(grs_bitmap *))
{
   gen_flat8_ulmap_setup(bm, caller);
}

void opaque_8to8_umap(grs_bitmap *bm, int n, g2s_point **vpl)
{
   g2_lin_umap_setup(bm);
   g2d_pp.poly_func(n, vpl);
}

void opaque_8to8_pmap(grs_bitmap *bm, int n, g2s_point **vpl)
{
   g2_per_umap_setup(bm);
   g2d_pp.poly_func(n, vpl);
}
