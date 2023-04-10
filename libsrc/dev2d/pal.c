// $Header: x:/prj/tech/libsrc/dev2d/RCS/pal.c 1.5 1998/01/27 16:41:08 JAEMZ Exp $
// Routines and data for non-hardware-dependent palette control.

#include <string.h>

#include <buffer.h>
#include <grdev.h>
#include <grd.h>
#include <rgb.h>
#include <pal16.h>
#include <mode.h>
#include <pal.h>
#include <pal_.h>

palSettingCback grd_pal_callbacks[NUM_PAL_CBACKS];

/* copy user's palette into shadow palette, then set real palette. */
void gr_set_pal (int start, int n, uchar *pal_data)
{
   int i;
   uchar r,g,b;            /* red,green,blue values */
   uchar *src,*dst;
   uchar buff[768];

   if (n <= 0)
      return;

   memcpy (grd_pal+3*start, pal_data, 3*n);
   for (i=start; i<start+n; i++) {
      r = grd_pal[3*i];
      g = grd_pal[3*i+1];
      b = grd_pal[3*i+2];
      grd_bpal[i] = gr_bind_rgb (r, g, b);
   }

   src = grd_pal;
   dst = buff;

   // Go through the list and do any modifying callbacks that
   // need to be done.
   for (i=0;i<NUM_PAL_CBACKS;++i) {
      if (grd_pal_callbacks[i]) {
         grd_pal_callbacks[i](dst,src,start, n);
         src = dst;
      }
   }
   gr_set_screen_pal(start, n, src+3*start);
}

/* copy the shadow palette to a destination buffer. */
void gr_get_pal (int start, int n, uchar *pal_data)
{
   if (n <= 0)
      return;
   memcpy (pal_data, grd_pal+3*start, 3*n);
}

extern void (*grd_set_mode_callback)();
static bool callback_installed = FALSE;
static void (*callback_chain)() = NULL;
static uchar *default_pal = NULL;
static void set_default_pal()
{
   if ((grd_mode_info[grd_mode].bitDepth == 8)
                  && (default_pal != NULL))
      gr_set_pal(0, 256, default_pal);
   if (callback_chain != NULL)
      callback_chain();
}

// sets the default palette and adds set_default_pal() to 
// the set_mode_callback chain if it hasn't already been added.

void gr_set_default_pal(uchar *pal)
{
   default_pal = pal;
   if (callback_installed == TRUE)
      return;

   callback_chain = grd_set_mode_callback;
   grd_set_mode_callback = set_default_pal;
   callback_installed = TRUE;
}

