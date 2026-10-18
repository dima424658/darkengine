/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/viewmgr.c,v 1.62 2000/02/19 13:13:51 toml Exp $

// View management system

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <lg.h>
#include <2d.h>
#include <r3d.h>
#include <lgd3d.h>
#include <render_backend.h>
#include <mm.h>
#include <mprintf.h>

#include <portal.h>
#include <pt.h>
#include <wrfunc.h>

#include <command.h>
#include <editbr.h>
#include <ged_csg.h>   // level valid extern
#include <ged_rend.h>
#include <editsave.h>
#include <viewmgr_.h>
#include <viewmgr.h>
#include <vumanui.h>
#include <vmwincfg.h>
#include <brinfo.h>
#include <brrend.h>
#include <linkdraw.h>
#include <uiedit.h>

#include <gridsnap.h>
#include <rand.h>
#include <wrlimit.h>

#include <status.h>
#include <missiondiag.h>

#include <config.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


EXTERN void AIPathFindDrawDB(); // from aipthdbg
EXTERN BOOL g_lgd3d;
EXTERN BOOL g_zbuffer;
EXTERN void set_mm_sort(BOOL sort);
EXTERN ulong g_MeshRenderFlags;

void vm_set_synch(int r, bool flag);

//////////////////////////////
//
// partial update
//

static BOOL redraw, new_select, new_camera, new_render;
static bool vmRenderAlways=FALSE;  // needs to be bool for TOGGLE_BOOL in command

#define REDRAW_ALL()   redraw
#define REDRAW(c)      REDRAW_ALL()

#define UPDATE_CAMERA(c) (redraw = TRUE)
#define NEW_CAMERA(c)    (new_camera = TRUE)

void vm_redraw(void)
{
   redraw=TRUE;
}

// this is ugly, but we need the speed, sorry
void vm_redraw_from_select(void)
{
   new_select=TRUE;
}

void vm_redraw_from_camera(void)
{
   new_camera=TRUE;
}

void vm_redraw_from_new3d(void)
{
   new_render=TRUE;
}

////////////////////////////////////////////////////////////
//
//  screen area management (independent of UI)
//
//  Ok, Mahk and I haggled about this a bit, so we're
//  saving it until I have some more context to work in.

uchar camera_to_region_mapping[MAX_CAMERAS]=
{
   0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
};

uchar region_to_camera_mapping[MAX_CAMERAS]=
{
   0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
};

int vm_avail_regions = 0xf; // bitmask of allowed regions
int vm_cur_region, vm_cur_camera;
bool vm_xmouse;

void vm_set_cur_region(int n)
{
   vm_cur_region = n;
   vm_cur_camera = region_to_camera_mapping[n];
}

int vm_get_cur_region(void)
{
   return vm_cur_region;
}

void vm_cycle_camera(bool up)
{
   if (!vm_avail_regions) return;

   for(;;) {
      if (up) { if (++vm_cur_region >= MAX_CAMERAS) vm_cur_region=0; }
      else { if (--vm_cur_region < 0) vm_cur_region = MAX_CAMERAS-1; }

      if (vm_avail_regions & (1 << vm_cur_region))
         break;
   }

   // Need to call UPDATE_CAMERA; otherwise the frame to indicate
   // the current camera won't be redrawn.
   UPDATE_CAMERA(vm_cur_camera);
   vm_cur_camera = region_to_camera_mapping[vm_cur_region];
   UPDATE_CAMERA(vm_cur_camera);
}

// Need to call UPDATE_CAMERA; otherwise the frame to indicate
// the current camera won't be redrawn.
void vm_set_cur_camera(int n)
{
   UPDATE_CAMERA(vm_cur_camera);
   vm_cur_camera = n;
   vm_cur_region = camera_to_region_mapping[n];
   UPDATE_CAMERA(vm_cur_camera);
}

int vm_current_camera(void)
{
   return vm_cur_camera;
}

void vm_disable_region(int r)
{
   if (vm_avail_regions & (1 << r)) {
      vm_avail_regions ^= (1 << r);
      if (!VIS_CAM(vm_cur_camera)) vm_cycle_camera(TRUE);
   } else
      Warning(("vm_disable_region: tried to disable disabled region.\n"));
}

void vm_enable_region(int r)
{
   if (!(vm_avail_regions & (1 << r))) {
      vm_avail_regions ^= (1 << r);
      if (!VIS_CAM(vm_cur_camera)) vm_cycle_camera(TRUE);
   } else
      Warning(("vm_enable_region: tried to enable enabled region.\n"));
}

void vm_enable_all_regions(void)
{
   vm_avail_regions = 0xf;

   if (!VIS_CAM(vm_cur_camera)) vm_cycle_camera(TRUE);
}

void vm_swap_cameras(int x, int y)
{
   int rx,ry;

   rx = camera_to_region_mapping[x];
   ry = camera_to_region_mapping[y];

   camera_to_region_mapping[x] = ry;
   camera_to_region_mapping[y] = rx;

   region_to_camera_mapping[rx] = y;
   region_to_camera_mapping[ry] = x;

   vm_cur_camera = camera_to_region_mapping[vm_cur_region];
}


////////////////////////////////////////////////////////////
//
//  managing camera contexts
//

//
// render mode management
//

#define RM_WIREFRAME_BRUSHES 1
#define RM_SOLID_PORTAL 2
#define RM_WIREFRAME_CURRENT 4
#define RM_WIREFRAME_PORTAL 8

static uchar cycle_mode(uchar x)
{
   uchar y;

   if (x == RM_WIREFRAME_PORTAL)
      return RM_WIREFRAME_BRUSHES;

   y = x & RM_WIREFRAME_PORTAL;
   x &= ~RM_WIREFRAME_PORTAL;

     // guess what the "primary" mode
     // is, and go to the next one

   switch(x) {
      case RM_WIREFRAME_BRUSHES:
         return RM_SOLID_PORTAL | y;
      case RM_SOLID_PORTAL:
         return RM_SOLID_PORTAL | RM_WIREFRAME_CURRENT | y;
      case RM_SOLID_PORTAL | RM_WIREFRAME_CURRENT:
         return RM_SOLID_PORTAL | RM_WIREFRAME_BRUSHES | y;
      case RM_SOLID_PORTAL | RM_WIREFRAME_BRUSHES:
         return RM_WIREFRAME_BRUSHES | y;

      // other cases shouldn't happen
   }
   return RM_WIREFRAME_BRUSHES | y;
}

static uchar fix_mode(uchar x, uchar old)
{
   // have to have something
   if (!x) return RM_WIREFRAME_BRUSHES;

   if ((x & RM_WIREFRAME_BRUSHES) && (x & RM_WIREFRAME_CURRENT)) {
      // have to clear one.  wonder which one you just changed!
      if (old & RM_WIREFRAME_BRUSHES) {
         x &= ~RM_WIREFRAME_BRUSHES;
      } else if (old & RM_WIREFRAME_CURRENT) {
         x &= ~RM_WIREFRAME_CURRENT;
      } else {
         x &= ~RM_WIREFRAME_CURRENT;
      }
   }

   if (x == RM_WIREFRAME_CURRENT)
      return RM_WIREFRAME_BRUSHES;

   return x;
}

//
//  axis definitions for axis-aligned cameras
//
//
// 0 = top-down
// 1 = front
// 2 = side (from right)
// 3 = bottom-up
// 4 = back
// 5 = side (from left)

static char *axis_names[]={"top","front","right","bottom","back","left"};

static mxs_vector screen_x[6] =
{
   { 0, 1, 0 },
   { 0, 1, 0 },
   {-1, 0, 0 },
   { 0, 1, 0 },
   { 0,-1, 0 },
   { 1, 0, 0 }
};

