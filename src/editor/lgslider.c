/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/*
 * $Source: r:/t2repos/thief2/src/editor/lgslider.c,v $
 * $Revision: 1.3 $
 * $Author: toml $
 * $Date: 2000/02/19 13:10:54 $
 *
 * Implements a pretty standard cup/handle type of slider gadget built on top of
 * the LGadToggle gadget.  Movement can be either horizontal or vertical and can
 * be quantized. No 'nudge' capability exists, 'cause I don't need it yet.
 *
 */

// library includes
#ifdef _WIN32
#include <comtools.h>
#include <appagg.h>
#include <dispapi.h>
#include <mprintf.h>
#endif

#undef BTYPE

#include <event.h>
#include <gadget.h>
#include <lgslider.h>
#include <hotkey.h>
#include <mouse.h>
#include <string.h>
#include <lgsprntf.h>
#include <keydefs.h>
#include <math.h>
#include <limits.h>
#include <config.h>
#include <cfg.h>
#include <rect.h>
#include <cfgdbg.h>
#include <fixreal.h> // hmmm will need to deal with this eventually

// our gadget includes
#include <drawelem.h>
#include <util2d.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

extern short lgad_pal_blacks[];
extern short lgad_pal_text[];
extern int root_paltype;

// Are we doing 16 bit stuff?
extern bool lgad_use_pal16;
extern Id lgad_pal_id;

extern void LGadInitToggle(LGadToggle *vt);
extern void LGadComputeVarSize(LGadToggle *vt, short *wp, short *hp);
extern void LGadComputeSize(LGadButton *vbutt, short *wp, short *hp);

static void SliderChangeValue( LGadSlider* vs )
{
   long newval;

   // Quantize the value
   // (the factor of 2 is to handle the case where increm == 1 )
   newval = (long)vs->max_val * 2 * vs->handlePos / vs->screenRange;
   newval = ((newval + vs->increm) /  (vs->increm*2)) * vs->increm;

   if ( *vs->val_ptr != newval )
   {
      *vs->val_ptr = newval;
      if (vs->buttonfunc)
	 vs->buttonfunc(0, NULL, (LGadBox*)vs);

   }
   
}

static void SliderSetGrab( LGadSlider* vs, short x, short y)
{
   Rect* r;

   r = (BOX_RECT(VB(vs)));

   x = x - r->ul.x;  // to compensate for position of gadget
   y = y - r->ul.y;

   if ( vs->orient == LGSLIDER_HORIZONTAL )
   {
      vs->grabPos = x - vs->handlePos;
   }
   else
   {
      vs->grabPos = y - vs->handlePos;
   }

}

static void SliderMoveHandle(LGadSlider* vs, short x, short y)
{
   Rect* r;

   r = (BOX_RECT(VB(vs)));

   x = x - r->ul.x;  // to compensate for position of gadget
   y = y - r->ul.y;

   if ( vs->orient == LGSLIDER_HORIZONTAL )
   {
      vs->handlePos = x;
      vs->grabPos   = vs->handleSpan / 2;
   }
   else
   {
      vs->handlePos = y;
      vs->grabPos   =   vs->handleSpan / 2;
   }

}

static bool PointOverHandle( LGadSlider* vs, short x, short y)
{
   bool retval = TRUE;
   Rect* r;

   r = (BOX_RECT(VB(vs)));

   if ( !RECT_TEST_PT(r, MakePoint(x,y) ))
      retval = FALSE;

   x = x - r->ul.x;  // to compensate for position of gadget
   y = y - r->ul.y;

   if ( vs->orient == LGSLIDER_HORIZONTAL )
   {
      if ( ( x < vs->handlePos ) || (x > vs->handlePos + vs->handleSpan ))
	 retval = FALSE;
   }
   else
   {
      if ( ( y < vs->handlePos ) || (y > vs->handlePos + vs->handleSpan ))
	 retval = FALSE;
   }

   return retval;
}


