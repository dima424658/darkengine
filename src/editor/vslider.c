/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/vslider.c,v 1.10 2000/02/19 13:13:45 toml Exp $
// arbitrary axis virtual slider gadgets

#include <stdlib.h>

#include <lg.h>
#include <2d.h>

#include <event.h>
#include <mouse.h>
#include <kbcook.h>

#include <mprintf.h>

#define __VSLIDER_C
#include <modalui.h>
#include <vslider.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static int last_x, last_y;
static float mouse_sens=1.0;

#define ismouse(ev) ((ev).type & (UI_EVENT_MOUSE | UI_EVENT_MOUSE_MOVE))
#define iskbd(ev)   ((ev).type == UI_EVENT_KBD_COOKED)

static short _vs_mouse_x, _vs_mouse_y;
static int   _vs_scr_wid, _vs_scr_hgt;

void _vslider_mouse_recenter(void)
{
   mouse_put_xy((short)(_vs_scr_wid/2), (short)(_vs_scr_hgt/2));
}

void _vslider_mouse_start(void)
{
   uiHideMouse(NULL);
   _vs_scr_wid=grd_visible_canvas->bm.w;
   _vs_scr_hgt=grd_visible_canvas->bm.h;
   mouse_get_xy(&_vs_mouse_x,&_vs_mouse_y);
   _vslider_mouse_recenter();
}

void _vslider_mouse_end(void)
{
   uiShowMouse(NULL);
   mouse_put_xy(_vs_mouse_x,_vs_mouse_y);
}

int _vslider_mouse_get(uiMouseEvent *mev, float *vals)
{
   float dist[2];
   if ((mev->pos.x<0)||(mev->pos.x>=_vs_scr_wid)||
       (mev->pos.y<0)||(mev->pos.y>=_vs_scr_hgt))
      dist[0]=dist[1]=0;
   else
   {
      dist[0]=((float)(mev->pos.x-(_vs_scr_wid/2)))/(float)(_vs_scr_wid/2);
      dist[1]=((float)(mev->pos.y-(_vs_scr_hgt/2)))/(float)(_vs_scr_hgt/2);
   }
   vals[0]=dist[0]*mouse_sens;
   vals[1]=dist[1]*mouse_sens;
   _vslider_mouse_recenter();
   return 2;         // mouse has 2 valid axis
}

void virtual_slider_mouse_start(void)
{
   last_x=-1;
   last_y=-1;
}

#define ForAllRealSliderAxis(exp,n) \
   for (i=0; i<min(n_axis,n); i++)  \
      if (axis[i]!=NULL) exp

#define parsefunc(i) (slds[i]->parse)

int virtual_slider(int n_axis, void **axis, void (*update_world)(void))
{
   bool          change=FALSE,in_loop=TRUE;  // we set this to false on up or ESC
   uiMouseEvent *use_event;                  // null if no event to process this frame
   uiEvent       ev;
   anonSlider  **slds=(anonSlider **)axis;
   int           i, rcode=VSLIDER_CHANGED;   // assume we are going to change

   if (!modal_ui_start(MODAL_CHECK_ALL)) return VSLIDER_MOUSEUP;
   _vslider_mouse_start();

   ForAllRealSliderAxis( (*parsefunc(i))(0.0,axis[i],SLIDER_START), n_axis );

   while (in_loop)
   {
      use_event=NULL;
      modal_ui_update();
      while (in_loop&&modal_ui_get_event(&ev))
      {
         if (ismouse(ev))
         {
            use_event=(uiMouseEvent *)&ev;
            if (use_event->action & (MOUSE_LUP|MOUSE_RUP))
               in_loop=FALSE;
         }
         else if (iskbd(ev))
         {
            uiCookedKeyEvent *kev = (uiCookedKeyEvent *) &ev;
            if (kev->code == (27 | KB_FLAG_DOWN))
            {
               ForAllRealSliderAxis(
                  (*parsefunc(i))(0.0,axis[i],SLIDER_ABORT), n_axis );
               use_event=NULL;
               in_loop=FALSE;
               rcode=VSLIDER_ABORTED;  // so we return ABORT, not SAMEVAL
            }
         }
      }
      if (use_event)
      {
         bool update=FALSE; float inputs[6]; int n_inp;
         
         n_inp=_vslider_mouse_get(use_event,inputs);
         ForAllRealSliderAxis(
            update|=(*parsefunc(i))(inputs[i],axis[i],SLIDER_SLIDE), n_inp );
         if (update) (*update_world)();
      }
   }
   ForAllRealSliderAxis(change|=(*parsefunc(i))(0.0,axis[i],SLIDER_END),n_axis);
   _vslider_mouse_end();
   modal_ui_end();
   if ((rcode==VSLIDER_CHANGED)&&!change)
      rcode=VSLIDER_SAMEVAL;   // we didnt abort, but we didnt change the values
   return rcode;
}
