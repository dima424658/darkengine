/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/vmwincfg.c,v 1.16 2000/02/19 13:13:44 toml Exp $
//
// view manager window configuation controller 
//  has mouse handlers, region size/shape computation, drag resize, 
//  and solo/nonsolo window sizing, as well as maybe multi layouts

// this system manages a single rectangular slab of screen, which it 
//   partitions as it sees fit into subwindows for display of 3d 
//   information in.  it supports a mouse UI for sliding around the 
//   "center" of this region, from which it derives the size/shape of 
//   the other windows.  it also supports "soloing" the current view 
//   to be full screen.  
// it also may someday support multiple "layouts".  for now, the only
//   supported layout is a 2x2, with the center partitioning it out.
//   the next obvious layout is a 1 next to 3, or 1 above 3, with one
//   big window and 3 little ones next to or below it.

// WARNING: the rest of this code base (vumanui in particular) presupposes
//   (in its own way) a virtually unlimited number of windows, and makes 
//   reference to this in the MAX_REAL_CAMERAS and, in particular, the 
//   ViewGrid thing.  this WinCfg UI _KNOWS THERE ARE 4_ and knows in
//   particular that row and col will be small.  if we really have some
//   cool mode which is not 4, we can add it later, i guess, but it will
//   probably be messy.

#include <2d.h>
#include <reg.h>
#include <mprintf.h>
#include <gadget.h>
#include <gcompose.h>
#include <kbcook.h>
#include <config.h>
#include <platform_services.h>
#include <status.h>
#include <scrnman.h>
#include <matrix.h>

#include <modalui.h>
#include <vumanui.h>
#include <vmwincfg.h>
#include <viewmgr.h>
#include <viewmgr_.h>
#include <editor.h>
#include <editorplatform.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// exported data
BOOL  vmSoloed=FALSE;     // are we currently soloed on a particular view
int   vmWinCfgMode=vmWINCFG_MODE_NORMAL;

// static layout control - shape and form fun
static BOOL  vmLayout2by2=TRUE;  // are we 2x2 or 3x1 (ignoring being Soloed, o'course)
static BOOL  vmLayoutVerticalSplit=TRUE; // if 3x1, which axis split
static Point vmGUIDims;          // size of the current GUI
static Point vmGUITop;           // absolute UpperLeft corner of GUI
static Point vmGUICenter={0,0};  // sliding center point of GUI - 0,0 means uninitialized
static Rect  vmGUIRect;
static float vmSplitX, vmSplitY;

// locals
static int par_handler_cookie;
static Region *vmWinParent=NULL;

// definitions/constants
#define VIEW_MARGIN_X 3  // xsize pixel border, per side

