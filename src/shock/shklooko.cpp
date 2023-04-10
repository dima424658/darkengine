/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shklooko.cpp,v 1.37 2000/02/19 13:25:37 toml Exp $

#include <dev2d.h>
#include <string.h>
#include <res.h>
#include <appagg.h>
#include <lgd3d.h>

#include <resapi.h>

#include <objsys.h>
#include <osysbase.h>
#include <mprintf.h>
#include <inv_rend.h>
#include <gamestr.h>

#include <scrnman.h>

#include <shkprop.h>
//#include <shkscrpt.h>
#include <shklooko.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkutils.h>
#include <shkgame.h>
#include <shkcurm.h>
#include <shkmfddm.h>
#include <shkobjst.h>
#include <shkiftul.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
}

IRes *gLookBack = NULL;
ObjID g_Lookobj = OBJ_NULL;
char g_Looktext[255];

static char gQueryText[2048];
static int gQueryOffset = 0;

static Rect full_rect = {{LMFD_X, LMFD_Y}, {LMFD_X + LMFD_W, LMFD_Y + LMFD_H}}; 
static Rect close_rect = {{163,8},{163 + 20, 8 + 21}};

static Rect text_rect = {{15,153},{138,285}};
static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

extern "C" BOOL g_lgd3d;

typedef enum eEmailButton { kQueryButtonUp, kQueryButtonPageUp, kQueryButtonPageDown, kQueryButtonDown};
#define NUM_query_BUTTONS 4
static Rect query_rects[NUM_query_BUTTONS] = {
   {{159,154},{159 + 18, 154 + 17}},   // lineup
   {{159,174},{159 + 18, 174 + 26}},   // pageup
   {{159,203},{159 + 18, 203 + 26}},   // pagedown
   {{159,232},{159 + 18, 232 + 17}},   // linedown
};

static LGadButtonList query_blist;
static LGadButtonListDesc query_blistdesc;
static DrawElement query_blist_elems[NUM_query_BUTTONS];
static IRes *query_handles[NUM_query_BUTTONS][2];
static grs_bitmap *query_bitmaps[NUM_query_BUTTONS][4];
static char *button_names[NUM_query_BUTTONS] = { "up","pgup","pgdn","down"};

//--------------------------------------------------------------------------------------
static invRendState *cur_inv_rend=NULL;
Rect *get_model_draw_rect(void);
void inv_show_model(void);

//--------------------------------------------------------------------------------------
void ShockLookInit(int which)
{
   int i,n;
   char temp[64];

   close_handles[0] = LoadPCX("CloseOff"); 
   close_handles[1] = LoadPCX("CloseOn"); 
   close_bitmaps[0] = (grs_bitmap *) close_handles[0]->Lock();
   close_bitmaps[1] = (grs_bitmap *) close_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      close_bitmaps[i] = close_bitmaps[0];
   }

   for (n=0; n < NUM_query_BUTTONS; n++)
   {
      for (i=0; i < 2; i++)
      {
         sprintf(temp,"%s%d",button_names[n],i);
         query_handles[n][i] = LoadPCX(temp); 
         query_bitmaps[n][i] = (grs_bitmap *) query_handles[n][i]->Lock();
      }
      for (i = 2; i < 4; i++)
      {
         query_bitmaps[n][i] = query_bitmaps[n][0];
      }
   }

   gLookBack = LoadPCX("query");
   //ShockOverlaySetRect(which, full_rect);
   SetLeftMFDRect(which, full_rect);
}

//--------------------------------------------------------------------------------------
void ShockLookTerm(void)
{
   int n;

   SafeFreeHnd(&gLookBack);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);

   for (n=0; n < NUM_query_BUTTONS; n++)
   {
      query_handles[n][0]->Unlock();
      query_handles[n][1]->Unlock();
      SafeFreeHnd(&query_handles[n][0]);
      SafeFreeHnd(&query_handles[n][1]);
   }
}

