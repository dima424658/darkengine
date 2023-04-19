/*
 * $Source: x:/prj/tech/libsrc/ui/RCS/cursors.c $
 * $Revision: 1.42 $
 * $Author: JAEMZ $
 * $Date: 1997/06/02 16:47:07 $
 *
 */

#include <stdlib.h>
#include <lg.h>
#include <2d.h>
#include <cursors.h> 
#include <curtyp.h>
#include <slab.h>
#include <string.h>
#include <vmouse.h>

#define SPEW_ANAL Spew

#define BMT_SAVEUNDER BMT_FLAT8
#define MAPSIZE(x,y) ((x)*(y))
#define CURSOR_STACKSIZE 5
#define STARTING_SAVEUNDER_WD 16
#define STARTING_SAVEUNDER_HT 16

// -------------------
// DEFINES AND GLOBALS
// -------------------

// Global: the saveunder for bitmap cursors
struct _cursor_saveunder SaveUnder;

extern uiSlab* uiCurrentSlab;
#define RootCursorRegion  (uiCurrentSlab->creg)

// The region currently occupied by the cursor, and the current cursor to be drawn

Region* CursorRegion  = NULL;
Cursor* CurrentCursor = NULL;


// The last cursor region, for when we undraw. 

Point LastCursorPos;
Region* LastCursorRegion = NULL;
Cursor* LastCursor = NULL;


// A semaphore which tells the mouse interrupt handler that the mouse is hidden
int MouseLock = 0;

// A protected rectangle stack for mouse hide/show
#define INITIAL_RECT_STACK   5
Rect* HideRect;
int numhiderects = INITIAL_RECT_STACK;
int curhiderect = 0;


// The canvas used by cursors
grs_canvas DefaultCursorCanvas;
grs_canvas* CursorCanvas = &DefaultCursorCanvas;
grs_canvas* SecondCursorCanvas = NULL;


// Number of pixels to move before interrupt handler draws
int CursorMoveTolerance = 0; 


// ------------------
// INTERNAL FUNCTIONS
// ------------------


static errtype grow_save_under(short x, short y)
{  
   char* tmp;
   int sz = 4*MAPSIZE(x,y)+8;
   SPEW_ANAL(DSRC_UI_Cursors,("grow_stave_under(%d,%d), mapsize = %d\n",x,y,SaveUnder.mapsize));
   if (SaveUnder.mapsize >= sz) return ERR_NOEFFECT;
   if (SaveUnder.bm.bits != NULL)
   {
      tmp = Realloc(SaveUnder.bm.bits,sz);
   }
   else tmp = Malloc(sz);
   if (tmp == NULL) return ERR_NOMEM;
   SaveUnder.bm.bits = tmp;
   SaveUnder.mapsize = sz;
   return OK;
}


typedef struct _cursor_callback_state
{
   Cursor** out;
   Region** reg;
} cstate;


#pragma disable_message(202)
bool cursor_get_callback(Region* reg, Rect* rect, cstate* s)
{
   cursor_stack* cs = (cursor_stack*)(reg->cursors);
   bool anal = FALSE;
#ifdef SPEW_ON
   DBG(DSRC_UI_Anal, { anal = TRUE;});
   if (anal) SPEW_ANAL(DSRC_UI_Cursor_Stack,("cursor_get_callback(%x,%x,%x)\n",reg,rect,s));
#endif 
   if (cs == NULL) *(s->out) = NULL;
   else
   {
      *(s->out) = cs->stack[cs->fullness-1];
      *(s->reg) = reg;
   }
   return *(s->out) != NULL;
}
#pragma enable_message(202)

#define cstack_init uiMakeCursorStack

errtype uiMakeCursorStack(cursor_stack* res)
{
   Spew(DSRC_UI_Cursor_Stack,("cstack_init(%x)\n",res));
   if (res == NULL) return ERR_NULL;
      res->size = CURSOR_STACKSIZE;
      res->stack = Malloc(sizeof(Cursor*)*res->size);
      if (res->stack == NULL)
      {
         Free(res);
         return ERR_NOMEM;
      }
      res->fullness = 1;
      res->stack[0] = NULL;
      return OK;
}

#define ui_destroy_cursor_stack uiDestroyCursorStack


