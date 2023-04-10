/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/vumanui.c,v 1.24 2000/02/19 13:13:47 toml Exp $

#include <string.h>

#include <2d.h>
#include <reg.h>
#include <mprintf.h>
#include <gadget.h>
#include <drawelem.h>
#include <gcompose.h>
#include <config.h>
#include <cfgdbg.h>

#include <command.h>
#include <vumanui.h>
#include <viewmgr_.h>
#include <viewmgr.h>
#include <vmwincfg.h>
#include <editor.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


Region vmParent;
Region vmRegions[MAX_CAMERAS_REAL];

typedef struct vmMouseData
{
   int viewnum;
} vmMouseData;

static vmMouseData ViewRegionData[MAX_CAMERAS_REAL];
static LGadRoot* view_roots[MAX_CAMERAS_REAL];

//////////////////////////////////////////
// VIEW MOUSE HANDLER
//

  // x&y are relative pixel coordinates for this camera
bool brush_edit_mouse(uiMouseEvent *, int camera, int x, int y);

void vm_popup_menu(uiMouseEvent *, int c);
bool vm_mouse_handler(uiEvent* _ev, Region* reg, void* _data)
{
   static int lastview = -1;
   extern bool vm_xmouse;

   uiMouseEvent* ev = (uiMouseEvent*)_ev;
   vmMouseData* data = (vmMouseData*)_data;
   int view = data->viewnum;

   if (!VIEW_AVAIL(view))
      return FALSE;

   if (lastview != view && vm_xmouse)
      vm_set_cur_region(view);
   lastview = view;
   
   // should we perhaps discard non-down events to save time???
   switch (vmWinCfgGetMode())
   {
   case vmWINCFG_MODE_NORMAL:
      if (!brush_edit_mouse(ev, region_to_camera_mapping[view],
                            ev->pos.x - reg->abs_x, ev->pos.y - reg->abs_y))
         vm_popup_menu(ev, region_to_camera_mapping[view]);
      break;
   case vmWINCFG_MODE_SCROLL:
      if (ev->action&MOUSE_LDOWN)
      {
         vmScrollCamera(region_to_camera_mapping[view]);
         vmWinCfgSetMode(vmWINCFG_MODE_NORMAL);
      }
      break;
   }
   return TRUE;
}


DrawElement titledraw = { DRAWTYPE_TEXT, "My Title", 0, 6, 1 };

static LGadMenu vmpop;
static int popup_cam;
static char **popup_commands;
extern bool vm_display_enable;

// is this a fuckup, or what - global state for menus in vm
// this means a menu is in progress, if you need to know
BOOL vm_menu_inprog=FALSE;
// set this if you want to leave "cur camera" focused
BOOL vm_menu_camera_lock=FALSE;
// will have mouse coordinates
int vm_menu_click_x, vm_menu_click_y;

static bool vm_popup_select(int res, LGadMenu *men)
{
   if (res >= 0) {
      int oc = vm_current_camera();
      vm_set_cur_camera(popup_cam);  // hmm, why isnt this happening
      vm_menu_inprog=TRUE; vm_menu_camera_lock=FALSE;
      CommandExecute(popup_commands[res]);
      vm_menu_inprog=FALSE;
      if (!vm_menu_camera_lock)
         vm_set_cur_camera(oc);
   }
   vm_display_enable = TRUE;
   return TRUE;
}

void vm_popup_menu(uiMouseEvent *ev, int c)
{
   extern void vm_get_popup_menu(int c, char ***, char ***, int *, int, int);
   DrawElement *elems;
   char **names, **commands;
   Region *reg=vmGetRegion(camera_to_region_mapping[c]);  // this is stupid
   int x,y, count, i;

   popup_cam = c;
   vm_menu_click_x=ev->pos.x-reg->abs_x;
   vm_menu_click_y=ev->pos.y-reg->abs_y;

   x = ev->pos.x - 20;
   y = ev->pos.y - 20;

   if (x + 40 > grd_bm.w) x = grd_bm.w - 60;
   if (y + 120 > grd_bm.h) y = grd_bm.h - 120;

   if (x < 0) x = 0;
   if (y < 0) y = 0;

   vm_display_enable = FALSE;

   vm_get_popup_menu(c, &names, &commands, &count, ev->pos.x, ev->pos.y);

   popup_commands = commands;

   elems = Malloc(sizeof(DrawElement) * count);

   for (i=0; i < count; ++i) 
   {
      ElementClear(&elems[i]);
      elems[i].draw_type = DRAWTYPE_TEXT;
      elems[i].draw_data = names[i];
   }

   LGadCreateMenuArgs(&vmpop, LGadCurrentRoot(), (short)x, (short)y, -1, -1,
       (short)count, (short)count, elems, vm_popup_select, 0,
       MENU_GRAB_FOCUS | MENU_OUTER_DISMISS | MENU_ALLOC_ELEMS,
       BORDER(DRAWFLAG_BORDER_OUTLINE), &titledraw, 0);
   Free(elems);
}

void vm_mouse_relativize(int c, int *x, int *y)
{
   Region *reg = &vmRegions[camera_to_region_mapping[c]];
   *x -= reg->abs_x;
   *y -= reg->abs_y;
}

//
// Create/Control the GUI
//