// THE BIG THING - determines where the windows are in real life...
// returns whether the rect is currently visible? fills in params
BOOL vmGetRegionInfo(int i, Point *origin, Point *size)
{
   BOOL rv=TRUE;

   *origin=vmGUITop;
   if (vmSoloed)
   {
      if (vm_get_cur_region()==i)
         *size=vmGUIDims;   // soloed and current region, so full screen
      else
      {
         *origin=*size=MakePoint(0,0);
         rv=FALSE;
      }
   }
   else if (vmLayout2by2)
   {
      int col = i % 2;  // was ViewGrid craziness, but lets face it, this stuff is 2x2 hardcoded
      int row = (i - (col)) / 2;

      if (col)
      {
         origin->x+=vmGUICenter.x + VIEW_MARGIN_X;
         size->x=vmGUIDims.x - vmGUICenter.x - VIEW_MARGIN_X - 1;
      }
      else
         size->x=vmGUICenter.x;

      if (row)
      {
         origin->y+=vmGUICenter.y + VM_VIEW_GAP_Y;
         size->y=vmGUIDims.y - vmGUICenter.y - VM_VIEW_GAP_Y;
      }
      else
         size->y=vmGUICenter.y;
   }
   else // thus, by implication, we are an unsoloed 3x1 on some axis or other
   {
      if (i==0) // base view
      {
         *size=vmGUIDims;
         if (vmLayoutVerticalSplit)
            size->x=vmGUICenter.x - VIEW_MARGIN_X;
         else
            size->y=vmGUICenter.y;
      }
      else  // equispaced other 3
      {
         if (vmLayoutVerticalSplit)
         {
            size->x=vmGUIDims.x - vmGUICenter.x - VIEW_MARGIN_X;
            const int height = vmGUIDims.y - 2 * VM_VIEW_GAP_Y;
            const int top = (i - 1) * height / 3;
            size->y = i * height / 3 - top;
            origin->x+=vmGUICenter.x + VIEW_MARGIN_X;
            origin->y += top + (i - 1) * VM_VIEW_GAP_Y;
         }
         else
         {
            size->x=vmGUIDims.x/3;
            size->y=vmGUIDims.y - vmGUICenter.y - VM_VIEW_GAP_Y;
            if (i!=1)  // keep first 1 flush
               origin->x+=(i-1)*size->x + VIEW_MARGIN_X;
            origin->y+=vmGUICenter.y + VM_VIEW_GAP_Y;
            if (i!=3)  // keep last 1 flush
               size->x-=VIEW_MARGIN_X;
         }
      }
   }
   return rv;
}

BOOL vmWinCfgGetBounds(Rect *bounds, Point *split)
{
   if (!vmWinParent || vmGUIDims.x <= 0 || vmGUIDims.y <= 0)
      return FALSE;
   *bounds = vmGUIRect;
   *split = vmGUICenter;
   return TRUE;
}

BOOL vmWinCfgUsesImGui(void)
{
   return !PlatformBatchMode();
}

static void cleanup_region(int which)
{
   Rect r;
   region_abs_rect(&vmRegions[which],vmRegions[which].r,&r);
   GUIErase(&r);
}

// actually toggle from soloed to nonsoloed
void vmWindowsRefresh(BOOL resize);

void vmToggleGUIFull(void)
{
   int i=vm_get_cur_region(), j;
   if (!vmAllViewsAvailable()) return;
   if (vmSoloed) cleanup_region(i);   // we were soloed
   else if (vm_menu_inprog) vm_menu_camera_lock=TRUE;  // we arent, so keep new window
   vmSoloed=!vmSoloed;
   for (j=0; j<MAX_CAMERAS_REAL; j++)
      if (i!=j)
      {
         region_set_invisible(&vmRegions[j],vmSoloed);
         if (vmSoloed)
         {
            cleanup_region(j);
            vm_disable_region(j);
         }
         else
            vm_enable_region(j);
      }
   vmWindowsRefresh(TRUE);
}

// correctly redraw all windows, if "resize" is set, first regenerate and size them
void vmWindowsRefresh(BOOL resize)
{
   //   GUIErase(&vmGUIRect);
   gr_set_fcolor(0);   // i just cant get this to work... argh!!!
   gr_rect(vmGUIRect.ul.x,vmGUIRect.ul.y,vmGUIRect.lr.x,vmGUIRect.lr.y+1);
   if (resize)
   {
      Point loc, size;
      Region *reg;
      int i;
      for (i=0; i<MAX_CAMERAS_REAL; i++)
         if (vmGetRegionInfo(i,&loc,&size))
         {
            reg=&vmRegions[i];
            region_resize(reg,size.x,size.y);
            region_move(reg,loc.x-vmGUITop.x,loc.y-vmGUITop.y,reg->z);
         }
   }
   vm_redraw();   // do we have to expose or something here?
}

