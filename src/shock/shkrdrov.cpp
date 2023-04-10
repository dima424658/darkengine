/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkrdrov.cpp,v 1.11 2000/02/19 13:26:03 toml Exp $

// radar overlay
#include <shkrdrov.h>

#include <2d.h>
#include <appagg.h>
#include <scrnmode.h>
#include <simtime.h>

#include <shkovrly.h>
#include <shkovcst.h>
#include <shkutils.h>
#include <shkrdrbs.h>
#include <shkanmsp.h>
#include <shkradar.h>
#include <shkplcst.h>
#include <shkpsapi.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static IRes *gBackHnd;
static IRes *gBlipHnd;
static IRes *gPowerupHnd;

#define RADAR_W   128
#define RADAR_H   128

ulong gRadarScanTime = 0;

#define RADAR_SCAN_PERIOD  1000
//--------------------------------------------------------------------------------------
void ShockRadarInit(int which)
{
   gBackHnd= LoadPCX("rdrback");
   gBlipHnd = LoadPCX("blipevil");
   gPowerupHnd = LoadPCX("blipgood");
   
   Rect r;
   sScrnMode smode;
   ScrnModeGet(&smode);

   r.ul.x = (smode.w - RADAR_W) / 2;
   r.ul.y = smode.h - RADAR_H;
   r.lr.x = r.ul.x + RADAR_W;
   r.lr.y = r.ul.y + RADAR_H;

   ShockOverlaySetRect(which, r);

   ShockOverlaySetFlags(which, kOverlayFlagTranslucent);
}

//--------------------------------------------------------------------------------------
void ShockRadarTerm(void)
{
   SafeFreeHnd(&gBackHnd);
   SafeFreeHnd(&gBlipHnd);
   SafeFreeHnd(&gPowerupHnd);
}

//--------------------------------------------------------------------------------------
void ShockRadarDraw(void)
{
   int numTargets;
   Point *pTargetPts;
   int *pTargetData;
   Point pt;
   Rect r = ShockOverlayGetRect(kOverlayRadar);
   int i;
   ulong curtime;

   // time for a new scan?
   curtime = GetSimTime();
   if (gRadarScanTime <= curtime)
   {
      gRadarScanTime = gRadarScanTime + RADAR_SCAN_PERIOD;
      RadarScan();

   }
   // update location of active targets
   g_radar.Status(&numTargets, &pTargetPts, &pTargetData);

   //DrawByHandle(gBackHnd,r.ul);

   AutoAppIPtr(PlayerPsi);
   for (i=0; i<numTargets; i++)
   {
      pt.x = r.ul.x+(RADAR_W/2)+pTargetPts[i].x;
      pt.y = r.ul.y+(RADAR_H/2)+pTargetPts[i].y;
      switch (pTargetData[i])
      {
      case kTargetEnemy:
         if (pPlayerPsi->IsActive(kPsiRadar))
            DrawByHandleCenter(gBlipHnd,pt);
         break;
      case kTargetPowerup:
         if (pPlayerPsi->IsActive(kPsiSeeker))
            DrawByHandleCenter(gPowerupHnd,pt);
         break;
      }
   }
}
//--------------------------------------------------------------------------------------
IRes *ShockRadarBitmap(void)
{
   return(gBackHnd);
}
//--------------------------------------------------------------------------------------
void ShockRadarStateChange(int which)
{
   if (ShockOverlayCheck(which))
   {
      // up
      RadarScan();
      gRadarScanTime = GetSimTime() + RADAR_SCAN_PERIOD;
   }
   else
      gRadarScanTime = 0;
}
//--------------------------------------------------------------------------------------
sOverlayFunc OverlayRadar = { 
   ShockRadarDraw, // draw
   ShockRadarInit, // init
   ShockRadarTerm, // term
   NULL, // mouse
   NULL, // dclick (really use)
   NULL, // dragdrop
   NULL, // key
   ShockRadarBitmap, // bitmap
   "", // upschema
   "", // downschema
   ShockRadarStateChange, // state
   NULL, // transparency
   0, // distance
   FALSE, // needmouse
   128,// alpha
};
