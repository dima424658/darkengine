/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/*
 * $Source: r:/t2repos/thief2/src/dark/gadslide.cpp,v $
 * $Revision: 1.7 $
 * $Author: toml $
 * $Date: 2000/02/19 13:09:00 $
 *
 * Implements a pretty standard cup/handle type of slider gadget built on top of
 * the LGadToggle gadget.  Movement can be either horizontal or vertical and can
 * be quantized.
 *
 */

// library includes
#include <comtools.h>
#include <appagg.h>
#include <dispapi.h>
#include <mprintf.h>

#undef BTYPE

#include <event.h>
#include <gadget.h>  
#include <gadint.h>
#include <gadslide.h>
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

#include <drawelem.h>
#include <util2d.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 




static void SliderChangeValue( LGadSlider* vs, short action )
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
	      vs->buttonfunc(action, NULL, (LGadBox*)vs);

   }
   else
      if (vs->buttonfunc)
      	 vs->buttonfunc(1, NULL, (LGadBox*)vs);
}

static void SliderSetGrab(LGadSlider* vs, short x, short y)
{
   Rect* r;

   r = (BOX_RECT(VB(vs)));

   x = short(x - r->ul.x);  // to compensate for position of gadget
   y = short(y - r->ul.y);

   if ( vs->orient == LGSLIDER_HORIZONTAL )
   {
      vs->grabPos = short(x - vs->handlePos);
   }
   else
   {
      vs->grabPos = short(y - vs->handlePos);
   }

}

static void SliderMoveHandle(LGadSlider* vs, short x, short y)
{
   Rect* r;

   r = (BOX_RECT(VB(vs)));

   x = short(x - r->ul.x);  // to compensate for position of gadget
   y = short(y - r->ul.y);

   vs->grabPos = vs->handleSpan / 2;
   if ( vs->orient == LGSLIDER_HORIZONTAL )
   {
      vs->handlePos = x - vs->grabPos;
   }
   else
   {
      vs->handlePos = y - vs->grabPos;
   }

}

