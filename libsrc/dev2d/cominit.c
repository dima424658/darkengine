/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/cominit.c $
 * $Revision: 1.6 $
 * $Author: TOML $
 * $Date: 1996/12/13 16:01:44 $
 *
 */

#ifdef WIN32

#include <grc.h>
#include <grd.h>
#include <mode.h>
#include <devtab.h>
#include <dftctab.h>
#include <comdfcn.h>

#include <comtools.h>
#include <appagg.h>
#include <dispapi.h>

int com_detect(grs_sys_info *info)
{
   // For the COM interface, the application is assumed to
   // have gotten hold of a display device interface corresponding
   // to the device it wants to draw to. We'll have that device
   // and hook the 2D's internal function table to it.
   // Because the application is doing this, we assume that
   //    1) The COM detection will always succeed since the application
   //       has already done the detection.
   //    2) The canvas tables used are flat.

   // Another assumption is that the modes will not be set here,
   // but will instead be set in the device init function.
   // We therefore start with an empty mode list.
   // We also don't care about the amount of memory here; the device
   // initialization function will set it.

   // The global com_display_interface stores the interface to the
   // com object.

   info->id_maj = 0;
   info->id_min = 0;
   info->memory = 0;

   grd_device_table = com_device_table;

   if (g_pDev2dDisplayDevice)
   {

      if (IDisplayDevice_Open(g_pDev2dDisplayDevice, &grd_mode_cap, 0) != kGrOk)
         return kGrFail;

      // Get a list of all supported modes
      IDisplayDevice_GetInfo(g_pDev2dDisplayDevice, info, grd_mode_info);
   }
   return kGrOk;
}

//-----------------------------------------------------------------------------
// This stores the interface to the display device that the COM device
// functions use.
//-----------------------------------------------------------------------------

void gd_use_com()
{
   // We Hate Globals. But I'm going to use one anyways. poo
   g_pDev2dDisplayDevice = AppGetObj(IDisplayDevice);

   // For the COM interface, the application is assumed to
   // have gotten hold of a display device interface corresponding
   // to the device it wants to draw to. We'll have that device
   // and hook the 2D's internal function table to it.
   // Because the application is doing this, we assume that
   //    1) The COM detection will always succeed since the application
   //       has already done the detection.
   //    2) The canvas tables used are flat.

   gdd_detect_func         = com_detect;

   // Jacobson, 3-5-96
   // About 1/3 of the machines we've tested on require aligned
   // memory accesses. Since we believe that using aligned blitting
   // functions won't slow down rendering operations, we'll now try
   // to hook in the aligned function tables as a default.
   // (I haven't made an aligned 16 bit function table yet).

   // kevin, 4-8-96 I'll deal with this later.

   gdd_flat8_canvas_table   = gdd_default_flat8_canvas_table;
   gdd_flat16_canvas_table  = gdd_default_flat16_canvas_table;
   gdd_dispdev_canvas_table = gdd_default_dispdev_canvas_table;
}


void gd_release_com()
{
   SafeRelease(g_pDev2dDisplayDevice);
}

/*
extern int gd_init();

int gd_com_init(IDisplayDevice *dev, int descFlags)
{
   gd_use_com(dev, descFlags);
   return gd_init();
}

*/

#endif
