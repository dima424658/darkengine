/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/dragbox.c,v 1.53 2000/02/19 12:27:56 toml Exp $
// mouse drag box for brush creation

#include <stdlib.h>
#include <math.h>

#include <2d.h>
#include <lg.h>
#include <mprintf.h>
#include <r3d.h>
#include <event.h>
#include <kbcook.h>
#include <config.h>

#include <editbr.h>
#include <editbr_.h>
#include <brinfo.h>
#include <brlist.h>
#include <gedit.h>
#include <status.h>
#include <brundo.h>

#include <viewmgr.h>
#include <viewmgr_.h>
#include <modalui.h>
#include <vslider.h>
#include <brushgfh.H>
#include <vbrush.h>
#include <guistyle.h>
#include <ccompose.h>
#include <curdat.h>

#include <gridsnap.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

extern void vm_mouse_relativize(int, int *, int *);

mxs_vector gb_start, gb_end;
static Rect gb_drag_box;
static bool gb_drag_from_center;
static int gb_cam;
static bool gb_currently_drawn;

static editBrush *dragbox_br;

// Drag scroll code
static bool oldsynch;
//static bool oldgrid;
static int oldmode;
static int oldcam;
static void drag_setup(int c)
{
   oldcam=vm_current_camera();
   oldmode=vm_get_mode(c);
   //oldgrid=vm_show_grid;
   oldsynch = vm_get_synch(c);

   vm_set_cur_camera(c);
   vm_set_render_mode(c,RM_WIREFRAME_BRUSHES);
   //vm_show_grid=FALSE;     // for now, show current brush, grid, and camera only
   vm_set_synch(c,FALSE); // desynch for scrolling
}

static void drag_cleanup(int c)
{
   //vm_show_grid=oldgrid;
   vm_set_render_mode(c,oldmode);
   vm_set_synch(c,oldsynch);
   vm_set_cur_camera(oldcam);
   vm_redraw();
}

// ensures that x,y is on screen
static void drag_scroll(int c, int x, int y)
{
   mxs_vector delta;
   mxs_vector curloc;
   int w,h;
   float dist;
   mxs_vector xvec,yvec;

   // find out where the edges of the screen are, so to figure whether the
   // mouse is onscreen or not
   vm_camera_scr_size(c, &w, &h);
   delta.x = delta.y = delta.z = 0;
   //vm_get_camera_axis(c, &xaxis, &yaxis);
   vm_screen_axes(c, &xvec,&yvec);
   //mprintf("(%d,%d) vs (%d,%d)\n",x,y,w,h);
   if (x < 0)
   {
      dist = x * 1.0F;
      if (dist > 5.0F)
         dist = 5.0F;
      gb_drag_box.ul.x -= (int)dist;
      mx_scale_addeq_vec(&delta,&xvec,dist);
   }
   if (y < 0)
   {
      dist = y * 1.0F;
      if (dist > 5.0F)
         dist = 5.0F;
      gb_drag_box.ul.y -= (int)dist;
      mx_scale_addeq_vec(&delta,&yvec,dist);
   }
   if (x > w)
   {
      dist = (x-w) * 1.0F;
      if (dist > 5.0F)
         dist = 5.0F;
      gb_drag_box.ul.x -= (int)dist;
      mx_scale_addeq_vec(&delta,&xvec,dist);
   }
   if (y > h)
   {
      dist = (y-h) * 1.0F;
      if (dist > 5.0F)
         dist = 5.0F;
      gb_drag_box.ul.y -= (int)dist;
      mx_scale_addeq_vec(&delta,&yvec,dist);
   }
   
   if (vm_get_3d(c))
    { Status("Cant scroll 3d cameras"); return; }

   curloc =*vm_get_loc_2d(c);
   //mprintf("curloc = %g, %g, %g\n",curloc.x,curloc.y,curloc.z);

   mx_addeq_vec(&curloc,&delta);
   vm_set_location(c,&curloc);
   vm_render_camera(c);

}

void brush_vs_update(void)
{
   GFHUpdate(GFH_MINIMAL);
   gedit_raw_change_brush(dragbox_br,FALSE,FALSE,FALSE);
   vm_render_cameras();
}

  // start drawing a box with a click in any TL corner
