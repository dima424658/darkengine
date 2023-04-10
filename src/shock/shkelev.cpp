/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkelev.cpp,v 1.16 2000/02/19 13:20:17 toml Exp $

#include <2d.h>
#include <appagg.h>
#include <res.h>
#include <cfgdbg.h>

#include <resapi.h>
#include <kbcook.h>

#include <editor.h>
#include <iobjnet.h>
#include <dbfile.h>
#include <questapi.h>
#include <playrobj.h>
#include <netman.h>
#include <netmsg.h>

#include <scrptapi.h>
#include <shkscrm.h>

#include <shkprop.h>
#include <shkelev.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkutils.h>
#include <shkmfddm.h>
#include <shkmulti.h>
#include <shkparam.h>
#include <shkiftul.h>
#include <transmod.h>
#include <shklding.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
}

#define ELEVATOR_MARKER 22

int ElevCurrentLevel(void);

static IRes *gElevBack;
static IRes *gElevNoPower;

static Rect full_rect = {{LMFD_X, LMFD_Y}, {LMFD_X + LMFD_W, LMFD_Y + LMFD_H}}; 
static Rect close_rect = {{163,8},{163 + 20, 8 + 21}};

#define NUM_ELEV_BUTTONS  5

static LGadButtonList elev_blist;
static LGadButtonListDesc elev_blistdesc;
static DrawElement elev_blist_elems[NUM_ELEV_BUTTONS];
static grs_bitmap *elev_blist_bitmaps[NUM_ELEV_BUTTONS][4];
static IRes *elev_blist_handles[NUM_ELEV_BUTTONS][2];

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

#define BUTTON_W  142 
// 44
#define BUTTON_H  54 
// 60
#define BUTTON_RECT(x,y) {{(x),(y)}, {(x) + BUTTON_W, (y) + BUTTON_H}}
//#define BUTTON_RECT2(x,y) {{(x),(y)}, {(x) + , (y) + BUTTON_H}}
static Rect elev_rects[NUM_ELEV_BUTTONS] = { 
   BUTTON_RECT(13,11),
   BUTTON_RECT(13,67),
   BUTTON_RECT(13,123),
   BUTTON_RECT(13,179),
   BUTTON_RECT(13,235),
};

extern grs_font *gKeypadFont;

//--------------------------------------------------------------------------------------
// Actually do a level transition. This only gets run on the world-host.
void ElevButton(int button_num)
{
   // We need to make sure we don't re-enter this. TransModeSwitchLevel
   // processes net messages under the hood, so re-entrance is an issue.
   if (gbShockTransporting)
   {
      ConfigSpew("net_spew", 
                 ("Redundant call to level transport rejected.\n"));
      return;
   }

   if (button_num >= 0)
   {
      sElevParams *levels;
      levels = GetElevParams();

      // clean up anything relevant
      AutoAppIPtr(NetManager);
      AutoAppIPtr(ScriptMan);

      ObjID netobj;

      FOR_ALL_PLAYERS(pNetManager,&netobj)
      {
         sScrMsg msg(netobj, "EndLevel"); 
         msg.flags |= kSMF_MsgPostToOwner;
         pScriptMan->SendMessage(&msg); 
      }

      TransModeSwitchLevel(levels->m_levels[(NUM_ELEV_BUTTONS - 1) - button_num],ELEVATOR_MARKER,SMF_ELEVATOR); 
   }
}
//--------------------------------------------------------------------------------------
// NETWORKING CODE
//

// The change-levels message
//
// Level changing must happen on the default host, to avoid raceway
// conditions. So here's a dumb little message for it.
static cNetMsg *g_pElevButtonMsg = NULL;

static sNetMsgDesc sElevButtonDesc =
{
   kNMF_SendToHost,
   "ElevButton",
   "Elevator Button Pushed",
   NULL,
   ElevButton,
   {{kNMPT_Int, kNMPF_None, "Button #"},
    {kNMPT_End}}
};

