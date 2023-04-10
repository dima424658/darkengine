/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkovrld.cpp,v 1.11 2000/02/19 13:25:49 toml Exp $

// overload swing meter
#include <2d.h>
#include <appagg.h>

#include <resapi.h>
#include <playrobj.h>
#include <scrnmode.h>
#include <simtime.h>
#include <scrptapi.h>
#include <scrptmsg.h>
#include <mprintf.h>

#include <schbase.h>
#include <schema.h>

#include <shkovrld.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkutils.h>
#include <shkparam.h>
#include <shkpsipr.h>
#include <shkplayr.h>
#include <shkplcst.h>
#include <shkpltyp.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

IRes *gOverloadBack;
IRes *gOverloadMeter;
IRes *gOverloadBurnout;
IRes *gOverloadPip;
IRes *gOverloadGood;

ulong gOverloadTime;

static int gPsiAudio = -1;

static Rect full_rect = {{200,379},{200 + 64, 379 + 16}};
//--------------------------------------------------------------------------------------
ulong ShockOverloadBurnoutTime()
{
   sOverloadParams *p = GetOverloadParams();
   sPsiState* pPsiState = PsiStateGet(PlayerObject());
   float t;
   int level;
   ePsiPowers curpower;

   AutoAppIPtr(ShockPlayer);

   curpower = pPsiState->m_currentPower;
   level = pShockPlayer->FindPsiLevel(curpower);
   t = p->m_length[level-1];
   //mprintf("burnout time t = %g, level = %d, curpower = %d\n",t,level,curpower);

   if (t == 0)
      t = 1;
   return((int)(t * 1000));
}
//--------------------------------------------------------------------------------------
ulong ShockOverloadMinTime()
{
   sOverloadParams *p = GetOverloadParams();
   float thresh; // the quake playing float
   int psistat;
   ulong maxval;

   AutoAppIPtr(ShockPlayer);

   psistat = pShockPlayer->GetStat(kStatPsi);
   // since stat starts at 1, start the param at 0
   thresh = p->m_thresh[psistat-1];
   if (thresh == 0)
      thresh = 0.5;
   maxval = ShockOverloadBurnoutTime();
   return(maxval * thresh);
}
//--------------------------------------------------------------------------------------
static void SchemaDoneCallback(int /*hSchema*/, ObjID /*schemaID*/, void* /*pData*/)
{
   gPsiAudio = -1;
}
//--------------------------------------------------------------------------------------
void ShockOverloadInit(int which)
{
   gOverloadMeter = LoadPCX("loadmetr");
   gOverloadBack = LoadPCX("loadback");
   gOverloadBurnout = LoadPCX("loadburn");
   gOverloadPip = LoadPCX("loadpip");
   gOverloadGood = LoadPCX("loadgood");

   Rect use_rect;
   sScrnMode smode;
   ScrnModeGet(&smode);

   use_rect.ul.x = (smode.w - RectWidth(&full_rect)) / 2;
   use_rect.ul.y = smode.h - (480 - full_rect.ul.y);
   use_rect.lr.x = use_rect.ul.x + RectWidth(&full_rect);
   use_rect.lr.y = use_rect.ul.y + RectHeight(&full_rect);

   ShockOverlaySetRect(which,use_rect);
   ShockOverlaySetFlags(which, kOverlayFlagBufferTranslucent);
}

//--------------------------------------------------------------------------------------
void ShockOverloadTerm(void)
{
   SafeFreeHnd(&gOverloadBack);
   SafeFreeHnd(&gOverloadMeter);
   SafeFreeHnd(&gOverloadBurnout);
   SafeFreeHnd(&gOverloadPip);
   SafeFreeHnd(&gOverloadGood);
}

//--------------------------------------------------------------------------------------
#define OL_X   0
#define OL_Y   0
#define BURNOUT_DISPLAY_TIME  5000

static tSimTime gLastTime = 0;

