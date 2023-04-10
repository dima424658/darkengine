/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Keypad gump
#include <2d.h>
#include <appagg.h>
#include <res.h>

#include <resapi.h>
#include <kbcook.h>
#include <keydefs.h>

#include <editor.h>

#include <fonrstyp.h>

#include <iobjnet.h>
#include <schema.h>

#include <gen_bind.h>
#include <contexts.h>
#include <mprintf.h>

#include <scrptapi.h>
#include <dpcscrm.h>

#include <dpcprop.h>
#include <dpckeypd.h>
#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpcutils.h>
#include <dpcmfddm.h>
#include <dpchplug.h>
#include <dpcobjst.h>
#include <dpciftul.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
}

IRes *gKeypadBack;
// okay, so maybe I should Class-ify this
static int keypad_num = 0;
static int keypad_digits = 0;

static Rect full_rect = {{LMFD_X, LMFD_Y}, {LMFD_X + LMFD_W, LMFD_Y + LMFD_H}}; 
static Rect close_rect = {{163,8},{163 + 20, 8 + 21}};

#define NUM_KEYPAD_BUTTONS  11

static LGadButtonList keypad_blist;
static LGadButtonListDesc keypad_blistdesc;
static DrawElement keypad_blist_elems[NUM_KEYPAD_BUTTONS];
static grs_bitmap *keypad_blist_bitmaps[NUM_KEYPAD_BUTTONS][4];
static IRes *keypad_blist_handles[NUM_KEYPAD_BUTTONS][2];

static LGadButton close_button;
static DrawElement close_elem;
static IRes *close_handles[2];
static grs_bitmap *close_bitmaps[4];

#define BUTTON_W  44
#define BUTTON_H  60
#define BUTTON_RECT(x,y) {{(x),(y)}, {(x) + BUTTON_W, (y) + BUTTON_H}}
//#define BUTTON_RECT2(x,y) {{(x),(y)}, {(x) + , (y) + BUTTON_H}}
static Rect keypad_rects[NUM_KEYPAD_BUTTONS] = { 
   BUTTON_RECT(15,43),
   BUTTON_RECT(62,43),
   BUTTON_RECT(109,43),
   BUTTON_RECT(15,104),
   BUTTON_RECT(62,104),
   BUTTON_RECT(109,104),
   BUTTON_RECT(15,165),
   BUTTON_RECT(62,165),
   BUTTON_RECT(109,165),
   BUTTON_RECT(15,226),
   BUTTON_RECT(62,226),
};

char *key_info[NUM_KEYPAD_BUTTONS] = 
{ 
  "1","2","3",
  "4","5","6",
  "7","8","9",
  "0","CE",
};

int key_value[NUM_KEYPAD_BUTTONS] = 
{  1, 2, 3,
   4, 5, 6,
   7, 8, 9, 
      0, -1,
};

grs_font *gKeypadFont = NULL;
IRes *gKeypadFontRes = NULL;

grs_canvas gKeypadCanvas;
grs_bitmap *gKeypadBitmap;

//--------------------------------------------------------------------------------------
void DPCKeypadClear(void)
{
   keypad_num = 0;
   keypad_digits = 0;
}
//--------------------------------------------------------------------------------------
void DPCKeypadInit(int which)
{
// @NOTE:  TEMPCODE
#if 0
    //char temp[40];
   int i; // ,s;

   close_handles[0] = LoadPCX("CloseOff"); 
   close_handles[1] = LoadPCX("CloseOn"); 
   close_bitmaps[0] = (grs_bitmap *) close_handles[0]->Lock();
   close_bitmaps[1] = (grs_bitmap *) close_handles[1]->Lock();
   for (i = 2; i < 4; i++)
   {
      close_bitmaps[i] = close_bitmaps[0];
   }

   /*
   for (s = 0; s < NUM_KEYPAD_BUTTONS; s++)
   {
      for (i = 0; i < 2; i++)
      {
         //sprintf(temp,"key%c%d",key_info[s],i);
         sprintf(temp,"key%d",i);
         keypad_blist_handles[s][i] = LoadPCX(temp);
         keypad_blist_bitmaps[s][i] = (grs_bitmap *) keypad_blist_handles[s][i]->Lock();
      }
      for (i = 2; i < 4; i++)
      {
         keypad_blist_bitmaps[s][i] = keypad_blist_bitmaps[s][0];
      }
   }
   */

   gKeypadBack = LoadPCX("keypad2");
   SetLeftMFDRect(which, full_rect);

   AutoAppIPtr(ResMan);
   
   gKeypadFontRes = pResMan->Bind("keyfonta",RESTYPE_FONT, NULL, "fonts\\");
   if (gKeypadFontRes != NULL)
      gKeypadFont = (grs_font *)gKeypadFontRes->Lock();

   // set the right magic font
   // cheat by just grabbing the palette from the main font
   gKeypadFont->pal_id = gDPCFontAA->pal_id;

   /*
   short w,h;
   w = RectWidth(&full_rect);
   h = RectHeight(&full_rect);
   gKeypadBitmap = gr_alloc_bitmap(BMT_FLAT16, BMF_TRANS, w, h); // BMF_TRANS
   gr_make_canvas(gKeypadBitmap, &gKeypadCanvas);
   */
#endif // 0
}

