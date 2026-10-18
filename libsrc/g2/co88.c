#include <dev2d.h>

extern void gen_flat8_ulmap_setup(grs_bitmap *bm,
                                  void (*caller)(grs_bitmap *));

void opaque_clut_8to8_setup(grs_bitmap *bm, void (*caller)(grs_bitmap *))
{
   gen_flat8_ulmap_setup(bm, caller);
}
