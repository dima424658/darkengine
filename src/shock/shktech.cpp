/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shktech.cpp,v 1.9 2000/02/19 13:26:23 toml Exp $

// tech-hacking MFD
#include <dev2d.h>
#include <mprintf.h>
#include <appagg.h>
#include <res.h>

#include <resapi.h>

#include <rand.h>
#include <simtime.h>
#include <schema.h>

#include <shkinv.h>
#include <shktech.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkutils.h>
#include <shkscrpt.h>
#include <shkprop.h>

#include <shkres.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

typedef struct {
   int playerScore;
   int bustScore;

   int dealerScore;
   tSimTime dealerPipTime;
   int dealerPipLoc;

   int state;
   bool stand;

   int costCharge;
   int costOverload;

   int winTime;
} sHackingData;

static IRes *hackBackHnd;
static IRes *hackMeterHnd;
static IRes *hackMeterLoseHnd;
static IRes *hackLightHnd[2];
static IRes *hackPlayerPipHnd;
static IRes *hackDealerPipHnd;

#define HACKDIM_W 215   
#define HACKDIM_H 139

#define NUM_CARDS 13
static int cardData[NUM_CARDS] = { 11,2,3,4,5,6,7,8,9,10,10,10,10};
static sHackingData gHackData;

#define kStateNormal 0
#define kStateWin    1
#define kStateLose   2

#define PIP_ANIM_SPEED  30
#define PIX_PER_POINT   4.62F
//--------------------------------------------------------------------------------------
static int DrawCard(void)
{
   int i;
   i = Rand() % NUM_CARDS;
   return(cardData[i]);
}
//--------------------------------------------------------------------------------------
bool DealerAI(void)
{
   // dealer draws if <= 16
   if (gHackData.dealerScore <= 16)
   {
      gHackData.dealerPipLoc = (float)gHackData.dealerScore * PIX_PER_POINT;
      gHackData.dealerScore += DrawCard();
      if (gHackData.dealerScore >= gHackData.bustScore + 1)
         gHackData.dealerScore = gHackData.bustScore + 1;
      gHackData.dealerPipTime = GetSimTime();
   }
   return(gHackData.dealerScore <= 16);
}
//--------------------------------------------------------------------------------------
int CheckGameState()
{
   if (gHackData.playerScore > gHackData.bustScore)
      return(kStateLose);
   if (gHackData.dealerPipTime != 0)
      return(kStateNormal);

   int retval = kStateNormal;
   if (gHackData.dealerScore > gHackData.bustScore)
      retval = kStateWin;
   else if (gHackData.stand)
   {
      if (gHackData.dealerScore > gHackData.playerScore)
         retval = kStateLose;
      else 
         retval = kStateWin;
   }
   return(retval);
}
//--------------------------------------------------------------------------------------
void ShockHackingReset(void)
{
   gHackData.playerScore = 0; // DrawCard() + DrawCard();
   gHackData.dealerScore = 0; // DrawCard();
   gHackData.dealerPipTime = 0;
   mprintf("initial draws, player = %d, dealer = %d\n",gHackData.playerScore,gHackData.dealerScore);
   gHackData.bustScore = 21;
   gHackData.state = kStateNormal;
   gHackData.stand = FALSE;
   gHackData.costCharge = 1;
   gHackData.costOverload = 25;
   gHackData.winTime = 0;
}
//--------------------------------------------------------------------------------------
void ShockHackingInit(int which)
{
   Rect r;
   r.ul.x = 0;
   r.ul.y = 0;
   r.lr.x = HACKDIM_W;
   r.lr.y = HACKDIM_H;

   /*
   hackBackHnd = LoadPCX("hacking");
   hackLightHnd[0] = LoadPCX("hacklt00");
   hackLightHnd[1] = LoadPCX("hacklt01");
   hackMeterHnd = LoadPCX("hackmetr");
   hackMeterLoseHnd = LoadPCX("hackcrak");
   hackDealerPipHnd = LoadPCX("hackpip1");
   hackPlayerPipHnd = LoadPCX("hackpip2");
   */
   ShockOverlaySetRect(which,r);
}

