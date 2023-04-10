/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkiface.cpp,v 1.50 2000/02/19 13:25:28 toml Exp $

// Main shock interface code

#include <2d.h>

#include <res.h>
#include <guistyle.h>
#include <config.h>

#include <resapi.h>

#include <frobctrl.h>
#include <mprintf.h>

#include <playrobj.h>
#include <schema.h>
#include <scrnmode.h>
#include <questapi.h>

#include <shkgame.h>
#include <shkplayr.h>
#include <shkiface.h>
#include <shkinv.h>
#include <shkutils.h>
#include <shkprop.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkfsys.h>
#include <shklooko.h>
#include <shkcurm.h>
#include <shkcyber.h>
#include <shkchat.h>
#include <shkplcst.h>
#include <shkifstr.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
#include <gadbase.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 
}

ObjID g_ifaceFocusObj = OBJ_NULL;
BOOL gFocusTouched = FALSE;

int gMFDRight = kOverlayStats;

#define NUM_IFACE_BUTTONS  6

static LGadButtonList iface_blist;
static LGadButtonListDesc iface_blistdesc;
static DrawElement iface_blist_elems[NUM_IFACE_BUTTONS];
static grs_bitmap *iface_blist_bitmaps[NUM_IFACE_BUTTONS][4];
static IRes *iface_blist_handles[NUM_IFACE_BUTTONS][2];

#define AMMO_X 378
#define AMMO_Y 414

#define BIO_X  2
#define BIO_Y  414

static Rect iface_rects[NUM_IFACE_BUTTONS] = { 
   {{AMMO_X + 5, AMMO_Y + 18},{AMMO_X + 5 + 38, AMMO_Y + 18 + 36}},
   {{AMMO_X + 44,AMMO_Y + 18},{AMMO_X + 44 + 38, AMMO_Y + 18 + 36}},
   {{AMMO_X + 82,AMMO_Y + 16},{AMMO_X + 82 + 32, AMMO_Y + 16 + 40}},

   {{BIO_X + 148, BIO_Y + 17}, {BIO_X + 148 + 32, BIO_Y + 17 + 18}},
   {{BIO_X + 115, BIO_Y + 17}, {BIO_X + 115 + 32, BIO_Y + 17 + 40}},
   {{BIO_X + 148, BIO_Y + 37}, {BIO_X + 148 + 32, BIO_Y + 37 + 18}},
};

static Rect use_rects[NUM_IFACE_BUTTONS];

#define OBJNAME_X (12)
#define OBJNAME_Y (0)
#define OBJNAME_WIDTH  140
#define OBJNAME_HEIGHT 10

//--------------------------------------------------------------------------------------
// Main interface initializer
//--------------------------------------------------------------------------------------

#define FRAME_W   (262 - 130)
#define FRAME_H   64
static Rect frame_rect = {{130, 414}, {130 + FRAME_W, 414 + FRAME_H}};
void ShockInterfaceInit(int which)
{
   char temp[40];
   int i,s;

   for (s = 0; s < NUM_IFACE_BUTTONS; s++)
   {
      for (i = 0; i < 2; i++)
      {
         sprintf(temp,"ifbtn%d%d",s,i); // stupid +2 for cyber & inv button elimination
         iface_blist_handles[s][i] = LoadPCX(temp);
         iface_blist_bitmaps[s][i] = (grs_bitmap *) iface_blist_handles[s][i]->Lock();
      }
      for (i = 2; i < 4; i++)
      {
         iface_blist_bitmaps[s][i] = iface_blist_bitmaps[s][0];
      }
   }

   Rect use_rect;
   sScrnMode smode;
   ScrnModeGet(&smode);
   use_rect.ul.x = frame_rect.ul.x;
   use_rect.ul.y = smode.h - (480 - frame_rect.ul.y);
   use_rect.lr.x = use_rect.ul.x + RectWidth(&frame_rect);
   use_rect.lr.y = use_rect.ul.y + RectHeight(&frame_rect);

   ShockOverlaySetRect(which,use_rect);
}

//--------------------------------------------------------------------------------------
// Main interface shutdown
//--------------------------------------------------------------------------------------
void ShockInterfaceTerm(void)
{
   int s,i;

   for (s = 0; s < NUM_IFACE_BUTTONS; s++)
   {
      for (i = 0; i < 2; i++)
      {
         iface_blist_handles[s][i]->Unlock();
         SafeFreeHnd(&iface_blist_handles[s][i]);
      }
   }
   //SafeFreeHnd(&gHndFrame);

   ShockChatTerm();
}