errtype uiDestroyCursorStack(cursor_stack* cstack)
{
   Spew(DSRC_UI_Cursor_Stack,("ui_destroy_cursor_stack(%x) \n",cstack));
   if (cstack == NULL) return ERR_NULL;
   if (cstack->size==  0) return ERR_NOEFFECT;
   cstack->size = 0;
   Free(cstack->stack);
   return OK;
}



errtype uiSetRegionCursorStack(Region* r, uiCursorStack* cs)
{
   if (r == NULL) return ERR_NULL;
   r->cursors = cs;
   return OK;
}

errtype uiGetSlabCursorStack(uiSlab* slab, uiCursorStack** cs)
{
   if (slab == NULL) return ERR_NULL;
   *cs = &slab->cstack;
   return OK;
}

errtype uiSetDefaultCursor(uiCursorStack* cs, Cursor* c)
{
   if (cs == NULL) return ERR_NULL;
   cs->stack[0] = c;
   return OK;
}


errtype uiGetDefaultCursor(uiCursorStack* cs, Cursor** c)
{
   if (cs == NULL) return ERR_NULL;
   *c = cs->stack[0];
   return OK;
}


#define cs_push uiPushCursor

errtype uiPushCursor(cursor_stack* cs, Cursor* c)
{
   if (cs == NULL) return ERR_NULL;
   if (cs->fullness >= cs->size)
   {
      Cursor** tmp = Malloc(cs->size*2*sizeof(Cursor*));
      SPEW_ANAL(DSRC_UI_Cursor_Stack,("cs_push(%x,%x), growing stack\n",cs,c));
      if (tmp == NULL) return ERR_NOMEM;
      memcpy(tmp,cs->stack,cs->size*sizeof(Cursor*));
      Free(cs->stack);
      cs->stack = tmp;
      cs->size *= 2;
   }
   cs->stack[cs->fullness++] = c;
   return OK;
}


errtype uiPopCursor(uiCursorStack* cs)
{
   if (cs == NULL) return ERR_NULL;
   if (cs->fullness <= 1) return ERR_DUNDERFLOW;
   cs->fullness--;
   return OK;
}

errtype uiGetTopCursor(uiCursorStack* cs, Cursor** c)
{
   if (cs == NULL) return ERR_NULL;
   if (cs->fullness <= 1)
      return ERR_DUNDERFLOW;
   *c = cs->stack[cs->fullness-1];
   return OK;
}


errtype uiPushCursorOnce(uiCursorStack* cs, Cursor* c)
{
   Cursor* top = NULL;
   errtype err = uiGetTopCursor(cs,&top);
   if (err == ERR_DUNDERFLOW) top = NULL;
   else if (err != OK) return err;
   if (top != c)
      err = uiPushCursor(cs,c);
   return err;
}

// I wish I had time to implement a non-boneheaded recursive version.
errtype uiPopCursorEvery(uiCursorStack* cs, Cursor* c)
{
   Cursor* top = NULL;
   errtype err = uiGetTopCursor(cs,&top);
   if (err == ERR_DUNDERFLOW) return OK;
   else if (err != OK) return err;
   uiPopCursor(cs);
   err = uiPopCursorEvery(cs,c);
   if (top != c)
   {
      errtype newerr = uiPushCursor(cs,top);
      if (newerr != OK) return newerr;
   }
   return err;
}


#define get_region_stack uiGetRegionCursorStack
 
errtype get_region_stack(Region*r, cursor_stack** cs)
{
   cursor_stack* res = (cursor_stack*)(r->cursors);
   if (res == NULL)
   {
      errtype err;
      SPEW_ANAL(DSRC_UI_Cursor_Stack,("get_region_stack(%x,%x), creating stack\n",r,cs));
      r->cursors = res = Malloc(sizeof(cursor_stack));
      if (res == NULL)
         return ERR_NOMEM;
      err = cstack_init(res);
      if (err != OK) return err;
   }
   *cs = res;
   return OK;
}

// --------------------
// UI Toolkit internals
// --------------------


static uiCursorCallbackId;

errtype ui_init_cursor_stack(uiSlab* slab, Cursor* default_cursor)
{
   errtype err = cstack_init(&slab->cstack);
   Spew(DSRC_UI_Cursor_Stack,("ui_init_cursor_stack(%x,%x) err = %d\n",slab,default_cursor,err));
   if (err != OK) return err;
   slab->cstack.stack[0] = default_cursor;
   return OK;
}