void vmWinCfgSetSplit(Point split)
{
   const int minWidth = min(80,vmGUIDims.x/2);
   const int minHeight = min(80,vmGUIDims.y/2);
   const Point center = MakePoint(max(minWidth,min(split.x,vmGUIDims.x-minWidth)),
                                  max(minHeight,min(split.y,vmGUIDims.y-minHeight)));
   if (!vmWinParent || (center.x == vmGUICenter.x && center.y == vmGUICenter.y))
      return;

   vmGUICenter = center;
   vmSplitX = (float)center.x / vmGUIDims.x;
   vmSplitY = (float)center.y / vmGUIDims.y;
   vmWindowsRefresh(TRUE);
}

// set a new bounding rectangle
// why cant we set an origin too? who knows, have to fix it, clearly
void vmNewBoundingRect(Rect *bounds)
{
   vmGUIDims   = MakePoint(RectWidth(bounds)-1,RectHeight(bounds));
   vmGUIRect   = *bounds;
   vmGUITop    = bounds->ul;

   vmGUICenter = MakePoint((int)(vmSplitX * vmGUIDims.x + 0.5f), (int)(vmSplitY * vmGUIDims.y + 0.5f));
}

// utilities to find out about scale/size of current window layout
void vmGetReferenceSize(Point *size)
{
   *size = MakePoint(vmGUIDims.x/2,vmGUIDims.y/2);
}

// returns X size ratio by default
float vmGetSizeRatio(int r, float *xrat, float *yrat)
{
   Point orig, our_size, ref_size;
   float xr;

   vmGetRegionInfo(r,&orig,&our_size);
   vmGetReferenceSize(&ref_size);
   xr=(float)our_size.x/(float)ref_size.x;
   if (xrat) *xrat=xr;
   if (yrat) *yrat=(float)our_size.y/(float)ref_size.y;
   return xr;
}

///////////////////
// window layout control UI

#define ismouse(ev) ((ev).type & (UI_EVENT_MOUSE | UI_EVENT_MOUSE_MOVE))
#define iskbd(ev) ((ev).type == UI_EVENT_KBD_COOKED)

#define DEAD_SPOT_SIZE (8)
static BOOL _doWinScroll(int c)
{
   mxs_vector start_loc=*vm_get_location(c);  // this is where we started
   mxs_vector cur_loc=start_loc;             // which, for now, is where we are
   mxs_vector x_sc, y_sc;                     // pixel to world ratio on each axis
   Point loc, size;
   short x_cen, y_cen;                        // absolute screen coord of center
   uiEvent ev;

   vmGetRegionInfo(camera_to_region_mapping[c],&loc,&size);
   vm_screen_axes(c,&x_sc,&y_sc);
   x_cen=(short)(loc.x+(size.x/2));
   y_cen=(short)(loc.y+(size.y/2));
   vm_render_camera(c);
   if (!modal_ui_start(MODAL_CHECK_LEFT)) return FALSE;
   mouse_put_xy(x_cen,y_cen);
   do {
      modal_ui_update();
      while (modal_ui_get_event(&ev))
      {
         if (ismouse(ev))
         {
            uiMouseEvent *mev = (uiMouseEvent *)&ev;
            int x_off=mev->pos.x-x_cen, y_off=mev->pos.y-y_cen;
            BOOL chg=FALSE;
            if (mev->action & MOUSE_LUP)
               goto mouse_up;
            if (abs(x_off)>DEAD_SPOT_SIZE)
            {
               mx_scale_addeq_vec(&cur_loc,&x_sc,((float)x_off*2.0f/(float)DEAD_SPOT_SIZE));
               chg=TRUE;
            }
            if (abs(y_off)>DEAD_SPOT_SIZE)
            {
               mx_scale_addeq_vec(&cur_loc,&y_sc,((float)y_off*2.0f/(float)DEAD_SPOT_SIZE));
               chg=TRUE;
            }
            if (chg)  // need to redraw somehow
            {
               extern BOOL zip;
               vm_set_location(c,&cur_loc);
               vm_render_camera(c);
            }
         }
         else if (iskbd(ev))
         {
            uiCookedKeyEvent *kev = (uiCookedKeyEvent *) &ev;
            if (kev->code == (27 | KB_FLAG_DOWN))
               goto esc;
         }
      }
   } while (1);

esc:
   cur_loc=start_loc;
   vm_set_location(c,&cur_loc);
   vm_render_camera(c);
mouse_up:
   modal_ui_end();
   return (start_loc.x!=cur_loc.x)||(start_loc.y!=cur_loc.y)||(start_loc.z!=cur_loc.z);
}

