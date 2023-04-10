/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shktrain.cpp,v 1.17 2000/02/19 13:26:27 toml Exp $

#include <dev2d.h>
#include <string.h>
#include <res.h>
#include <appagg.h>

#include <resapi.h>
#include <mprintf.h>
#include <schema.h>
#include <scrnmode.h>
#include <fonrstyp.h>
#include <questapi.h>

#include <playrobj.h>

#include <shkprop.h>
#include <shktrain.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkutils.h>
#include <shkgame.h>
#include <shkcurm.h>
#include <shkplayr.h>
#include <shkplcst.h>
#include <shkplprp.h>
#include <shkiftul.h>
#include <shktrait.h>
#include <shktrcst.h>
#include <shkmfddm.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
}

static IRes *gStatBuy = NULL;
static IRes *gCostBlank = NULL;

static Rect full_rect= {{LMFD_X, LMFD_Y}, {LMFD_X + LMFD_W, LMFD_Y + LMFD_H}}; 
static Rect close_rect = {{163,8},{163 + 20, 8 + 21}};

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

static LGadButton undo_button;
static DrawElement undo_elem;
static IRes *undo_handles[2];
static grs_bitmap *undo_bitmaps[4];

static sStatsDesc gUndoStats;
static sWeaponSkills gUndoWeapon;
static sTechSkills gUndoTech;
static int gUndoPool;

IRes *gButtonMaxed = NULL;

typedef enum eTrainingMode { kTrainStats, kTrainTech, kTrainWeapon, };
static eTrainingMode gTrainMode = kTrainStats;
static int gTrainOverlay;
static char *train_bases[] = { "StatName", "TechSkill", "WpnSkill" };

#define POOL_X    68 // 99
#define POOL_Y    195

#define ANNELID_QUESTDATA "AlienWeapons"

#define NUM_BUY_BUTTONS 5
static Rect buy_rects[] = {
   {{13,21},{13 + 90, 21 + 32}}, // STR
   {{13,55},{13 + 90, 55 + 32}}, // END
   {{13,89},{13 + 90, 89 + 32}}, // PSI
   {{13,123},{13 + 90, 123 + 32}}, // AGI
   {{13,157},{13 + 90, 157 + 32}}, // CYB
};

static Rect undo_rect = {{158,146},{158 + 18, 146 + 44}}; // undo

static Rect text_rect = {{13,214},{172,288}};

static LGadButtonList buy_blist;
static LGadButtonListDesc buy_blistdesc;
static DrawElement buy_blist_elems[NUM_BUY_BUTTONS];
static IRes *buy_handles[NUM_BUY_BUTTONS][2];
static grs_bitmap *buy_bitmaps[NUM_BUY_BUTTONS][4];

static char *button_names[NUM_BUY_BUTTONS] = { "tbut1", "tbut1", "tbut1", "tbut1", "tbut1", };