extern void cursor_draw_callback(lgMouseEvent* e, void* data);
extern void bitmap_cursor_drawfunc(int cmd, Region* r, Cursor* c, Point pos);

static int gInitedCursor = FALSE;

errtype ui_init_cursors(void)
{
   errtype err;
   Spew(DSRC_UI_Cursors ,("ui_init_cursors()\n"));
   grow_save_under(STARTING_SAVEUNDER_WD,STARTING_SAVEUNDER_HT);
   LastCursor = NULL;
   MouseLock = 0;
   if (grd_visible_canvas != NULL)
   {
      if (gInitedCursor)
         gr_close_canvas(&DefaultCursorCanvas);
      else
         gInitedCursor = TRUE;
      gr_init_sub_canvas(grd_visible_canvas,&DefaultCursorCanvas,0,0,grd_cap->w,grd_cap->h);
      gr_cset_cliprect(&DefaultCursorCanvas,0,0,grd_cap->w,grd_cap->h);
   }
   err = mouse_set_callback(cursor_draw_callback,NULL,&uiCursorCallbackId);
   if (err != OK) return err;
   HideRect = Malloc(sizeof(Rect)*INITIAL_RECT_STACK);
   HideRect[0].ul.x = -32768;
   HideRect[0].ul.y = -32768;
   HideRect[0].lr = HideRect[0].ul;
   return OK;
}

errtype uiUpdateScreenSize(Point size)
{
   short w = size.x;
   short h = size.y;
   uiHideMouse(NULL);
   if (size.x == UI_DETECT_SCREEN_SIZE.x)
      w = grd_visible_canvas->bm.w;
   if (size.y == UI_DETECT_SCREEN_SIZE.y)
      h = grd_visible_canvas->bm.h;
   if (gInitedCursor)
      gr_close_canvas(&DefaultCursorCanvas);
   else
      gInitedCursor = TRUE;
   gr_init_sub_canvas(grd_visible_canvas,&DefaultCursorCanvas,0,0,w,h);
   gr_cset_cliprect(&DefaultCursorCanvas,0,0,w,h);
   mouse_set_screensize(w,h);
//   if (size.y == UI_DETECT_SCREEN_SIZE.y)
//      h = grd_cap->h; // this is a total hack to take page flipping into account
   mouse_constrain_xy(0,0,w,h);
   uiShowMouse(NULL);
   return(OK);
}

errtype ui_shutdown_cursors(void)
{
   errtype err;
   Spew(DSRC_UI_Cursors,("ui_shutdown_cursors()\n"));
   Free(SaveUnder.bm.bits);

   Free(HideRect);
   err = mouse_unset_callback(uiCursorCallbackId);
   return err;
}

bool ui_set_current_cursor(Point pos)
{
   cstate s;

   ui_mouse_do_conversion(&(pos.x),&(pos.y),TRUE);
   Spew(DSRC_UI_Cursors,("ui_set_current_cursor(<%d,%d>)\n",pos.x,pos.y));
   if (uiCurrentSlab == NULL)
   {
      SPEW_ANAL(DSRC_UI_Cursors,("ui_set_current_cursor(): no current slab\n"));
      CurrentCursor = NULL;
      goto out;
   }
   if (uiCurrentSlab->cstack.fullness > 1)
   {
      CurrentCursor = uiCurrentSlab->cstack.stack[uiCurrentSlab->cstack.fullness-1];
      CursorRegion = (uiCurrentSlab->creg);
      goto out;
   }
   if (RootCursorRegion == NULL)
   {
      SPEW_ANAL(DSRC_UI_Cursors,("ui_set_current_cursor(): no root region\n"));
      CurrentCursor = NULL;
      goto out;
   }
   s.out = &CurrentCursor;
   s.reg = &CursorRegion;

   region_traverse_point(RootCursorRegion,pos,cursor_get_callback,TOP_TO_BOTTOM,&s);
   if (CurrentCursor == NULL && uiCurrentSlab != NULL)
   {
#ifdef SPEW_ON
      {
         bool anal = FALSE;
         DBG(DSRC_UI_Anal,{ anal = TRUE; });
         if (anal) SPEW_ANAL(DSRC_UI_Cursor_Stack,("cursor_get_callback(): using global default\n"));
      }
#endif 
      CurrentCursor = uiCurrentSlab->cstack.stack[0];
      CursorRegion = NULL;
   }
 out:
   Spew(DSRC_UI_Cursors,("ui_set_current_cursor(): current cursor = %x\n",CurrentCursor));
   return CurrentCursor != NULL;
}