//--------------------------------------------------------------------------------------
void DPCKeypadTerm(void)
{
// @NOTE:  TEMPCODE
#if 0
   /*
   int s,i;
   for (s = 0; s < NUM_KEYPAD_BUTTONS; s++)
   {
      for (i = 0; i < 2; i++)
      {
         keypad_blist_handles[s][i]->Unlock();
         SafeFreeHnd(&keypad_blist_handles[s][i]);
      }
   }
   */

   SafeFreeHnd(&gKeypadBack);

   close_handles[0]->Unlock();
   close_handles[1]->Unlock();
   SafeFreeHnd(&close_handles[0]);
   SafeFreeHnd(&close_handles[1]);

   gKeypadFontRes->Unlock();
   gKeypadFontRes->Release();

   //gr_free(gKeypadBitmap);
#endif
}

//--------------------------------------------------------------------------------------
void DPCKeypadDrawCore(void)
{
 // !// @NOTE:  TEMPCODE
#if 0
   int dx,dy; // ,i;
   //int w,h;
   char format[32], temp[255];
   //Rect r2 = DPCOverlayGetRect(kOverlayKeypad);
   Rect r = DPCOverlayGetRect(kOverlayKeypad);
   /*
   Rect r;

   r.ul.x = 0;
   r.ul.y = 0;
   r.lr.x = RectWidth(&r2);
   r.lr.y = RectHeight(&r2);
   */

   DrawByHandle(gKeypadBack,r.ul);

   //LGadDrawBox(VB(&keypad_blist),NULL);
   LGadDrawBox(VB(&close_button),NULL);

   dx = r.ul.x + 14;
   dy = r.ul.y + 14;
   sprintf(format,"%%0%dd",keypad_digits);
   sprintf(temp,format,keypad_num);
   gr_set_fcolor(gDPCTextColor);
   gr_font_string(gKeypadFont,temp,dx,dy);

   /*
   // draw the buttons & numbers on top of the buttons
   for (i=0; i < NUM_KEYPAD_BUTTONS; i++)
   {
      dx = r.ul.x + keypad_rects[i].ul.x;
      dy = r.ul.y + keypad_rects[i].ul.y;
      DrawByHandle(keypad_blist_handles[i][0],dx,dy);
      
      sprintf(temp,"%s",key_info[i]);

      w = gr_font_string_width(gKeypadFont, temp);
      h = gr_font_string_height(gKeypadFont, temp);
      dx = r.ul.x + keypad_rects[i].ul.x + (RectWidth(&keypad_rects[i]) - w) / 2;
      dy = r.ul.y + keypad_rects[i].ul.y + (RectHeight(&keypad_rects[i]) - h) / 2;
      gr_font_string(gKeypadFont, temp, dx, dy);
   }
   */

#endif // 
}
//--------------------------------------------------------------------------------------
void DPCKeypadDraw(unsigned long inDeltaTicks)
{
 // !// @NOTE:  TEMPCODE
#if 0
   DPCKeypadDrawCore();

   /*
   Rect r = DPCOverlayGetRect(kOverlayKeypad);

   gr_push_canvas(&gKeypadCanvas);
   DPCKeypadDrawCore();
   gr_pop_canvas();

   gr_bitmap(gKeypadBitmap, r.ul.x, r.ul.y);
   */
#endif // 
}
//--------------------------------------------------------------------------------------
void DeferKeypad(void *data)
{
   ObjID o = (ObjID)data;
   // okay, we have a complete code, send it off to the script
   AutoAppIPtr(ScriptMan);
   sKeypadMsg message(o,keypad_num);
   // Even if we don't own the keypad, run this script locally:
   message.flags |= kSMF_MsgSendToProxy;

   pScriptMan->SendMessage(&message);
}
//--------------------------------------------------------------------------------------
void KeypadButton(int button_num)
{
   if (button_num >= 0)
   {
      SchemaPlay((Label *)"bkeypad",NULL);

      if (keypad_digits >= 5)
      {
         DPCKeypadClear();
      }
      keypad_num = (keypad_num * 10) + button_num;
      //keypad_num = keypad_num + (button_num * pow(10, 2 - keypad_digits));
      keypad_digits++;
      if (keypad_digits == 5)
      {
         uiDefer(DeferKeypad, (void *)DPCOverlayGetObj());
      }
   }
}
//--------------------------------------------------------------------------------------
#pragma off(unreferenced)
/*
static bool key_handler_func(uiEvent* ev, Region* r, void* data)
{
   uiCookedKeyEvent* kev = (uiCookedKeyEvent*)ev;

   int keycode = kev->code;
   int kc;

   if (keycode&KB_FLAG_DOWN)
   {
      kc=keycode&~KB_FLAG_DOWN;
      if ((kc >= '0') && (kc <= '9'))
      {
         KeypadButton(kc - '0');
         return(TRUE);
      }
   }
   return(FALSE);
}
*/
static bool rawkey_handler_func(uiEvent* ev, Region* r, void* data)
{
   ObjID o;
   o = DPCOverlayGetObj();
   if (ObjGetObjState(o) == kObjStateBroken)
      return(FALSE);
   
   ushort keycode;
   bool result;
   
   kbs_event kbe;
   kbe.code = ((uiRawKeyEvent*)ev)->scancode;
   kbe.state = ((uiRawKeyEvent*)ev)->action;

   kb_cook(kbe,&keycode,&result);

   int kc;
   if (keycode&KB_FLAG_DOWN)
   {
      kc=keycode&~KB_FLAG_DOWN;
      {
         switch(kc)
         {
         case '0':
         case KEY_PAD_INS:
            KeypadButton(0);
            return(TRUE);
         case '1':
         case KEY_PAD_END:
            KeypadButton(1);
            return(TRUE);
         case '2':
         case KEY_PAD_DOWN:
            KeypadButton(2);
            return(TRUE);
         case '3':
         case KEY_PAD_PGDN:
            KeypadButton(3);
            return(TRUE);
         case '4':
         case KEY_PAD_LEFT:
            KeypadButton(4);
            return(TRUE);
         case '5':
         case KEY_PAD_CENTER:
            KeypadButton(5);
            return(TRUE);
         case '6':
         case KEY_PAD_RIGHT:
            KeypadButton(6);
            return(TRUE);
         case '7':
         case KEY_PAD_HOME:
            KeypadButton(7);
            return(TRUE);
         case '8':
         case KEY_PAD_UP:
            KeypadButton(8);
            return(TRUE);
         case '9':
         case KEY_PAD_PGUP:
            KeypadButton(9);
            return(TRUE);
         case KEY_PAD_DEL:
            DPCKeypadClear();
            return(TRUE);
         }
         /*
         int val = (kc & 0xff) - '0';
         if ((val >= 0) && (val <= 9))
         {
            KeypadButton(val);
            return(TRUE);
         }
         */
      }
   }
   return(FALSE);
}

