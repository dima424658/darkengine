/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Deep Cover HUD overlay elements

#include <2d.h>

#include <res.h>
#include <guistyle.h>
#include <rect.h>
#include <appagg.h>

#include <resapi.h>
#include <netman.h>
#include <playrobj.h>

#include <mprintf.h>

#include <dlistsim.h>
#include <dlisttem.h>
#include <gamestr.h>

#include <objtype.h>
#include <objhp.h>

#include <scrnmode.h>

#include <dpcgame.h>
#include <dpchud.h>
#include <dpcutils.h>
#include <dpcovrly.h>
#include <dpcprop.h>
#include <dpccmapi.h>

#define NUM_BRACKETS 4
IRes *gHndBrackets[NUM_BRACKETS];
#define NUM_HPBARS 3 
IRes *gHPHnds[NUM_HPBARS];

typedef struct sHUDElem
{
   Rect r;
   ObjID o;
} sHUDElem;

// that's the Cannibalistic Human Underground Dweller RectList to you, bud.
typedef cSimpleDList<sHUDElem> cHUDRectList; 

#define MAX_HUD_WIDTH   320
#define MAX_HUD_HEIGHT  240
#define MIN_HUD_X       10
#define MIN_HUD_DX      20
#define MIN_HUD_Y       25
#define MIN_HUD_DY      20
#define MAX_HUD_X       (640 - 10)
#define MAX_HUD_Y       (480 - 25)

cHUDRectList hud_rects;
//--------------------------------------------------------------------------------------
// Main interface initializer
//--------------------------------------------------------------------------------------
static int max_x = -1;
static int max_y = -1;
void DPCHUDInit(int which)
{
   int i;
   char temp[40];
   for (i=0; i < NUM_BRACKETS; i++)
   {
      sprintf(temp,"brack%d",i);
      gHndBrackets[i] = LoadPCX(temp);
	   AssertMsg(gHndBrackets[i], "DPCHUDInit:  Hud Bracket art missing");
   }

   for (i=0; i < NUM_HPBARS; i++)
   {
      sprintf(temp,"hpbar%d",i);
      gHPHnds[i] = LoadPCX(temp);
	   AssertMsg(gHndBrackets[i], "DPCHUDInit:  Hit Point bar art missing");
   }

   // DPCOverlaySetRect(which,interface_rect);
   sScrnMode smode;
   ScrnModeGet(&smode);
   max_x = smode.w - (640 - MAX_HUD_X);
   max_y = smode.h - (480 - MAX_HUD_Y);
}

//--------------------------------------------------------------------------------------
// Main interface shutdown
//--------------------------------------------------------------------------------------
void DPCHUDTerm(void)
{
   int i;

   for (i = 0; i < NUM_BRACKETS; i++)
   {
      SafeFreeHnd(&gHndBrackets[i]);
   }

   for (i = 0; i < NUM_HPBARS; i++)
   {
      SafeFreeHnd(&gHPHnds[i]);
   }
}

