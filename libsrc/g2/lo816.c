#include <dev2d.h>

extern void gen_flat8_lit_ulmap_setup(grs_bitmap *bm,
                                      void (*caller)(grs_bitmap *));

void opaque_lit_8to16_setup(grs_bitmap *bm, void (*caller)(grs_bitmap *))
{
   gen_flat8_lit_ulmap_setup(bm, caller);
}
