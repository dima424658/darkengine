/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Main Deep Cover (in-game) interface code

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

#include <dpcgame.h>
#include <dpcplayr.h>
#include <dpciface.h>
#include <dpcinv.h>
#include <dpcutils.h>
#include <dpcprop.h>
#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpcfsys.h>
#include <dpclooko.h>
#include <dpccurm.h>
#include <dpcchat.h>
#include <dpcplcst.h>
#include <dpcifstr.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
#include <gadbase.h>
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

static Rect iface_rects[NUM_IFACE_BUTTONS] = 
{ 
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
void DPCInterfaceInit(int which)
{
// @TODO:Hack for Microsoft Demo
#if 0
    char temp[40];
   int i,s;

   for (s = 0; s < NUM_IFACE_BUTTONS; s++)
   {
      for (i = 0; i < 2; i++)
      {
         sprintf(temp,"ifbtn%d%d",s,i); // stupid +2 for cyber & inv button elimination
         iface_blist_handles[s][i] = LoadPCX(temp);
         AssertMsg(iface_blist_handles[s][i], "DPCInterfaceInit:  Missing iface button art.");
         iface_blist_bitmaps[s][i] = (grs_bitmap *) iface_blist_handles[s][i]->Lock();
      }
      for (i = 2; i < 4; i++)
      {
         iface_blist_bitmaps[s][i] = iface_blist_bitmaps[s][0];
      }
   }

#endif // 0
   
   sScrnMode smode;
   ScrnModeGet(&smode);
   Rect use_rect;
   use_rect.ul.x = frame_rect.ul.x;
   use_rect.ul.y = smode.h - (480 - frame_rect.ul.y);
   use_rect.lr.x = use_rect.ul.x + RectWidth(&frame_rect);
   use_rect.lr.y = use_rect.ul.y + RectHeight(&frame_rect);

   DPCOverlaySetRect(which,use_rect);
}

//--------------------------------------------------------------------------------------
// Main interface shutdown
//--------------------------------------------------------------------------------------
void DPCInterfaceTerm(void)
{
// @TODO:Hack for Microsoft Demo
#if 0
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

   DPCChatTerm();
#endif // 0
}

//--------------------------------------------------------------------------------------
static bool iface_select_cb(ushort action, int button, void* data, LGadBox* vb)
{
// @TODO:Hack for Microsoft Demo
#if 0

   //if (action == MOUSE_LDOWN)
   //   SchemaPlay((Label *)"bclick1",NULL);

//   if (action != BUTTONGADG_LCLICK)
//      return(TRUE);

   // @TODO need to make this bindable   
   if ((action != MOUSE_LDOWN) && (action != MOUSE_LUP))
      return(TRUE);

   if (DPC_cursor_mode == SCM_DRAGOBJ)
      return(TRUE);

   AutoAppIPtr(DPCPlayer);

   if (DPC_cursor_mode == SCM_LOOK)
   {
      if (action == MOUSE_LUP)
      {
         ObjID fakeobj = OBJ_NULL;
         switch (button)
         {
         case 0: // logs
            fakeobj = pDPCPlayer->GetEquip(PlayerObject(), kEquipFakeLogs);
            break;
         case 1: // keys
            fakeobj = pDPCPlayer->GetEquip(PlayerObject(), kEquipFakeKeys);
            break;
         }
         if (fakeobj != OBJ_NULL)
            DPCLookPopup(fakeobj);
         ClearCursor();
      }
      return(TRUE);
   }
  
   switch (button)
   {
   case 0: // logs
      DPCOverlayChange(kOverlayPDA, kOverlayModeToggle);
      if (DPCOverlayCheck(kOverlayPDA))
         SchemaPlay((Label *)"subpanel_op",NULL);
      else
         SchemaPlay((Label *)"subpanel_cl",NULL);
      break;
   case 1: // keys
      DPCOverlayChange(kOverlaySecurity, kOverlayModeToggle);
      if (DPCOverlayCheck(kOverlaySecurity))
         SchemaPlay((Label *)"subpanel_op",NULL);
      else
         SchemaPlay((Label *)"subpanel_cl",NULL);
      break;
   case 2: // mfd
      int which;
      which = gMFDRight;
      DPCOverlayChange(which,kOverlayModeToggle);
      // hm, is this right?
      /*
      if (DPCOverlayCheck(which))
         DPCOverlayChange(kOverlayInv,kOverlayModeOn);
      else
         DPCOverlayChange(kOverlayInv,kOverlayModeOff);
      */
      if (DPCOverlayCheck(which))
         SchemaPlay((Label *)"subpanel_op",NULL);
      else
         SchemaPlay((Label *)"subpanel_cl",NULL);
      break;
   case 3: // query
      DPCLookCursor();
      break;
   case 4: // research
      DPCOverlayChange(kOverlayResearch, kOverlayModeToggle);
      if (DPCOverlayCheck(kOverlayResearch))
         SchemaPlay((Label *)"subpanel_op",NULL);
      else
         SchemaPlay((Label *)"subpanel_cl",NULL);
      break;
   case 5: // maps
      DPCOverlayChange(kOverlayMap, kOverlayModeToggle);
      break;
   }
#endif // 0
   return TRUE;
}
//--------------------------------------------------------------------------------------
static void BuildInterfaceButtons(void)
{
// @TODO:Hack for Microsoft Demo
#if 0
   int n;
   Rect r = {{0,0},{0,0}}; // DPCOverlayGetRect(kOverlayFrame);

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
#endif // 0
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
// @TODO:Hack for Microsoft Demo
#if 0
	LGadDestroyButtonList(&iface_blist);
#endif // 0
}
//--------------------------------------------------------------------------------------
void DPCInterfaceStateChange(int which)
{
// @TODO:Hack for Microsoft Demo
#if 0
   if (DPCOverlayCheck(which))
   {
      // just got put up
      BuildInterfaceButtons();
   }
   else
   {
      // just got taken down
      DestroyInterfaceButtons();
   }
#endif // 0
}
//--------------------------------------------------------------------------------------
// Draw a string in the interface readout
//--------------------------------------------------------------------------------------
void DPCInterfaceDrawString(char *raw, int color, int cursor)
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

      r = DPCOverlayGetRect(kOverlayMiniFrame);
      gr_set_fcolor(color);
      if (strlen(str) == 0) {
         // We just need a decent cursor height:
         h = gr_font_string_height(gDPCFont, "I");
      } else {
         // Xemu 12/9/98: disabled word wrap 
         //gr_font_string_wrap(gDPCFont, str, OBJNAME_WIDTH);
         // compute where to draw the centered text
         h = gr_font_string_height(gDPCFont, str);
         w = gr_font_string_width(gDPCFont, str);
         dx = r.ul.x + OBJNAME_X; // + (OBJNAME_WIDTH - w)/2;
         dy = r.ul.y + OBJNAME_Y + (OBJNAME_HEIGHT - h)/2;
         // draw it!
         gr_font_string(gDPCFont, str, dx,dy);
         // cleanup
         //gr_font_string_unwrap(str);
      }
      if (cursor >= 0) {
         // Draw a cursor as a simple line for now
         // First figure out where it is:
         char c = str[cursor];
         str[cursor] = '\0';
         int curx;
         curx = r.ul.x + OBJNAME_X + gr_font_string_width(gDPCFont, str);
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
char gHelpString[255] = {0};

static void DrawObjName(void)
{
   char temp[255];

   strcpy(temp,"");

   if (strlen(gHelpString) > 0)
   {
      DPCInterfaceDrawString(gHelpString, gDPCTextColor, -1);
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

   DPCInterfaceDrawString(temp, gDPCTextColor, -1);
}

//--------------------------------------------------------------------------------------
// Draw main interface
//--------------------------------------------------------------------------------------
static char *helpnames[] = { "", "", "MouseHelpMFD","MouseHelpQuery", "MouseHelpResearch", "MouseHelpMaps" };

void DPCInterfaceDraw(unsigned long inDeltaTicks)
{
// @TODO:  Bodisafa 10/30/1999
// Removed shock interface drawing for Microsoft Demo.
#if 0
   //grs_bitmap *bm = (grs_bitmap *) gHndFrame->Lock();
   //Rect r = DPCOverlayGetRect(kOverlayFrame);
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
            DPCStringFetch(gHelpString,sizeof(gHelpString),helpnames[i],"misc");
         break;
      }
   }

   /*
   if (DPCChatGettingInput()) {
      DPCChatDrawInput();
   } else {
      DrawObjName();
   }
   */
#endif // 0
}

//--------------------------------------------------------------------------------------
// Basic interface handling.  Also covers the inventory popup by virtue of
// calling the InvFindObjSlot which covers both quick slots & real slots
//--------------------------------------------------------------------------------------
// anywhere in the main interface bar is a valid drop
bool DPCInterfaceHandleMouse(Point pos)
{
   /*
   bool retval = TRUE;

   if (DPC_mouse && (DPC_cursor_mode == SCM_DRAGOBJ)) 
   {
      int i;
      if (RectTestPt(&invdrop_rect, pos) && !RectTestPt(&iface_rects[0],pos))
      {
         // should we feedback with sound or something if this fails?
         DPCInvAddObj(PlayerObject(),drag_obj);

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
bool DPCInterfaceCheckTransp(Point pt)
{
   return(FALSE);
   /*
   bool retval = FALSE;
   //Rect r;
   //r = DPCOverlayGetRect(kOverlayFrame);
   int pix = HandleGetPix(gHndFrame,pt);
   if (pix == 0)
      retval = TRUE;
   //mprintf("SICT: %d (%d)\n",retval,pix);
   return(retval);
   */
}

//--------------------------------------------------------------------------------------
void DPCInterfaceMouseOver(ObjID o)
{
// @TODO:  Bodisafa 10/30/1999
// Hack for Microsoft Demo.
#if 0
   gFocusTouched = TRUE;
   BOOL slim_mode;
   AutoAppIPtr(QuestData);
   slim_mode = pQuestData->Get("HideInterface");

   if (!slim_mode)
   {
      if ((o != OBJ_NULL) && (o != frobWorldFocusObj) && (o != g_ifaceFocusObj))
         SchemaPlay((Label *)"rollover", NULL);
   }
#endif // 1
}

//--------------------------------------------------------------------------------------
// "mini-frame" for object name text in mouselook mode
//--------------------------------------------------------------------------------------
static IRes *gMiniFrameHnd;

static Rect mini_rect = {{192, 0},{192 + 255, 0 + 17}};
//--------------------------------------------------------------------------------------
void DPCMiniFrameInit(int which)
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
   DPCOverlaySetRect(which,use_rect);

   DPCChatInit(OBJNAME_X + use_rect.ul.x, OBJNAME_Y + use_rect.ul.y, OBJNAME_WIDTH, OBJNAME_HEIGHT);
}

//--------------------------------------------------------------------------------------
void DPCMiniFrameTerm(void)
{
   SafeFreeHnd(&gMiniFrameHnd);
}

//--------------------------------------------------------------------------------------
void DPCMiniFrameDraw(unsigned long inDeltaTicks)
{

    Rect r = DPCOverlayGetRect(kOverlayMiniFrame);
   DrawByHandle(gMiniFrameHnd,r.ul);
   if (DPCChatGettingInput()) 
   {
      DPCChatDrawInput();
   }
   else
   {
       DrawObjName();
   }
}

