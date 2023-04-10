// $Header: x:/prj/tech/libsrc/g2/RCS/f16lm.c 1.2 1998/04/09 10:00:13 KEVIN Exp $

#include <lgassert.h>
#include <dbg.h>

#include <tftype.h>

extern void (*flat16_ulmap_setup_func[])();
extern void (*flat16_lit_ulmap_setup_func[])();
#if 0
void flat16_ulmap_setup(grs_bitmap *bm)
{
   AssertMsg(bm->type<BMT_TYPES,"flat16_ulmap_setup(): invalid bitmap type!");
   flat16_ulmap_setup_func[make_index_bmt_fill_bmf(bm)]
      (bm, flat16_ulmap_setup);
}
#endif
void flat16_lit_ulmap_setup(grs_bitmap *bm)
{
   AssertMsg(bm->type<BMT_TYPES,"flat16_lit_ulmap_setup(): invalid bitmap type!");
   flat16_lit_ulmap_setup_func[make_index_bmt_fill_bmf(bm)]
      (bm, flat16_lit_ulmap_setup);
}