static mxs_vector screen_y[6] =
{
   { 1,0,0 },
   { 0,0,-1 },
   { 0,0,-1 },
   { -1,0,0 },
   { 0,0,-1 },
   { 0,0,-1 }
};

  // which axis the view is along
static int bogus_axis[6] = { 2,0,1,2,0,1 };
static int x_axis[6] = { 1,1,0,1,1,0 };
static int y_axis[6] = { 0,2,2,0,2,2 };
static int normal_axis[6] = { 2,0,1,5,3,4 };

static mxs_angvec axis_angles[6] =
{
   { 0, 0x4000, 0x8000 },
   { 0, 0, 0x8000 },
   { 0, 0, 0xc000 },
   { 0, 0xc000, 0 },
   { 0, 0, 0 },
   { 0, 0, 0x4000 }
};

//
// camera structure
//

typedef struct
{
     // are we in 3d or 2d mode?
   bool camera_3d;
   uchar render_mode;

     // 3d state
   Location loc;
   mxs_angvec ang;
   mxs_real  zoom;
   bool perspective;

     // 2d state
   Location loc_2d;
   mxs_real scale;
   int axis;    // axis we're looking along
   bool synch;  // do we just follow the 1st 3d view?
} CameraView;

static CameraView camera[MAX_CAMERAS];

#define zero_ang(a) ((a).tx = 0, (a).ty = 0, (a).tz = 0)
#define zero_loc(a) MakeLocation(&(a), 0,0,0)

#define CM camera[c]
void vm_init_camera(int c)
{
   static int view_axis[4] = { 0,0,2,1 };
   UPDATE_CAMERA(c);
   switch (c) {
      case 0:
         CM.camera_3d = TRUE;
         CM.render_mode = RM_SOLID_PORTAL;
         zero_loc(CM.loc);
         zero_ang(CM.ang);
         CM.zoom = 1.0;
         CM.perspective = TRUE;

         zero_loc(CM.loc_2d);
         CM.scale = 64.0;
         CM.axis = 0;
         CM.synch = FALSE;
         break;

      case 1: case 2: case 3:
         CM.camera_3d = FALSE;
         CM.render_mode = 1;
         zero_loc(CM.loc);
         zero_ang(CM.ang);
         CM.zoom = 1.0;
         CM.perspective = TRUE;

         zero_loc(CM.loc_2d);
         CM.scale = 64.0;
         CM.axis = view_axis[c];
         CM.synch = TRUE;
         break;
    }
}
#undef CM

float vm_get_effective_region_scale(int c)
{
   int r = camera_to_region_mapping[c];
   return camera[c].scale * vmGetSizeRatio(r,NULL,NULL);
}

void vm_init_cameras(void)
{
   int i;
   for (i=0; i < MAX_CAMERAS_REAL; ++i)
      vm_init_camera(i);
}

static int synch_camera(void)
{
   int i;
   for (i=0; i < MAX_CAMERAS; ++i)
      if (camera[i].camera_3d && VIS_CAM(i))
         return i;
   return -1;
}

static int real_camera(int r)
{
   int n;
   if (camera[r].camera_3d || !camera[r].synch)
      return r;
   n = synch_camera();
   return n == -1 ? r : n;
}

bool vm_spotlight_loc(mxs_vector **loc, mxs_angvec **ang)
{
   int n = synch_camera();
   if (n == -1)
      return FALSE;

   *loc = &camera[n].loc.vec;
   *ang = &camera[n].ang;
   return TRUE;
}

#define mmax(x,y)  ((x) < (y) ? (y) : (x))

  // fit all the cameras to the current region, even cameras
  // that aren't currently exposed, but only 2d cameras
void vm_fit_cameras_to_region(mxs_vector *start, mxs_vector *end)
{
   mxs_vector center;
   mxs_vector length;
   int i, rc;

   mx_scale_vec(&center, start, 0.5);
   mx_scale_addeq_vec(&center, end, 0.5);

   mx_sub_vec(&length, start, end);
   length.x = fabsf(length.x);
   length.y = fabsf(length.y);
   length.z = fabsf(length.z);

   for (i=0; i < MAX_CAMERAS; ++i)
      if (!camera[i].camera_3d && camera[i].synch)
         break;

   if (i < MAX_CAMERAS) {
      rc = synch_camera();
      if (rc >= 0)
         MakeLocationFromVector(&camera[rc].loc, &center);
   }

   for (i=0; i < MAX_CAMERAS; ++i)
      if (!camera[i].camera_3d)
      {
         int a = camera[i].axis;
         int x = x_axis[a], y = y_axis[a];
         float xsc,ysc;

         UPDATE_CAMERA(i);

         MakeLocationFromVector(&camera[i].loc, &center);

         // ok, I don't know why 5/3, I think that's approximately
         // the screen size ratio of x & y default window sizes;
         // yet it doesn't account for aspect ratio.  Huh.  The 1.2
         // is there to give a bit of a boundary; maybe it's effectively
         // giving some boundary in x, and doing aspect ratio in y.
         if (vmGetSizeRatio(camera_to_region_mapping[i],&xsc,&ysc))
            camera[i].scale=mmax(length.el[x]/xsc,length.el[y]*5/3/ysc)*1.2f;
      }
}

  // get location of current view
mxs_vector *vm_cur_loc(void)
{
   int n = real_camera(vm_cur_camera);
   if (camera[n].camera_3d)
      return &camera[n].loc.vec;
   else
      return &camera[n].loc_2d.vec;
}

void vm_cur_loc_set(mxs_vector *loc)
{
   int n = real_camera(vm_cur_camera);
   UPDATE_CAMERA(n);
   if (camera[n].camera_3d)
      MakeLocationFromVector(&camera[n].loc, loc);
   else
      MakeLocationFromVector(&camera[n].loc_2d, loc);
}

bool vm_cur_3d(void)
{
   return camera[vm_cur_camera].camera_3d;
}

static int vm_get_3d_camera_to_use(void)
{
   int camera_to_use=vm_cur_camera, i;
   if (!camera[camera_to_use].camera_3d)
      for (i=0; i<MAX_CAMERAS_REAL; i++)
         if (i!=vm_cur_camera)
            if (camera[i].camera_3d)
               camera_to_use=i;
   return camera_to_use;
}

void vm_cur_slew(mxs_vector *slew)
{
   int camera_to_use=vm_get_3d_camera_to_use();
   if (camera[camera_to_use].camera_3d)
   {
      NEW_CAMERA(camera_to_use);
      r3_start_frame();
      r3_set_view_angles(&camera[camera_to_use].loc.vec,&camera[camera_to_use].ang,R3_DEFANG);
      mx_scale_addeq_vec(&camera[camera_to_use].loc.vec, r3_get_forward_slew(), slew->x);
      mx_scale_addeq_vec(&camera[camera_to_use].loc.vec, r3_get_left_slew(), slew->y);
      mx_scale_addeq_vec(&camera[camera_to_use].loc.vec, r3_get_up_slew(), slew->z);
      UpdateChangedLocation(&camera[camera_to_use].loc);
      r3_end_frame();
   } else {
      Warning(("vm_cur_slew: tried slewing 2d camera - no 3d cameras avialable\n"));
   }
}

void vm_cur_scroll(mxs_real x, mxs_real y)
{
   if (camera[vm_cur_camera].camera_3d) {
      Warning(("vm_cur_scroll: tried to scroll a 3d camera.\n"));
   } else {
      int a = camera[vm_cur_camera].axis, n = vm_cur_camera;
      // auto unsynch
      UPDATE_CAMERA(n);
      if (camera[n].synch)
         vm_set_synch(n, FALSE);
      mx_scale_addeq_vec(&camera[n].loc_2d.vec, &screen_x[a], x);
      mx_scale_addeq_vec(&camera[n].loc_2d.vec, &screen_y[a], y);
      UpdateChangedLocation(&camera[n].loc_2d);
   }
}

