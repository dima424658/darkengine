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
#ifdef _WIN32
   // @Note (toml 12-13-96): this is costly, but for now is isolated
   // to DD in a window so it's probably okay.  Performance sensitive
   // clients can/should circumvent this function, or we need a
   // better protocol
   IDeviceBitmap * dst;
   IDeviceBitmap * src;

   IDisplayDevice_FindBitmap(g_pDev2dDisplayDevice, &grd_canvas->bm, &dst);
   IDisplayDevice_FindBitmap(g_pDev2dDisplayDevice, bm, &src);

   AssertMsg1(dst && src, "dispdev_ubitmap() requires 2 device bitmaps, had %d", (!!(dst) + !!(src)));

   if (dst && src)
      IDeviceBitmap_SimpleBlt(dst, x, y, src);

   SafeRelease(dst);
   SafeRelease(src);
#else
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
#endif
}

#pragma off (unreferenced)
gdubm_func *dispdev_ubitmap_expose(grs_bitmap *bm, int x, int y)
{
   return dispdev_ubitmap;
}
#pragma on (unreferenced)