void ui_update_cursor(Point pos)
{
   bool show; 
//   ui_mouse_do_conversion(&(pos.x),&(pos.y),FALSE);
   if (MouseLock > 0)
   {
      CurrentCursor = NULL;
      return;
   }
   show = ui_set_current_cursor(pos);
   if (show && LastCursor != NULL && !PointsEqual(pos,LastCursorPos))
   {
      MouseLock++;
      LastCursor->func(CURSOR_UNDRAW,LastCursorRegion,LastCursor,LastCursorPos);
      LastCursor = NULL;
      LastCursorPos = pos;
      LastCursorPos.x -= CurrentCursor->hotspot.x;
      LastCursorPos.y -= CurrentCursor->hotspot.y;
      CurrentCursor->func(CURSOR_DRAW,
                                CursorRegion,
                                CurrentCursor,
                              LastCursorPos);
      LastCursor = CurrentCursor;
      LastCursorRegion = CursorRegion;
      MouseLock--;
   }
}
  

// -------------
// API FUNCTIONS
// -------------


errtype uiSetCursor(void)
{
   Point pos;
   bool show = MouseLock++ == 0;
   errtype retval = OK;
   Spew(DSRC_UI_Cursors,("uiSetCursor(), MouseLock = %d\n",MouseLock));
   mouse_get_xy(&pos.x,&pos.y);
   if (!ui_set_current_cursor(pos))
   {
      retval = ERR_NULL;
   }
   MouseLock--;
   if (show && CurrentCursor != LastCursor)
   {
      uiShowMouse(NULL);
   }
   return(retval);
}

errtype uiSetRegionDefaultCursor(Region* r, Cursor* c)
{
   cursor_stack* cs;
   errtype err = get_region_stack(r,&cs);
   Spew(DSRC_UI_Cursor_Stack,("uiSetRegionDefaultCursor(%x,%x)\n",r,c));
   if (err != OK) return err;
   cs->stack[0] = c;
   uiSetCursor();
   return OK;
}

errtype uiPushRegionCursor(Region* r, Cursor* c)
{
   cursor_stack* cs;
   errtype err = get_region_stack(r,&cs);
   Spew(DSRC_UI_Cursor_Stack,("uiPushRegionCursor(%x,%x)\n",r,c));
   if (err != OK) return err;
   err = cs_push(cs,c);
   if (err != OK) return err;
   return uiSetCursor();
}


errtype uiPopRegionCursor(Region* r)
{
   cursor_stack *cs;
   if (r == NULL) return ERR_NULL;
   cs  = (cursor_stack*)(r->cursors);
   if (cs == NULL)
      return ERR_DUNDERFLOW;
   else
   {
      Spew(DSRC_UI_Cursor_Stack,("uiPopRegionCursor(%x)\n",r));
      if (cs->fullness <= 1) return ERR_DUNDERFLOW;
      cs->fullness--;
      uiSetCursor();
   }
   return OK;
}

errtype uiGetRegionCursor(Region* r,Cursor** c)
{
   cursor_stack *cs;
   if (r == NULL) return ERR_NULL;
   cs = (cursor_stack*)(r->cursors);
   if (cs == NULL)
   {
      *c = NULL;
   }
   else
   {
      Spew(DSRC_UI_Cursor_Stack,("uiGetRegionCursor(%x,%x)\n",r,c));
      *c = cs->stack[cs->fullness-1];
   }
   return OK;
}


errtype uiShutdownRegionCursors(Region* r)
{
   cursor_stack* cs = (cursor_stack*)(r->cursors);
   Spew(DSRC_UI_Cursor_Stack,("uiShutdownRegionCursors(%x)\n",r));
   if (cs == NULL) return ERR_NOEFFECT;
   Free(cs->stack);
   Free(cs);
   r->cursors = NULL;
   uiSetCursor();
   return OK;
}

