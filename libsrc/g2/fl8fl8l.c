/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/fl8fl8l.c $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1996/05/01 15:58:35 $
 *
 * Linear mapper shell.
 *
 * This file is part of the g2 library.
 *
 */

#include <dev2d.h>
#include <tmapd.h>
#include <tmapfcn.h>
#include <dbg.h>

extern void flat8_flat8_opaque_p2_wrap_il();
extern void flat8_flat8_opaque_np2_nowrap_il();

static void tile(grs_bitmap *bm, int n, grs_vertex **vpl, g2s_tmap_info *ti)
{
   int i;

   Assrt(bm->h<=256);
   Assrt(bm->w<=256);
   Assrt(bm->row<=256);
   ti->p_src = bm->bits;
   ti->mask = ((bm->h-1) << (15+bm->wlog)) + (bm->w-1);
   ti->il_func = flat8_flat8_opaque_p2_wrap_il;
// translate 16.16 values to 8.24 for u and 16-wlog.16+wlog for v
   for (i=0; i<n; i++) {
      vpl[i]->u<<=7;
      vpl[i]->v<<=(bm->wlog-1);
   }
   do_lmap(n, vpl, lmap_shell);
// restore original values.
   for (i=0; i<n; i++) {
      vpl[i]->u>>=7;
      vpl[i]->v>>=(bm->wlog-1);
   }
}

void flat8_flat8_opaque_ulmap(grs_bitmap *bm, int n, grs_vertex **vpl)
{
   g2s_tmap_info *ti=&g2d_tmap_info;

   ti->bm = bm;
   ti->drow = grd_bm.row;
   if ((bm->row!=1<<bm->wlog)||
       (((bm->w-1)&(bm->w))!=0)||
       (((bm->h-1)&(bm->h))!=0)) {
      ti->il_func = flat8_flat8_opaque_np2_nowrap_il;
      do_lmap(n, vpl, lmap_shell);
   } else {
      tile(bm, n, vpl, ti);
   }
}