void vm_cur_rotate(mxs_angvec *rot)
{
   int camera_to_use=vm_get_3d_camera_to_use();
   if (camera[camera_to_use].camera_3d) {
      NEW_CAMERA(camera_to_use);
      camera[camera_to_use].ang.tx += rot->tx;
      camera[camera_to_use].ang.ty += rot->ty;
      camera[camera_to_use].ang.tz += rot->tz;
   } else {
      Warning(("vm_cur_rotate: tried rotating 2d camera - no 3d cameras available\n"));
   }
}

void vm_cur_level (int axis)
{
   if (!camera[vm_cur_camera].camera_3d)
   {
      Warning (("vm_cur_level: tried to level a 2d camera.\n"));
      return;
   }

   if (axis < 0 || axis > 2)
   {
      Warning (("vm_cur_level: illegal axis %d, must be between 0 and 2\n"));
      return;
   }

   UPDATE_CAMERA(vm_cur_camera);
   camera[vm_cur_camera].ang.el[axis] = 0;
}

void vm_cur_scale(mxs_real scale)
{
   if (camera[vm_cur_camera].camera_3d)
      Warning(("vm_cur_scale: tried to rescale a 3d camera.\n"));
   else {
      UPDATE_CAMERA(vm_cur_camera);
      if (vm_menu_inprog)
      {
         mxs_vector temp, *loc;
         int a = vm_map_screen_to_world(vm_cur_camera, &temp, vm_menu_click_x, vm_menu_click_y);
         camera[vm_cur_camera].synch=FALSE;
         loc=vm_cur_loc();
         loc->el[(a+1)%3] = temp.el[(a+1)%3];
         loc->el[(a+2)%3] = temp.el[(a+2)%3];
      }
      camera[vm_cur_camera].scale *= scale;
   }
}

void vm_cur_cycle_mode(void)
{
   UPDATE_CAMERA(vm_cur_camera);
   camera[vm_cur_camera].render_mode = cycle_mode(camera[vm_cur_camera].render_mode);
}

void vm_cur_set_mode(int n)
{
   UPDATE_CAMERA(vm_cur_camera);
   camera[vm_cur_camera].render_mode = fix_mode((uchar)n, camera[vm_cur_camera].render_mode);
}

void vm_cur_toggle_mode(uchar toggle)
{
   UPDATE_CAMERA(vm_cur_camera);
   camera[vm_cur_camera].render_mode = fix_mode((uchar)(camera[vm_cur_camera].render_mode ^ toggle), camera[vm_cur_camera].render_mode);
}

void vm_cur_toggle_3d(void)
{
   UPDATE_CAMERA(vm_cur_camera);
   camera[vm_cur_camera].camera_3d = !camera[vm_cur_camera].camera_3d;
}

void vm_cur_toggle_persp(void)
{
   UPDATE_CAMERA(vm_cur_camera);
   if (camera[vm_cur_camera].camera_3d)
      camera[vm_cur_camera].perspective = !camera[vm_cur_camera].perspective;
   else
      Warning(("vm_cur_toggle_persp: tried to toggle perspective in 2d view.\n"));
}

void vm_cur_toggle_synch(void)
{
   UPDATE_CAMERA(vm_cur_camera);
   if (camera[vm_cur_camera].camera_3d)
      Warning(("vm_cur_toggle_synch: tried to toggle synch mode on a 3d view.\n"));
   else
      vm_set_synch(vm_cur_camera, (bool)(!camera[vm_cur_camera].synch));
}

void vm_set_synch_all(bool mode)
{
   int i;
   redraw = TRUE;
   for (i=0; i < MAX_CAMERAS; ++i)
      vm_set_synch(i, mode);
}

void vm_scale_all(mxs_real scale)
{
   int i;
   redraw = TRUE;
   for (i=0; i < MAX_CAMERAS; ++i)
      camera[i].scale *= scale;
}


////////////////////////////////////////////////////////////
//
//  rendering the cameras
//

static Location *cur_loc;
static mxs_angvec *cur_ang;

void vm_start_3d(int c)
{
   int rc = real_camera(c);

   r3_start_frame();
   if (camera[c].camera_3d) {
      cur_loc = &camera[c].loc;
      cur_ang = &camera[c].ang;

      r3_set_view_angles(&cur_loc->vec, cur_ang,R3_DEFANG);
      if (camera[c].perspective)
      {  // hmmm.. try project space?
//         r3_set_space(R3_CLIPPING_SPACE);
         r3_set_space(R3_PROJECT_SPACE);
      }
      else
      {
         r3_set_space(R3_LINEAR_SPACE);
         r3_set_linear_screen_width(16.0);  // why 16?
      }
   } else {
      cur_loc = camera[rc].camera_3d ? &camera[rc].loc : &camera[rc].loc_2d;
      cur_ang = &axis_angles[camera[c].axis];
      r3_set_view_angles(&cur_loc->vec, cur_ang, R3_DEFANG);
      r3_set_space(R3_LINEAR_SPACE);
      r3_set_linear_screen_width(vm_get_effective_region_scale(c));
   }
}

void vm_get_3d_camera_loc(mxs_vector **pos, mxs_angvec **ang)
{
   if (pos) *pos=&cur_loc->vec;
   if (ang) *ang=cur_ang;
}

// fills in x and y, returns bogus axis
int vm_get_camera_axis(int c, int *x, int *y)
{
   if (x) *x=x_axis[camera[c].axis];
   if (y) *y=y_axis[camera[c].axis];
   return bogus_axis[camera[c].axis];
}

void vm_end_3d(int c)
{
   r3_end_frame();
}

  // map point in camera c's (x,y) coordinates
  // to an output location, returning which axis
  // number is bogus; -1 means it's not a 2d camera

int vm_map_screen_to_world(int c, mxs_vector *dest, int x, int y)
{
   int reg = camera_to_region_mapping[c];
   Region* uireg = vmGetRegion(reg);
   int rc, a;
   mxs_real sx,sy,scale;
   int xsz = RectWidth(uireg->r);
   int ysz = RectHeight(uireg->r);

   if (camera[c].camera_3d) return -1;

   sx = x / (float) xsz - 0.5f;
   sy = (y - ysz/2) / (float) xsz;

   rc = real_camera(c);
   if (c == rc)
      *dest = camera[rc].loc_2d.vec;
   else
      *dest = camera[rc].loc.vec;

   a = camera[c].axis;
   scale=vm_get_effective_region_scale(c);  // was camera[c].scale
   mx_scale_addeq_vec(dest, &screen_x[a], sx * scale);
   mx_scale_addeq_vec(dest, &screen_y[a], sy * scale);

   return bogus_axis[a];
}

BOOL vm_camera_scr_size(int c, int *w, int *h)
{
   int reg = camera_to_region_mapping[c];
   Region* uireg = vmGetRegion(reg);

   *w = RectWidth(uireg->r);
   *h = RectHeight(uireg->r);
   return TRUE;
}

int vm_normal_axis(int c)
{
   if (camera[c].camera_3d) return -1;
   return normal_axis[camera[c].axis];
}