//--------------------------------------------------------------------------------------
void ShockElevInit(int which)
{
   char temp[40];
   int i,s;

   close_handles[0] = LoadPCX("CloseOff"); 
   close_handles[1] = LoadPCX("CloseOn"); 
   close_bitmaps[0] = (grs_bitmap *) close_handles[0]->Lock();
   close_bitmaps[1] = (grs_bitmap *) close_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      close_bitmaps[i] = close_bitmaps[0];
   }

   for (s = 0; s < NUM_ELEV_BUTTONS; s++)
   {
      for (i = 0; i < 2; i++)
      {
         //sprintf(temp,"key%c%d",key_info[s],i);
         sprintf(temp,"elev%d%d",((NUM_ELEV_BUTTONS - 1) - s)+1,i);
         elev_blist_handles[s][i] = LoadPCX(temp);
         elev_blist_bitmaps[s][i] = (grs_bitmap *) elev_blist_handles[s][i]->Lock();
      }
      for (i = 2; i < 4; i++)
      {
         elev_blist_bitmaps[s][i] = elev_blist_bitmaps[s][0];
      }
   }

   gElevBack = LoadPCX("elev");
   gElevNoPower = LoadPCX("power");
   SetLeftMFDRect(which, full_rect);
}

//--------------------------------------------------------------------------------------
void ShockElevTerm(void)
{
   int s,i;

   for (s = 0; s < NUM_ELEV_BUTTONS; s++)
   {
      for (i = 0; i < 2; i++)
      {
         elev_blist_handles[s][i]->Unlock();
         SafeFreeHnd(&elev_blist_handles[s][i]);
      }
   }

   SafeFreeHnd(&gElevBack);
   SafeFreeHnd(&gElevNoPower);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);
}

//--------------------------------------------------------------------------------------
// Networking init/term must happen only once
void ShockElevNetInit()
{
   g_pElevButtonMsg = new cNetMsg(&sElevButtonDesc);
}
void ShockElevNetTerm()
{
   delete g_pElevButtonMsg;
}

