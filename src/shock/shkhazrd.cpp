/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkhazrd.cpp,v 1.14 2000/02/19 13:25:20 toml Exp $

// radiation meter
#include <2d.h>

#include <resapi.h>
#include <playrobj.h>
#include <scrnmode.h>
#include <mprintf.h>

#include <schema.h>

#include <shkhazrd.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkutils.h>
#include <shkhazpr.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

IRes *gRadMeter;
IRes *gRadBack;
IRes *gRadIcon;
IRes *gRadGray;

static Rect rad_rect = {{10,379},{10 + 128, 379 + 32}};
//--------------------------------------------------------------------------------------
void ShockRadInit(int which)
{
   gRadMeter = LoadPCX("radmeter");
   gRadBack = LoadPCX("radback");
   gRadIcon = LoadPCX("radicon");
   gRadGray = LoadPCX("radgray");

   Rect use_rect;
   sScrnMode smode;
   ScrnModeGet(&smode);

   use_rect.ul.x = rad_rect.ul.x;
   use_rect.ul.y = smode.h - (480 - rad_rect.ul.y);
   use_rect.lr.x = use_rect.ul.x + RectWidth(&rad_rect);
   use_rect.lr.y = use_rect.ul.y + RectHeight(&rad_rect);

   ShockOverlaySetRect(which,use_rect);
   ShockOverlaySetFlags(which, kOverlayFlagBufferTranslucent);
}

//--------------------------------------------------------------------------------------
void ShockRadTerm(void)
{
   SafeFreeHnd(&gRadBack);
   SafeFreeHnd(&gRadMeter);
   SafeFreeHnd(&gRadIcon);
   SafeFreeHnd(&gRadGray);
}

//--------------------------------------------------------------------------------------
#define RAD_X  33
#define RAD_Y  0
void ShockRadDraw(void)
{
   //Point p;
   grs_bitmap *bm;
   grs_clip saveClip;
   float radval, maxval;
   int clippix;
   ObjID obj = PlayerObject();
   Rect r = {{0,0},{0,0}}; //ShockOverlayGetRect(kOverlayRadiation);

   if (!gPropRadLevel->Get(obj, &radval))
      radval = 0;

   maxval = 35.0; // how to determine for real??

   // fill in the meter from the left
   bm = (grs_bitmap *) gRadMeter->Lock();

   saveClip = grd_gc.clip;                 // Clip it good!

   if ((radval == 0) || (maxval == 0))
      clippix = 0;
   else
      clippix = ((bm->w - RAD_X) * radval / maxval);
   if (clippix < 0)
      clippix = 0;

   gr_safe_set_cliprect(r.ul.x + RAD_X, r.ul.x + RAD_Y, r.ul.x + RAD_X + clippix, r.ul.y + RAD_Y + bm->h);
   gr_bitmap(bm, r.ul.x + RAD_X, r.ul.y + RAD_Y);
   gRadMeter->Unlock();

   grd_gc.clip = saveClip;

   // draw the framing element
   /*
   p.x = r.ul.x + RAD_X;
   p.y = r.ul.y + RAD_Y;
   DrawByHandle(gRadBack,p);
   */

   // draw the icon to the side
   float radamb = 0;
   gPropRadAmbient->Get(PlayerObject(),&radamb);

   if (radamb == 0)
      DrawByHandle(gRadGray,r.ul);
   else
      DrawByHandle(gRadIcon,r.ul);
     
}
//--------------------------------------------------------------------------------------
IRes *ShockRadBitmap(void)
{
   return(gRadBack);
}
//--------------------------------------------------------------------------------------
IRes *gPoisonIcon;
IRes *gPoisonBack;

static Rect tox_rect = {{10,345},{10 + 128, 345 + 32}};

static float gPoisonVal = -1;
//--------------------------------------------------------------------------------------
void ShockPoisonInit(int which)
{
   gPoisonIcon = LoadPCX("poisicon");
   gPoisonBack = LoadPCX("poisback");

   Rect use_rect;
   sScrnMode smode;
   ScrnModeGet(&smode);

   use_rect.ul.x = tox_rect.ul.x;
   use_rect.ul.y = smode.h - (480 - tox_rect.ul.y);
   use_rect.lr.x = use_rect.ul.x + RectWidth(&tox_rect);
   use_rect.lr.y = use_rect.ul.y + RectHeight(&tox_rect);

   //gPoisonVal = -1;

   ShockOverlaySetRect(which,use_rect);
   ShockOverlaySetFlags(which, kOverlayFlagBufferTranslucent);
}

//--------------------------------------------------------------------------------------
void ShockPoisonTerm(void)
{
   SafeFreeHnd(&gPoisonIcon);
   SafeFreeHnd(&gPoisonBack);
}

//--------------------------------------------------------------------------------------
void ShockPoisonDraw(void)
{
   Point p;
   float poisonval;
   ObjID obj = PlayerObject();
   Rect r = ShockOverlayGetRect(kOverlayPoison);
   int i;
 
   if (!gPropToxin->Get(obj, &poisonval))
      return;

   gPoisonVal = poisonval;
   //mprintf("drawing, with poisonval of %g",poisonval);
   
   p.x = 0; // r.ul.x;
   p.y = 0; // r.ul.y;
   for (i=0; i < poisonval; i++)
   {
      DrawByHandle(gPoisonIcon,p);      
      p.x = p.x + 22;
   }

}
//--------------------------------------------------------------------------------------
IRes *ShockPoisonBitmap(void)
{
   return(gPoisonBack);
}
//--------------------------------------------------------------------------------------
bool ShockPoisonUpdate(void)
{
   // what the hell, the first time I draw it doesn't show up, so
   // this code doesn't really work.
   // Argh.
   return(TRUE);
   /*
   float newpois = -1;
   gPropToxin->Get(PlayerObject(), &newpois);

   //mprintf("newpois %g vs gPois %g\n",newpois,gPoisonVal);
   if (newpois != gPoisonVal)
      return(TRUE);
   return(FALSE);
   */
}
//--------------------------------------------------------------------------------------
void ShockPoisonStateChange(int which)
{
   static float lastval = 0;
   float poisonval = 0;
   gPropToxin->Get(PlayerObject(), &poisonval);
   if (ShockOverlayCheck(which))
   {
      if ((lastval != poisonval) && (poisonval > 0))
         SchemaPlay((Label *)"bb12",NULL);         
   }
   lastval = poisonval;
   gPoisonVal = -1;
}
//--------------------------------------------------------------------------------------
sOverlayFunc OverlayRad = { 
   ShockRadDraw, // draw
   ShockRadInit, // init
   ShockRadTerm, // term
   NULL, // mouse
   NULL, // dclick (really use)
   NULL, // dragdrop
   NULL, // key
   ShockRadBitmap, // bitmap
   "", // upschema
   "", // downschema
   NULL, // state
   ShockOverlayFullTransp, // transparency
   0, // distance
   FALSE, // needmouse
   180, // alpha
};

sOverlayFunc OverlayPoison = { 
   ShockPoisonDraw, // draw
   ShockPoisonInit, // init
   ShockPoisonTerm, // term
   NULL, // mouse
   NULL, // dclick (really use)
   NULL, // dragdrop
   NULL, // key
   ShockPoisonBitmap, // bitmap
   "", // upschema
   "", // downschema
   ShockPoisonStateChange, // state
   ShockOverlayFullTransp, // transparency
   0, // distance
   FALSE, // needmouse
   180, // alpha
   ShockPoisonUpdate, // update
};
