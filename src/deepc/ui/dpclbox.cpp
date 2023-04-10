/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// container gump
#include <2d.h>

#include <resapi.h>

#include <dpclbox.h>
#include <dpcovrly.h>
#include <dpcutils.h>
#include <dpcovcst.h>

IRes *gLetterboxHnd;

//--------------------------------------------------------------------------------------
void DPCLetterboxInit(int which)
{
   gLetterboxHnd= LoadPCX("lettrbox");
}

//--------------------------------------------------------------------------------------
void DPCLetterboxTerm(void)
{
   SafeFreeHnd(&gLetterboxHnd);
}

//--------------------------------------------------------------------------------------
#define LBOX_H    69
void DPCLetterboxDraw(unsigned long inDeltaTicks)
{
   Point p;
   p.x = 0;
   p.y = 0;
   DrawByHandle(gLetterboxHnd,p);
   p.y = (480 - LBOX_H);
   DrawByHandle(gLetterboxHnd,p);

}


//--------------------------------------------------------------------------------------
void DPCLetterboxSet(BOOL lbox)
{
   int m1,m2;
   if (!lbox)
   {
      m1 = kOverlayModeOn;
      m2 = kOverlayModeOff;
   }
   else
   {
      m1 = kOverlayModeOff;
      m2 = kOverlayModeOn;
   }

   DPCOverlayChange(kOverlayMeters,m1);
   DPCOverlayChange(kOverlayMiniFrame,m1);
   DPCOverlayChange(kOverlayAmmo,m1);
   DPCOverlayChange(kOverlayCrosshair,m1);
   DPCOverlayChange(kOverlayLetterbox,m2);
}

