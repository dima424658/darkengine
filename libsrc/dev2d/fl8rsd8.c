/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/fl8rsd8.c $
 * $Revision: 1.4 $
 * $Author: TOML $
 * $Date: 1996/10/16 16:06:38 $
 *
 * Routines for drawing rsd bitmaps into a flat 8 canvas.
 *
 * This file is part of the dev2d library.
 *
 */

#include <string.h>
#include <grd.h>

extern void flat8_rsd8_blit (uchar *rsd_src, uchar *dst, int grd_bm_row, int bm_w);

void flat8_rsd8_ubitmap(grs_bitmap *bm, short x, short y)
{
   uchar *p_dst;
   uchar *rsd_src;                     /* rsd source buffer */

   rsd_src = bm->bits;
   p_dst = grd_bm.bits + grd_bm.row*y + x;
   flat8_rsd8_blit(rsd_src,p_dst,grd_bm.row,bm->w);
}