//--------------------------------------------------------------------------------------
void ShockHackingTerm(void)
{
   /*
   SafeFreeHnd(&hackBackHnd);
   SafeFreeHnd(&hackMeterHnd);
   SafeFreeHnd(&hackMeterLoseHnd);
   SafeFreeHnd(&hackLightHnd[0]);
   SafeFreeHnd(&hackLightHnd[1]);
   SafeFreeHnd(&hackPlayerPipHnd);
   SafeFreeHnd(&hackDealerPipHnd);
   */
}

//--------------------------------------------------------------------------------------
static void DrawPips (IRes *pipHnd, int x, int drawy)
{
   Rect r = ShockOverlayGetRect(kOverlayHacking);
   Point drawloc;
   grs_bitmap *bm;
   
   bm = (grs_bitmap *) pipHnd->Lock();
   drawloc.y = r.ul.y + drawy;
   drawloc.x = r.ul.x + 45 + x - (bm->w / 2); /* ((float)usev * PIX_PER_POINT)  */
   gr_bitmap(bm,drawloc.x,drawloc.y);
   pipHnd->Unlock();
}
//--------------------------------------------------------------------------------------
void ShockHackingDraw(void)
{
   Rect r = ShockOverlayGetRect(kOverlayHacking);
   ObjID o = ShockOverlayGetObj();
   Point drawloc;
   int i,lit;
   int clippix;
   int piploc;
   grs_bitmap *bm;
   grs_clip saveClip;
   grs_font *font;
   int state;
   char temp[255];

   if (gHackData.state == kStateNormal)
   {
      state = CheckGameState();
      if (state == kStateLose)
      {
         mprintf("score %d vs %d, you lose!\n",gHackData.playerScore,gHackData.dealerScore);
         SchemaPlay((Label *)"video_lose",NULL);
      }
      if (state == kStateWin)
      {
         mprintf("score %d vs %d, you win!\n",gHackData.playerScore,gHackData.dealerScore);
         gHackData.winTime = GetSimTime() + 4000;
         ObjSetHackLevel(o,ObjGetHackLevel(o) + 1);
         if (ObjGetHackLevel(o) >= ObjGetHackDiff(o))
         {
            // send a "done hacking" message to the object
            AutoAppIPtr(ScriptMan);
            sHackingMsg message(o);

            pScriptMan->SendMessage(&message);
            //ShockOverlayChange(kOverlayHacking,kOverlayModeOff);         

            //ObjSetHackLevel(hackobj,0);
            SchemaPlay((Label *)"video_win",NULL);
         }
      }
      if ((state == kStateLose) || (state == kStateWin))
      {
         gHackData.state = state;
      }
   }

   font = (grs_font *)ResLock (RES_TechHackingFont);

   DrawByHandle(hackBackHnd,r.ul);

   // now, blit the complex parts
   // the meter
   drawloc.x = r.ul.x + 45;
   drawloc.y = r.ul.y + 36;
   if (gHackData.state == kStateLose) 
   {
      DrawByHandle(hackMeterLoseHnd,drawloc);
   }
   else
   {
      saveClip = grd_gc.clip;                 // Clip it good!

      bm = (grs_bitmap *) hackMeterHnd->Lock();
      //clippix = (bm->w * ((float)gHackData.playerScore / (float)gHackData.bustScore));
      clippix = ((float)gHackData.playerScore * PIX_PER_POINT);

      gr_safe_set_cliprect(drawloc.x,drawloc.y,drawloc.x + clippix, drawloc.y + bm->h);
      gr_bitmap(bm, drawloc.x,drawloc.y);
      hackMeterHnd->Unlock();

      grd_gc.clip = saveClip;
   }

   // target bar
   int targx = (float)gHackData.dealerScore * PIX_PER_POINT;
   if (gHackData.dealerPipTime == 0)
      piploc = targx;
   else
   {
      while (gHackData.dealerPipTime < GetSimTime())
      {
         gHackData.dealerPipLoc += 1;
         gHackData.dealerPipTime += PIP_ANIM_SPEED;
         if (gHackData.dealerPipLoc >= targx)
         {
            gHackData.dealerPipTime = 0;
            gHackData.dealerPipLoc = targx;
            break;
         }
      }
      piploc = gHackData.dealerPipLoc;
   }
   DrawPips(hackDealerPipHnd, piploc, r.ul.y + 28);
   // next card bar
   //DrawPips(hackPlayerPipHnd, gHackData.playerScore + 1, gHackData.playerScore + 10, r.ul.y + 52);
   // current nanites
   sprintf(temp,"%04d",ShockInvNaniteTotal());
   gr_font_string(font,temp,r.ul.x + 26, r.ul.y + 67);

   // per-try cost
   sprintf(temp,"%03d",gHackData.costCharge);
   gr_font_string(font,temp,r.ul.x + 26, r.ul.y + 88);

   // overload cost
   sprintf(temp,"%04d",gHackData.costOverload);
   gr_font_string(font,temp,r.ul.x + 26, r.ul.y + 109);

   // success lights
   drawloc.x = r.ul.x + 52;
   drawloc.y = r.ul.y + 17;
   if ((gHackData.state == kStateWin) && (gHackData.winTime != 0))
   {
      //mprintf("GST = %x\n",GetSimTime() >> 8);
      if ((GetSimTime() >> 8) & 0x1)
      {
         for (i=0; i < 5; i++)
         {
            DrawByHandle(hackLightHnd[1],drawloc);
            drawloc.x = drawloc.x + 22;
         }
      }
      if (GetSimTime() >= gHackData.winTime)
         gHackData.winTime = 0;
   }
   else
   {
      for (i = 0; i < ObjGetHackDiff(o); i++)
      {
         lit = ((i + 1) <= ObjGetHackLevel(o));
         DrawByHandle(hackLightHnd[lit],drawloc);
         drawloc.x = drawloc.x + 22;
      }
   }

   ResUnlock(RES_TechHackingFont);
}
//--------------------------------------------------------------------------------------
bool ShockHackingHandleMouse(uiMouseEvent *mev)
{
   Rect done_rect = {{0,0},{29,40}};
   Rect hit_rect = {{71,62},{109,100}};
   Rect stand_rect = {{114,62},{154,100}};
   Rect reset_rect = {{157,61},{175,90}};
   Rect r = ShockOverlayGetRect(kOverlayHacking);
   ObjID hackobj = ShockOverlayGetObj();
   Point mpt;
   mpt.x = mev->pos.x - r.ul.x;
   mpt.y = mev->pos.y - r.ul.y;

   if (mev->action & MOUSE_LUP)
   {
      if (RectTestPt(&done_rect,mpt))
         ShockOverlayChange(kOverlayHacking, kOverlayModeOff);
      else if (RectTestPt(&hit_rect,mpt) && (gHackData.state == kStateNormal))
      {
         if (ShockInvPayNanites(gHackData.costCharge))
         {
            gHackData.playerScore += DrawCard();
            if (gHackData.playerScore >= gHackData.bustScore + 1)
               gHackData.playerScore = gHackData.bustScore + 1;
            else
               SchemaPlay((Label *)"video_progress",NULL);
            DealerAI();
         }
      }
      else if (RectTestPt(&stand_rect,mpt) && (gHackData.state == kStateNormal))
      {
         // just keep running DealerAI as long as it is returning TRUE
         while (DealerAI());
         gHackData.stand = TRUE;
      }
      else if (RectTestPt(&reset_rect,mpt)) // && (gHackData.state != kStateNormal)
      {
         ShockHackingReset();
         SchemaPlay((Label *)"video_beep",NULL);
      }
      return(TRUE);
   }
   return(FALSE);
}
//--------------------------------------------------------------------------------------
void ShockHackingStateChange(int which)
{
}
//--------------------------------------------------------------------------------------
void ShockHackingBegin(ObjID o)
{
   ShockOverlaySetObj(kOverlayHacking,o);
   ShockHackingReset();
}
//--------------------------------------------------------------------------------------
// Check for transparency in the source art
//--------------------------------------------------------------------------------------
bool ShockHackingCheckTransp(Point pt)
{
   bool retval = FALSE;
   Point p;
   Rect r;
   r = ShockOverlayGetRect(kOverlayHacking);
   p.x = pt.x - r.ul.x;
   p.y = pt.y - r.ul.y;
   int pix = HandleGetPix(hackBackHnd,p);
   if (pix == 0)
      retval = TRUE;
   //mprintf("SICT: %d (%d)\n",retval,pix);
   return(retval);
}
