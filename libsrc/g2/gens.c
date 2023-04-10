/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/gens.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:31:23 $
 * 
 * This file is part of the g2 library.
 *
 */

#include <sftype.h>
#include <dbg.h>

extern void (*gen_uscale_func[])();
extern void (*gen_scale_func[])();

#pragma off(unreferenced)
g2us_func *gen_uscale_expose(grs_bitmap *bm, int x, int y, int w, int h)
{
   Assrt(bm->type<BMT_TYPES);
   return (g2us_func *)gen_uscale_func[bm->type];
}
g2s_func *gen_scale_expose(grs_bitmap *bm, int x, int y, int w, int h)
{
   Assrt(bm->type<BMT_TYPES);
   return (g2s_func *)gen_scale_func[bm->type];
}
#pragma on(unreferenced)

void gen_uscale(grs_bitmap *bm, int x, int y, int w, int h)
{
   (gen_uscale_expose(bm, x, y, w, h))(bm, x, y, w, h);
}

int gen_scale(grs_bitmap *bm, int x, int y, int w, int h)
{
   return (gen_scale_expose(bm, x, y, w, h))(bm, x, y, w, h);
}
