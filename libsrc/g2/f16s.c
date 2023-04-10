/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/f16s.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/05/20 10:40:21 $
 * 
 * This file is part of the g2 library.
 *
 */

#include <sftype.h>

extern void (*flat16_uscale_func[])();
extern void (*flat16_scale_func[])();

#pragma off(unreferenced)
g2us_func *flat16_uscale_expose(grs_bitmap *bm, int x, int y, int w, int h)
{
   int i = make_index_bmt_fill_bmf(bm);
   return (g2us_func *)flat16_uscale_func[i];
}
g2s_func *flat16_scale_expose(grs_bitmap *bm, int x, int y, int w, int h)
{
   int i = make_index_bmt_fill_bmf(bm);
   return (g2s_func *)flat16_scale_func[i];
}
#pragma on(unreferenced)

void flat16_uscale(grs_bitmap *bm, int x, int y, int w, int h)
{
   (flat16_uscale_expose(bm, x, y, w, h))(bm, x, y, w, h);
}

int flat16_scale(grs_bitmap *bm, int x, int y, int w, int h)
{
   return (flat16_scale_expose(bm, x, y, w, h))(bm, x, y, w, h);
}
