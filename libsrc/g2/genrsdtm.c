// $Header: x:/prj/tech/libsrc/g2/RCS/genrsdtm.c 1.2 1997/05/16 09:53:04 KEVIN Exp $

#include <tmapd.h>

void gen_rsd8_ulmap_setup(grs_bitmap *bm, void (*caller)(grs_bitmap *))
{
   grs_bitmap tbm;
   if (gr_rsd8_convert(bm, &tbm)==GR_UNPACK_RSD8_OK) {
      g2d_pp.bm = &tbm;
      caller(&tbm);
   }
}