void vmCreateGUI(Region* parent, Rect* bounds)
{
   int i;
   make_region(parent,&vmParent,bounds,0,NULL);
   parent = &vmParent;
   vmWinCfgCreate(parent,bounds);
   for (i = 0; i < MAX_CAMERAS_REAL; i++)
   {
      Region* reg = &vmRegions[i];
      Point base, size;
      Rect area;  // area for new region
      int cookie;

      vmGetRegionInfo(i,&base,&size);
      area.ul=base;
      area.lr.x=area.ul.x + size.x;
      area.lr.y=area.ul.y + size.y;
      
      // actually make the region
      make_region(parent,reg,&area,0,NULL);

      ViewRegionData[i].viewnum = i;   // fill in data      
      view_roots[i] = NULL;            // initialize roots      

      // install mouse handler
      uiInstallRegionHandler(reg, UI_EVENT_MOUSE|UI_EVENT_MOUSE_MOVE, vm_mouse_handler, &ViewRegionData[i], &cookie);
   }
}

void vmDestroyGUI(void)
{
   int i;
   vmReleaseAllViews();
   for (i = 0; i < MAX_CAMERAS_REAL; i++)
      region_destroy(&vmRegions[i],FALSE);
   vmWinCfgDestroy();   
}

////////////////////////////////////////////////////////////
// Set/unset region canvas

static int vm_viewreg;
static grs_canvas vm_canv; 
static GUIcompose vm_compose;   

void vm_set_region_canvas(int view)
{
   Region* reg = &vmRegions[view]; 
   Rect r;

   vm_viewreg = view; 
   region_abs_rect(reg,reg->r,&r);
   uiHideMouse(&r); 
   GUIsetup(&vm_compose,&r,ComposeFlagRead,GUI_CANV_ANY); 
}

void vm_unset_region_canvas(void)
{

   GUIdone(&vm_compose);
   uiShowMouse(&vm_compose.area); 

}

////////////////////////////////////////////////////////////
// View grabbage
//

// an array of masks of valid view combos by parm, in default priority order
// @MEMORY: second dim doesn't need to be quite this big

// really needs a 3x1 as well

#define VIEW(x) (1 << (x))
static ubyte view_layouts[vmNumParms][8] = 
{
   // vmGrabSingle
   { 
      VIEW(3),
      VIEW(2), 
      VIEW(1), 
      VIEW(0), 
      0,
   },
   // vmGrabTwoWide
   {
      VIEW(2)|VIEW(3),
      VIEW(0)|VIEW(1),
      0,
   },
   // vmGrabTwoHigh
   {
      VIEW(1)|VIEW(3),
      VIEW(0)|VIEW(2),
      0,
   },
};


ubyte find_view_mask(vmGrabParm parm)
{
   int i;
   ubyte* layouts = &view_layouts[parm][0];
   for (i = 0; layouts[i] != 0; i++)
   {
      ubyte mask = layouts[i];
      bool accept = TRUE;
      int j;
      // check the mask for availible views
      for (j = 0; j < MAX_CAMERAS_REAL; j++)
         if (mask & (1 << j))
            if (view_roots[j] != NULL)
            {
               accept = FALSE;
               break;
            }
      if (accept) return mask;
   }
   return 0;
}

#define MIN_COORD (1 << 16)
#define MAX_COORD (MIN_COORD - 1)

static void build_mask_rect(ulong mask, Rect* r)
{
   bool first = TRUE;
   int i;

   for (i = 0; i < MAX_CAMERAS_REAL; i++)
      if (mask & (1 << i))
      {     
         Region* reg = &vmRegions[i];
         Rect regr;

         region_abs_rect(reg,reg->r,&regr);
         
         if (first) 
         {
            *r = regr;
            first = FALSE;
         }
         else
         {
            RECT_UNION(&regr,r,r);
         }
      }
}

LGadRoot* vmGrabViews(vmGrabParm parm)
{
   ubyte layout = find_view_mask(parm);
   Rect r;
   LGadRoot* root;
   int i;
   
   if (layout == 0)
   {
      Warning(("Failed to grab views. parm = %d\n",parm));
      return NULL;
   }
   
   build_mask_rect(layout,&r);
   root = LGadSetupSubRoot(NULL, LGadCurrentRoot(),r.ul.x,r.ul.y,
        (short)RectWidth(&r),(short)RectHeight(&r));
   GUIErase(&r);

   for (i = 0; i < MAX_CAMERAS_REAL; i++)
      if (layout & (1 << i))
      {
         view_roots[i] = root;
         vm_disable_region(i);
      }
   return root;
}

////////////////////////////////////////

int vmReleaseViews(LGadRoot* release)
{
   int i;
   int count = 0;
   for (i = 0; i < MAX_CAMERAS_REAL; i++)
   {
      if (view_roots[i] == release)
      {
         view_roots[i] = NULL;
         vm_enable_region(i);
         count++;
      }
   }
   if (count > 0)
   {  
      LGadDestroyRoot(release);  
      vm_redraw();
   }
   else
   {
      Warning(("Tried to release a root with no associated views\n"));
   }
   return count;
}

int vmReleaseAllViews(void)
{
   int i;
   int count = 0;

   for(i = 0; i < MAX_CAMERAS_REAL; i++)
      if (view_roots[i] != NULL)
         count += vmReleaseViews(view_roots[i]);
   return count;
}

BOOL vmAllViewsAvailable(void)
{
   int i;
   for(i = 0; i < MAX_CAMERAS_REAL; i++)
      if (view_roots[i] != NULL)
         return FALSE;
   return TRUE;
}
