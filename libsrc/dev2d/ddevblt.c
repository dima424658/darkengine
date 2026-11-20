/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/ddevblt.c $
 * $Revision: 1.2 $
 * $Author: TOML $
 * $Date: 1996/12/13 16:55:38 $
 *
 * Display device bitmap functions
 *
 * This file is part of the dev2d library.
 *
 */

#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <dispapi.h>

#include <grs.h>
#include <grd.h>
#include <gdbm.h>
#include <comdfcn.h>
#include <canvas.h>
#include <bmftype.h>
#include <indexmac.h>

void dispdev_ubitmap(grs_bitmap *bm, int x, int y)
{
   ushort old_canvas_flags, old_bitmap_flags;
   old_canvas_flags = grd_canvas->bm.flags;
   old_bitmap_flags = bm->flags;

   bm->flags &= ~BMF_DEV_MUST_BLIT;
   grd_canvas->bm.flags &= ~BMF_DEV_MUST_BLIT;

   gr_push_canvas(grd_canvas);
   gd_ubitmap(bm, x, y);
   gr_pop_canvas();

   bm->flags = old_bitmap_flags;
   grd_canvas->bm.flags = old_canvas_flags;
}

gdubm_func *dispdev_ubitmap_expose(grs_bitmap *bm, int x, int y)
{
   return dispdev_ubitmap;
}