//--------------------------------------------------------------------------------------
#define NAME_X 24
#define NAME_Y 133
void ShockLookDraw(void)
{
   Rect r;
   Point drawpt;
   int dx,dy;
   char temp[255];
   grs_clip saveclip;

   AutoAppIPtr(GameStrings);

   r = ShockOverlayGetRect(kOverlayLook);
   drawpt = r.ul;
   DrawByHandle(gLookBack,drawpt);

   inv_show_model();

   // draw in the shortname
   eObjState st;
   st = ObjGetObjState(g_Lookobj);

   ObjGetObjShortNameSubst(g_Lookobj,temp,sizeof(temp));

   dx = r.ul.x + NAME_X;
   dy = r.ul.y + NAME_Y;
   gr_set_fcolor(gShockTextColor);
   gr_font_string(gShockFont,temp,dx,dy);

   // main text
   dx = text_rect.ul.x + r.ul.x;
   dy = text_rect.ul.y + r.ul.y - gQueryOffset;

   memcpy(&saveclip,&grd_clip,sizeof(grs_clip)); // gr_save_cliprect(&saveclip);
   gr_set_cliprect(text_rect.ul.x + r.ul.x, text_rect.ul.y + r.ul.y, text_rect.lr.x + r.ul.x, text_rect.lr.y + r.ul.y);
   if (st == kObjStateUnresearched)
   {
      char temp[255];
      ShockStringFetch(temp,sizeof(temp),"QueryUnresearched","research");
      gr_font_string_wrap(gShockFont,temp, RectWidth(&text_rect));
      gr_font_string(gShockFont, temp, dx, dy);
      strcpy(gQueryText, temp);
   }
   else
   {
      gr_font_string(gShockFont,gQueryText,dx,dy);
   }
   memcpy(&grd_clip,&saveclip,sizeof(grs_clip)); //gr_restore_cliprect(&saveclip);

   // scroll buttons
   LGadDrawBox(VB(&query_blist),NULL);

   LGadDrawBox(VB(&close_button),NULL);
}
//--------------------------------------------------------------------------------------
bool ShockLookHandleMouse(Point pt)
{
   return(FALSE);
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   if (action == BUTTONGADG_LCLICK)
   {
      uiDefer(DeferOverlayClose,(void *)kOverlayLook);
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
#define SCROLL_LINE     12
#define SCROLL_PAGE     (175 - (SCROLL_LINE * 2))
#define SCROLL_BUFFER   40
static bool query_cb(ushort action, int button, void* data, LGadBox* vb)
{
   if (action != BUTTONGADG_LCLICK)
      return(FALSE);

   int h,max_offset;
   int scrollamt;

   switch(button)
   {
   case kQueryButtonDown:
      scrollamt = SCROLL_LINE;
      // fallthrough
   case kQueryButtonPageDown:
      if (button == kQueryButtonPageDown)
         scrollamt = SCROLL_PAGE;
      h = gr_font_string_height(gShockFont, gQueryText);

      // only bother if the whole thing does not fit on one screen
      if (h > RectHeight(&text_rect))
      {
         gQueryOffset += scrollamt;
         max_offset = h - RectHeight(&text_rect) + SCROLL_BUFFER;
         if (gQueryOffset > max_offset)
            gQueryOffset = max_offset;
      }
      break;
   case kQueryButtonUp:
      scrollamt = SCROLL_LINE;
      // fallthrough
   case kQueryButtonPageUp:
      if (button == kQueryButtonPageUp)
         scrollamt = SCROLL_PAGE;
      gQueryOffset -= scrollamt;
      if (gQueryOffset < 0)
         gQueryOffset = 0;
      break;
   }
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   int n;
   Rect r = ShockOverlayGetRect(kOverlayLook);
   static Rect use_rects[NUM_query_BUTTONS];

   close_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   close_elem.draw_data = close_bitmaps;
   close_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
      RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);

   for (n=0; n < NUM_query_BUTTONS; n++)
   {
      DrawElement *elem = &query_blist_elems[n];
      elem->draw_type = DRAWTYPE_BITMAPOFFSET;
      elem->draw_data = query_bitmaps[n];
      elem->draw_data2 = (void *)4; // should be 2 but hackery required

      use_rects[n].ul.x = query_rects[n].ul.x + r.ul.x;
      use_rects[n].ul.y = query_rects[n].ul.y + r.ul.y;
      use_rects[n].lr.x = query_rects[n].lr.x + r.ul.x;
      use_rects[n].lr.y = query_rects[n].lr.y + r.ul.y;
   }
   
   query_blistdesc.num_buttons = NUM_query_BUTTONS;
   query_blistdesc.button_rects = use_rects;
   query_blistdesc.button_elems = query_blist_elems;
   query_blistdesc.cb = query_cb;

   LGadCreateButtonListDesc(&query_blist, LGadCurrentRoot(), &query_blistdesc);
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   LGadDestroyBox(VB(&close_button),FALSE);
   LGadDestroyBox(VB(&query_blist),FALSE);
}
//--------------------------------------------------------------------------------------
void ShockLookStateChange(int which)
{
   if (ShockOverlayCheck(which))
   {
      gQueryOffset = 0;
      BuildInterfaceButtons();
      // just got put up
      if (!cur_inv_rend)
      {
         cur_inv_rend=invRendBuildState(0,g_Lookobj,get_model_draw_rect(), NULL);
         invRendUpdateState(cur_inv_rend,INVREND_ROTATE|INVREND_HARDWARE_IMMEDIATE|INVREND_SET,OBJ_NULL,NULL,NULL);
      }
   }
   else
   {
      DestroyInterfaceButtons();
      // just got taken down
      if (cur_inv_rend)
      {
         invRendFreeState(cur_inv_rend);
         cur_inv_rend=NULL;
      }
   }
}
//--------------------------------------------------------------------------------------
// Check for transparency in the source art
//--------------------------------------------------------------------------------------
bool ShockLookCheckTransp(Point pt)
{
   return(FALSE);
   /*
   bool retval;
   retval = ShockOverlayCheckTransp(pt, kOverlayLook, gLookBack);
   return(retval);
   */
}
//--------------------------------------------------------------------------------------
void ShockLookPopup(ObjID o) // , const char *text)
{
   cStr str;

   // kill any old query box
   if (ShockOverlayCheck(kOverlayLook))
      ShockOverlayChange(kOverlayLook, kOverlayModeOff);

   AutoAppIPtr(GameStrings);

   g_Lookobj = o;
   
   str = pGameStrings->FetchObjString(g_Lookobj,PROP_OBJLOOKSTRING_NAME);
   strncpy(gQueryText,str,sizeof(gQueryText));
   // if we don't have a query text, just use the object's full name
   if (strlen(str) == 0)
   {
      str = pGameStrings->FetchObjString(g_Lookobj, PROP_OBJNAME_NAME);
      strncpy(gQueryText,str,sizeof(gQueryText));
   }
   gr_font_string_wrap(gShockFont,gQueryText,RectWidth(&text_rect));

   ShockOverlayChange(kOverlayLook, kOverlayModeOn);
   ShockOverlaySetObj(kOverlayLook, o);
}
//--------------------------------------------------------------------------------------
// puts cursor in look-at mode
void ShockLookCursor(void)
{
   IRes *hnd;

   if (shock_cursor_mode == SCM_LOOK)
   {
      ClearCursor();
      return;
   }

   ClearCursor();
   shock_cursor_mode = SCM_LOOK;
   //mprintf("SLC!\n");
   hnd = LoadPCX("lookcur","iface\\");
   SetCursorByHandle(hnd); 
   SafeFreeHnd(&hnd);
}

//--------------------------------------------------------------------------------------
// and now, a bunch of stuff stolen and slightly modified from Dark's inv_hack.c
//--------------------------------------------------------------------------------------
#define OBJAREA_X 15
#define OBJAREA_Y 14
static Rect *get_model_draw_rect(void)
{
   static Rect draw_rect;

   draw_rect = ShockOverlayGetRect(kOverlayLook);
   draw_rect.ul.x += OBJAREA_X;
   draw_rect.ul.y += OBJAREA_Y;
   draw_rect.lr.x = draw_rect.ul.x + 138;
   draw_rect.lr.y = draw_rect.ul.y + 109;
   return &draw_rect;
}
//--------------------------------------------------------------------------------------
static void inv_show_model(void)
{
   if (cur_inv_rend)
   {
      invRendUpdateState(cur_inv_rend,0,g_Lookobj,get_model_draw_rect(),NULL);
      invRendDrawUpdate(cur_inv_rend);
   }
}
//--------------------------------------------------------------------------------------
sOverlayFunc OverlayLook = { 
   ShockLookDraw, // draw
   ShockLookInit, // init
   ShockLookTerm, // term
   ShockLookHandleMouse, // mouse
   NULL, // dclick (really use)
   NULL, // dragdrop
   NULL, // key
   NULL, // bitmap
   "subpanel_op", // upschema
   "subpanel_cl", // downschema
   ShockLookStateChange, // state
   ShockLookCheckTransp, // transparency
   0, // distance
   TRUE, // needmouse
};