//--------------------------------------------------------------------------------------
#define HPBUFFER  2
void DPCHUDDraw(unsigned long inDeltaTicks)
{
   int i;
   int x,y,dx,dy, w;
   grs_bitmap *bm;
   Rect r;
   //int hp;
   ObjID o;

#ifdef EDITOR
   AutoAppIPtr(DPCCamera);
   if (pDPCCamera->GetEditMode() != kCameraNormal)
      return;
#endif

   for (cHUDRectList::cIter iter = hud_rects.Iter(); !iter.Done(); iter.Next())
   {
      //Add(targ,iter.Value().obj,iter.Value().type, addflags); 
      r = iter.Value().r;
      o = iter.Value().o;
      for (i=0; i < NUM_BRACKETS; i++)
      {
         bm = (grs_bitmap *) gHndBrackets[i]->Lock();
         switch(i)
         {
         case 0:
            x = r.ul.x;
            y = r.ul.y;
            break;
         case 1:
            x = r.lr.x - bm->w;
            y = r.ul.y;
            break;
         case 2:
            x = r.lr.x - bm->w;
            y = r.lr.y - bm->h;
            break;
         case 3:
            x = r.ul.x;
            y = r.lr.y - bm->h;
            break;

         }
         gr_bitmap(bm, x, y);
         gHndBrackets[i]->Unlock();
      }

      if (o != OBJ_NULL)
      {
         char temp[1024];
         int hp, hpmax;
         AutoAppIPtr(GameStrings);
         BOOL drawhp = FALSE;
         gPropShowHP->Get(o,&drawhp);

         if (ObjGetHitPoints(o,&hp) && ObjGetMaxHitPoints(o,&hpmax) && drawhp)
         {
            dx = r.ul.x;
            dy = r.ul.y - 14;
            /*
            sprintf(temp,"HP: %d",hp);
            gr_font_string(gDPCFont, temp, x, y);
            */
            // draw a goofy meter
            grs_clip saveClip = grd_gc.clip;                 // Clip it good!
            IRes *hnd;
            float ratio;
            int usehnd, clippix;

            // choose one of three different handles based on 
            // general health ratio
            if (hp <= 0)
               ratio = 0;
            else
               ratio = (float)(hp + HPBUFFER) / (float)(hpmax + HPBUFFER);

            usehnd =  int(ratio * NUM_HPBARS);
            //mprintf("ratio is %g, usehnd is %d\n",ratio,usehnd);
            if (usehnd < 0) usehnd = 0;
            if (usehnd >= NUM_HPBARS) usehnd = NUM_HPBARS - 1;
            hnd = gHPHnds[usehnd];

            // compute cliprect
            bm = (grs_bitmap *) hnd->Lock();
            if ((hp == 0) || (hpmax == 0))
               clippix = 0;
            else
               clippix = (bm->w * ratio);
            if (clippix < 0)
               clippix = 0;
            gr_safe_set_cliprect(dx, dy, dx + clippix, dy + bm->h);

            // draw it & clean up
            gr_bitmap(bm, dx , dy);
            hnd->Unlock();

            grd_gc.clip = saveClip;
         }

         cStr str = pGameStrings->FetchObjString(o,PROP_HUDUSE_NAME);
         strncpy(temp,str,sizeof(temp));
         if (strlen(temp) > 0)
         {
            sScrnMode smode;
            ScrnModeGet(&smode);

            w = gr_font_string_width(gDPCFont, temp);
            dx = r.ul.x; // ((RectWidth(&r) - w) / 2);
            if (dx + w + 10> smode.w)
               dx = smode.w - w - 10;

            dy = r.lr.y + 4;
            gr_set_fcolor(gDPCTextColor);
            gr_font_string(gDPCFontMono, temp, dx, dy);
         }

         if (IsAPlayer(o))
         {
            AutoAppIPtr(NetManager);
            const char *pName = pNetManager->GetPlayerName(o);
            if (strlen(pName) > 0)
            {
               // ideally this would be the player's chat color?
               gr_set_fcolor(gDPCTextColor);
               // draw the actual name
               gr_font_string(gDPCFontMono, (char *) pName, 
                              r.ul.x + 6, r.ul.y + 3);
            }
         }
      }
   }

   hud_rects.DestroyAll(); // is this the right way to erase the list?
}

//--------------------------------------------------------------------------------------

void DPCHUDDrawRect(Rect r, ObjID o)
{
   sHUDElem hudelem;
   //int dx,dy;
   Rect use_rect;

   // make sure that our rectangle is not too big
   // alternatively, should we constrain to not being within a certain border around the edges?
   use_rect = r;

   /*
   dx = RectWidth(&r) - MAX_HUD_WIDTH;
   dy = RectHeight(&r) - MAX_HUD_HEIGHT;
   if (dx > 0)
   {
      use_rect.ul.x += (dx / 2);
      use_rect.lr.x -= (dx / 2);
   }
   if (dy > 0)
   {
      use_rect.ul.y += (dy / 2);
      use_rect.lr.y -= (dy / 2);
   }
   */
   if (use_rect.ul.x < MIN_HUD_X)
   {
      use_rect.ul.x = MIN_HUD_X;
      if (use_rect.lr.x < MIN_HUD_X + MIN_HUD_DX)
         use_rect.lr.x = MIN_HUD_X + MIN_HUD_DX;
   }
   if (use_rect.lr.x > max_x)
   {
      use_rect.lr.x = max_x;
      //mprintf("capping rect lr.x of %d to %d\b",use_rect.lr.x,max_x);
      if (use_rect.ul.x > max_x - MIN_HUD_DX)
         use_rect.ul.x = max_x - MIN_HUD_DX;
   }
   if (use_rect.ul.y < MIN_HUD_Y)
   {
      use_rect.ul.y = MIN_HUD_Y;
      if (use_rect.lr.y < MIN_HUD_Y + MIN_HUD_DY)
         use_rect.lr.y = MIN_HUD_Y + MIN_HUD_DY;
   }
   if (use_rect.lr.y > max_y)
   {
      use_rect.lr.y = max_y;
      if (use_rect.ul.y > max_y - MIN_HUD_DY)
         use_rect.ul.y = max_y - MIN_HUD_DY;
   }

   hudelem.r = use_rect;
   hudelem.o = o;
   hud_rects.Append(hudelem);
}
//--------------------------------------------------------------------------------------
void DPCHUDClear()
{
   hud_rects.DestroyAll(); 
}
//--------------------------------------------------------------------------------------