// this is really ugly, eh?
BOOL vmScrollCamera(int c)
{
   int oc=vm_current_camera();
   int omode=vm_get_mode(c);
   bool oldgrid=vm_show_grid;
   BOOL rv=FALSE;
   
   if (vm_get_3d(c))
    { Status("Cant scroll 3d cameras"); return rv; }
   vm_set_cur_camera(c);
   vm_set_render_mode(c,RM_WIREFRAME_CURRENT);
   vm_show_grid=TRUE;     // for now, show current brush, grid, and camera only
   vm_set_synch(c,FALSE); // desynch always - perhaps should save state for abort case?
   rv=_doWinScroll(c);
   vm_show_grid=oldgrid;
   vm_set_render_mode(c,omode);
   vm_set_cur_camera(oc);
   vm_redraw();
   return rv;
}

// mouse handler for clicks which make it onto the parent region - ie the cracks between the views
// sadly this is untrue, since the parent region seems to be the whole display... so we check rect bounds now
// return TRUE if i eat the event
static BOOL vm_parent_mouse_handler(uiEvent* _ev, Region* reg, void* _data)
{
   uiMouseEvent* ev = (uiMouseEvent*)_ev;
   BOOL changed_setup=FALSE;

   if (vmWinCfgMode==vmWINCFG_MODE_SCROLL)
   {  // this scroll handler is just here to catch scroll inits not on vm views
      if (ev->action&MOUSE_LDOWN)  // clicked outside - just punt
         vmWinCfgSetMode(vmWINCFG_MODE_NORMAL);
      return TRUE;
   }

   if ((ev->pos.x>vmGUIDims.x)||(ev->pos.y>vmGUIDims.y))
      return FALSE;

   // this is the "modal" selector up front
   switch (vmWinCfgMode)
   {
   case vmWINCFG_MODE_NORMAL:     // if we made it to here, we are doing a standard parent mouse handler window resizing thing
      if (vmSoloed || !vmAllViewsAvailable())
         return FALSE;
      
      if (ev->action&MOUSE_RDOWN)
      {  // for now, we cycle through the available layouts
         vmWinCfgCycleLayout(vmWINCFG_C_NOUPDATE);
         changed_setup=TRUE;
      }
      else
         return FALSE;
      vmWindowsRefresh(changed_setup);
      return TRUE;
   }
   return FALSE;
}

// takes input from vmWINCFG_ defines in header
void vmWinCfgCycleLayout(int new_layout)
{
   BOOL refresh=TRUE, noSolo=FALSE;;
   switch (new_layout)
   {
   case vmWINCFG_C_NOUPDATE:
      refresh=FALSE; // really just for usage by the callback - so fall through
   case vmWINCFG_CYCLE:
      noSolo=TRUE;
      if (vmLayout2by2)
         vmLayout2by2=!vmLayout2by2;
      else
      {
         vmLayoutVerticalSplit=!vmLayoutVerticalSplit;
         if (vmLayoutVerticalSplit)
            vmLayout2by2=!vmLayout2by2;
      }
      break;
   case vmWINCFG_2by2:       vmLayout2by2=TRUE;  vmLayoutVerticalSplit=TRUE;  noSolo=TRUE; break;
   case vmWINCFG_1by3_VERT:  vmLayout2by2=FALSE; vmLayoutVerticalSplit=TRUE;  noSolo=TRUE; break;
   case vmWINCFG_1by3_HORIZ: vmLayout2by2=FALSE; vmLayoutVerticalSplit=FALSE; noSolo=TRUE; break;
   case vmWINCFG_RECENTER:
      vmSplitX = vmSplitY = 0.5f;
      vmGUICenter = MakePoint(vmGUIDims.x/2,vmGUIDims.y/2);
      break;
   case vmWINCFG_SOLO:       vmToggleGUIFull(); return;
   }
   if (noSolo && vmSoloed)
      vmToggleGUIFull();
   if (refresh)
      vmWindowsRefresh(TRUE);
}

