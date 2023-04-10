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
#include <status.h>
#include <matrix.h>

#include <modalui.h>
#include <vumanui.h>
#include <vmwincfg.h>
#include <viewmgr.h>
#include <viewmgr_.h>
#include <editor.h>
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

// locals
static int par_handler_cookie;
static Region *vmWinParent=NULL;

// definitions/constants
#define VIEW_MARGIN_X 3  // xsize pixel border, per side
#define VIEW_MARGIN_Y 2  // ysize pixel border, per side

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
         origin->y+=vmGUICenter.y + VIEW_MARGIN_Y;
         size->y=vmGUIDims.y - vmGUICenter.y - VIEW_MARGIN_Y - 1;
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
            size->y=vmGUICenter.y - VIEW_MARGIN_Y;
      }
      else  // equispaced other 3
      {
         if (vmLayoutVerticalSplit)
         {
            size->x=vmGUIDims.x - vmGUICenter.x - VIEW_MARGIN_X;
            size->y=vmGUIDims.y/3;
            origin->x+=vmGUICenter.x + VIEW_MARGIN_X;
            if (i!=1)  // keep first 1 flush
               origin->y+=(i-1)*size->y + VIEW_MARGIN_Y;
            if (i!=3)  // keep last 1 flush
               size->y-=VIEW_MARGIN_Y;
         }
         else
         {
            size->x=vmGUIDims.x/3;
            size->y=vmGUIDims.y - vmGUICenter.y - VIEW_MARGIN_Y;
            if (i!=1)  // keep first 1 flush
               origin->x+=(i-1)*size->x + VIEW_MARGIN_X;
            origin->y+=vmGUICenter.y + VIEW_MARGIN_Y;
            if (i!=3)  // keep last 1 flush
               size->x-=VIEW_MARGIN_X;
         }
      }
   }
   return rv;
}

// draw outline skeletons of each window
void vmDrawWindowSkeletons(void)
{
   Point loc, size;
   int i;
   
   GUIErase(&vmGUIRect);
   gr_set_fcolor(guiStyleGetColor(NULL,StyleColorFG));
   for (i=0; i<MAX_CAMERAS_REAL; i++)
      if (vmGetRegionInfo(i,&loc,&size))  // draw the rectangle for our region info, eh?
         gd_box(loc.x,loc.y,loc.x+size.x-2,loc.y+size.y-2);
}

// do region math and then clear the area out
static void cleanup_region(int which)
{
   Rect r;
   region_abs_rect(&vmRegions[which],vmRegions[which].r,&r);
   GUIErase(&r);
}

// actually toggle from soloed to nonsoloed
void vmToggleGUIFull(void)
{
   int i=vm_get_cur_region(), j;
   Region *reg=&vmRegions[i];
   Point loc, size;

   if (!vmAllViewsAvailable()) return;
   if (vmSoloed) cleanup_region(i);   // we were soloed
   else if (vm_menu_inprog) vm_menu_camera_lock=TRUE;  // we arent, so keep new window
   vmSoloed=!vmSoloed;
   if (!vmGetRegionInfo(i,&loc,&size))
   {
      Warning(("Hey! GetRegionInfo of bogus region %d in ToggleFull\n",i));
      return;
   }
   region_resize(reg,size.x,size.y);
   region_move(reg,loc.x,loc.y,reg->z);
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
   vm_redraw();
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
            region_move(reg,loc.x,loc.y,reg->z);
         }
   }
   vm_redraw();   // do we have to expose or something here?
}

