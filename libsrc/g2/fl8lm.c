// $Header: x:/prj/tech/libsrc/g2/RCS/fl8lm.c 1.2 1997/05/16 09:53:18 KEVIN Exp $

#include <tftype.h>
#include <lgassert.h>

extern tmap_setup_func *flat8_ulmap_setup_func[];
extern tmap_setup_func *flat8_lit_ulmap_setup_func[];

void flat8_ulmap_setup(grs_bitmap *bm)
{
   AssertMsg(bm->type<BMT_TYPES,"flat8_ulmap_setup(): invalid bitmap type!");
   flat8_ulmap_setup_func[make_index_bmt_fill_bmf(bm)]
      (bm, flat8_ulmap_setup);
}

void flat8_lit_ulmap_setup(grs_bitmap *bm)
{
   AssertMsg(bm->type<BMT_TYPES,"flat8_lit_ulmap_setup(): invalid bitmap type!");
   flat8_lit_ulmap_setup_func[make_index_bmt_fill_bmf(bm)]
      (bm, flat8_lit_ulmap_setup);
}
