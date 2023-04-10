/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/f16bm.c $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1996/04/11 14:31:40 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#include <bmftype.h>
#include <indexmac.h>
extern void (*flat16_ubitmap_func[])();

#pragma off(unreferenced)
gdubm_func *flat16_ubitmap_expose(grs_bitmap *bm, int x, int y)
{
   int i = make_index_bmt_fill_bmf(bm);
   return (gdubm_func *)flat16_ubitmap_func[i];
}
#pragma on(unreferenced)

void flat16_ubitmap(grs_bitmap *bm, int x, int y)
{
   (flat16_ubitmap_expose(bm, x, y))(bm, x, y);
}