void gb_start_box_corner(int camera, int x, int y)
{
   gb_cam = camera;
   gb_drag_box.ul.x = x;
   gb_drag_box.ul.y = y;
   gb_drag_box.lr.x = x;
   gb_drag_box.lr.y = y;

   gb_currently_drawn = FALSE;
   gb_drag_from_center = FALSE;

   vm_map_screen_to_world(camera, &gb_start, x, y);      
}

  // start drawing a box with a click at the center
void gb_start_box_center(int camera, int x, int y)
{
   gb_cam = camera;
   gb_drag_box.ul.x = x;
   gb_drag_box.ul.y = y;
   gb_drag_box.lr.x = x;
   gb_drag_box.lr.y = y;

   gb_currently_drawn = FALSE;
   gb_drag_from_center = TRUE;

   vm_map_screen_to_world(camera, &gb_start, x, y);      
}


void gb_box(void)
{
   int x0,y0,x1,y1;

   x0 = gb_drag_box.ul.x;
   y0 = gb_drag_box.ul.y;

   x1 = gb_drag_box.lr.x;
   y1 = gb_drag_box.lr.y;

   if (gb_drag_from_center) {
      x0 = x0*2 - x1;
      y0 = y0*2 - y1;
   }

   if (x0 > x1) { int temp = x0; x0 = x1; x1 = temp; }
   if (y0 > y1) { int temp = y0; y0 = y1; y1 = temp; }

   MouseLock++;
   compose_cursor(CURSOR_UNDRAW); 

   gr_box(x0,y0,x1,y1);

   compose_cursor(CURSOR_DRAW);
   MouseLock--;


}

void gb_update_box(int camera)
{
   int old_fill;
   bool draw_new;

   if (abs(gb_drag_box.lr.x-gb_drag_box.ul.x) < 2
        && abs(gb_drag_box.lr.y-gb_drag_box.ul.y) < 2)
      draw_new = FALSE;
   else
      draw_new = TRUE;

   if (gb_currently_drawn || draw_new) 
   {
      vm_set_region_canvas(camera);
      old_fill = gr_get_fill_type();
      gr_set_fill_type(FILL_XOR);
      gr_set_fcolor(guiStyleGetColor(NULL,StyleColorWhite)); 

      gb_box();

      gr_set_fill_type(old_fill);
      vm_unset_region_canvas();
      gb_currently_drawn = !gb_currently_drawn;
   }
}

#define DEF_DRAG_TOLERANCE 3

int gb_end_box(int camera)
{
   int drag_tol=DEF_DRAG_TOLERANCE;
   config_get_int("drag_tol",&drag_tol);

   if (config_is_defined("drag_scroll"))
      drag_cleanup(camera);

   if (gb_currently_drawn)
      gb_update_box(camera);

   if (abs(gb_drag_box.lr.x-gb_drag_box.ul.x) <= drag_tol ||
       abs(gb_drag_box.lr.y-gb_drag_box.ul.y) <= drag_tol)
      return -1;
   else {
      return vm_map_screen_to_world(camera, &gb_end,
                           gb_drag_box.lr.x, gb_drag_box.lr.y);
   }
}

uiMouseEvent *gb_mouse_event(uiEvent *ev, int camera)
{
   uiMouseEvent *mev = (uiMouseEvent *) ev;
   int x,y;
   x = mev->pos.x;
   y = mev->pos.y;
   vm_mouse_relativize(camera, &x, &y);
   if (!gb_currently_drawn) {
      gb_drag_box.lr.x = x;
      gb_drag_box.lr.y = y;
   }
   if (config_is_defined("drag_scroll"))
      drag_scroll(camera, x, y);
   mev->pos.x = x;
   mev->pos.y = y;
   return mev;
}

#define ismouse(ev) ((ev).type & (UI_EVENT_MOUSE | UI_EVENT_MOUSE_MOVE))
#define iskbd(ev)   ((ev).type == UI_EVENT_KBD_COOKED)

void select_brush(int camera, int x, int y);

#define is_Geom(type) \
 ((type==brType_TERRAIN)||(type==brType_ROOM)|| \
   (type==brType_FLOW)||(type==brType_HOTREGION))