//--------------------------------------------------------------------------------------
static int FindStatSlot(Point mpt)
{
   int retval = -1;
   int i;

   for (i=0; i < NUM_BUY_BUTTONS; i++)
   {
      if (RectTestPt(&buy_rects[i], mpt))
      {
         // totally zany alien dynamicness-itude
         if ((gTrainMode == kTrainWeapon) && (i == 3))
         {
            AutoAppIPtr(QuestData);
            if (!pQuestData->Get(ANNELID_QUESTDATA))
               retval = -1;
            else
               retval = i;
         }
         else
            retval = i;
         break;
      }
   }
   return(retval);
}
//--------------------------------------------------------------------------------------
static void DrawMouseOverText(ObjID obj, Rect r)
{
   Point mpt;
   int i;
   char temp[255];
   int dx,dy;
   static char *names[]={"Text","Tech","Weapon"};
   static char *tables[] = { "stathelp","skilhelp","skilhelp"};
   int strnum = 0;

   mouse_get_xy(&mpt.x,&mpt.y);
   mpt.x = mpt.x - r.ul.x;
   mpt.y = mpt.y - r.ul.y;
   i = FindStatSlot(mpt);
   if (i != -1)
   {
      strnum = i;

      ShockStringFetch(temp,sizeof(temp),names[gTrainMode],tables[gTrainMode],strnum);
      gr_font_string_wrap(gShockFontAA,temp,RectWidth(&text_rect));
      dx = r.ul.x + text_rect.ul.x; //+ (RectWidth(&text_rect) - w) / 2;
      dy = r.ul.y + text_rect.ul.y; //+ (RectHeight(&text_rect) - h) / 2;
      gr_set_fcolor(gShockTextColor);
      gr_font_string(gShockFontAA,temp,dx,dy);
   }
}
//--------------------------------------------------------------------------------------
void ShockBuyStatsInit(int which)
{
   int i,n;
   char temp[255];

   close_handles[0] = LoadPCX("CloseOff"); 
   close_handles[1] = LoadPCX("CloseOn"); 
   close_bitmaps[0] = (grs_bitmap *) close_handles[0]->Lock();
   close_bitmaps[1] = (grs_bitmap *) close_handles[1]->Lock();

   for (i = 2; i < 4; i++)
   {
      close_bitmaps[i] = close_bitmaps[0];
   }

   undo_handles[0] = LoadPCX("undo0"); 
   undo_handles[1] = LoadPCX("undo1"); 
   undo_bitmaps[0] = (grs_bitmap *) undo_handles[0]->Lock();
   undo_bitmaps[1] = (grs_bitmap *) undo_handles[1]->Lock();

   for (i = 2; i < 4; i++)
   {
      undo_bitmaps[i] = undo_bitmaps[0];
   }

   gCostBlank = LoadPCX("tbutcost");
   gStatBuy = LoadPCX("train");
   gButtonMaxed = LoadPCX("tbutmax");

   for (n=0; n < NUM_BUY_BUTTONS; n++)
   {
      for (i=0; i < 2; i++)
      {
         sprintf(temp,"%s%d",button_names[n],i);
         buy_handles[n][i] = LoadPCX(temp); 
         buy_bitmaps[n][i] = (grs_bitmap *) buy_handles[n][i]->Lock();
      }
      for (i = 2; i < 4; i++)
      {
         buy_bitmaps[n][i] = buy_bitmaps[n][0];
      }
   }

   // this is kind of a hack
   SetLeftMFDRect(kOverlayBuyStats, full_rect);
}

void ShockBuyTechInit(int which)
{
   SetLeftMFDRect(kOverlayBuyTech, full_rect);
}

