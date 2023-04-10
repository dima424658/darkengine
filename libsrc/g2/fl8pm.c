// $Header: x:/prj/tech/libsrc/g2/RCS/fl8pm.c 1.5 1997/11/21 14:00:04 KEVIN Exp $

#include <dev2d.h>
#include <lgassert.h>

extern void gen_upmap_setup(grs_bitmap *bm);
extern void gen_lit_upmap_setup(grs_bitmap *bm);

BOOL g2d_use_portal=TRUE;

void flat8_upmap_setup(grs_bitmap *bm)
{
   uint ft = gr_get_fill_type();

   AssertMsg(bm->type<BMT_TYPES,"flat8_ulmap_setup(): invalid bitmap type!");
   if (g2d_use_portal&&
       (bm->type == BMT_FLAT8)&&
       ((bm->flags&BMF_TRANS)==0)&&
       ((ft == FILL_NORM)||(ft==FILL_CLUT))) {

      extern void g2ptmap_setup_unlit(grs_bitmap *bm);

      g2ptmap_setup_unlit(bm);
   }
   else
      gen_upmap_setup(bm);
}

void flat8_lit_upmap_setup(grs_bitmap *bm)
{
   uint ft = gr_get_fill_type();

   AssertMsg(bm->type<BMT_TYPES,"flat8_ulmap_setup(): invalid bitmap type!");
   if (g2d_use_portal&&
       (bm->type == BMT_FLAT8)&&
       ((bm->flags&BMF_TRANS)==0)&&
       ((ft == FILL_NORM)||(ft==FILL_CLUT))) {

      extern void g2ptmap_setup_lit(grs_bitmap *bm);

      g2ptmap_setup_lit(bm);
   }
   else
      gen_lit_upmap_setup(bm);
}