bool vm_screen_axes(int c, mxs_vector *x, mxs_vector *y)
{
   int a, reg = camera_to_region_mapping[c];
   Region *uireg = vmGetRegion(reg);
   mxs_real scale;

   int xsz = RectWidth(uireg->r);
   //int ysz = RectHeight(uireg->r);

   mxs_real sx,sy;

   if (camera[c].camera_3d) return FALSE;

   sx = 1 / (float) xsz;
   sy = 1 / (float) xsz;

   a = camera[c].axis;

   scale=vm_get_effective_region_scale(c);  // was camera[c].scale
   mx_scale_vec(x, &screen_x[a], sx * scale);
   mx_scale_vec(y, &screen_y[a], sy * scale);

   return TRUE;
}

// Retail DromEd shows the construction grid in the orthographic panes.  Keep
// the perspective grid available through grid_3d, but do not enable it by
// default.
bool vm_show_grid=TRUE, vm_show_grid_3d=FALSE;
int vm_3d_grid_axis=2;
mxs_real vm_3d_grid_height;
extern Grid cur_global_grid;

int view_grid_color=7, view_select_color=1;

#define GRID_COLOR uiRGB(0x39,0x39,0x39)   // some kinda grey
#define SELECT_COLOR uiRGB(0xba,0xba,0xba) // off-white
void vm_pick_colors(void)
{
   view_grid_color = guiScreenColor(GRID_COLOR);
   view_select_color = guiScreenColor(SELECT_COLOR);
}



static void render_grid_with_color(int c, int color)
{
   mxs_vector start,end;
   r3_set_color(color);
   if (camera[c].camera_3d) {
      start = end = camera[c].loc.vec;

      start.x -= 20 * cur_global_grid.line_spacing;
      start.y -= 20 * cur_global_grid.line_spacing;
      start.z -= 20 * cur_global_grid.line_spacing;

      end.x += 20 * cur_global_grid.line_spacing;
      end.y += 20 * cur_global_grid.line_spacing;
      end.z += 20 * cur_global_grid.line_spacing;

      gedit_render_grid(&cur_global_grid, vm_3d_grid_axis, vm_3d_grid_height, &start, &end);
   } else {
      int rc = real_camera(c), a = camera[c].axis;
      mxs_real x_scale, y_scale;

      vmGetSizeRatio(camera_to_region_mapping[c],&x_scale,&y_scale);
      x_scale*=camera[c].scale; y_scale*=camera[c].scale;

      if (c == rc)
         start = end = camera[rc].loc_2d.vec;
      else
         start = end = camera[rc].loc.vec;

      start.el[x_axis[a]] -= x_scale/2;  // these were all camera[c].scale
        end.el[x_axis[a]] += x_scale/2;

      start.el[y_axis[a]] -= y_scale/2;
        end.el[y_axis[a]] += y_scale/2;

      gedit_render_grid(&cur_global_grid, bogus_axis[a], 0, &start, &end);
   }
}

static void render_grid(int c)
{
   render_grid_with_color(c, view_grid_color);
}

static void render_hardware_grid(int c)
{
   // guiScreenColor() produces a native 16-bit software-canvas value.  The
   // hardware primitive path consumes 0xRRGGBB instead, so supply the grid's
   // source RGB while palette conversion is disabled.
   lgd3d_disable_palette();
   render_grid_with_color(c, 0x393939);
   lgd3d_enable_palette();
}

void vm_set_camera_canvas(int c)
{
   int reg = camera_to_region_mapping[c];
   vm_set_region_canvas(reg);
}

void vm_unset_camera_canvas(void)
{
   vm_unset_region_canvas();
}

bool vm_display_enable=TRUE, show_raycasts = FALSE;
extern void render_failures(void);

BOOL vm_pick_camera(int c, int x, int y, int *brush, int *face)
{
   BOOL rv=FALSE;
   uchar mode;

   if (!vm_display_enable) return rv;
   if (!VIS_CAM(c)) return rv;

   vm_set_camera_canvas(c);
   mode = camera[c].render_mode;

   if (mode & RM_SOLID_PORTAL)
      if (gedcsg_level_valid)
      {
         extern int *wr_brfaces[MAX_REGIONS];   // this is brush_id*256 + face
         int rc = real_camera(c);
         Position pos;
         int prend_ret;  // this is cell*256 + poly
         int cell, poly;

         pos.loc = *(camera[rc].camera_3d ? &camera[rc].loc : &camera[rc].loc_2d);
         pos.fac = *(camera[c].camera_3d ? &camera[c].ang :
                     &axis_angles[camera[c].axis]);
         prend_ret=PortalRenderPick(&pos,x,y,camera[c].zoom);
         poly=prend_ret&0xff;
         cell=(prend_ret-poly)>>8;
         if (cell!=CELL_INVALID)
         {
            *face=wr_brfaces[cell][poly]&0xff;
            *brush=wr_brfaces[cell][poly]>>8;
            rv=TRUE;
         }
      }
   vm_unset_camera_canvas();
   return rv;
}

// help functions for render_camera
static void _vm_rend_start(int c, BOOL show_grid)
{
   gr_clear(0);
   if (show_grid)
      render_grid(c);
}

static void _vm_rend_message_2line(char *line1, char *line2)
{
   guiStyleSetupFont(NULL,StyleFontNormal);
   gr_set_fcolor(guiStyleGetColor(NULL,StyleColorFG));
   gr_string(line1,
             (grd_canvas->bm.w-gr_string_width(line1))/2,
             (grd_canvas->bm.h/2)-15);
   gr_string(line2,
             (grd_canvas->bm.w-gr_string_width(line2))/2,
             (grd_canvas->bm.h/2)+5);
   guiStyleCleanupFont(NULL,StyleFontNormal);
}

#define CORNER_RIGHT  1
#define CORNER_BOTTOM 2
static int _vm_name_corner=0;
static void _vm_rend_message_corner(int corner_id, char *msg)
{
   int x=1,y=1;
   if (corner_id==-1) return;
   guiStyleSetupFont(NULL,StyleFontNormal);
   gr_set_fcolor(guiStyleGetColor(NULL,StyleColorFG));
   if (corner_id&CORNER_RIGHT)
      x=grd_canvas->bm.w-gr_string_width(msg)-1;
   if (corner_id&CORNER_BOTTOM)
      y=grd_canvas->bm.h-gr_string_height(msg)-1;
   gr_string(msg,x,y);
   guiStyleCleanupFont(NULL,StyleFontNormal);
}

// Match DromEd's normal terrain-brush color.  Yellow is reserved for lights,
// red for objects, and magenta for the synchronized camera marker.
#define COMPILED_GEOMETRY_COLOR uiRGB(0x63,0xa3,0xb8)
#define COMPILED_GEOMETRY_LABEL_COLOR uiRGB(0x80,0xd0,0xe8)

typedef struct
{
   int quantized[6];
   mxs_vector point[2];
   mxs_vector normal;
   int references;
   BOOL has_crease;
   BOOL used;
} CompiledEdgeHashEntry;

typedef struct
{
   mxs_vector point[2];
} CompiledDisplayEdge;

static CompiledDisplayEdge *g_CompiledDisplayEdges;
static int g_CompiledDisplayEdgeCount;
static ulong g_CompiledDisplayEdgeGeneration;
static BOOL g_CompiledDisplayEdgeCacheValid;

static void _vm_free_compiled_geometry_cache(void)
{
   if (g_CompiledDisplayEdges!=NULL)
      Free(g_CompiledDisplayEdges);
   g_CompiledDisplayEdges=NULL;
   g_CompiledDisplayEdgeCount=0;
   g_CompiledDisplayEdgeGeneration=0;
   g_CompiledDisplayEdgeCacheValid=FALSE;
}

static int _vm_quantize_compiled_coordinate(mxs_real value)
{
   return (int)floor(value*256.0+0.5);
}

