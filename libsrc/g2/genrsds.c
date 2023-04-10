/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/genrsds.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:31:08 $
 *
 * This file is part of the g2 library.
 *
 */

#include <grscale.h>

void gen_rsd8_uscale(grs_bitmap *bm, int x, int y, int w, int h)
{
   grs_bitmap tbm;
   if (gr_rsd8_convert(bm, &tbm)==GR_UNPACK_RSD8_OK)
      gr_scale_ubitmap(&tbm, x, y, w, h);
}

int gen_rsd8_scale(grs_bitmap *bm, int x, int y, int w, int h)
{
   grs_bitmap tbm;
   if (gr_rsd8_convert(bm, &tbm)==GR_UNPACK_RSD8_OK)
      return gr_scale_bitmap(&tbm, x, y, w, h);
   return CLIP_ALL;
}
