// $Header: x:/prj/tech/libsrc/g2/RCS/genlm.c 1.2 1997/05/16 09:53:00 KEVIN Exp $

#include <tftype.h>
#include <lgassert.h>

extern tmap_setup_func *gen_ulmap_setup_func[];
extern tmap_setup_func *gen_lit_ulmap_setup_func[];

void gen_ulmap_setup(grs_bitmap *bm)
{
   AssertMsg(bm->type<BMT_TYPES,"gen_ulmap_setup(): invalid bitmap type!");
   gen_ulmap_setup_func[bm->type] (bm, gen_ulmap_setup);
}

void gen_lit_ulmap_setup(grs_bitmap *bm)
{
   AssertMsg(bm->type<BMT_TYPES,"gen_lit_ulmap_setup(): invalid bitmap type!");
   gen_lit_ulmap_setup_func[bm->type] (bm, gen_lit_ulmap_setup);
}