//--------------------------------------------------------------------------------------
static bool iface_select_cb(ushort action, int button, void* data, LGadBox* vb)
{
   //if (action == MOUSE_LDOWN)
   //   SchemaPlay((Label *)"bclick1",NULL);

//   if (action != BUTTONGADG_LCLICK)
//      return(TRUE);

   // @TODO need to make this bindable   
   if ((action != MOUSE_LDOWN) && (action != MOUSE_LUP))
      return(TRUE);

   if (shock_cursor_mode == SCM_DRAGOBJ)
      return(TRUE);

   AutoAppIPtr(ShockPlayer);

   if (shock_cursor_mode == SCM_LOOK)
   {
      if (action == MOUSE_LUP)
      {
         ObjID fakeobj = OBJ_NULL;
         switch (button)
         {
         case 0: // logs
            fakeobj = pShockPlayer->GetEquip(PlayerObject(), kEquipFakeLogs);
            break;
         case 1: // keys
            fakeobj = pShockPlayer->GetEquip(PlayerObject(), kEquipFakeKeys);
            break;
         }
         if (fakeobj != OBJ_NULL)
            ShockLookPopup(fakeobj);
         ClearCursor();
      }
      return(TRUE);
   }
  
   switch (button)
   {
   case 0: // logs
      ShockOverlayChange(kOverlayPDA, kOverlayModeToggle);
      if (ShockOverlayCheck(kOverlayPDA))
         SchemaPlay((Label *)"subpanel_op",NULL);
      else
         SchemaPlay((Label *)"subpanel_cl",NULL);
      break;
   case 1: // keys
      ShockOverlayChange(kOverlaySecurity, kOverlayModeToggle);
      if (ShockOverlayCheck(kOverlaySecurity))
         SchemaPlay((Label *)"subpanel_op",NULL);
      else
         SchemaPlay((Label *)"subpanel_cl",NULL);
      break;
   case 2: // mfd
      int which;
      which = gMFDRight;
      ShockOverlayChange(which,kOverlayModeToggle);
      // hm, is this right?
      /*
      if (ShockOverlayCheck(which))
         ShockOverlayChange(kOverlayInv,kOverlayModeOn);
      else
         ShockOverlayChange(kOverlayInv,kOverlayModeOff);
      */
      if (ShockOverlayCheck(which))
         SchemaPlay((Label *)"subpanel_op",NULL);
      else
         SchemaPlay((Label *)"subpanel_cl",NULL);
      break;
   case 3: // query
      ShockLookCursor();
      break;
   case 4: // research
      ShockOverlayChange(kOverlayResearch, kOverlayModeToggle);
      if (ShockOverlayCheck(kOverlayResearch))
         SchemaPlay((Label *)"subpanel_op",NULL);
      else
         SchemaPlay((Label *)"subpanel_cl",NULL);
      break;
   case 5: // maps
      ShockOverlayChange(kOverlayMap, kOverlayModeToggle);
      break;
   }
   return TRUE;
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
   int n;
   Rect r = {{0,0},{0,0}}; // ShockOverlayGetRect(kOverlayFrame);

   sScrnMode smode;
   ScrnModeGet(&smode);
   // yes, I am awful for having this number just sitting here
   r.ul.y = (smode.h - 480);

	// make a button for each service, ie a button list
   for (n=0; n < NUM_IFACE_BUTTONS; n++)
   {
      DrawElement *elem = &iface_blist_elems[n];
      elem->draw_type = DRAWTYPE_BITMAPOFFSET;
      elem->draw_data = iface_blist_bitmaps[n];
      elem->draw_data2 = (void *)4; // should be 2 but hackery required

      use_rects[n].ul.x = iface_rects[n].ul.x + r.ul.x;
      if (n <= 2)
         use_rects[n].ul.x += (smode.w - 640);
      use_rects[n].ul.y = iface_rects[n].ul.y + r.ul.y;
      use_rects[n].lr.x = use_rects[n].ul.x + RectWidth(&iface_rects[n]);
      use_rects[n].lr.y = use_rects[n].ul.y + RectHeight(&iface_rects[n]);
      /*
      mprintf("use_rect[%d] = %d,%d  %d,%d  from  %d,%d %d,%d\n",n,
         use_rects[n].ul.x,use_rects[n].ul.y,use_rects[n].lr.x,use_rects[n].lr.y,
         iface_rects[n].ul.x,iface_rects[n].ul.y,iface_rects[n].lr.x,iface_rects[n].lr.y);
      */   
   }
   
   iface_blistdesc.num_buttons = NUM_IFACE_BUTTONS;
   iface_blistdesc.button_rects = use_rects;
   iface_blistdesc.button_elems = iface_blist_elems;
   iface_blistdesc.cb = iface_select_cb;
   //iface_blistdesc.flags = BUTTONLIST_RADIO_FLAG;

   LGadCreateButtonListDesc(&iface_blist, LGadCurrentRoot(), &iface_blistdesc);
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   LGadDestroyButtonList(&iface_blist);
}
//--------------------------------------------------------------------------------------
void ShockInterfaceStateChange(int which)
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
// Draw a string in the interface readout
//--------------------------------------------------------------------------------------
void ShockInterfaceDrawString(char *raw, int color, int cursor)
{
   char str[512];
   strcpy(str,raw);
   
   // remove pesky newlines
   int i;
   for (i=0; i < strlen(str); i++)
   {
      if (str[i] == '\n')
      {
         str[i] = '\0';
         break;
      }
   }

   // now draw it proper
   if (strlen(str) >= 0)
   {
      Rect r;
      int dx,dy,w,h;

      r = ShockOverlayGetRect(kOverlayMiniFrame);
      gr_set_fcolor(color);
      if (strlen(str) == 0) {
         // We just need a decent cursor height:
         h = gr_font_string_height(gShockFont, "I");
      } else {
         // Xemu 12/9/98: disabled word wrap 
         //gr_font_string_wrap(gShockFont, str, OBJNAME_WIDTH);
         // compute where to draw the centered text
         h = gr_font_string_height(gShockFont, str);
         w = gr_font_string_width(gShockFont, str);
         dx = r.ul.x + OBJNAME_X; // + (OBJNAME_WIDTH - w)/2;
         dy = r.ul.y + OBJNAME_Y + (OBJNAME_HEIGHT - h)/2;
         // draw it!
         gr_font_string(gShockFont, str, dx,dy);
         // cleanup
         //gr_font_string_unwrap(str);
      }
      if (cursor >= 0) {
         // Draw a cursor as a simple line for now
         // First figure out where it is:
         char c = str[cursor];
         str[cursor] = '\0';
         int curx;
         curx = r.ul.x + OBJNAME_X + gr_font_string_width(gShockFont, str);
         str[cursor] = c;
         // Now draw it:
         int centery;
         centery = r.ul.y + OBJNAME_Y + (OBJNAME_HEIGHT/2);
         dy = h/2;
         gr_vline(curx, centery - dy, centery + dy);
      }
   }
}