errtype uiSetSlabDefaultCursor(uiSlab* slab, Cursor* c)
{
   Spew(DSRC_UI_Cursor_Stack,("uiSetSlabDefaultCursor(%x,%x)\n",slab,c));
   if (slab == NULL) return ERR_NULL;
   slab->cstack.stack[0] = c;
   uiSetCursor();
   return OK;
}

errtype uiSetGlobalDefaultCursor(Cursor* c)
{
   return uiSetSlabDefaultCursor(uiCurrentSlab,c);
}

errtype uiPushSlabCursor(uiSlab* slab, Cursor* c)
{
   errtype err;
   Spew(DSRC_UI_Cursor_Stack,("uiPushSlabCursor(%x,%x)\n",slab,c));
   if (slab == NULL) return ERR_NULL;
   err = cs_push(&slab->cstack,c);
   uiSetCursor();
   return err;
}

errtype uiPushGlobalCursor(Cursor* c)
{
   return uiPushSlabCursor(uiCurrentSlab,c);
}

errtype uiPopSlabCursor(uiSlab* slab)
{
   Spew(DSRC_UI_Cursor_Stack,("uiPopSlabCursor(%x)\n",slab));
   if (slab == NULL) return ERR_NULL;
   if (slab->cstack.fullness <= 1)
      return ERR_DUNDERFLOW;
   slab->cstack.fullness--;
   uiSetCursor();
   return OK;
}

errtype uiPopGlobalCursor(void)
{
   return uiPopSlabCursor(uiCurrentSlab);
}

errtype uiGetSlabCursor(uiSlab* slab, Cursor** c)
{
   Spew(DSRC_UI_Cursor_Stack,("uiGetSlabCursor(%x,%x)\n",slab,c));
   if (slab == NULL) return ERR_NULL;
   *c = slab->cstack.stack[slab->cstack.fullness-1];
   uiSetCursor();
   return OK;
}

errtype uiGetGlobalCursor(Cursor** c)
{
   return uiGetSlabCursor(uiCurrentSlab,c);
}

errtype uiHideMouse(Rect* r)
{
   Rect mr;
   bool hide = r == NULL || LastCursor == NULL;
   if (r != NULL)
      Spew(DSRC_UI_Cursors,("uiHideMouse([%d,%d,%d,%d])\n",RECT_EXPAND_ARGS(r)));
   else
      Spew(DSRC_UI_Cursors,("uiHideMouse([NULL])\n"));
   MouseLock++; // hey, don't move the mouse while we're doing this.
   if (!hide)
   {
      mr.ul = LastCursorPos;
      ui_mouse_do_conversion(&(mr.ul.x),&(mr.ul.y),TRUE);
      if (LastCursor != NULL)
      {
         mr.lr.x = LastCursorPos.x + LastCursor->w;
         mr.lr.y = LastCursorPos.y + LastCursor->h;
         ui_mouse_do_conversion(&mr.lr.x,&mr.lr.y,TRUE);
      }
      else mr.lr = mr.ul;
      curhiderect++;
      if (curhiderect >= numhiderects)
      {
         Rect* tmp = HideRect;
         HideRect = Malloc(numhiderects*2*sizeof(Rect));
         memcpy(HideRect,tmp,numhiderects*sizeof(Rect));
         numhiderects *= 2;
         Free(tmp);
      }
      if (curhiderect == 1) HideRect[curhiderect] = *r;
      else RECT_UNION(&HideRect[curhiderect-1],r,&HideRect[curhiderect]);
      hide = RECT_TEST_SECT(&HideRect[curhiderect],&mr);
   }
   // Undraw the mouse.
   if (hide)
   {
      Spew(DSRC_UI_Cursors,("uiHideMouse(): region intersects\n"));
      if (LastCursor != NULL)
      {
         LastCursor->func(CURSOR_UNDRAW,LastCursorRegion,LastCursor,LastCursorPos);
      }
      LastCursor = NULL;
      CurrentCursor = NULL;
   }

//#define FREEZE_ON_HIDE
#ifndef FREEZE_ON_HIDE
   else
   {
      MouseLock--;
      return ERR_NOEFFECT;
   }
#endif
   return OK;
}