static BOOL typeCompat(int old_type, int new_type)
{
   if (old_type==new_type)
      return TRUE;
   if (is_Geom(old_type)&&is_Geom(new_type))
      return TRUE;
   return FALSE;
}

void finishNewBrush(int xtra_axis, mxs_vector *p0, mxs_vector *p1)
{
   int i, new_type=gedit_default_brush_type();
   editBrush *us=blistGet(), *tmp;
   if (new_type==brType_TERRAIN)
      tmp=brushInstantiate(DEF_PRIMAL);
   else
      tmp=brushInstantiate(0); // we arent terrain, so primal 0
   for (i=0; i<3; i++)
      if (xtra_axis==i)    // bonus axis, look at current brush
      {
         if ((us!=NULL)&&typeCompat(brushGetType(us),new_type))
         {                 // broken if current is not our type!
            tmp->sz.el[i]=us->sz.el[i];
            tmp->pos.el[i]=us->pos.el[i];
         }
         else
         {
            tmp->sz.el[i]=1.0;
            if (us!=NULL)
               tmp->pos.el[i]=us->pos.el[i];            
         }
      }
      else
      {
         tmp->sz.el[i]=fabs(p0->el[i]-p1->el[i])/2.0;
         tmp->pos.el[i]=(p0->el[i]+p1->el[i])/2;
      }
   gedit_full_create_brush(tmp,us,GEDIT_CREATE_AT_END,new_type);
   vBrush_NewBrush(tmp);
}

#define DRAGOP_ABORT (-1)
#define DRAGOP_NOP   ( 0)
#define DRAGOP_DIDIT ( 1)

// -1 abort, 0 no brush, 1 new brush
int do_modal_drag_create(int camera, int x, int y)
{
   int rv;
   uiEvent ev;

   if (!modal_ui_start(MODAL_CHECK_LEFT))
      return DRAGOP_ABORT;

   Status("Drag to create; ESC to abort");
   gb_start_box_corner(camera, x, y);
   if (config_is_defined("drag_scroll"))
      drag_setup(camera);

   for(;;) {
      modal_ui_update();

      while (modal_ui_get_event(&ev)) {
         if (ismouse(ev)) {
            uiMouseEvent *mev = gb_mouse_event(&ev, camera);
            if (mev->action & MOUSE_LUP)
               goto mouse_up;
         } else if (iskbd(ev)) {
            uiCookedKeyEvent *kev = (uiCookedKeyEvent *) &ev;
            if (kev->code == (27 | KB_FLAG_DOWN))
               goto esc;
         }
      }
      gb_update_box(camera);
   }

mouse_up:
   rv = gb_end_box(camera);
   if (rv >= 0)
      finishNewBrush(rv, &gb_start, &gb_end);
   modal_ui_end();
   return rv >= 0 ? DRAGOP_DIDIT: DRAGOP_NOP;

esc:
   gb_end_box(camera);
   modal_ui_end();
   return DRAGOP_ABORT;
}

#define CAM_SCALE_FAC (5.0)

static mxs_vector vb_delta;
mxs_vector *get_vbrush_delta(editBrush *us)
{
   if (brSelect_Flag(us)==brSelect_VIRTUAL)
   {
      editBrush *focus_br=vBrush_GetSel();
      mx_sub_vec(&vb_delta,&focus_br->pos,&us->pos);
   }
   else
      mx_zero_vec(&vb_delta);
   return &vb_delta;
}

static void keep_val_in_range(floatSlider *s)
{
   if ((*s->val)<s->lo) s->lo=*s->val;
   if ((*s->val)>s->hi) s->hi=*s->val;
}