bool SliderMotionHandler(short x, short y, LGadBox *vb)
{
   LGadSlider *vs;
   Rect *r;
   bool retval = FALSE;

   vs = (LGadSlider *)vb;

   if (vs->state == LGSLIDER_SELECTED)
   {
      short closeval, closepos, notch;

      r = (BOX_RECT(vb));

      x = x - r->ul.x;  // to compensate for position of gadget
      y = y - r->ul.y;

      if ( vs->orient == LGSLIDER_HORIZONTAL )
	 vs->handlePos = x - vs->grabPos;
      else
	 vs->handlePos = y - vs->grabPos;

      if ( vs->handlePos > vs->screenRange )
         vs->handlePos = vs->screenRange;
         
      if ( vs->handlePos < 0 )
         vs->handlePos = 0;

      // Here is a little goofy bit:  If the handle, while being dragged, is
      // close to a 'detent', then move it there, just for feel.

//      pixPerNotch = (long)vs->handleRange * vs->increm / vs->max_val; 

      closeval = (long)vs->max_val * 2 * vs->handlePos / vs->screenRange;
      notch    = ((closeval + vs->increm) /  (vs->increm*2));

      closepos = ( (long)vs->screenRange * vs->increm / vs->max_val ) * notch;

      if ( abs(closepos - vs->handlePos) < vs->handleSpan)
	 vs->handlePos = closepos;

      if ( vs->handlePos != vs->oldHandlePos )
      {
	 SliderChangeValue( vs );
	 vs->oldHandlePos = vs->handlePos;
	 LGadDrawBox(vb,NULL); // redraw with new value
      }
	 
      retval = TRUE;

   }
   else  // is not selected
   {
      if ( PointOverHandle( vs, x, y ))
      {
	 if ( vs->state != LGSLIDER_HIGHLIT )
	 {
	    vs->state = LGSLIDER_HIGHLIT;
	    LGadDrawBox(vb,NULL); // redraw with new value
	    uiGrabFocus(LGadBoxRegion(vs),UI_EVENT_MOUSE_MOVE);
	 }
	 retval = TRUE; 
      }
      else
      {
	 if ( vs->state != LGSLIDER_DEFAULT )
	 {
	    vs->state = LGSLIDER_DEFAULT;
	    LGadDrawBox(vb,NULL); // redraw with new value
	    uiReleaseFocus(LGadBoxRegion(vs),UI_EVENT_MOUSE_MOVE);
	 }
      }

      // don't eat the event

   }

   return(retval);
}

bool SliderMouseHandler(short x, short y, short action, short wheel, LGadBox *vb)
{
   LGadSlider *vs;
   bool retval = FALSE;

   vs = (LGadSlider *)vb;
   // compensate for internal drawelement stuff

   if (action & MOUSE_LDOWN )
   {
      vs->state = LGSLIDER_SELECTED;

      if ( PointOverHandle( vs, x,y ) )
	 SliderSetGrab(vs,x,y);
      else // move the handle over to the mouse pointer
	 SliderMoveHandle(vs,x,y);

      LGadDrawBox(vb,NULL); // redraw (in case the up/down art is different)
      uiGrabFocus(LGadBoxRegion(vs),UI_EVENT_MOUSE);
      uiSetMouseMotionPolling(TRUE);
      retval = TRUE;

   }

   if ((action & MOUSE_LUP) && (vs->state == LGSLIDER_SELECTED))
   {
      
      SliderChangeValue(vs);

      vs->state = LGSLIDER_DEFAULT;
      uiSetMouseMotionPolling(FALSE);
      uiReleaseFocus(LGadBoxRegion(vs),UI_EVENT_MOUSE_MOVE);
      uiReleaseFocus(LGadBoxRegion(vs),UI_EVENT_MOUSE);
      LGadDrawBox((LGadBox*)vs,NULL); // redraw with new value

   }
   return(retval);
}

void SliderDraw(void *data, LGadBox *vb)
{
   int x = 0;
   int y = 0;
   LGadSlider *vs = (LGadSlider *)vb;
   Rect* r;

   r = (BOX_RECT(vb));

//  If the box is selected, the the user is dragging it, and handlePos
//  should be used to determine the position.  If it is not selected,
//  the position should be computed from the value pointer.  This allows
//  the slider to track its associated variable sort-of automagically
//  while still allowing free motion when it is being dragged

   if ( vs->state != LGSLIDER_SELECTED )
   {
      vs->handlePos = (short)((long)vs->screenRange * (*vs->val_ptr) / vs->max_val) ;
   }

   if ( vs->orient == LGSLIDER_HORIZONTAL )
      x = vs->handlePos;
   else
      y = vs->handlePos;
      

   uiHideMouse(r);
   if ( vs->suPos.x >= 0 )
      gr_bitmap( &vs->saveUnder, vs->suPos.x, vs->suPos.y );
   
   gr_get_bitmap( &vs->saveUnder, x, y);
   vs->suPos.x = x;
   vs->suPos.y = y;
   
   // note assumption that we are in a canvas just right for us to draw into
   //   ElementDraw(&LGadButtonDrawElem(vs),(void*)vs->state,x,y,grd_canvas->bm.w,grd_canvas->bm.h);
   ElementDraw(&LGadButtonDrawElem(vs),vs->state,x,y,grd_canvas->bm.w,grd_canvas->bm.h);
   uiShowMouse(r);

}
#pragma on(unreferenced)
void LGadInitSlider(LGadSlider *vs)
{
   memset(vs,0,sizeof(LGadSlider));
   LGadInitToggle((LGadToggle *)vs);
   ElementClear(&LGadButtonDrawElem(vs));
}