//--------------------------------------------------------------------------------------
#define TEXT_X 60
void ShockElevDraw(void)
{
   int dx,dy,i;
   int w,h;
   char temp[255];
   Point drawpt;
   Rect r = ShockOverlayGetRect(kOverlayElevator);


   AutoAppIPtr(QuestData);
   int elevstate = pQuestData->Get("ElevState");
   if (elevstate == 0)
   {
      DrawByHandle(gElevNoPower, r.ul);
      LGadDrawBox(VB(&close_button),NULL);
   }
   else
   {
      DrawByHandle(gElevBack,r.ul);
      LGadDrawBox(VB(&close_button),NULL);

      LGadDrawBox(VB(&elev_blist),NULL);

      int level = (NUM_ELEV_BUTTONS - 1) - ElevCurrentLevel();
      /*
      ShockStringFetch(temp,sizeof(temp),"ElevLevel","misc",level+1);
      dx = r.ul.x + 15;
      dy = r.ul.y + 13;
      gr_font_string(gShockFont, temp, dx, dy);
      */

      // draw in the button for the current level
      if (level != -1)
      {
         drawpt.x = r.ul.x + elev_rects[level].ul.x;
         drawpt.y = r.ul.y + elev_rects[level].ul.y;
         DrawByHandle(elev_blist_handles[level][1],drawpt);
      }

      // draw the numbers on top of the buttons
      for (i=NUM_ELEV_BUTTONS - 1; i >= 0; i--)
      {
         int j = (NUM_ELEV_BUTTONS - 1) - i;
         ShockStringFetch(temp,sizeof(temp),"ElevLevel","misc",i+1);
         //sprintf(temp,"%d",i);

         w = gr_font_string_width(gShockFont, temp);
         h = gr_font_string_height(gShockFont, temp);
         dx = r.ul.x + elev_rects[j].ul.x + TEXT_X; // (RectWidth(&elev_rects[i]) - w) / 2;
         dy = r.ul.y + elev_rects[j].ul.y + (RectHeight(&elev_rects[j]) - h) / 2;
         gr_font_string(gShockFont, temp, dx, dy);
      }
   }
}
//--------------------------------------------------------------------------------------
void sendElevButton(int button_num)
{
   g_pElevButtonMsg->Send(OBJ_NULL, button_num);
}
//--------------------------------------------------------------------------------------
bool ShockElevHandleMouse(Point pt)
{
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool elev_select_cb(ushort action, int button, void* data, LGadBox* vb)
{
   if (action != BUTTONGADG_LCLICK)
      return(FALSE);

   int lvl = ElevCurrentLevel();
   if (button != ((NUM_ELEV_BUTTONS - 1) - lvl))
   {
      // check certain quest items
      AutoAppIPtr(QuestData);
      int elevstate = pQuestData->Get("ElevState");
      switch (elevstate)
      {
      case 0:  // no power on, start of game
         break;
      case 1: // power on, but blocked by worm goo
         if (button >= 2)
            sendElevButton(button);
         else
         {
            // text feedback
            char temp[255];
            ShockStringFetch(temp,sizeof(temp),"ElevBlocked","misc");
            ShockOverlayAddText(temp,DEFAULT_MSG_TIME);
         }
         break;
      case 2: // fully accessible
         sendElevButton(button);
         break;
      }
   }
   return TRUE;
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   uiDefer(DeferOverlayClose,(void *)kOverlayElevator);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   int n;
   Rect r = ShockOverlayGetRect(kOverlayElevator);
   static Rect use_rects[NUM_ELEV_BUTTONS];

	// make a button for each service, ie a button list
   for (n=0; n < NUM_ELEV_BUTTONS; n++)
   {
      DrawElement *elem = &elev_blist_elems[n];
      elem->draw_type = DRAWTYPE_BITMAPOFFSET;
      elem->draw_data = elev_blist_bitmaps[n];
      elem->draw_data2 = (void *)4; // should be 2 but hackery required

      use_rects[n].ul.x = elev_rects[n].ul.x + r.ul.x;
      use_rects[n].ul.y = elev_rects[n].ul.y + r.ul.y;
      use_rects[n].lr.x = elev_rects[n].lr.x + r.ul.x;
      use_rects[n].lr.y = elev_rects[n].lr.y + r.ul.y;
   }
   
   elev_blistdesc.num_buttons = NUM_ELEV_BUTTONS;
   elev_blistdesc.button_rects = use_rects;
   elev_blistdesc.button_elems = elev_blist_elems;
   elev_blistdesc.cb = elev_select_cb;
   //elev_blistdesc.flags = BUTTONLIST_RADIO_FLAG;

   LGadCreateButtonListDesc(&elev_blist, LGadCurrentRoot(), &elev_blistdesc);

   // set up the continue button
   close_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   close_elem.draw_data = close_bitmaps;
   close_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
      RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   LGadDestroyButtonList(&elev_blist);
   LGadDestroyBox(VB(&close_button),FALSE);
}
//--------------------------------------------------------------------------------------
void ShockElevStateChange(int which)
{
   if (ShockOverlayCheck(which))
   {
      // just got put up
      BuildInterfaceButtons();
   }
   else
   {
      // just got taken down
      DestroyInterfaceButtons();
   }
}
//--------------------------------------------------------------------------------------
// Check for transparency in the source art
//--------------------------------------------------------------------------------------
bool ShockElevCheckTransp(Point pt)
{
   return(FALSE); 
   //return(ShockOverlayCheckTransp(pt,kOverlayelev, gelevBack));
}


//--------------------------------------------------------------------------------------
int ElevCurrentLevel(void)
{
   char curfile[255], temp[255];
   dbCurrentFile(curfile, sizeof(curfile));
   strcpy(temp, curfile); 
   temp[strlen(curfile) - 4] = '\0'; // to strip out extension
   
   int i;
   sElevParams *levels;
   levels = GetElevParams();
   for (i=0; i < NUM_ELEV_BUTTONS; i++)
   {
      if (!stricmp(temp,levels->m_levels[i]))
         return(i);
   }
   return(-1);
}
