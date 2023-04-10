/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/screen.c $
 * $Revision: 1.7 $
 * $Author: KEVIN $
 * $Date: 1998/04/28 13:46:00 $
 *
 * Screen handling routines.
 *
 * This file is part of the dev2d library.
 *
 */

#include <lg.h>
#include <bitmap.h>
#include <canvas.h>
#include <context.h>
#include <grd.h>
#include <grdev.h>
#include <grmalloc.h>
#include <mode.h>
#include <valloc.h>

/* set a screen */

void gr_set_screen(grs_screen *s)
{
   grd_screen = s;
   grd_visible_canvas = s->c+1;
   grd_back_canvas = grd_screen_canvas = s->c;
   gr_set_canvas(grd_back_canvas);
}

/* allocate enough video memory for a screen of the specified size.  then set
   up the screen structure describing this screen and the 2 system canvases
   for drawing on it.  return a pointer to the new screen structure. */

grs_screen *gr_alloc_screen(int w, int h)
{
   grs_screen *s;
   grs_canvas *c;
   uchar *p;
#ifndef WIN32
   uchar *b;
#endif

   w = gr_set_width(w);
   if ((w < grd_cap->w) || (h < grd_cap->h))
      return NULL;
#ifndef WIN32
   if (h > grd_screen_max_height)
      return NULL;
#endif

// get memory for screen structure itself and 2 system canvases,
// and video ram for the screen itself.

   if ((p = (uchar *)gr_malloc(sizeof(*s)+2*sizeof(*c))) == NULL)
      goto bailout2;
// Point the screen at the first part of allocated memory and the
// first canvas following that in memory.
   s = (grs_screen *)p;
   c = (grs_canvas *)(p+sizeof (*s));

   /* start with upper left visible. */
   s->x = 0;
   s->y = 0;
   s->c = c;

#ifndef WIN32
   if (vAlloc(&b, w, h))
      vUnmonitor(&b);   //presumably, b won't move around until after we're done with it
   if (b == (uchar *)-1)
      goto bailout1;

// Make ourselves a bitmap of device type.  Will use grd_screen_row
// to know the offset to next scan line.
   gr_init_bitmap(&s->bm,b,BMT_DEVICE_VIDMEM,0,w,h);

// First set up the full screen canvas
   gr_init_canvas(c,s->bm.bits,BMT_DEVICE_VIDMEM,w,h);

// Then set up the visible screen canvas (which has clip rect equal to physical screen size)
   gr_init_canvas(c+1,s->bm.bits,BMT_DEVICE_VIDMEM,w,h);
   gr_cset_fix_cliprect(c+1,0,0,fix_make(grd_cap->w,0),fix_make(grd_cap->h,0));
   c[1].bm.w = grd_cap->w;    // and its true width is that of the physical screen
   c[1].bm.h = grd_cap->h;    // and its true height is that of the physical screen too
   return s;
bailout1:
   gr_free (s);
#else
// set up the visible canvas:

   gr_init_bm(&s->bm, grd_cap->vbase, BMT_DEVICE_VIDMEM, 0, grd_cap->w, grd_cap->h);
   gr_init_canvas(c, s->bm.bits, BMT_DEVICE_VIDMEM, grd_cap->w, grd_cap->h);
   gr_cset_fix_cliprect(c, 0, 0, fix_make(grd_cap->w,0), fix_make(grd_cap->h,0));

// set up the back canvas, if necessary
   if (h >= 2*grd_cap->h) {
      AssertMsg(grd_cap->vbase2 != NULL, "Expected a back buffer");
      gr_init_bm(&s->bm2, grd_cap->vbase2, BMT_DEVICE_VIDMEM, 0, grd_cap->w, grd_cap->h);
   }
   gr_init_canvas(c+1, s->bm.bits, BMT_DEVICE_VIDMEM, grd_cap->w, grd_cap->h);
   gr_cset_fix_cliprect(c+1, 0, 0, fix_make(grd_cap->w,0), fix_make(grd_cap->h,0));
   return s;
#endif
bailout2:
   return NULL;
}

/* free memory for screen and its related data structures. */
void gr_free_screen(grs_screen *s)
{
   grs_canvas *c = (grs_canvas *)(s + 1);
   gr_close_bitmap(&s->bm);
   gr_close_bitmap(&s->bm2);
   gr_close_canvas(c);
   gr_close_canvas(c+1);

#ifndef WIN32
   vFree (&s->bm.bits);
#endif

   gr_free (s);
}