void ShockOverloadDraw(void)
{
   Point p;
   grs_bitmap *bm;
   grs_clip saveClip;
   float radval, maxval,minval;
   int clippix;
   ObjID obj = PlayerObject();
   Rect r = {{0,0},{0,0}}; // ShockOverlayGetRect(kOverlayOverload);
 
   radval = GetSimTime() - gOverloadTime;
   gLastTime = GetSimTime();
   maxval = ShockOverloadBurnoutTime(); 
   minval = ShockOverloadMinTime();
   //mprintf("drawing: delta %d vs max %d, min %d\n",(int)radval,(int)maxval,(int)minval);
   /*
   if (radval > maxval + BURNOUT_DISPLAY_TIME)
   {
      return;
   }
   else 
   */
   if (radval > maxval)
   {
      p.x = r.ul.x + OL_X;
      p.y = r.ul.y + OL_Y;
      DrawByHandle(gOverloadBurnout,p);
   }
   else
   {
      // fill in the meter from the left

      IRes *meter = gOverloadMeter;
      if (radval > ShockOverloadMinTime())
         meter = gOverloadGood;

      bm = (grs_bitmap *) meter->Lock();

      saveClip = grd_gc.clip;                 // Clip it good!

      if ((radval == 0) || (maxval == 0))
         clippix = 0;
      else
         clippix = (bm->w * radval / maxval);
      if (clippix < 0)
         clippix = 0;

      gr_safe_set_cliprect(r.ul.x + OL_X, r.ul.x + OL_Y, r.ul.x + OL_X + clippix, r.ul.y + OL_Y + bm->h);
      gr_bitmap(bm, r.ul.x + OL_X, r.ul.y + OL_Y);
      meter->Unlock();

      grd_gc.clip = saveClip;

      p.x = r.ul.x + OL_X + (bm->w * (minval / maxval));
      p.y = r.ul.y + OL_Y;
      DrawByHandle(gOverloadPip,p);
   }

   // draw the framing element
   /*
   p.x = r.ul.x + OL_X;
   p.y = r.ul.y + OL_Y;
   DrawByHandle(gOverloadBack,p);
   */
}
//--------------------------------------------------------------------------------------
void ShockOverloadStateChange(int which)
{
   //mprintf("OverloadStateChange: %d\n",which);
   if (ShockOverlayCheck(which))
   {
      if (gOverloadTime != 0)
      {
         Warning(("Overload time is non-zero at start overload!\n"));
      }
      // start timing
      gOverloadTime = GetSimTime();

      // Set up callback which resets our global upon play completion.
      sSchemaCallParams schParams;
      memset (&schParams, 0, sizeof (schParams));;
      schParams.flags = SCH_SET_CALLBACK;
      schParams.sourceID = OBJ_NULL;
      schParams.callback = SchemaDoneCallback;
      //   schParams.pData = ;         // user data for callback

      // Halt if an email is already playing.
      if (gPsiAudio != -1)
      {
         SchemaPlayHalt(gPsiAudio);
         gPsiAudio = -1;
      }

      // Play the email.
      gPsiAudio = SchemaPlay((Label *)"psiactive", &schParams); //NULL);
   }
   else
      gOverloadTime = 0;
}

//--------------------------------------------------------------------------------------
int ShockOverloadFinish()
{
   int delta;
   int retval;

   //mprintf("OverloadFinish\n");

   AutoAppIPtr(ScriptMan);

   if (gPsiAudio != -1)
   {
      SchemaPlayHalt(gPsiAudio);
      gPsiAudio = -1;
   }

   if (gOverloadTime == 0)
   {
      // too-rapid clicking can cause us to get a finish while the
      // overlay is down, and thus invalid
      ShockOverlayChange(kOverlayOverload,kOverlayModeOff);
      return(kOverloadNormal);
   }

   // look at how far the meter went
   // use the data as of the last time we went to draw, so that it is by definition accurate
   if (gLastTime == 0)
      delta = 0;
   else
      delta = gLastTime - gOverloadTime;

   if (delta < 0)
   {
      // take down the overlay
      ShockOverlayChange(kOverlayOverload,kOverlayModeOff);
      return(kOverloadNormal);
   }

//   mprintf("delta %d vs %d, vs %d\n",delta,ShockOverloadBurnoutTime(),ShockOverloadMinTime());
   if (delta < ShockOverloadBurnoutTime())
   {
      if (delta > ShockOverloadMinTime())
      {
         retval = kOverloadSuccess;
         // feedback
         sScrMsg msg(PlayerObject(),"OverloadSuccess"); 
         msg.flags |= kSMF_MsgPostToOwner;
         pScriptMan->SendMessage(&msg); 
      }
      else
         retval = kOverloadNormal;
   }
   else
   {  
      // do damage
      sScrMsg msg(PlayerObject(),"OverloadDamage"); 
      msg.flags |= kSMF_MsgPostToOwner;
      pScriptMan->SendMessage(&msg); 

      retval = kOverloadBurnout;
   }

   // take down the overlay
   ShockOverlayChange(kOverlayOverload,kOverlayModeOff);

   return(retval);
}
//--------------------------------------------------------------------------------------
IRes *ShockOverloadBitmap(void)
{
   return(gOverloadBack);
}
//--------------------------------------------------------------------------------------
sOverlayFunc OverlayOverload = { 
   ShockOverloadDraw, // draw
   ShockOverloadInit, // init
   ShockOverloadTerm, // term
   NULL, // mouse
   NULL, // dclick (really use)
   NULL, // dragdrop
   NULL, // key
   ShockOverloadBitmap, // bitmap
   "", // upschema
   "", // downschema
   ShockOverloadStateChange, // state
   NULL, // transparency
   0, // distance
   FALSE, // needmouse
   180, // alpha
};