static bool PointOverHandle( LGadSlider* vs, short x, short y)
{
   bool retval = TRUE;
   Rect* r;

   r = (BOX_RECT(VB(vs)));

   if ( !RECT_TEST_PT(r, MakePoint(x,y) ))
      retval = FALSE;

   x = short(x - r->ul.x);  // to compensate for position of gadget
   y = short(y - r->ul.y);

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


static void UpdateHandle (LGadSlider* vs)
{
   short closeval, closepos, notch;
   // Here is a little goofy bit:  If the handle, while being dragged, is
   // close to a 'detent', then move it there, just for feel.

//   pixPerNotch = (long)vs->handleRange * vs->increm / vs->max_val; 

   closeval = (long)vs->max_val * 2 * vs->handlePos / vs->screenRange;
   notch    = ((closeval + vs->increm) /  (vs->increm*2));

   closepos = ( (long)vs->screenRange * vs->increm / vs->max_val ) * notch;

   if ( abs(closepos - vs->handlePos) < vs->handleSpan)
      vs->handlePos = closepos;

   if ( vs->handlePos != vs->oldHandlePos )
   {
      SliderChangeValue( vs, 0 );
      vs->oldHandlePos = vs->handlePos;
      if ( !vs->bSkipSaveUnder)
         LGadDrawBox((LGadBox *)vs,NULL); // redraw with new value
   }
}

bool SliderMotionHandler(short x, short y, LGadBox *vb)
{
   LGadSlider *vs;
   Rect *r;
   bool retval = FALSE;

   vs = (LGadSlider *)vb;

   if (vs->state == LGSLIDER_DEPRESSED)
   {

      r = (BOX_RECT(vb));

      x = short(x - r->ul.x);  // to compensate for position of gadget
      y = short(y - r->ul.y);

      if ( vs->orient == LGSLIDER_HORIZONTAL )
         vs->handlePos = short(x - vs->grabPos);
      else
         vs->handlePos = short(y - vs->grabPos);

      if ( vs->handlePos > vs->screenRange )
         vs->handlePos = vs->screenRange;
         
      if ( vs->handlePos < 0 )
         vs->handlePos = 0;

      UpdateHandle (vs);

      retval = TRUE;  
   }
   else  // is not selected
   {
      if ( PointOverHandle( vs, x, y ))
      {
         if ( vs->state != LGSLIDER_HIGHLIT )
         {
            vs->state = LGSLIDER_HIGHLIT;
            if ( !vs->bSkipSaveUnder)
               LGadDrawBox(vb,NULL); // redraw with new value
            uiGrabFocus(LGadBoxRegion(vs),UI_EVENT_MOUSE_MOVE);
         }
         retval = TRUE; 
      }
      else
      {
         if ( vs->state != LGSLIDER_NORMAL )
         {
            vs->state = LGSLIDER_NORMAL;
            if ( !vs->bSkipSaveUnder)
               LGadDrawBox(vb,NULL); // redraw with new value
            uiReleaseFocus(LGadBoxRegion(vs),UI_EVENT_MOUSE_MOVE);
         }
      }

      // don't eat the event

   }

   if ( vs->state == LGSLIDER_DEPRESSED ) {
      // if handle is grabbed, eat all events
      retval = TRUE;
   }

   return(retval);
}

bool SliderMouseHandler(short x, short y, short action, short wheel, LGadBox *vb)
{
   LGadSlider *vs;
   bool retval = FALSE;

   vs = (LGadSlider *)vb;

   if (action & MOUSE_LDOWN )
   {

      if (PointOverHandle(vs,x,y))
      {
         vs->state = LGSLIDER_DEPRESSED;
	      SliderSetGrab(vs,x,y);

         if ( !vs->bSkipSaveUnder)
            LGadDrawBox(vb,NULL); // redraw (in case the up/down art is different)
         uiGrabFocus(LGadBoxRegion(vs),UI_EVENT_MOUSE);
      }
      else
      {
	      SliderMoveHandle(vs,x,y);
         UpdateHandle (vs);
      }

      retval = TRUE;
   }

   if ((action & MOUSE_LUP) && vs->state == LGSLIDER_DEPRESSED)
   {
      
      SliderChangeValue(vs, 1);
      
      vs->state = LGSLIDER_NORMAL;
      uiReleaseFocus(LGadBoxRegion(vs),UI_EVENT_MOUSE_MOVE);
      uiReleaseFocus(LGadBoxRegion(vs),UI_EVENT_MOUSE);
      if ( !vs->bSkipSaveUnder)
         LGadDrawBox((LGadBox*)vs,NULL); // redraw with new value

   }

   if ( vs->state == LGSLIDER_DEPRESSED ) {
      // if handle is grabbed, eat all events
      retval = TRUE;
   }

   return(retval);
}

void SliderDraw(void *, LGadBox *vb)
{
   int x = 0;
   int y = 0;
   LGadSlider *vs = (LGadSlider *)vb;
   Rect* r;
   long nOldType;

   r = (BOX_RECT(vb));

//  If the box is selected, the the user is dragging it, and handlePos
//  should be used to determine the position.  If it is not selected,
//  the position should be computed from the value pointer.  This allows
//  the slider to track its associated variable sort-of automagically
//  while still allowing free motion when it is being dragged

   if ( vs->state != LGSLIDER_DEPRESSED )
   {
      vs->handlePos = (short)((long)vs->screenRange * (*vs->val_ptr) / vs->max_val) ;
   }

   if ( vs->orient == LGSLIDER_HORIZONTAL )
      x = vs->handlePos;
   else
      y = vs->handlePos;
      

   uiHideMouse(r);

   // saveunders from the screen should never be clutted
   if ( !vs->bSkipSaveUnder )
   {
      nOldType = gr_get_fill_type();
      gr_set_fill_type(FILL_NORM);

      if ( vs->suPos.x >= 0 )
         gr_bitmap( &vs->saveUnder, vs->suPos.x, vs->suPos.y );
   
      gr_get_bitmap( &vs->saveUnder, x, y);

      gr_set_fill_type(nOldType);
   }

   vs->suPos.x = (short)x;
   vs->suPos.y = (short)y;
   
   // note assumption that we are in a canvas just right for us to draw into
   ElementDraw(&LGadButtonDrawElem(vs),(DrawElemState)vs->state,(short)x,(short)y,1,1);
   uiShowMouse(r);

}
//#pragma on(unreferenced)
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
   short elemw,elemh; 

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


   elemw=w;
   elemh=h;

   if ((w <= 0) || (h <= 0))
   {
      LGadComputeVarSize((LGadToggle *)vs,&elemw,&elemh);
   }

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
   //   if (w <= 0)
      w = elemw;
      //   if (h <= 0)
      h = elemh;

   if ( grd_mode_info[grd_mode].bitDepth == 8)
   {
      p = (uchar *)Malloc(w*h);
      gr_init_bitmap(&(vs->saveUnder),p,BMT_FLAT8,0, suW, suH);
   }
   else
   {
      p = (uchar *)Malloc(w*h*sizeof(short));
      gr_init_bitmap(&(vs->saveUnder),p,BMT_FLAT16,0, suW, suH);
   }

   vs->suPos.x = -1; // No saveunder taken yet

   LGadCreateBox((LGadBox *)vs,vr,x,y,w,h,SliderMouseHandler,NULL,SliderDraw,paltype);

   //   vs->box_flags |= BOXFLAG_DIRECT;   &&&&&&&

   LGadBoxMouseMotion((LGadBox *)vs,SliderMotionHandler);

   LGadDrawBox ((LGadBox*)vs, NULL);

   return(vs);
}

LGadSlider *LGadCreateSliderArgs(LGadSlider *vs, LGadRoot *vr, short x, short y, 
				 short w, short h, LGadButtonCallback bfunc, int *val_ptr, short max_val, 
				 short increm, short screenRange, char orient, char paltype)

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

void LGadSliderSetNoSaveUnder(LGadSlider *vs)
{
   vs->bSkipSaveUnder = TRUE;
}
