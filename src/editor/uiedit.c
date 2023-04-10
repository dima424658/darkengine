/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/uiedit.c,v 1.33 2000/02/19 13:13:40 toml Exp $
#include <lg.h>
#include <stdlib.h>
#include <comtools.h>
#include <res.h>
#include <gadget.h>
#include <guistyle.h>
#include <gcompose.h>
#include <2d.h>
#include <config.h>

#include <uiedit.h>
#include <viewmgr.h>
#include <vumanui.h>
#include <scrnman.h>
#include <uiapp.h>
#include <hotkey.h>
#include <brushgfh.h>
#include <cmdterm.h>
#include <txtrpal.h>
#include <status.h>

#include <mprintf.h>

// for windows common controls
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>

#include <comtools.h>
#include <wappapi.h>
#include <appagg.h>
// END for windows common controls

// res files
#include <editor.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

extern guiStyle editStyle;   // at the bottom

//------------------------------------------------------------
// Screen layout stuff 
//

// table mapping modes to layouts

typedef struct Layout Layout;

static struct Layout
{
   Point dims;
   Id id;
}
Layouts[] =
{
   { { 640, 480 }, RES_EditorLayout640x480},
   { {1024, 768 }, RES_EditorLayout1024x768},
   { { 800, 600 }, RES_EditorLayout800x600},
   { {1280,1024 }, RES_EditorLayout1280x1024}, 
};


#define NUM_LAYOUTS (sizeof(Layouts)/sizeof(Layouts[0]))

//
// Find the layout for our screenmode
//

static Layout* screen_layout(void)
{
   Region* root = GetRootRegion();
   Point rootdims = MakePoint(RectWidth(root->r),RectHeight(root->r));
   int i;
   ulong besterror = 0xFFFFFFFF;
   int besti = 0;

   for (i = 0; i < NUM_LAYOUTS; i++)
   {
      Layout* l = &Layouts[i];
      ulong error = abs(rootdims.x - l->dims.x) * abs(rootdims.y - l->dims.y);
      if (error < besterror)
      {
         besterror = error;
         besti = i;
      } 
   }
   return &Layouts[besti];
}

static Rect* get_layout_rect(Layout* lay, int idx)
{
   static Rect outrect;

   Region* root = GetRootRegion();
   Point rootdims = MakePoint(RectWidth(root->r),RectHeight(root->r));   
   Rect* r = &outrect;

   // load rect from resource
   *r = *(Rect*)RefGet(MKREF(lay->id,idx));
   // scale
   r->ul.x = r->ul.x * rootdims.x / lay->dims.x;
   r->lr.x = r->lr.x * rootdims.x / lay->dims.x;
   r->ul.y = r->ul.y * rootdims.y / lay->dims.y;
   r->lr.y = r->lr.y * rootdims.y / lay->dims.y;   

   return r;
}

////////////////////////////////////////////////////////////


void EditorCreateGUI(void)
{
   extern void StatusSetRect(Rect *);

   Region* root = GetRootRegion();
   Layout* lay = screen_layout();

   GUIErase(root->r);

   uieditStyleSetup();
   SetCurrentStyle(&editStyle);

   vmCreateGUI(root,get_layout_rect(lay,REFINDEX(REF_RECT_layViewMan)));
   StatusSetRect(get_layout_rect(lay,REFINDEX(REF_RECT_layStatus)));
   CreateBrushGFH(get_layout_rect(lay,REFINDEX(REF_RECT_layGFH)));
   CreateCommandTerminal(LGadCurrentRoot(), get_layout_rect(lay,REFINDEX(REF_RECT_layCommand)),kCmdTermNoFlags);


   // make sure the windows common controls are loaded
   InitCommonControls();
}

void EditorDestroyGUI(void)
{
   IWinApp* pWA = AppGetObj(IWinApp);
   HWND hWnd, hPrevWnd, hMainWnd = IWinApp_GetMainWnd(pWA);
   SafeRelease(pWA);

   // close all child and owned windows - let's go both ways (next and prev) to make sure
   // we get them all
   hWnd = hPrevWnd = hMainWnd;
   while ((hWnd = GetNextWindow(hWnd, GW_HWNDPREV)) != NULL)
   {
      if (GetWindow(hWnd, GW_OWNER) == hMainWnd)
      {
         DestroyWindow(hWnd);
         hWnd = hPrevWnd;
      }
      else
         hPrevWnd = hWnd;
   }

   hWnd = hPrevWnd = hMainWnd;
   while ((hWnd = GetNextWindow(hWnd, GW_HWNDNEXT)) != NULL)
   {
      if (GetWindow(hWnd, GW_OWNER) == hMainWnd)
      {
         DestroyWindow(hWnd);
         hWnd = hPrevWnd;
      }
      else
         hPrevWnd = hWnd;
   }

   if (TexturePaletteVisible())
      DestroyTexturePalette();
   DestroyCommandTerminal();
   DestroyBrushGFH();
   vmDestroyGUI();
   uieditStyleCleanup(); 
}

////////////////////////////////////////

guiStyle editStyle;     // this is the current usable style

guiStyle masterEditStyle = 
{
   0, // palette
   {  // colors
      uiRGB(255,255,255), // fg
      uiRGB( 10, 10, 10), // bg
      uiRGB( 40,200,200), // text
      uiRGB(255,  0,255), // hilite
      uiRGB(255,255,255), // bright
      uiRGB( 96, 96, 96), // dim
      uiRGB(255,255,255), // fg2
      uiRGB( 64, 64, 64), // bg2
      uiRGB( 40,200,200), // border
      uiRGB(255,255,255), // white
      uiRGB(  5,  5,  5), // black
      1, // xor
      1, // light bevel
      0, // dark bevel
   },

}; 

   
void uieditStyleSetup(void)
{
   guiStyle style = masterEditStyle;
   uieditStyleCleanup(); 

   uiGameLoadStyle("edit_",&style,NULL); 
   guiCompileStyleColors(&editStyle,&style); 
   SetCurrentStyle(&editStyle); 
}

void uieditStyleCleanup()
{
   uiGameUnloadStyle(&editStyle); 
}

void uieditRedrawAll(void)
{
   LGadDrawBox(VB(LGadCurrentRoot()),NULL);
}

void redraw_all_cmd(void)
{
   uieditRedrawAll();
   vm_redraw();
   StatusDrawStringAll();
}  