static int _vm_compare_quantized_points(const int *a, const int *b)
{
   int axis;
   for (axis=0; axis<3; ++axis)
   {
      if (a[axis]<b[axis]) return -1;
      if (a[axis]>b[axis]) return 1;
   }
   return 0;
}

static uint _vm_hash_compiled_edge(const int *coordinates)
{
   uint hash=2166136261u;
   int coordinate;
   for (coordinate=0; coordinate<6; ++coordinate)
   {
      hash^=(uint)coordinates[coordinate];
      hash*=16777619u;
   }
   return hash;
}

static BOOL _vm_same_compiled_edge(const CompiledEdgeHashEntry *entry,
                                   const int *coordinates)
{
   int coordinate;
   for (coordinate=0; coordinate<6; ++coordinate)
      if (entry->quantized[coordinate]!=coordinates[coordinate])
         return FALSE;
   return TRUE;
}

static void _vm_add_compiled_edge(CompiledEdgeHashEntry *table, int table_size,
                                  const mxs_vector *first,
                                  const mxs_vector *second,
                                  const mxs_vector *normal)
{
   int first_quantized[3], second_quantized[3], coordinates[6];
   const mxs_vector *ordered_first=first;
   const mxs_vector *ordered_second=second;
   uint slot;
   int axis;

   for (axis=0; axis<3; ++axis)
   {
      first_quantized[axis]=_vm_quantize_compiled_coordinate(first->el[axis]);
      second_quantized[axis]=_vm_quantize_compiled_coordinate(second->el[axis]);
   }
   if (_vm_compare_quantized_points(first_quantized,second_quantized)>0)
   {
      int swap[3];
      memcpy(swap,first_quantized,sizeof(swap));
      memcpy(first_quantized,second_quantized,sizeof(first_quantized));
      memcpy(second_quantized,swap,sizeof(second_quantized));
      ordered_first=second;
      ordered_second=first;
   }
   memcpy(coordinates,first_quantized,sizeof(first_quantized));
   memcpy(coordinates+3,second_quantized,sizeof(second_quantized));

   slot=_vm_hash_compiled_edge(coordinates)&(table_size-1);
   while (table[slot].used &&
          !_vm_same_compiled_edge(&table[slot],coordinates))
      slot=(slot+1)&(table_size-1);

   if (!table[slot].used)
   {
      CompiledEdgeHashEntry *entry=&table[slot];
      entry->used=TRUE;
      entry->references=1;
      memcpy(entry->quantized,coordinates,sizeof(entry->quantized));
      entry->point[0]=*ordered_first;
      entry->point[1]=*ordered_second;
      entry->normal=*normal;
   }
   else
   {
      CompiledEdgeHashEntry *entry=&table[slot];
      mxs_real alignment=mx_dot_vec(&entry->normal,normal);
      ++entry->references;
      // Normal direction can be reversed on the opposite side of a portal.
      // Parallel planes in either direction are still coplanar seams.
      if (fabs(alignment)<0.999)
         entry->has_crease=TRUE;
   }
}

static void _vm_build_compiled_geometry_cache(void)
{
   CompiledEdgeHashEntry *table;
   int maximum_edges=0, table_size=1, cell_index, edge_index, compact_index=0;
   ulong generation=editor_BrushDataGeneration();

   if (g_CompiledDisplayEdgeCacheValid &&
       g_CompiledDisplayEdgeGeneration==generation)
      return;
   _vm_free_compiled_geometry_cache();
   g_CompiledDisplayEdgeGeneration=generation;

   for (cell_index=0; cell_index<wr_num_cells; ++cell_index)
   {
      PortalCell *cell=WR_CELL(cell_index);
      int poly_index;
      if (cell==NULL || cell->poly_list==NULL)
         continue;
      for (poly_index=0; poly_index<cell->num_render_polys; ++poly_index)
         maximum_edges+=cell->poly_list[poly_index].num_vertices;
   }
   if (maximum_edges==0)
      return;
   while (table_size<maximum_edges*2)
      table_size<<=1;

   table=(CompiledEdgeHashEntry *)Malloc(table_size*sizeof(*table));
   if (table==NULL)
      return;
   memset(table,0,table_size*sizeof(*table));

   for (cell_index=0; cell_index<wr_num_cells; ++cell_index)
   {
      PortalCell *cell=WR_CELL(cell_index);
      int poly_index, vertex_offset=0;
      if (cell==NULL || cell->vpool==NULL || cell->poly_list==NULL ||
          cell->vertex_list==NULL || cell->plane_list==NULL)
         continue;
      for (poly_index=0; poly_index<cell->num_render_polys; ++poly_index)
      {
         PortalPolygonCore *poly=&cell->poly_list[poly_index];
         int vertex_index;
         if (poly->planeid>=cell->num_planes)
         {
            vertex_offset+=poly->num_vertices;
            continue;
         }
         for (vertex_index=0; vertex_index<poly->num_vertices; ++vertex_index)
         {
            int first=cell->vertex_list[vertex_offset+vertex_index];
            int second=cell->vertex_list[vertex_offset+
               ((vertex_index+1)%poly->num_vertices)];
            if (first<cell->num_vertices && second<cell->num_vertices)
               _vm_add_compiled_edge(table,table_size,&cell->vpool[first],
                                     &cell->vpool[second],
                                     &cell->plane_list[poly->planeid].normal);
         }
         vertex_offset+=poly->num_vertices;
      }
   }

   // Portalization splits source surfaces across cells.  Shared coplanar
   // edges are compiler seams rather than geometry; keep actual boundaries
   // and non-coplanar crease edges.  The cache is not marked valid until this
   // complete world pass finishes, so an early load frame cannot suppress the
   // real edges permanently.
   for (edge_index=0; edge_index<table_size; ++edge_index)
      if (table[edge_index].used &&
          (table[edge_index].references==1 || table[edge_index].has_crease))
         ++g_CompiledDisplayEdgeCount;

   if (g_CompiledDisplayEdgeCount>0)
      g_CompiledDisplayEdges=(CompiledDisplayEdge *)Malloc(
         g_CompiledDisplayEdgeCount*sizeof(*g_CompiledDisplayEdges));
   if (g_CompiledDisplayEdges!=NULL)
   {
      for (edge_index=0; edge_index<table_size; ++edge_index)
      {
         CompiledEdgeHashEntry *entry=&table[edge_index];
         if (entry->used &&
             (entry->references==1 || entry->has_crease))
         {
            g_CompiledDisplayEdges[compact_index].point[0]=entry->point[0];
            g_CompiledDisplayEdges[compact_index].point[1]=entry->point[1];
            ++compact_index;
         }
      }
   }
   else
      g_CompiledDisplayEdgeCount=0;
   Free(table);
   g_CompiledDisplayEdgeCacheValid=TRUE;
}

// Retail mission files retain the portalized world but normally omit BRLIST,
// the editable source brushes from which it was built.  This renderer is
// deliberately display-only: selection and editing continue to operate only
// on real editor brushes.
static BOOL _vm_has_compiled_geometry_fallback(void)
{
   return !editor_BrushDataPresent() && gedcsg_level_valid && wr_num_cells > 0;
}