// set a new bounding rectangle
// why cant we set an origin too? who knows, have to fix it, clearly
void vmNewBoundingRect(Rect *bounds)
{
   float scale_x=(float)RectWidth(bounds)/(float)RectWidth(&vmGUIRect);
   float scale_y=(float)RectHeight(bounds)/(float)RectHeight(&vmGUIRect);

   vmGUIDims   = MakePoint(RectWidth(bounds),RectHeight(bounds));
   vmGUIRect   = *bounds;

   vmGUICenter.x = (short) (scale_x * vmGUICenter.x);
   vmGUICenter.y = (short) (scale_y * vmGUICenter.y);
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

#define AXIS_MASK_X        0x01
#define AXIS_MASK_Y        0x02

// maybe someday?
#define AXIS_MASK_LEFT_EDGE    0x04
#define AXIS_MASK_RIGHT_EDGE   0x08
#define AXIS_MASK_TOP_EDGE     0x10
#define AXIS_MASK_BOTTOM_EDGE  0x20

#define ismouse(ev) ((ev).type & (UI_EVENT_MOUSE | UI_EVENT_MOUSE_MOVE))
#define iskbd(ev)   ((ev).type == UI_EVENT_KBD_COOKED)

// modal drag window layout commands

#define DRAG_BORDER 10

// should we perhaps put the mouse back in the right place at some point?
// returns whether we moved anything or not
static BOOL _doWinDrag(int axis_mask)
{
   Point init_center=vmGUICenter;
   uiEvent ev;

   if (!modal_ui_start(MODAL_CHECK_LEFT)) return FALSE;
   uiHideMouse(NULL);
   do {
      modal_ui_update();
      while (modal_ui_get_event(&ev))
      {
         if (ismouse(ev))
         {
            BOOL slam_mouse=FALSE;
            uiMouseEvent *mev = (uiMouseEvent *)&ev;
            if (mev->action & MOUSE_LUP)
               goto mouse_up;
            if ((mev->pos.x<0)||(mev->pos.x>=grd_visible_canvas->bm.w)||
                (mev->pos.y<0)||(mev->pos.y>=grd_visible_canvas->bm.h))
               slam_mouse=TRUE;
            else if (axis_mask&AXIS_MASK_X)
            {
               vmGUICenter.x=mev->pos.x;  // must clip to our box
               if (vmGUICenter.x<vmGUITop.x+DRAG_BORDER)
                  vmGUICenter.x=vmGUITop.x+DRAG_BORDER;
               else if (vmGUICenter.x>=vmGUITop.x+vmGUIDims.x-DRAG_BORDER)
                  vmGUICenter.x=vmGUITop.x+vmGUIDims.x-DRAG_BORDER-1;
               slam_mouse|=(vmGUICenter.x!=mev->pos.x);
            }
            if (axis_mask&AXIS_MASK_Y)
            {
               vmGUICenter.y=mev->pos.y;
               if (vmGUICenter.y<vmGUITop.y+DRAG_BORDER)
                  vmGUICenter.y=vmGUITop.y+DRAG_BORDER;
               else if (vmGUICenter.y>=vmGUITop.y+vmGUIDims.y-DRAG_BORDER)
                  vmGUICenter.y=vmGUITop.y+vmGUIDims.y-DRAG_BORDER-1;
               slam_mouse|=(vmGUICenter.y!=mev->pos.y);
            }
            if (slam_mouse)
               mouse_put_xy(vmGUICenter.x,vmGUICenter.y);
            vmDrawWindowSkeletons();
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
   vmGUICenter=init_center;
mouse_up:
   modal_ui_end();
   uiShowMouse(NULL);
   return (vmGUICenter.x!=init_center.x)||(vmGUICenter.y!=init_center.y);
}

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
               mx_scale_addeq_vec(&cur_loc,&x_sc,((float)x_off*2.0/(float)DEAD_SPOT_SIZE));
               chg=TRUE;
            }
            if (abs(y_off)>DEAD_SPOT_SIZE)
            {
               mx_scale_addeq_vec(&cur_loc,&y_sc,((float)y_off*2.0/(float)DEAD_SPOT_SIZE));
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

#define CENTER_TOL (VIEW_MARGIN_X*4)
// actually setup for and execute the windows resizing drag
static BOOL setup_center_drag(int x_pos, int y_pos)
{
   Point offset;
   int allowAxisMask;
   
   offset.x=x_pos-vmGUICenter.x;   // get distance from center of GUI
   offset.y=y_pos-vmGUICenter.y;
   
   if ((abs(offset.x)<CENTER_TOL)&&(abs(offset.y)<CENTER_TOL))
      allowAxisMask=AXIS_MASK_X|AXIS_MASK_Y;
   else
      if (abs(offset.x)>abs(offset.y))
         allowAxisMask=AXIS_MASK_Y;
      else
         allowAxisMask=AXIS_MASK_X;
   if (!vmLayout2by2)  // 1by3, dont allow off axis motion
      if (vmLayoutVerticalSplit) allowAxisMask&=~AXIS_MASK_Y;
      else                       allowAxisMask&=~AXIS_MASK_X;
   if (allowAxisMask&AXIS_MASK_X) x_pos=vmGUICenter.x;
   if (allowAxisMask&AXIS_MASK_Y) y_pos=vmGUICenter.y;
   mouse_put_xy(x_pos,y_pos);
   return _doWinDrag(allowAxisMask);
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
static bool vm_parent_mouse_handler(uiEvent* _ev, Region* reg, void* _data)
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
      
      if (ev->action&MOUSE_LDOWN)
         changed_setup=setup_center_drag(ev->pos.x,ev->pos.y);
      else if (ev->action&MOUSE_RDOWN)
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
   case vmWINCFG_RECENTER:   vmGUICenter.x=vmGUIDims.x/2; vmGUICenter.y=vmGUIDims.y/2; break;
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
   vmGUIDims   = MakePoint(RectWidth(bounds)-1,RectHeight(bounds)-1);
   vmGUIRect   = *bounds;

   config_get_float("vmCenterX",&cenX);
   config_get_float("vmCenterY",&cenY);
   vmGUICenter = MakePoint((int)(RectWidth(bounds)*cenX),(int)RectHeight(bounds)*cenY);

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
   float cenX=(float)vmGUICenter.x/(float)vmGUIDims.x;
   float cenY=(float)vmGUICenter.y/(float)vmGUIDims.y;
   uiRemoveRegionHandler(vmWinParent,par_handler_cookie);
   vmWinParent=NULL;
   config_set_float_from_var("vmCenterX",cenX);
   config_set_float_from_var("vmCenterY",cenY);
   config_set_priority("vmCenterX",CONFIG_DFT_LO_PRI);
   config_set_priority("vmCenterY",CONFIG_DFT_LO_PRI);   
}

// this is the beginnings of a "move the whole thing"
// currently just takes bounds, clearly not enough
void vmWinCfgResize(Rect *bounds)
{
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
