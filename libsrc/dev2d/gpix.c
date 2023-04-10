// $Header: x:/prj/tech/libsrc/dev2d/RCS/gpix.c 1.1 1996/07/23 09:52:42 KEVIN Exp $

#include <grs.h>
#include <bitmap.h>
#include <grnull.h>
int gr_get_pixel_bm(grs_bitmap *bm, int x, int y)
{
   uchar *p;
   p = bm->bits + y * bm->row;
   switch (bm->type) {
   case BMT_MONO:
   {
      uchar c = p[((x+bm->align)>>3)];
      return (c>>(x&7))&1;
   }
   case BMT_FLAT8:
      return p[x];
   case BMT_FLAT16:
      return ((ushort *)p)[x];
   default:
      gr_scream();
   }
   return -1;
}