static void _vm_render_compiled_geometry(int c)
{
   int edge_index;
   int color = guiScreenColor(COMPILED_GEOMETRY_COLOR);

   r3_set_color(color);
   gr_set_fcolor(color);
   // Keep the clip mode selected by vm_start_3d(). Perspective cameras use
   // the 3D clipper, while orthographic R3_LINEAR_SPACE cameras intentionally
   // use R3_NO_CLIP plus dev2d's screen-line clipper. Forcing R3_CLIP here is
   // invalid because the linear transform does not produce perspective clip
   // codes; interpreting that unset state creates viewport-spanning streaks.
   r3_start_block();

   _vm_build_compiled_geometry_cache();

   for (edge_index=0; edge_index<g_CompiledDisplayEdgeCount; ++edge_index)
   {
      CompiledDisplayEdge *edge=&g_CompiledDisplayEdges[edge_index];
      r3s_point points[2];
      r3_transform_block(2,points,edge->point);
      r3_draw_line(&points[0],&points[1]);
   }

   r3_end_block();
}

static void _vm_rend_compiled_geometry_label(int c)
{
   char message[]="Compiled terrain view; editable source brushes unavailable";

   // One notice in the selected pane is enough. The other panes show the
   // same compiled world and retain their normal view labels.
   if (c != vm_cur_camera)
      return;

   guiStyleSetupFont(NULL,StyleFontNormal);
   gr_set_fcolor(guiScreenColor(COMPILED_GEOMETRY_LABEL_COLOR));
   gr_string(message,1,1);
   guiStyleCleanupFont(NULL,StyleFontNormal);
}

static void _vm_show_camera_in2d(int c)
{
   mxs_vector *locp;
   mxs_angvec *angp;

   if (vm_spotlight_loc (&locp, &angp))       // hmmm, should this one stay the old way?
      gedrendRenderCameraLoc (locp, angp, vm_get_effective_region_scale(c));
}

static grs_bitmap *camera_saves[MAX_CAMERAS_REAL]={NULL,NULL,NULL,NULL};

#ifdef WE_NEVER_CALL_THIS_SHOULD_WE
static void _vm_camera_saves_init(void)
{
   int i;
   for (i=0; i<MAX_CAMERAS_REAL; i++)
      camera_saves[i]=NULL;
}
#endif

static void _vm_camera_saves_term(void)
{
   int i;
   for (i=0; i<MAX_CAMERAS_REAL; i++)
      if (camera_saves[i])
      {
         gr_free(camera_saves[i]);
         camera_saves[i]=NULL;
      }
}

static void _vm_save_camera_bitmap(int c)
{
   BOOL get_new_bm=TRUE;
   if (camera_saves[c]!=NULL)
      if ((camera_saves[c]->w==grd_canvas->bm.w)&&(camera_saves[c]->h==grd_canvas->bm.h))
         get_new_bm=FALSE;
      else
      {
         gr_free(camera_saves[c]);
         camera_saves[c]=NULL;
      }
   if (get_new_bm)
      camera_saves[c]=gr_alloc_bitmap(grd_canvas->bm.type,0,grd_canvas->bm.w,grd_canvas->bm.h);
   gr_get_bitmap(camera_saves[c],0,0);  // get the current canvas to save
}

static void _vm_restore_camera_bitmap(int c)
{
   if (camera_saves[c])
       gr_bitmap(camera_saves[c],0,0);
}

extern void cam_render_scene(Position *pos, float zoom);
static int vm_lgd3d_frame;

