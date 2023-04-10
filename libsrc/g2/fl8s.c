/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/fl8s.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:26:31 $
 * 
 * This file is part of the g2 library.
 *
 */

#include <sftype.h>

extern void (*flat8_uscale_func[])();
extern void (*flat8_scale_func[])();

#pragma off(unreferenced)
g2us_func *flat8_uscale_expose(grs_bitmap *bm, int x, int y, int w, int h)
{
   int i = make_index_bmt_fill_bmf(bm);
   return (g2us_func *)flat8_uscale_func[i];
}
g2s_func *flat8_scale_expose(grs_bitmap *bm, int x, int y, int w, int h)
{
   int i = make_index_bmt_fill_bmf(bm);
   return (g2s_func *)flat8_scale_func[i];
}
#pragma on(unreferenced)

void flat8_uscale(grs_bitmap *bm, int x, int y, int w, int h)
{
   (flat8_uscale_expose(bm, x, y, w, h))(bm, x, y, w, h);
}

int flat8_scale(grs_bitmap *bm, int x, int y, int w, int h)
{
   return (flat8_scale_expose(bm, x, y, w, h))(bm, x, y, w, h);
}
