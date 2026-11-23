/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/uiedit.c,v 1.33 2000/02/19 13:13:40 toml Exp $
#include <lg.h>
#include <stdlib.h>
#include <string.h>
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
#include <vmwincfg.h>
#include <scrnman.h>
#include <uiapp.h>
#include <hotkey.h>
#include <brushgfh.h>
#include <txtrpal.h>
#include <editorplatform.h>

#include <mprintf.h>
#include <imgui_workspace.h>

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
   // RES_EditorLayout1280x1024 is only an alias for the 1024x768 resource.
   // Advertising it as a distinct layout scales its 1024-wide coordinates
   // by 1920/1280, leaving the editor canvas capped at 1536 pixels.
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
   memcpy(r, RefGet(MKREF(lay->id,idx)), sizeof(*r));
   // scale
   r->ul.x = r->ul.x * rootdims.x / lay->dims.x;
   r->lr.x = r->lr.x * rootdims.x / lay->dims.x;
   r->ul.y = r->ul.y * rootdims.y / lay->dims.y;
   r->lr.y = r->lr.y * rootdims.y / lay->dims.y;

   return r;
}

////////////////////////////////////////////////////////////


static sEditorWorkspaceLayout workspace;
static sEditorWorkspaceLayout workspace_default;
static float workspace_width_ratio = 1.0f;
static float workspace_height_ratio = 1.0f;
static BOOL workspace_active;

const sEditorWorkspaceLayout *EditorWorkspaceLayout(void)
{
   return workspace_active ? &workspace : NULL;
}

// Scale brush controls and the alignment panel together within the lower-left strip.
static Point map_brush_panel_point(Point point)
{
   const Rect *align = GFHGetAlignLayoutBounds();
   const Rect *source = &workspace_default.tools;
   const Rect *target = &workspace.tools;
   const int source_width = (align ? align->lr.x : source->lr.x) - source->ul.x;
   return MakePoint(target->ul.x + (point.x - source->ul.x) * RectWidth(target) / source_width,
                    target->ul.y + (point.y - source->ul.y) * RectHeight(target) / RectHeight(source));
}

Rect EditorMapBrushPanelRect(Rect rect)
{
   Rect mapped = {map_brush_panel_point(rect.ul), map_brush_panel_point(rect.lr)};
   return mapped;
}

static void set_workspace_split(Point split)
{
   const int left = workspace_default.tools.ul.x;
   const int right = workspace_default.console.lr.x;
   const int top = workspace_default.views.ul.y;
   const int bottom = workspace_default.console.lr.y;
   const int default_width = workspace_default.console.ul.x - left;
   const int default_height = bottom - workspace_default.console.ul.y;
   const int min_width = default_width * 3 / 5;
   const int min_height = default_height * 3 / 4;
   const int max_width = max(default_width, right - left - 150);
   const int max_height = max(default_height, bottom - top - 150);
   const int width = max(min_width, min(split.x - left, max_width));
   const int height = max(min_height, min(bottom - split.y, max_height));

   workspace.console.ul.x = left + width;
   workspace.console.ul.y = bottom - height;
   workspace.views.lr.y = workspace.console.ul.y - VM_VIEW_GAP_Y;
   workspace.tools.ul.y = workspace.console.ul.y;
   workspace.tools.lr.x = workspace.console.ul.x;
   workspace.tools.lr.y = workspace.console.lr.y;
}

void EditorResizeWorkspace(Point split)
{
   const int old_x = workspace.console.ul.x;
   const int old_y = workspace.console.ul.y;
   if (!workspace_active)
      return;
   set_workspace_split(split);
   if (workspace.console.ul.x != old_x)
      workspace_width_ratio = (float)(workspace.console.ul.x - workspace_default.tools.ul.x) /
         (workspace_default.console.ul.x - workspace_default.tools.ul.x);
   if (workspace.console.ul.y != old_y)
   {
      workspace_height_ratio = (float)RectHeight(&workspace.console) / RectHeight(&workspace_default.console);
      vmWinCfgResize(&workspace.views);
   }
}

void EditorCreateGUI(void)
{
   Region* root = GetRootRegion();
   Layout* lay = screen_layout();

   GUIErase(root->r);

   uieditStyleSetup();
   SetCurrentStyle(&editStyle);

   workspace_default.views = *get_layout_rect(lay,REFINDEX(REF_RECT_layViewMan));
   workspace_default.tools = *get_layout_rect(lay,REFINDEX(REF_RECT_layGFH));
   workspace_default.console = *get_layout_rect(lay,REFINDEX(REF_RECT_layCommand));
   workspace_default.status = *get_layout_rect(lay,REFINDEX(REF_RECT_layStatus));
   workspace = workspace_default;
   set_workspace_split(MakePoint(
      workspace_default.tools.ul.x + (int)((workspace_default.console.ul.x - workspace_default.tools.ul.x) *
         workspace_width_ratio + 0.5f),
      workspace_default.console.lr.y - (int)(RectHeight(&workspace_default.console) * workspace_height_ratio + 0.5f)));
   vmCreateGUI(root, &workspace.views);
   workspace_active = TRUE;
   InitializeEditorWorkspace();
   CreateBrushGFH(&workspace_default.tools);

   EditorPlatformInitializeControls();
}

void EditorDestroyGUI(void)
{
   workspace_active = FALSE;
   EditorPlatformDestroyAuxiliaryWindows();

   if (TexturePaletteVisible())
      DestroyTexturePalette();
   ShutdownEditorWorkspace();
   DestroyBrushGFH();
   vmDestroyGUI();
   uieditStyleCleanup();
}

////////////////////////////////////////

guiStyle editStyle;     // this is the current usable style

const guiStyle masterEditStyle =
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
   GUIErase(GetRootRegion()->r);
}

void redraw_all_cmd(void)
{
   uieditRedrawAll();
   vm_redraw();
}
