//------------------------------------------------------------------------------
// $Source: x:/prj/tech/libsrc/dev2d/RCS/comdfcn.cc $
// $Author: KEVIN $
// $Date: 1998/03/12 10:43:14 $
// $Revision: 1.11 $
//------------------------------------------------------------------------------
// Description :
// This file contains all of the COM-based device functions that can be
// accessed from the device function table.
//------------------------------------------------------------------------------
#ifdef WIN32

#include <string.h>
#include <inttype.h>

//------------------------------------------------------------------------------
// COM stuff
//------------------------------------------------------------------------------

#include <comdfcn.h>
#include <dispapi.h>

//------------------------------------------------------------------------------
// Low-level 2d structures.
//------------------------------------------------------------------------------

#include <grc.h>
#include <grd.h>

#include <mode.h>
#include <bitmap.h>

#include <lg.h>

#include <dbg.h>

extern "C"
{
IDisplayDevice * g_pDev2dDisplayDevice;
}

//------------------------------------------------------------------------------
//* This function sets up the device after the mode is set. But really,
//* for this implementation, all of the hard work is done in the setmode
//* routine.
//------------------------------------------------------------------------------

int com_init(grs_sys_info *)
{
   return kGrOk;
}

//------------------------------------------------------------------------------
//* This function closes the device and destroys any currently active
//* windows.
//------------------------------------------------------------------------------

int com_close(grs_sys_info *)
{
   if (!g_pDev2dDisplayDevice)
      return kGrFail;

   return g_pDev2dDisplayDevice->Close();       // Call the Device COM interface!
}

//------------------------------------------------------------------------------
//* Set the mode, creating a new window (and perhaps clearing the window
//* as well.
//------------------------------------------------------------------------------

int   com_set_mode(int mode, int clear)
{
   AssertMsg(g_pDev2dDisplayDevice != NULL, "Cannot manipulate frame buffer without an IDisplayDevice");
   if (!g_pDev2dDisplayDevice)
      return kGrFail;

   // First thing to do is to call the COM set mode routine, which
   // will create a window and set the mode, returning failure if
   // it couldn't.

   if (g_pDev2dDisplayDevice->SetMode(mode, clear) != kGrOk) {
      Warning(("com_set_mode(): mode %i is not supported by IDisplayDevice!\n", mode));
      return kGrFail;
   }

   // Find out which mode we _actually_ got...
   grd_mode = g_pDev2dDisplayDevice->GetMode(NULL);

   if (mode != grd_mode) Warning((
      "com_set_mode(): tried to set mode %i. IDisplayDevice returned mode %i\n",
      mode, grd_mode));

   // We set the mode ok. Now we've got to set some global 2D variables
   // to make everything happy and joyous.

   uchar bitDepth = 0;
   for (grs_mode_info *pModeInfo = grd_mode_info + GRD_MODES -1;
        pModeInfo >= grd_mode_info; pModeInfo--) 
      if (pModeInfo->mode_2d == grd_mode) {
         bitDepth = pModeInfo->bitDepth;
         break;
      }

   switch (bitDepth) {
   case 0:
      CriticalMsg1("com_set_mode(): Current IDisplayDevice mode %i is unsupported!", grd_mode);
      break;
   case 8:
      grd_screen_bmt = BMT_FLAT8;
      break;
   case 15:
   case 16:
      grd_screen_bmt = BMT_FLAT16;
      break;
   default:
      Warning(("com_set_mode(): current mode %i has unsupported bitDepth %i\n",
         grd_mode, bitDepth));
      grd_screen_bmt = -1;
      return kGrFail;
   }

   grd_screen_row = grd_cap->rowBytes;

   // Do we have two pages of video memory?

   if (grd_cap->vbase2 == NULL)
      grd_screen_max_height = grd_cap->h;
   else
      grd_screen_max_height = 2*grd_cap->h;

   return kGrOk;
}

//------------------------------------------------------------------------------
// Return the mode associated with the current window.
//------------------------------------------------------------------------------

int   com_get_mode(void)
{
   AssertMsg(g_pDev2dDisplayDevice != NULL, "Cannot manipulate frame buffer without an IDisplayDevice");
   if (!g_pDev2dDisplayDevice)
      return kGrFail;
   return g_pDev2dDisplayDevice->GetMode(NULL);
}

//------------------------------------------------------------------------------
// Set the palette associated with the window.
//------------------------------------------------------------------------------

void  com_set_pal(int start, int n, uint8 *pal)
{
   AssertMsg(g_pDev2dDisplayDevice != NULL, "Cannot manipulate frame buffer without an IDisplayDevice");
   if (!g_pDev2dDisplayDevice)
      return;
   Assrt(pal != NULL);
   if (g_pDev2dDisplayDevice->SetPalette(start, n, pal) != TRUE) {
      Warning(("Unexpected palette set failure!\n"));
   }

   // Jacobson, 1/21
   // Should we at this point change grd_screen's pal field??
}

