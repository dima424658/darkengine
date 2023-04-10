// $Header: x:/prj/tech/libsrc/g2/RCS/f16pm.c 1.2 1998/04/03 16:39:07 KEVIN Exp $

#include <dev2d.h>
#include <lgassert.h>

extern void gen_upmap_setup(grs_bitmap *bm);
extern void gen_lit_upmap_setup(grs_bitmap *bm);

void flat16_upmap_setup(grs_bitmap *bm)
{
   uint ft = gr_get_fill_type();

   AssertMsg(bm->type<BMT_TYPES,"flat8_ulmap_setup(): invalid bitmap type!");
   if ((bm->type == BMT_FLAT16)&&
       ((bm->flags&BMF_TRANS)==0)&&
       (ft == FILL_NORM)) {

      extern void g2ptmap_setup_unlit16(grs_bitmap *bm);

      g2ptmap_setup_unlit16(bm);
   }
   else
      gen_upmap_setup(bm);
}

void flat16_lit_upmap_setup(grs_bitmap *bm)
{
   uint ft = gr_get_fill_type();

   AssertMsg(bm->type<BMT_TYPES,"flat8_ulmap_setup(): invalid bitmap type!");
   if ((bm->type == BMT_FLAT8)&&
       ((bm->flags&BMF_TRANS)==0)&&
       (ft == FILL_NORM)&&
       (grd_ltab816_list!=NULL))
   {
      extern void g2ptmap_setup_lit16(grs_bitmap *bm);

      grd_ltab816 = grd_ltab816_list[bm->align];
      AssertMsg(grd_ltab816!=NULL, "flat16_lit_upmap_setup(): Null 8 to 16 lighting table!");
      g2ptmap_setup_lit16(bm);
   }
   else
      gen_lit_upmap_setup(bm);
}