// TODO: these compute s->scale idiotically
void *brushVSsetupCameraPos(int camera, floatSlider *s, editBrush *us, int axis, float scale)
{
   mxs_vector ul, lr, *delta;
   int w,h;
   
   VSliderfloatBuild(s);
   s->val=&us->pos.el[axis];
   vm_camera_scr_size(camera, &w, &h);
   vm_map_screen_to_world(camera,&ul,0,0);
   vm_map_screen_to_world(camera,&lr,w-1,h-1);
   delta=get_vbrush_delta(us);
   if (config_is_defined("drag_unbounded"))
   {
      s->lo = s->hi = 0;
      // goddamn this number is bogus
      s->scale*=100*scale;
   }
   else
   {
      s->lo=ul.el[axis]-delta->el[axis];
      s->hi=lr.el[axis]-delta->el[axis];
      //   s->scale*=CAM_SCALE_FAC*(s->hi-s->lo)/(float)w;
      s->scale*=6*scale;
      if (s->lo>s->hi)
       { float tmp=s->hi; s->hi=s->lo; s->lo=tmp; }
      //   mprintf("set %g (old %g) (from %g %g %d)\n",s->scale,6*scale,s->lo,s->hi,w);
      // ok, these need to be constrained to keep the val in them
      keep_val_in_range(s);
   }
   return (void *)s;
}

void *brushVSsetupPos(floatSlider *s, editBrush *us, int axis, float scale)
{
   VSliderfloatBuild(s);
   s->val=&us->pos.el[axis];
   if (config_is_defined("drag_unbounded"))
   {
      s->lo = s->hi = 0;
      // goddamn this number is bogus
      s->scale*=100*scale;
   }
   else
   {
      // ok, these need to be constrained to keep the val in them
      s->lo=us->pos.el[axis]-128*fabs(scale);  // should pick based on current zoom
      s->hi=us->pos.el[axis]+128*fabs(scale);
      s->scale*=6*scale;
      keep_val_in_range(s);
   }
   return (void *)s;
}

void *brushVSsetupSz(floatSlider *s, editBrush *us, int axis, float scale)
{
   VSliderfloatBuild(s);
   s->val=&us->sz.el[axis];
   if (config_is_defined("drag_unbounded"))
   {
      s->lo = s->hi = 0;
      // goddamn this number is bogus
      s->scale*=100*scale;
   }
   else
   {
      s->lo=0.0;
      s->hi=128*fabs(scale);
      s->scale*=6*fabs(scale);
      keep_val_in_range(s);
   }
   return (void *)s;
}

void *brushVSsetupAng(fixangSlider *s, editBrush *us, int axis)
{
   VSliderfixangBuild(s);
   s->val=&us->ang.el[axis%3];
   s->lo=0;
   s->hi=0;
   s->scale*=65536.0/32.0;     // ???????
   if (axis>=3) s->scale*=-1;
   return (void *)s;
}

#define SHIFT_STATES (KBM_LSHIFT|KBM_RSHIFT)
#define CTRL_STATES  (KBM_LCTRL|KBM_RCTRL)
#define ALT_STATES   (KBM_LALT|KBM_RALT)
#define ALL_STATES   (SHIFT_STATES|CTRL_STATES|ALT_STATES)

// in killme.c.  should fix this to be right
extern int hack_for_kbd_state(void);

// reads and returns DRAGOP_ codes
static int check_create(int camera, int x, int y)
{
   int kb_mods=hack_for_kbd_state();

   if ((kb_mods&ALL_STATES)==0)
   {
      int rv=do_modal_drag_create(camera,x,y);
      Status("");
      if (rv==DRAGOP_DIDIT)
         i_changed_the_brush(blistGet(),TRUE,TRUE,FALSE);
      return rv;
   }
   return DRAGOP_NOP;
}