LGadSlider *LGadCreateSlider(LGadSlider *vs, LGadRoot *vr, short x, short y, 
			     short w, short h, char paltype)
{
   uchar* p;
   short suW = 0, suH = 0;

   if (vs == NULL)
   {
      vs = (LGadSlider *)Malloc(sizeof(LGadSlider));
      LGadInitSlider(vs);
   }
   else
   {
      if (LGadBoxFlags(vs) & BOXFLAG_ACTIVE)
      {
         Warning(("LGadCreateSlider: vs %x is already active!\n",vs));
         return(NULL);
      }
   }

   if (vs == NULL)
   {
      Warning(("LGadCreateSlider: vs is NULL after Malloc!\n"));
      return(NULL);
   }
   if (vs->max_val % vs->increm)
      Warning(("LGadCreateSlider: %d not a mult of %d!\n",vs->max_val,vs->increm));


   // Wire up data2 to variable
   switch(LGadButtonDrawElem(vs).draw_type)
   {
   case DRAWTYPE_VAR:
   case DRAWTYPE_VARSTRING:
   case DRAWTYPE_VARRES:
   case DRAWTYPE_FORMAT:
   case DRAWTYPE_FORMATREF:
      LGadButtonDrawElem(vs).draw_data2 = vs->val_ptr;
      break;
   }

   if ((w <= 0) || (h <= 0))
   {
      short elemw,elemh; 

      elemw=w;
      elemh=h;
      LGadComputeVarSize((LGadToggle *)vs,&elemw,&elemh);

      // Offset the box upper/left coords

      x -= ( elemw / 2);
      y -= ( elemh / 2);

      suH = elemh;
      suW = elemw;

      if ( vs->orient == LGSLIDER_HORIZONTAL )
      {
	 vs->handleSpan = elemw;
	 elemw += vs->screenRange;
      }
      else
      {
	 vs->handleSpan = elemh;
	 elemh += vs->screenRange;
      }

      // plug in computed vals
      if (w <= 0)
         w = elemw;
      if (h <= 0)
         h = elemh;

   }
   else
   {
      suH = h;
      suW = w;
   }

   if ( grd_mode_info[grd_mode].bitDepth == 16)
   {
      p = (uchar *)Malloc(w*h*sizeof(short));
      gr_init_bitmap(&(vs->saveUnder),p,BMT_FLAT16,0, suW, suH);
   }
   else
   {
      p = (uchar *)Malloc(w*h);
      gr_init_bitmap(&(vs->saveUnder),p,BMT_FLAT8,0, suW, suH);
   }

   vs->suPos.x = -1; // No saveunder taken yet

   LGadCreateBox((LGadBox *)vs,vr,x,y,w,h,SliderMouseHandler,NULL,SliderDraw,paltype);

   //   vs->box_flags |= BOXFLAG_DIRECT;   &&&&&&&

   LGadBoxMouseMotion((LGadBox *)vs,SliderMotionHandler);
   return(vs);
}

LGadSlider *LGadCreateSliderArgs(LGadSlider *vs, LGadRoot *vr, short x, short y, 
				 short w, short h,  DrawElement *draw, 
				 LGadButtonCallback bfunc, int *val_ptr, short max_val, 
				 short increm, short screenRange, char orient, 
				 char paltype)
{
   if (vs == NULL)
   {
      vs = (LGadSlider *)Malloc(sizeof(LGadSlider));
      LGadInitSlider(vs);
   }
   else
   {
      if (LGadBoxFlags(vs) & BOXFLAG_ACTIVE)
      {
         Warning(("LGadCreateSliderArgs: vs %x is already active!\n",vs));
         return(NULL);
      }
   }

   if (vs == NULL)
   {
      Warning(("LGadCreateSliderArgs: vs is NULL after Malloc!\n"));
      return(NULL);
   }

   vs->val_ptr = val_ptr;
   vs->max_val = max_val;
   vs->increm = increm;
   vs->screenRange = screenRange;
   vs->buttonfunc = bfunc;
   LGadButtonSetDrawElem(vs,*draw);
   vs->orient = orient;
   
   return(LGadCreateSlider(vs,vr,x,y,w,h,paltype));
}


int LGadDestroySlider(LGadSlider *vb, bool free_self)
{

   uiReleaseFocus(LGadBoxRegion(vb),ALL_EVENTS);

   if (vb->saveUnder.bits )
   {
      Free( vb->saveUnder.bits) ;
      vb->saveUnder.bits = NULL;
   }

   LGadDestroyBox((LGadBox *)vb,free_self);

   return 0;

}