errtype uiShowMouse(Rect* r)
{
   extern errtype _mouse_get_xy(short *x, short *y);
   errtype ret;
   Rect mr;
   bool show = LastCursor == NULL || r == NULL;
   MouseLock++;
   if (r != NULL)
      Spew(DSRC_UI_Cursors,("uiShowMouse([%d,%d,%d,%d])\n",RECT_EXPAND_ARGS(r)));
   else
      Spew(DSRC_UI_Cursors,("uiShowMouse([NULL])\n"));
   if (!show)
   {
      mr.ul = LastCursorPos;
      ui_mouse_do_conversion(&(mr.ul.x),&(mr.ul.y),TRUE);
      if (LastCursor != NULL)
      {
         mr.lr.x = LastCursorPos.x + LastCursor->w;
         mr.lr.y = LastCursorPos.y + LastCursor->h;
         ui_mouse_do_conversion(&mr.lr.x,&mr.lr.y,TRUE);
      }
      else mr.lr = mr.ul;
//      Warning(("(%d,%d),(%d,%d) vs (%d,%d),(%d,%d)!\n",RECT_EXPAND_ARGS(r),RECT_EXPAND_ARGS(&mr)));
      show = RECT_TEST_SECT(r,&mr);
   }
   if (show)
   {
      Spew(DSRC_UI_Cursors,("uiShowMouse(): region intersects ML = %d\n",MouseLock));
      if (MouseLock <= 2)
      {
         MouseLock = 2;
         if (LastCursor != NULL)
         {
            LastCursor->func(CURSOR_UNDRAW,LastCursorRegion,LastCursor,LastCursorPos);
            LastCursor = NULL;
         }
         // This uses the _ version because mouse_get_xy adds to the
         // recording, and this doesn't want to do that because cursor
         // rendering does NOT play back from recordings, but is dependent
         // on the pumping of windows events, which is NOT recorded
         _mouse_get_xy(&LastCursorPos.x,&LastCursorPos.y);
         Spew(DSRC_UI_Cursors, ("LastCursorPos = %d,%d\n",LastCursorPos.x,LastCursorPos.y));
         if (ui_set_current_cursor(LastCursorPos))
         {
            LastCursorPos.x -= CurrentCursor->hotspot.x;
            LastCursorPos.y -= CurrentCursor->hotspot.y;
            CurrentCursor->func(CURSOR_DRAW,
                                CursorRegion,
                                CurrentCursor,
                              LastCursorPos);
            LastCursor = CurrentCursor;
            LastCursorRegion = CursorRegion;
         }
         else
         {
            Spew(DSRC_UI_Cursors, ("ui_set_current_cursor fails!\n"));
            LastCursor = NULL;
         }
      }
      MouseLock--;
      ret = OK;
   }
#ifndef FREEZE_ON_HIDE
   else
   {
      ret = ERR_NOEFFECT;
   }
#else
   else
   {
      if (MouseLock <= 2) MouseLock = 2;
      MouseLock--;
   }
#endif
   if (--curhiderect < 0) curhiderect = 0;
   MouseLock--;
   return ret;
}

errtype uiMakeBitmapCursor(Cursor* c,grs_bitmap* bm, Point hotspot)
{
   Spew(DSRC_UI_Cursors,("uiMakeBitmapCursor(%x,%x,<%d %d>)\n",c,bm,hotspot.x,hotspot.y));
   grow_save_under(bm->w,bm->h);
   c->func = bitmap_cursor_drawfunc;
   c->state = bm;
   c->hotspot = hotspot;
   c->w = bm->w;
   c->h = bm->h;
   return OK;
}



void uiComposedBlit(grs_bitmap* bm, Point xy)
{
   grs_canvas bmcanv;
   bool curdraw;
   Point pos = LastCursorPos;
   grs_canvas* save_canvas = CursorCanvas;

   MouseLock++;

   curdraw = MouseLock == 1 && CurrentCursor != NULL;

   if (curdraw)
   {
      gr_make_canvas(bm,&bmcanv);
      CursorCanvas = &bmcanv;
      pos.x -= xy.x;
      pos.y -= xy.y;
      LastCursor->func(CURSOR_REDRAW,CursorRegion,LastCursor,pos);
      gr_close_canvas(&bmcanv);
   }

   Assrt((bm->flags & BMF_RELOCATABLE) == 0);
   gr_bitmap(bm,xy.x,xy.y);

   if (curdraw)
   {
      LastCursor->func(CURSOR_UNDRAW,CursorRegion,LastCursor,pos);
      CursorCanvas = save_canvas;
   }

   MouseLock--;
}