void ShockBuyWeaponInit(int which)
{
   SetLeftMFDRect(kOverlayBuyWeapon, full_rect);
}
//--------------------------------------------------------------------------------------
void ShockTrainingTerm(void)
{
   int n;

   SafeFreeHnd(&gStatBuy);
   SafeFreeHnd(&gCostBlank);
   SafeFreeHnd(&gButtonMaxed);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);

   undo_handles[0]->Unlock();
   undo_handles[1]->Unlock();
   SafeFreeHnd(&undo_handles[0]);
   SafeFreeHnd(&undo_handles[1]);

   for (n=0; n < NUM_BUY_BUTTONS; n++)
   {
      buy_handles[n][0]->Unlock();
      buy_handles[n][1]->Unlock();
      SafeFreeHnd(&buy_handles[n][0]);
      SafeFreeHnd(&buy_handles[n][1]);
   }
}
//--------------------------------------------------------------------------------------
static int GetCharValue(int index)
{
   int retval = 0;
   ObjID obj = PlayerObject();
   sStatsDesc *stats;
   sTechSkills *techs;
   sWeaponSkills *weapons;
   AutoAppIPtr(ShockPlayer);

   switch (gTrainMode)
   {
   case kTrainStats:
      //retval = pShockPlayer->GetStat((eStats)index);
      if (g_BaseStatsProperty->Get(obj, &stats))
      {
         return(stats->m_stats[index]);
      }
      break;
   case kTrainTech:
      //retval = pShockPlayer->GetTechSkill((eTechSkills)index);
      if (g_BaseTechProperty->Get(obj, &techs))
      {
         return(techs->m_tech[index]);
      }
      break;
   case kTrainWeapon:
      //retval = pShockPlayer->GetWeaponSkill((eWeaponSkills)index);
      if (g_BaseWeaponProperty->Get(obj, &weapons))
      {
         return(weapons->m_wpn[index]);
      }
      break;
   }
   return(retval);
}
//--------------------------------------------------------------------------------------
static int GetCharCost(int index)
{
   int retval = 0;
   AutoAppIPtr(ShockPlayer);

   switch (gTrainMode)
   {
   case kTrainStats:
      retval = pShockPlayer->StatCost(index);
      break;
   case kTrainTech:
      retval = pShockPlayer->TechSkillCost(index);
      break;
   case kTrainWeapon:
      retval = pShockPlayer->WeaponSkillCost(index);
      break;
   }
   return(retval);
}
//--------------------------------------------------------------------------------------
void ShockTrainingDraw(void)
{
   Rect r;
   char temp[255];
   int dx,dy;
   int pool, val, cost;
   int buy = 2;
   int n,i,h;
   ObjID obj;
   Point drawpt;

   AutoAppIPtr(ShockPlayer);

   obj = PlayerObject();
   
   // draw the background
   r = ShockOverlayGetRect(gTrainOverlay);
   DrawByHandle(gStatBuy,r.ul);

   gr_set_fcolor(gShockTextColor);

   // save off explicitly since being used again to compute possible buy
   pool = pShockPlayer->GetPool(obj);
   sprintf(temp,"%d",pool);
   dx = r.ul.x + POOL_X;
   dy = r.ul.y + POOL_Y;
   gr_font_string(gShockFont,temp,dx,dy);

   // draw the current stat description
   DrawMouseOverText(obj,r);

   // draw text labels
   static char *labels[] = {"TrainHeading","TrainCost","TrainPoints"};
   Point coords[] = {{13,9},{120,9},{16,195}};

   for (i=0; i < 3; i++)
   {
      ShockStringFetch(temp,sizeof(temp),labels[i],"misc");
      dx = r.ul.x + coords[i].x;
      dy = r.ul.y + coords[i].y;
      gr_font_string(gShockFont,temp,dx,dy);
   }

   // buy buttons
   LGadDrawBox(VB(&buy_blist),NULL);

   // draw in the content of the buttons
   if (gTrainMode == kTrainWeapon)
   {
      n = 4;
      AutoAppIPtr(QuestData);
      if (!pQuestData->Get(ANNELID_QUESTDATA))
         n -= 1;
   }
   else
      n = 5;
   for (i=0; i < n; i++)
   {
      drawpt.x = r.ul.x + buy_rects[i].lr.x + 2;
      drawpt.y = r.ul.y + buy_rects[i].ul.y;
      DrawByHandle(gCostBlank,drawpt);            

      val = GetCharValue(i);
      if (val >= 6)
      {
         drawpt.x = buy_rects[i].ul.x + r.ul.x;
         drawpt.y = buy_rects[i].ul.y + r.ul.y;
         DrawByHandle(gButtonMaxed, drawpt);
      }

      ShockStringFetch(temp,sizeof(temp),train_bases[gTrainMode],"misc",i);
      dx = r.ul.x + buy_rects[i].ul.x + 5;
      h = gr_font_string_height(gShockFont,temp);
      dy = r.ul.y + buy_rects[i].ul.y + ((RectHeight(&buy_rects[i]) - h) / 2) - 1;
      gr_font_string(gShockFont,temp,dx,dy);

      if (val < 6) // MAX_STAT_VAL)
      {
         //sprintf(temp,"%d  ->  %d",val, val + 1);
         int j;
         static int xcoords[] = {51,76,94};
         for (j=0; j < 3; j++)
         {
            switch (j)
            {
            case 0: sprintf(temp,"%d",val); break;
            case 1: sprintf(temp,"%d",val+1); break;
            case 2: 
               cost = GetCharCost(i);
               sprintf(temp,"%d",cost); 
               break;
            }
            h = gr_font_string_height(gShockFont,temp);
            dx = r.ul.x + buy_rects[i].ul.x + xcoords[j];
            dy = r.ul.y + buy_rects[i].ul.y + ((RectHeight(&buy_rects[i]) - h) / 2);
            gr_font_string(gShockFont,temp,dx,dy);
         }
      }
      else
      {
         ShockStringFetch(temp,sizeof(temp),"MaxedOut","misc");
         dx = r.ul.x + buy_rects[i].ul.x + 56;
         h = gr_font_string_height(gShockFont, temp);
         dy = r.ul.y + buy_rects[i].ul.y + ((RectHeight(&buy_rects[i]) - h) / 2);
         gr_font_string(gShockFont,temp,dx,dy);
      }
   }

   LGadDrawBox(VB(&close_button),NULL);
   LGadDrawBox(VB(&undo_button),NULL);
}
//--------------------------------------------------------------------------------------
bool ShockTrainingHandleMouse(Point mpt)
{
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   SchemaPlay((Label *)"subpanel_cl",NULL);

   uiDefer(DeferOverlayClose,(void *)gTrainOverlay);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool undo_cb(short action, void* data, LGadBox* vb)
{
   SchemaPlay((Label *)"subpanel_cl",NULL);

   AutoAppIPtr(ShockPlayer);
   ObjID o = PlayerObject();
   pShockPlayer->SetPool(o,gUndoPool);

   switch (gTrainMode)
   {
   case kTrainStats:
      g_BaseStatsProperty->Set(o, &gUndoStats);
      break;
   case kTrainTech:
      g_BaseTechProperty->Set(o, &gUndoTech);
      break;
   case kTrainWeapon:
      g_BaseWeaponProperty->Set(o, &gUndoWeapon);
      break;
   }

   pShockPlayer->RecalcData(o);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool buy_cb(ushort action, int button, void* data, LGadBox* vb)
{
   int cost, pool;
   int val;
   sStatsDesc *statp, stats;
   sWeaponSkills *pWeapon, w;
   sTechSkills *pTech, t;
   ObjID obj;
   AutoAppIPtr(ShockPlayer);

   if (action != BUTTONGADG_LCLICK)
      return(FALSE);

   obj = PlayerObject();

   // click on arrows
   cost = GetCharCost(button);
   pool = pShockPlayer->GetPool(obj);
   val = GetCharValue(button);

   // feedback the failure cases
   char temp[255];
   if (val >= 6)
   {
      ShockStringFetch(temp,sizeof(temp),"ErrorMaxed", "misc");
      ShockOverlayAddText(temp,DEFAULT_MSG_TIME);
   }
   else if (pool < cost)
   {
      ShockStringFetch(temp,sizeof(temp),"ErrorExpensive","misc");
      ShockOverlayAddText(temp,DEFAULT_MSG_TIME);
   }
   else
   {
      switch (gTrainMode)
      {
      case kTrainStats:
         g_BaseStatsProperty->Get(obj, &statp);
         memcpy(&stats,statp,sizeof(sStatsDesc));
         // pay the price
         pShockPlayer->AddPool(obj,-1 * cost);

         // get the points
         stats.m_stats[button] += 1;
         g_BaseStatsProperty->Set(obj, &stats);
         break;
      case kTrainTech:
         g_BaseTechProperty->Get(obj, &pTech);
         memcpy(&t,pTech,sizeof(sTechSkills));
         // pay the price
         pShockPlayer->AddPool(obj,-1 * cost);

         // get the points
         t.m_tech[button] += 1;
         g_BaseTechProperty->Set(obj, &t);
         break;
      case kTrainWeapon:
         g_BaseWeaponProperty->Get(obj, &pWeapon);
         memcpy(&w,pWeapon,sizeof(sWeaponSkills));
         // pay the price
         pShockPlayer->AddPool(obj,-1 * cost);

         // get the points
         w.m_wpn[button] += 1;
         g_BaseWeaponProperty->Set(obj, &w);
         break;
      }
      pShockPlayer->RecalcData(obj);
   }
   
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   int n;
   Rect r = ShockOverlayGetRect(gTrainOverlay);
   static Rect use_rects[NUM_BUY_BUTTONS];

   close_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   close_elem.draw_data = close_bitmaps;
   close_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
      RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);

   undo_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   undo_elem.draw_data = undo_bitmaps;
   undo_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&undo_button, LGadCurrentRoot(), undo_rect.ul.x + r.ul.x, undo_rect.ul.y + r.ul.y,
      RectWidth(&undo_rect), RectHeight(&undo_rect), &undo_elem, undo_cb, 0);

   for (n=0; n < NUM_BUY_BUTTONS; n++)
   {
      DrawElement *elem = &buy_blist_elems[n];
      elem->draw_type = DRAWTYPE_BITMAPOFFSET;
      elem->draw_data = buy_bitmaps[n];
      elem->draw_data2 = (void *)4; // should be 2 but hackery required

      use_rects[n].ul.x = buy_rects[n].ul.x + r.ul.x;
      use_rects[n].ul.y = buy_rects[n].ul.y + r.ul.y;
      use_rects[n].lr.x = buy_rects[n].lr.x + r.ul.x;
      use_rects[n].lr.y = buy_rects[n].lr.y + r.ul.y;
   }
   
   if (gTrainMode != kTrainWeapon)
      buy_blistdesc.num_buttons = NUM_BUY_BUTTONS;
   else
   {
      // weapons
      buy_blistdesc.num_buttons = NUM_BUY_BUTTONS - 1;
      // if we haven't set the right magic quest data,
      // have even one less button, to not allow
      // alien weapons
      AutoAppIPtr(QuestData);
      if (!pQuestData->Get(ANNELID_QUESTDATA))
      {
         buy_blistdesc.num_buttons -= 1;
      }
   }
   buy_blistdesc.button_rects = use_rects;
   buy_blistdesc.button_elems = buy_blist_elems;
   buy_blistdesc.cb = buy_cb;

   LGadCreateButtonListDesc(&buy_blist, LGadCurrentRoot(), &buy_blistdesc);
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   LGadDestroyBox(VB(&close_button),FALSE);
   LGadDestroyBox(VB(&undo_button),FALSE);
   LGadDestroyBox(VB(&buy_blist),FALSE);
}
//--------------------------------------------------------------------------------------
static void CoreStateChange(int which)
{
   gTrainOverlay = which;
   if (ShockOverlayCheck(which))
   {
      BuildInterfaceButtons();
      sStatsDesc *pStat;
      sWeaponSkills *pWeapon;
      sTechSkills *pTech;
      ObjID obj = PlayerObject();

      switch (gTrainMode)
      {
      case kTrainStats:
         g_BaseStatsProperty->Get(obj, &pStat);
         memcpy(&gUndoStats, pStat,sizeof(sStatsDesc));
         break;
      case kTrainTech:
         g_BaseTechProperty->Get(obj, &pTech);
         memcpy(&gUndoTech,pTech,sizeof(sTechSkills));
         break;
      case kTrainWeapon:
         g_BaseWeaponProperty->Get(obj, &pWeapon);
         memcpy(&gUndoWeapon,pWeapon,sizeof(sWeaponSkills));
         break;
      }
      AutoAppIPtr(ShockPlayer);
      gUndoPool = pShockPlayer->GetPool(obj);
   }
   else
      DestroyInterfaceButtons();
}
//--------------------------------------------------------------------------------------
void ShockBuyStatsStateChange(int which)
{
   gTrainMode = kTrainStats;
   CoreStateChange(which);
}
//--------------------------------------------------------------------------------------
void ShockBuyTechStateChange(int which)
{
   gTrainMode = kTrainTech;
   CoreStateChange(which);
}
//--------------------------------------------------------------------------------------
void ShockBuyWeaponStateChange(int which)
{
   gTrainMode = kTrainWeapon;
   CoreStateChange(which);
}
//--------------------------------------------------------------------------------------