static BOOL update_links = TRUE;
// ok, secretly knows there is the "new_camera" and "new_select" subflags of redraw....
void vm_render_camera(int c)
{
   static BOOL render_test_logged=FALSE;
   int reg, rv_flag=0;
   BOOL restore_bm=FALSE, show_grid, compiled_fallback;
   uchar mode;

   if (!render_test_logged && c==0 && config_is_defined("render_test"))
   {
      char diagnostic[256];
      snprintf(diagnostic,sizeof(diagnostic),
         "Render-test viewport entry: display=%d visible=%d camera3d=%d "
         "mode=0x%x redraw=%d camera=%d render=%d",
         vm_display_enable,VIS_CAM(c),camera[c].camera_3d,
         (uint)camera[c].render_mode,redraw,new_camera,new_render);
      diagnostic[sizeof(diagnostic)-1]='\0';
      MissionDiagnosticsIssue("INFO","editor-view",diagnostic);
      render_test_logged=TRUE;
   }
   if (!vm_display_enable) return;
   if (!VIS_CAM(c)) return;

   if (update_links)
      DrawRelations();

   reg = camera_to_region_mapping[c];
   // Mode dispatch can deliver an initial frame before the editor client has
   // finished constructing its child regions.  The legacy renderer assumed
   // those callbacks were always strictly ordered and dereferenced a null
   // region rectangle during modern startup.  Leave the redraw flags set and
   // draw this camera on the next frame after EditorCreateGUI has completed.
   if (vmGetRegion(reg)->r == NULL)
      return;
   vm_set_region_canvas(reg);
   vm_start_3d(c);

   mode = camera[c].render_mode;
   if (camera[c].camera_3d)
      RenderBackendTrace(
         "editor-render index=%d requested-mode=0x%x level-valid=%d "
         "world-cells=%d cell=%d pos=%.3f,%.3f,%.3f",
         c,(uint)mode,gedcsg_level_valid,wr_num_cells,
         CellFromLoc(&camera[c].loc),camera[c].loc.vec.x,
         camera[c].loc.vec.y,camera[c].loc.vec.z);

// New stuff here:
   if (!config_is_defined("Editor_NoInvalidCellCheck"))
   {
      if (camera[c].camera_3d)
      {
         if (CellFromLoc(&camera[c].loc)==CELL_INVALID)
         {
            mode &= ~RM_SOLID_PORTAL;
            mode |= RM_WIREFRAME_BRUSHES;
         }
      }
      else
      {
         if (CellFromLoc(&camera[c].loc_2d)==CELL_INVALID)
         {
            mode &= ~RM_SOLID_PORTAL;
            mode |= RM_WIREFRAME_BRUSHES;
         }
      }
   }

   if (!camera[c].camera_3d)
      rv_flag|=brFlag_IsoView;
   if (mode & RM_WIREFRAME_PORTAL)
      Warning(("vm_render_camera: Wireframe portal view not yet supported.\n"));
   show_grid=camera[c].camera_3d ? vm_show_grid_3d : vm_show_grid;
   compiled_fallback=_vm_has_compiled_geometry_fallback();

   if (camera[c].camera_3d)
   {
      BOOL redraw_3d=FALSE;
      BOOL hardware_grid;
      if (mode & RM_SOLID_PORTAL)
         if (redraw || new_camera || new_render)
            redraw_3d=TRUE;
      if (mode & RM_WIREFRAME_BRUSHES)
         if (redraw || new_camera)
            redraw_3d=TRUE;
      redraw_3d |= (show_grid && (redraw || new_camera));
      if (redraw_3d)
      {
         if (g_lgd3d && !(mode & RM_SOLID_PORTAL))
         {
            // Leaving the portal world (for example by slewing below the
            // floor) switches this pane to wireframe-brush mode.  The modern
            // presenter otherwise keeps compositing the last valid hardware
            // scene, making the world appear stationary while only the grid
            // follows the camera.  Retire that stale scene before drawing the
            // software fallback.
            lgd3d_scene_suspend();
         }

         // Draw the construction grid in the hardware scene so its color and
         // pane coordinates remain correct.  It is an editor overlay, not
         // world geometry: terrain must not depth-clip it into the finite
         // patch that appeared after the D3D11 conversion.
         hardware_grid = show_grid && g_lgd3d &&
                         (mode & RM_SOLID_PORTAL) && gedcsg_level_valid;
         _vm_rend_start(c,show_grid && !hardware_grid);
         if (mode & RM_SOLID_PORTAL)
         {
            if (gedcsg_level_valid)
            {
               int rc = real_camera(c);
               Position pos;
               // we probably could use the r3d frame stack and not bother
               // shutting it down and restarting, but I'm not gonna risk it
               vm_end_3d(c);
               pos.loc = *(camera[rc].camera_3d ? &camera[rc].loc : &camera[rc].loc_2d);
               pos.fac = *(camera[c].camera_3d ? &camera[c].ang : &axis_angles[camera[c].axis]);
               if (g_lgd3d)
               {
                  Rect screen_rect;
                  Region *view_region = vmGetRegion(reg);

                  region_abs_rect(view_region, view_region->r, &screen_rect);

                  // Editor cameras render into sub-canvases.  The modern
                  // hardware target covers the whole engine canvas, so map
                  // the camera-local coordinates back to the pane's absolute
                  // screen position and give the pane its own hardware frame.
                  lgd3d_set_offsets(screen_rect.ul.x, screen_rect.ul.y);
                  lgd3d_start_frame(vm_lgd3d_frame++);

                  // The editor's grids, brush outlines, and object markers
                  // are drawn into its legacy sub-canvas, so editor UI code
                  // normally leaves R3 using the software primitive table.
                  // Portal terrain bypasses that table and therefore still
                  // appeared through lgd3d, but MD/MM object polygons did
                  // not.  Select the hardware primitive table for the solid
                  // scene, then restore the editor's software table below.
                  r3_use_lgd3d();

                  // Game mode selects these pipelines when its render loop
                  // starts.  The editor renders the same portal scene
                  // directly, so establish the equivalent hardware state
                  // here for mesh objects (creatures and rigid fixtures).
                  set_mm_sort(!g_zbuffer);
                  g_MeshRenderFlags |= MMF_INDEXED;
               }
               cam_render_scene(&pos, (float)camera[c].zoom);
               if (g_lgd3d)
               {
                  if (hardware_grid)
                  {
                     int old_zwrite = lgd3d_is_zwrite_on();
                     int old_zcompare = lgd3d_is_zcompare_on();

                     // Re-establish the editor camera while the pane's
                     // hardware frame is live.  Disable depth comparison so
                     // the entire construction plane remains readable over
                     // the solid view, matching the editor's overlay model.
                     vm_start_3d(c);
                     r3_use_lgd3d();
                     lgd3d_set_fog_enable(FALSE);
                     lgd3d_set_blend(FALSE);
                     lgd3d_set_zcompare(FALSE);
                     lgd3d_set_zwrite(FALSE);
                     render_hardware_grid(c);
                     vm_end_3d(c);
                     lgd3d_set_zwrite(old_zwrite);
                     lgd3d_set_zcompare(old_zcompare);
                  }
                  lgd3d_end_frame();
                  lgd3d_set_offsets(0, 0);
                  r3_use_g2();
               }
               vm_start_3d(c);
            }
            else
               _vm_rend_message_2line("Current Portalization","Out of date");
         }

         if (mode & RM_WIREFRAME_BRUSHES)
         {
            gedrendRenderView(rv_flag, c);
            if (compiled_fallback)
               _vm_render_compiled_geometry(c);
         }

         if (show_raycasts)
            render_failures();   // ????????

         if (compiled_fallback && c == vm_cur_camera)
            _vm_rend_compiled_geometry_label(c);
         else if (_vm_name_corner!=-1)
            _vm_rend_message_corner(CORNER_RIGHT|CORNER_BOTTOM,"3d View");
      }
      else restore_bm=TRUE;
   }
   else
   {  // this is the 2d camera case, which neither camera nor select influences
      // how do we deal with it if the camera moves and we are synched...?
      if (redraw || (new_camera && camera[c].synch))
      {
         _vm_rend_start(c,show_grid);
         if (mode & RM_WIREFRAME_BRUSHES)
         {
            gedrendRenderView(rv_flag, c);
            if (compiled_fallback)
               _vm_render_compiled_geometry(c);
         }

         if (compiled_fallback && c == vm_cur_camera)
            _vm_rend_compiled_geometry_label(c);
         else
            _vm_rend_message_corner(_vm_name_corner,axis_names[camera[c].axis]);
      }
      else
         restore_bm=TRUE;
   }

   // This is here because it predates the modern line drawing system.
   AIPathFindDrawDB();

   if (restore_bm)
      _vm_restore_camera_bitmap(c) ; // get old bitmap save as background
   else
      _vm_save_camera_bitmap(c);     // here is where to save off "baseline" camera data

   if (!camera[c].camera_3d)
      _vm_show_camera_in2d(c);

   if (mode & (RM_WIREFRAME_BRUSHES | RM_WIREFRAME_CURRENT))
      gedrendRenderView(rv_flag | GEDREND_FLAG_CURSORONLY, c);

   vm_end_3d(c);
   gr_set_fcolor(c == vm_cur_camera ? view_select_color : view_grid_color );
   gr_box(0,0,grd_bm.w,grd_bm.h);
   vm_unset_region_canvas();
}

void vm_render_cameras(void)
{
   int i;

   if (!(redraw || new_select || new_camera || new_render))
      return;

   // i really really dont get this?
   // if we reportalized, we need to invalidate cached cell #s
   // ummmm.... right, so why do we do this each time?
   // and, while we are at it, why vm_cur_camera...
   for (i=0; i < MAX_CAMERAS; ++i) {
      UpdateChangedLocation(&camera[i].loc);    // these were both vm_cur_camera???
      UpdateChangedLocation(&camera[i].loc_2d); // not quite sure what is supposed to be happening here...
   }

   DrawRelations();
   update_links = FALSE;

   for (i=0; i < MAX_CAMERAS; ++i)
      vm_render_camera(i);

   update_links = TRUE;

   redraw = vmRenderAlways;
   new_select = new_camera = new_render = FALSE;
}

////////////////////////////////////////////////////////////
//
//  dealing with UI
//

static int warp_x_pos, warp_y_pos, warp_camera;
void vm_menu_warp_camera(void)
{
   mxs_vector *loc;
   mxs_angvec *fac;

   if (vm_spotlight_loc(&loc, &fac)) {
      mxs_vector temp;
      int a;
      a = vm_map_screen_to_world(warp_camera, &temp, warp_x_pos, warp_y_pos);

      loc->el[(a+1)%3] = temp.el[(a+1)%3];
      loc->el[(a+2)%3] = temp.el[(a+2)%3];
   }
}

void vm_swap_cameras_string(char *arg)
{
   int x,y;
   if (sscanf(arg, "%d%d", &x, &y) == 2) {
      if (x >= 0 && y >= 0 && x < MAX_CAMERAS && y < MAX_CAMERAS && x != y) {
         UPDATE_CAMERA(x);
         UPDATE_CAMERA(y);
         vm_swap_cameras(x,y);
      }
   } else
      Warning(("Invalid swap command.\n"));
}

void vm_move_grid(float x)
{
   redraw = TRUE;
   vm_3d_grid_height += x;
}

void vm_resynch_oneshot(void)
{
   vm_set_synch_all(TRUE);
   vm_redraw();
   vm_render_cameras();
   vm_set_synch_all(FALSE);
}

static void vm_teleport_cam(char *new_coords)
{
   mxs_vector loc;
   if (sscanf(new_coords,"%f,%f,%f",&loc.x,&loc.y,&loc.z)==3)
      vm_cur_loc_set(&loc);
   else
      Status("Couldnt parse location");
}

