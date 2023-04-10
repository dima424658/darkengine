/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shklbox.cpp,v 1.5 2000/02/19 13:25:35 toml Exp $

// container gump
#include <2d.h>

#include <resapi.h>

#include <shklbox.h>
#include <shkovrly.h>
#include <shkutils.h>
#include <shkovcst.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

IRes *gLetterboxHnd;

//--------------------------------------------------------------------------------------
void ShockLetterboxInit(int which)
{
   gLetterboxHnd= LoadPCX("lettrbox");
}

//--------------------------------------------------------------------------------------
void ShockLetterboxTerm(void)
{
   SafeFreeHnd(&gLetterboxHnd);
}

//--------------------------------------------------------------------------------------
#define LBOX_H    69
void ShockLetterboxDraw(void)
{
   Point p;
   p.x = 0;
   p.y = 0;
   DrawByHandle(gLetterboxHnd,p);
   p.y = (480 - LBOX_H);
   DrawByHandle(gLetterboxHnd,p);

}


//--------------------------------------------------------------------------------------
void ShockLetterboxSet(BOOL lbox)
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

   ShockOverlayChange(kOverlayMeters,m1);
   ShockOverlayChange(kOverlayMiniFrame,m1);
   ShockOverlayChange(kOverlayAmmo,m1);
   ShockOverlayChange(kOverlayCrosshair,m1);
   ShockOverlayChange(kOverlayLetterbox,m2);
}

