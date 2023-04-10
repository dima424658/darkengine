/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/bk8bm.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:47:46 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#include <bmftype.h>
#include <indexmac.h>
extern void (*bank8_ubitmap_func[])();

#pragma off(unreferenced)
gdubm_func *bank8_ubitmap_expose(grs_bitmap *bm, int x, int y)
{
   int i = make_index_bmt_fill_bmf(bm);
   return (gdubm_func *)bank8_ubitmap_func[i];
}
#pragma on(unreferenced)

void bank8_ubitmap(grs_bitmap *bm, int x, int y)
{
   (bank8_ubitmap_expose(bm, x, y))(bm, x, y);
}
