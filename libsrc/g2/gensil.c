/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/gensil.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:31:36 $
 * 
 * This file is part of the g2 library.
 *
 */

#include <tmapd.h>

void gen_sil(int x, int xf, fix i)
{
   g2s_tmap_info *ti=&g2d_tmap_info;
   int c, y;
   fix di;
   void (*pix_func)();
   int (*lpix_func)();

   di = ti->dix;
   c = ti->color;
   y = ti->y;
   pix_func = ti->pix_func;
   lpix_func = ti->lpix_func;
   for (;x<xf;x++) {
      pix_func(lpix_func(i, c), x, y);
      i += di;
   }
}