//--------------------------------------------------------------------------------------
// Draw object name text
//--------------------------------------------------------------------------------------
char gHelpString[255];

static void DrawObjName(void)
{
   char temp[255];

   strcpy(temp,"");

   if (strlen(gHelpString) > 0)
   {
      ShockInterfaceDrawString(gHelpString, gShockTextColor, -1);
      // clear it every frame so that it is not persistent
      // kind of dumb, I admit.
      gHelpString[0]= '\0';
      return;
   }

   if (frobWorldSelectObj != OBJ_NULL)
      ObjGetObjNameSubst(frobWorldSelectObj, temp, sizeof(temp));
   else if (g_ifaceFocusObj != OBJ_NULL)
   {
      ObjGetObjNameSubst(g_ifaceFocusObj, temp, sizeof(temp));
   }

   ShockInterfaceDrawString(temp, gShockTextColor, -1);
}

//--------------------------------------------------------------------------------------
// Draw main interface
//--------------------------------------------------------------------------------------
static char *helpnames[] = { "", "", "MouseHelpMFD","MouseHelpQuery", "MouseHelpResearch", "MouseHelpMaps" };

void ShockInterfaceDraw(void)
{
   //grs_bitmap *bm = (grs_bitmap *) gHndFrame->Lock();
   //Rect r = ShockOverlayGetRect(kOverlayFrame);
   //gr_set_bitmap_format(bm, (ushort)BMF_TRANS);
   //gr_bitmap(bm, r.ul.x, r.ul.y);
   //gHndFrame->Unlock();

   LGadDrawBox(VB(&iface_blist),NULL);

   int i;
   Point mpt;
   mouse_get_xy(&mpt.x,&mpt.y);
   for (i=0; i < NUM_IFACE_BUTTONS; i++)
   {
      if (RectTestPt(&use_rects[i],mpt))
      {
         if (strlen(helpnames[i]) > 0)
            ShockStringFetch(gHelpString,sizeof(gHelpString),helpnames[i],"misc");
         break;
      }
   }

   /*
   if (ShockChatGettingInput()) {
      ShockChatDrawInput();
   } else {
      DrawObjName();
   }
   */
}