int vmWinCfgGetLayout(void)
{
   if (vmSoloed)                   return vmWINCFG_SOLO;
   else if (vmLayout2by2)          return vmWINCFG_2by2;
   else if (vmLayoutVerticalSplit) return vmWINCFG_1by3_VERT;
   else                            return vmWINCFG_1by3_HORIZ;
}

//////////////////
// setup and overall control

// this is the actual setup of the regions and globals
void vmWinCfgCreate(Region* parent, Rect* bounds)
{
   float cenX=0.5, cenY=0.5;
   
   vmGUITop    = bounds->ul;  // should do this in some better way
   vmGUIDims   = MakePoint(RectWidth(bounds)-1,RectHeight(bounds));
   vmGUIRect   = *bounds;

   config_get_float("vmCenterX",&cenX);
   config_get_float("vmCenterY",&cenY);
   vmSplitX = cenX;
   vmSplitY = cenY;
   vmGUICenter = MakePoint((int)(vmGUIDims.x * vmSplitX + 0.5f), (int)(vmGUIDims.y * vmSplitY + 0.5f));

#ifdef SEE_BORDERS_CLEARLY
   vmGUIDims.x -= 10; vmGUIDims.y -= 10;
   vmGUITop.x  += 5;  vmGUITop.y  += 5;
#endif

   uiInstallRegionHandler(parent, UI_EVENT_MOUSE|UI_EVENT_MOUSE_MOVE, vm_parent_mouse_handler, NULL, &par_handler_cookie);
   vmWinParent=parent;
}

// close down the shop
void vmWinCfgDestroy(void)
{
   uiRemoveRegionHandler(vmWinParent,par_handler_cookie);
   vmWinParent=NULL;
   config_set_float_from_var("vmCenterX",vmSplitX);
   config_set_float_from_var("vmCenterY",vmSplitY);
   config_set_priority("vmCenterX",CONFIG_DFT_LO_PRI);
   config_set_priority("vmCenterY",CONFIG_DFT_LO_PRI);   
}

// this is the beginnings of a "move the whole thing"
// currently just takes bounds, clearly not enough
void vmWinCfgResize(Rect *bounds)
{
   if (bounds->lr.y < vmGUIRect.lr.y)
   {
      Rect exposed = {MakePoint(vmGUIRect.ul.x,bounds->lr.y), vmGUIRect.lr};
      GUIErase(&exposed);
   }
   region_resize(vmWinParent,RectWidth(bounds),RectHeight(bounds));
   vmNewBoundingRect(bounds);
   vmWindowsRefresh(TRUE);
}

void vmWinCfgSetMode(int new_mode)
{
   if (vmWinParent==NULL) return;
   if (vmWinCfgMode==new_mode) return;
   if (vmWinCfgMode==vmWINCFG_MODE_NORMAL)
   {
      uiGrabFocus(vmWinParent,ALL_EVENTS); // gonna need to grab focus, leaving normal
      Status("WinCfg going modal");
   }
   if (new_mode==vmWINCFG_MODE_NORMAL)
   {
      uiReleaseFocus(vmWinParent,ALL_EVENTS); // relinquish focus, done with special mode
      Status("WinCfg eschews modality");
   }
   vmWinCfgMode=new_mode;
}

EXTERN int vmWinCfgGetMode(void)
{
   return vmWinCfgMode;
}
