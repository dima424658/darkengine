/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/valloc.c $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1997/01/22 12:54:19 $
 *
 * Video memory management routines.
 *
 * This file is part of the dev2d library.
 *
 */

#include <grd.h>

#ifdef WIN32
#include <comtools.h>
#include <comdfcn.h>
#include <dispapi.h>
#endif

// returns whether the pointer is monitored or not (i.e., whether it is relocatable).
bool vMonitor(uchar **p)
{
#ifdef WIN32
   if (g_pDev2dDisplayDevice)
      if (IDisplayDevice_SetMonitor(g_pDev2dDisplayDevice, p, kGrDispPtrMonitorOn) != FALSE)
         return TRUE;
#endif
   return FALSE;
}

void vUnmonitor(uchar **p)
{
#ifdef WIN32
   // This function will try to unregister the bits pointer of the bitmap
   // should it be a relocatable pointer.
   if (g_pDev2dDisplayDevice)
      IDisplayDevice_SetMonitor(g_pDev2dDisplayDevice, p, kGrDispPtrMonitorOff);
#endif
}

// returns whether the pointer is monitored or not (i.e., whether it is relocatable).
bool vAlloc (uchar **p, int w, int h)
{
   if (grd_valloc_mode)
      *p = (uchar *)0;
   else
      *p = grd_cap->vbase;

   return vMonitor(p);
}

void vFree (uchar **p)
{
   vUnmonitor(p);
}