//------------------------------------------------------------------------------
// Get the palette associated with the window.
//------------------------------------------------------------------------------

void  com_get_pal(int start, int n, uint8 *pal)
{
   AssertMsg(g_pDev2dDisplayDevice != NULL, "Cannot manipulate frame buffer without an IDisplayDevice");
   if (!g_pDev2dDisplayDevice)
      return;
   Assrt(pal != NULL);
   if (g_pDev2dDisplayDevice->GetPalette(start, n, pal) != TRUE) {
      Warning(("Unexpected get Palette failure!\n"));
   }
}

//------------------------------------------------------------------------------
// Set the focus (page flip)
//------------------------------------------------------------------------------
int   com_set_width(int w)
{
   return w;
}

#pragma off(unreferenced)
void  com_set_focus(int x, int y)
{
   AssertMsg(g_pDev2dDisplayDevice != NULL, "Cannot manipulate frame buffer without an IDisplayDevice");
   if (!g_pDev2dDisplayDevice)
      return;
   Assrt(grd_screen != NULL);

   // First thing to do is to check to see if we're page flipping or not.
   // Also make sure we get valid inputs (0,0) and (0,h) are the only
   // things supported.
   Assrt(x == 0); Assrt((y == 0) || (y == grd_cap->h));

   if (y != grd_screen->y) {

      // So, the y's are unequal. page flip, baby.
      g_pDev2dDisplayDevice->PageFlip();
      grd_screen->y = (short )y;

   }
}
#pragma on(unreferenced)

//------------------------------------------------------------------------------
// Get the focus. We're spoofing this to only allow pageflipping, not
// panning. So there.
//------------------------------------------------------------------------------

void  com_get_focus(int *x, int *y)
{
   AssertMsg(g_pDev2dDisplayDevice != NULL, "Cannot manipulate frame buffer without an IDisplayDevice");
   if (!g_pDev2dDisplayDevice)
      return;
   Assrt(grd_screen != NULL);

   // We'll just look into the grd_screen to get the current coordinate.
   *x = grd_screen->x;
   *y = grd_screen->y;
}

//------------------------------------------------------------------------------
// Save the video state before we take over the screen. We're assuming
// that this is only called at startup time, and not during the middle
// of operation, which is a subset of what the 2D actually assumes (it
// assumes you can save and restore any time).
//------------------------------------------------------------------------------

int com_save_state(VideoState *)
{
//   Warning(("saving state.Bwahh hah ho.\n"));
   return kGrOk;
}

//------------------------------------------------------------------------------
// Restore a saved video state.
//------------------------------------------------------------------------------

int com_restore_state(VideoState *)
{
//   Warning(("restoring state.Oh hah hhawb.\n"));
   return kGrOk;
}

//------------------------------------------------------------------------------
// Check the status of the horitzontal retrace (??)
//------------------------------------------------------------------------------

int com_stat_htrace(void)
{
   AssertMsg(g_pDev2dDisplayDevice != NULL, "Cannot manipulate frame buffer without an IDisplayDevice");
   if (!g_pDev2dDisplayDevice)
      return kGrFail;
   return g_pDev2dDisplayDevice->StatHTrace();
}

//------------------------------------------------------------------------------
// Returns TRUE if we're in the blanking interval.
//------------------------------------------------------------------------------

int com_stat_vtrace(void)
{
   AssertMsg(g_pDev2dDisplayDevice != NULL, "Cannot manipulate frame buffer without an IDisplayDevice");
   if (!g_pDev2dDisplayDevice)
      return kGrFail;
   return g_pDev2dDisplayDevice->StatVTrace();
}


//------------------------------------------------------------------------------
// Get color bitmasks for the current mode
//------------------------------------------------------------------------------

void com_get_rgb_bitmask(grs_rgb_bitmask *bitmask)
{
   sGrModeInfoEx mode_info;
   int mode;

   if (!g_pDev2dDisplayDevice) {
      Warning(("com_get_rgb_bitmask(): no display device!\n"));
      return;
   }

   mode = g_pDev2dDisplayDevice->GetMode(&mode_info);
   if (mode >= 0) {
      bitmask->red = mode_info.redMask;
      bitmask->green = mode_info.greenMask;
      bitmask->blue = mode_info.blueMask;
   } else {
      Warning(("com_get_rgb_bitmask(): no mode set!\n"));
      bitmask->red = 0;
      bitmask->green = 0;
      bitmask->blue = 0;
   }
}

#endif