Command vm_commands[] =
{
   { "grid_3d", TOGGLE_BOOL, &vm_show_grid_3d, "toggle grid display in 3d" },
   { "grid_2d", TOGGLE_BOOL, &vm_show_grid, "toggle grid display in 2d" },
   { "grid_3d_axis", VAR_INT, &vm_3d_grid_axis, "3d grid normal axis #" },
   { "grid_move", FUNC_FLOAT, vm_move_grid, "translate 3d grid" },
   { "cycle_view", FUNC_BOOL, vm_cycle_camera, "change current camera" },
   { "toggle_3d", FUNC_VOID, vm_cur_toggle_3d, "change current 2d/3d mode" },
   { "zoom_all", FUNC_FLOAT, vm_scale_all, "zoom all 2d cameras" },
   { "zoom_2d", FUNC_FLOAT, vm_cur_scale, "zoom current camera" },
   { "synch_all", FUNC_BOOL, vm_set_synch_all, "set all 2d synch modes" },
   { "cam_swap", FUNC_STRING, vm_swap_cameras_string, "swap two cameras" },
   { "cycle_mode", FUNC_VOID, vm_cur_cycle_mode, "change rendering mode" },
   { "set_mode", FUNC_INT, vm_cur_set_mode, "set rendering mode" },
   { "toggle_synch", FUNC_VOID, vm_cur_toggle_synch, "toggle 2d synch mode" },
   { "toggle_persp", FUNC_VOID, vm_cur_toggle_persp, "toggle 3d perspective" },
   { "xmouse", TOGGLE_BOOL, &vm_xmouse, "auto-select camera under mouse" },
   { "cam_warp", FUNC_VOID, vm_menu_warp_camera, "warp camera to mouse loc" },
   { "toggle_mode", FUNC_INT, vm_cur_toggle_mode, "toggle a display aspect" },
   { "solo_toggle", FUNC_VOID, vmToggleGUIFull, "toggle this view being soloed" },
   { "redraw_always", TOGGLE_BOOL, &vmRenderAlways, "does the editor continually redraw" },
   { "vm_refresh", FUNC_VOID, vm_redraw, "redraw all views" },
   { "quick_resynch", FUNC_VOID, vm_resynch_oneshot, "synch; redraw; then desynch" },
   { "vm_win_mode", FUNC_INT, vmWinCfgSetMode, "wincfg set mode" },
   { "vm_layout", FUNC_INT, vmWinCfgCycleLayout, "cycle/set vm win layout" },
   { "vm_name_corner", VAR_INT, &_vm_name_corner, "set corner for names (-1 no draw)" },
   { "vm_teleport", FUNC_STRING, vm_teleport_cam, "move camera to x,y,z" },
};

void vm_init(void)
{
   COMMANDS(vm_commands, HK_EDITOR);
}

void vm_term(void)
{  // is this too late, ie. Malloc/Free is already dead?  i dont think so
   _vm_camera_saves_term();
   _vm_free_compiled_geometry_cache();
}

void vm_suspend(void)
{
   // Free the camera saves
   _vm_camera_saves_term();
}

void vm_resume(void)
{
   // A display-mode change (including a window resize) destroys and rebuilds
   // the viewport regions.  Never restore a bitmap captured for the old
   // regions into the new canvas; force a clean redraw at the new size.
   _vm_camera_saves_term();
   vm_redraw();
}

  // build a popup-menu for a given camera
void vm_get_popup_menu(int cam, char ***names, char ***command, int *count, int x, int y)
{
   extern void vm_mouse_relativize(int c, int *x, int *y);
   static char *command_list[32];
   static char *name_list[32];

   int i=0, c3 = camera[cam].camera_3d;
//   int persp = camera[cam].perspective;  // unloved by the people
   int cs = camera[cam].synch;

     // HACK: save away the location of the mouse and the
     // camera info in case it turns out to be cam_warp

   warp_x_pos = x;
   warp_y_pos = y;
   warp_camera = cam;
   vm_mouse_relativize(cam, &warp_x_pos, &warp_y_pos);

#define ADD(x,y)   (name_list[i]=(x),command_list[i]=(y),++i)
#define MULTI_NAME(w,x,y,z) ADD(((w) ? (x) : (y)), (z))

   if (c3)  MULTI_NAME(vm_show_grid_3d, "hide grid", "show grid", "grid_3d");
   if (!c3) MULTI_NAME(vm_show_grid,    "hide grid", "show grid", "grid_2d");

   if (!c3) ADD("teleport camera", "cam_warp");

   MULTI_NAME(vmSoloed, "un-solo", "solo view", "solo_toggle");

   // no one seems to love these
   //   MULTI_NAME(c3, "2d view", "3d view", "toggle_3d");
   //   if (c3)  MULTI_NAME(persp, "isometric", "perspective", "toggle_persp");

   if (c3) ADD("wireframe brushes", "set_mode 1");
   if (c3) ADD("solid world", "set_mode 2");
   if (c3) ADD("solid + selection", "set_mode 6");
   if (c3) ADD("solid + all wireframe", "set_mode 3");

   if (!c3 &&  cs) ADD("asynch all", "synch_all false");
   if (!c3 && !cs) ADD( "synch all", "synch_all true");

   if (!c3) ADD("zoom in", "zoom_2d 0.5");
   if (!c3) ADD("zoom out", "zoom_2d 2.0");

   if (!c3) ADD("zoom in all", "zoom_all 0.5");
   if (!c3) ADD("zoom out all", "zoom_all 2.0");

   if (!c3) ADD("fit to region", "fit_cameras false");

#undef ADD
#undef MULTI_NAME

   *names = name_list;
   *command = command_list;
   *count = i;
}

////////////////////////////////////////////////////////////
//
//  accessing methods
//

bool        vm_get_3d      (int c) { return camera[c].camera_3d; }
mxs_vector *vm_get_location(int c) { return &camera[c].loc.vec; }
mxs_angvec *vm_get_facing  (int c) { return &camera[c].ang; }
mxs_real    vm_get_zoom    (int c) { return camera[c].zoom; }
mxs_real    vm_get_scale   (int c) { return camera[c].scale; }
mxs_vector *vm_get_loc_2d  (int c) { return &camera[c].loc_2d.vec; }
bool        vm_get_synch   (int c) { return camera[c].synch; }
int         vm_get_mode    (int c) { return camera[c].render_mode; }

void vm_set_3d(int c, bool flag)
{
   UPDATE_CAMERA(c);
   camera[c].camera_3d = flag;
}

void vm_set_render_mode(int c, uchar mode)
{
   UPDATE_CAMERA(c);
   camera[c].render_mode = mode; // no longer force it to be valid - so it doesnt make me want to kill it
}

void vm_set_location(int c, mxs_vector *loc)
{
   UPDATE_CAMERA(c);
   if (camera[c].camera_3d)
   {
      camera[c].loc.vec = *loc;
      UpdateChangedLocation(&camera[c].loc);
   }
   else
   {
      camera[c].loc_2d.vec = *loc;
      UpdateChangedLocation(&camera[c].loc_2d);
   }
}

void vm_set_facing(int c, mxs_angvec *fac)
{
   UPDATE_CAMERA(c);
   camera[c].ang = *fac;
}

void vm_set_zoom(int c, mxs_real zoom)
{
   UPDATE_CAMERA(c);
   camera[c].zoom = zoom;
}

void vm_set_perspective(int c, bool flag)
{
   UPDATE_CAMERA(c);
   camera[c].perspective = flag;
}

void vm_set_scale(int c, mxs_real scale)
{
   UPDATE_CAMERA(c);
   camera[c].scale = scale;
}

void vm_set_axis(int c, int a)
{
   UPDATE_CAMERA(c);
   camera[c].axis = a;
}

void vm_set_synch(int c, bool flag)
{
   UPDATE_CAMERA(c);
   if (camera[c].synch && !flag) {
      int rc = synch_camera();
      if (rc >= 0) {
         camera[c].loc_2d = camera[rc].loc;
      }
   }
   camera[c].synch = flag;
}