// called for down events in move mode, returns whether to select
int do_brush_virtual_slider(int camera, int x, int y, bool on_axis)
{
   editBrush *us=vBrush_editBrush_Get();
   int i,j,axis[2]={-1,-1};
   float scale[2];
   void *sliders[2];
   floatSlider axsliders[2];
   fixangSlider angslider[2];
   mxs_vector axv[2];
   BOOL do_update=FALSE;
   BOOL move_mode = FALSE;
   int num_axis=1;      // default to one axis
   int kb_mods=hack_for_kbd_state();
   int vslider_ret;

   if ((kb_mods&ALL_STATES)==0)
      return DRAGOP_NOP;

   if (kb_mods&ALT_STATES)
   {
      if ((axis[0]=vm_normal_axis(camera))==-1)
         return DRAGOP_NOP;        // not in a 2d mode, this is a problem
      Status("Drag to rotate brush");
      if (!on_axis)
      {
         num_axis=2;
         sliders[0]=brushVSsetupAng(&angslider[0],us,(axis[0]+1)%6);
         sliders[1]=brushVSsetupAng(&angslider[1],us,(axis[0]+2)%6);
      }
      else
      {
         num_axis=1;
         sliders[0]=brushVSsetupAng(&angslider[0],us,axis[0]);
      }
   }
   else
   {  // returns the size, in world space, of a pixel
      if (!vm_screen_axes(camera,&axv[0],&axv[1]))   // hmmm
         return DRAGOP_NOP;  // was abort   // not in a 2d mode, that is a problem
      else                                   // interp axis
      {
         if (!on_axis)
         {
            num_axis=1;
            axis[0]=vm_normal_axis(camera);
            scale[0]=mx_mag_vec(&axv[0]);
         }
         else
         {
            num_axis=2;
            for (i=0; i<num_axis; i++)
            {
               mxs_vector *v=&axv[i];
               for (j=0; j<3; j++)
                  if (v->el[j]!=0)
                     break;
               if (j<3)
               {
                  axis[i]=j;
                  scale[i]=v->el[j];  // want to multiply by size of region, right?
               }
            }
         }
      }
      if (kb_mods&CTRL_STATES) {
         Status("Drag to stretch brush");
         for (i=0; i<num_axis; i++)
            sliders[i]=brushVSsetupSz(&axsliders[i],us,axis[i],scale[i]);
      } else {
         Status("Drag to move brush");
         move_mode = TRUE;
         for (i=0; i<num_axis; i++)
            sliders[i]=brushVSsetupCameraPos(camera,&axsliders[i],us,axis[i],scale[i]);
      }
   }  // now lets actually go do it
   editUndoStoreStart(us);
   // here is the slow setup thing, where the mouse might go up, really
   // @BUG: up event happens during redraw, then vslide punts, then we return NOP and select...
   gedit_setup_check_lazy_drag();
   dragbox_br=us;
   vslider_ret=virtual_slider(num_axis,sliders,brush_vs_update);
   if (vslider_ret==VSLIDER_CHANGED)
   {
      do_update=TRUE;
      i_changed_the_brush(us,do_update,FALSE,FALSE);
   }
   gedit_finish_check_lazy_drag();
   editUndoStoreBrush(us);
   Status("");
   
   if (vslider_ret==VSLIDER_ABORTED)
      return DRAGOP_ABORT;                       // abort means punt the event
   else
      return do_update?DRAGOP_DIDIT:DRAGOP_NOP;  // NOP means treat it as select
}

void select_brush(int camera, int x, int y)
{
   int kb_mods=hack_for_kbd_state(), flags=0;
   if (vm_get_3d(camera))         flags|=GEDIT_CLICK_IS3D;
   if ((kb_mods&SHIFT_STATES)!=0) flags|=GEDIT_CLICK_VBRUSH;
   if ((kb_mods&ALT_STATES)!=0)   flags|=GEDIT_CLICK_FANCY;
   if (gedit_click(flags,x,y,camera))
      Status("Selected brush");
   else
      Status("No brush found");
}
   
// returns FALSE if it wants the caller to do something with the event
bool brush_edit_mouse(uiMouseEvent *mev, int camera, int x, int y)
{
   if (mev->action&UI_MOUSE_LDOUBLE)
      select_brush(camera, x, y);
   else if (mev->action & (MOUSE_RDOWN|MOUSE_LDOWN))
   {
      bool left_button=mev->action&MOUSE_LDOWN;
      if (left_button)                 // left can do more stuff
         if (check_create(camera,x,y)!=DRAGOP_NOP) // NOP, so try vslide or select
            return TRUE;                 //  else we did or aborted, so we are done
      if (do_brush_virtual_slider(camera,x,y,left_button)==DRAGOP_NOP)
         if (mev->action&MOUSE_RDOWN)  // right button slides or menus
            return FALSE;       // go do the menu in viewmanager on right button
         else                          // the left buttons case can do a select
            select_brush(camera, x, y);
   }
   return TRUE;
}