//--------------------------------------------------------------------------------------
bool DPCKeypadHandleMouse(Point pt)
{
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static bool keypad_select_cb(ushort action, int button, void* data, LGadBox* vb)
{
   int button_num = -1;

   if (action != BUTTONGADG_LCLICK)
      return(FALSE);

   ObjID o;
   o = DPCOverlayGetObj();
   if (ObjGetObjState(o) == kObjStateBroken)
      return(TRUE);

   if (key_value[button] >= 0)
   {
      button_num = key_value[button];
      KeypadButton(button_num);
   }
   else
      DPCKeypadClear();
   return TRUE;
}
//--------------------------------------------------------------------------------------
static bool close_cb(short action, void* data, LGadBox* vb)
{
   uiDefer(DeferOverlayClose,(void *)kOverlayKeypad);
   SchemaPlay((Label *)"subpanel_cl",NULL);
   return(TRUE);
}
//--------------------------------------------------------------------------------------
static int key_handler_id;
static void BuildInterfaceButtons(void)
{
   int n;
   Rect r = DPCOverlayGetRect(kOverlayKeypad);
   static Rect use_rects[NUM_KEYPAD_BUTTONS];

	// make a button for each service, ie a button list
   for (n=0; n < NUM_KEYPAD_BUTTONS; n++)
   {
      /*
      DrawElement *elem = &keypad_blist_elems[n];
      elem->draw_type = DRAWTYPE_BITMAPOFFSET;
      elem->draw_data = keypad_blist_bitmaps[n];
      elem->draw_data2 = (void *)4; // should be 2 but hackery required
      */

      use_rects[n].ul.x = keypad_rects[n].ul.x + r.ul.x;
      use_rects[n].ul.y = keypad_rects[n].ul.y + r.ul.y;
      use_rects[n].lr.x = keypad_rects[n].lr.x + r.ul.x;
      use_rects[n].lr.y = keypad_rects[n].lr.y + r.ul.y;
   }
   
   keypad_blistdesc.num_buttons = NUM_KEYPAD_BUTTONS;
   keypad_blistdesc.button_rects = use_rects;
   keypad_blistdesc.button_elems = keypad_blist_elems;
   keypad_blistdesc.cb = keypad_select_cb;
   //keypad_blistdesc.flags = BUTTONLIST_RADIO_FLAG;

   LGadCreateButtonListDesc(&keypad_blist, LGadCurrentRoot(), &keypad_blistdesc);

   // set up the continue button
   close_elem.draw_type = DRAWTYPE_BITMAPOFFSET;
   close_elem.draw_data = close_bitmaps;
   close_elem.draw_data2 = (void *)4; // should be 2 but hackery required

   LGadCreateButtonArgs(&close_button, LGadCurrentRoot(), close_rect.ul.x + r.ul.x, close_rect.ul.y + r.ul.y,
      RectWidth(&close_rect), RectHeight(&close_rect), &close_elem, close_cb, 0);

   //mprintf("building buttons\n");
   uiInstallRegionHandler(LGadBoxRegion(VB(&keypad_blist)),UI_EVENT_KBD_RAW,rawkey_handler_func,NULL,&key_handler_id);
   uiGrabFocus(LGadBoxRegion(VB(&keypad_blist)), UI_EVENT_KBD_RAW);
}

//--------------------------------------------------------------------------------------
static void DestroyInterfaceButtons(void)
{
   //mprintf("destroying buttons\n");
   uiReleaseFocus(LGadBoxRegion(VB(&keypad_blist)), UI_EVENT_KBD_RAW);
   uiRemoveRegionHandler(LGadBoxRegion(VB(&keypad_blist)),key_handler_id);

   LGadDestroyButtonList(&keypad_blist);
   LGadDestroyBox(VB(&close_button),FALSE);
}
//--------------------------------------------------------------------------------------
void DPCKeypadStateChange(int which)
{
   if (DPCOverlayCheck(which))
   {
      // just got put up
      BuildInterfaceButtons();

      ObjID o;
      o = DPCOverlayGetObj();
      if (ObjGetObjState(o) == kObjStateBroken)
      {
         DPCHRMPlugSetMode(1, o);
         DPCOverlayChange(kOverlayHRMPlug, kOverlayModeOn);
      }
      else
      {
         // make sure we are hackable
         sTechInfo *hdiff;
         if (gPropHackDiff->Get(o,&hdiff))
         {
            if (hdiff->m_success > -1000)
            {
               DPCHRMPlugSetMode(0, o);
               DPCOverlayChange(kOverlayHRMPlug, kOverlayModeOn);
            }
         }
      }
   }
   else
   {
      // just got taken down
      DestroyInterfaceButtons();
      DPCOverlayChange(kOverlayHRMPlug, kOverlayModeOff);
   }
}
//--------------------------------------------------------------------------------------
void DPCKeypadOpen(ObjID o)
{
   DPCOverlayChangeObj(kOverlayKeypad, kOverlayModeOn,o);
   DPCKeypadClear();
}
//--------------------------------------------------------------------------------------
// Check for transparency in the source art
//--------------------------------------------------------------------------------------
bool DPCKeypadCheckTransp(Point pt)
{
   return(FALSE); 
   //return(DPCOverlayCheckTransp(pt,kOverlayKeypad, gKeypadBack));
}

sOverlayFunc OverlayKeypad = 
{ 
   DPCKeypadDraw,             // draw
   DPCKeypadInit,             // init
   DPCKeypadTerm,             // term
   DPCKeypadHandleMouse,      // mouse
   NULL,                      // dclick (really use)
   NULL,                      // dragdrop
   NULL,                      // key
   NULL,                      // bitmap
   "subpanel_op",             // upschema
   "",                        // downschema
   DPCKeypadStateChange,      // state
   DPCKeypadCheckTransp,      // transparency
   TRUE,                      // distance
   TRUE,                      // needmouse
};