//--------------------------------------------------------------------------------------
// Basic interface handling.  Also covers the inventory popup by virtue of
// calling the InvFindObjSlot which covers both quick slots & real slots
//--------------------------------------------------------------------------------------
// anywhere in the main interface bar is a valid drop
bool ShockInterfaceHandleMouse(Point pos)
{
   /*
   bool retval = TRUE;

   if (shock_mouse && (shock_cursor_mode == SCM_DRAGOBJ)) 
   {
      int i;
      if (RectTestPt(&invdrop_rect, pos) && !RectTestPt(&iface_rects[0],pos))
      {
         // should we feedback with sound or something if this fails?
         ShockInvAddObj(PlayerObject(),drag_obj);

      }
      // don't grab the event if one of those buttons is going
      // to get it instead.
      for (i =0; i < NUM_IFACE_BUTTONS; i++)
      {
         if (RectTestPt(&iface_rects[i],pos))
            return(TRUE);
      }

      // otherwise, put it in our inventory safe and sound
   }

   return(retval);
   */

   return(TRUE);
}
//--------------------------------------------------------------------------------------
// Check for transparency in the source art
//--------------------------------------------------------------------------------------
bool ShockInterfaceCheckTransp(Point pt)
{
   return(FALSE);
   /*
   bool retval = FALSE;
   //Rect r;
   //r = ShockOverlayGetRect(kOverlayFrame);
   int pix = HandleGetPix(gHndFrame,pt);
   if (pix == 0)
      retval = TRUE;
   //mprintf("SICT: %d (%d)\n",retval,pix);
   return(retval);
   */
}

//--------------------------------------------------------------------------------------
void ShockInterfaceMouseOver(ObjID o)
{
   gFocusTouched = TRUE;
   BOOL slim_mode;
   AutoAppIPtr(QuestData);
   slim_mode = pQuestData->Get("HideInterface");

   if (!slim_mode)
   {
      if ((o != OBJ_NULL) && (o != frobWorldFocusObj) && (o != g_ifaceFocusObj))
         SchemaPlay((Label *)"rollover", NULL);
   }
}

//--------------------------------------------------------------------------------------
// "mini-frame" for object name text in mouselook mode
//--------------------------------------------------------------------------------------
static IRes *gMiniFrameHnd;

static Rect mini_rect = {{192, 0},{192 + 255, 0 + 17}};
//--------------------------------------------------------------------------------------
void ShockMiniFrameInit(int which)
{
   gMiniFrameHnd= LoadPCX("frame");

   Rect use_rect;
   sScrnMode smode;
   ScrnModeGet(&smode);
   if (smode.w != 640)
      use_rect.ul.x = (smode.w - RectWidth(&mini_rect)) / 2;
   else
      use_rect.ul.x = mini_rect.ul.x;
   use_rect.ul.y = mini_rect.ul.y; // (smode.h - (480 - mini_rect.ul.y));
   use_rect.lr.x = use_rect.ul.x + RectWidth(&mini_rect);
   use_rect.lr.y = use_rect.ul.y + RectHeight(&mini_rect);
   ShockOverlaySetRect(which,use_rect);

   ShockChatInit(OBJNAME_X + use_rect.ul.x, OBJNAME_Y + use_rect.ul.y, OBJNAME_WIDTH, OBJNAME_HEIGHT);
}

//--------------------------------------------------------------------------------------
void ShockMiniFrameTerm(void)
{
   SafeFreeHnd(&gMiniFrameHnd);
}

//--------------------------------------------------------------------------------------
void ShockMiniFrameDraw(void)
{
   Rect r = ShockOverlayGetRect(kOverlayMiniFrame);
   DrawByHandle(gMiniFrameHnd,r.ul);

   if (ShockChatGettingInput()) {
      ShockChatDrawInput();
   } else {
      DrawObjName();
   }
}

