/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/gadget/gcompose.c,v 1.9 1998/04/09 14:06:24 mahk Exp $

#include <string.h>

#include <2dres.h>
#include <gfile.h>

#include <gcompose.h>

#include <dispapi.h>
#include <appagg.h>


//
// Current GUI methods
//

static GUImethods* curr_gui_methods = &DefaultGUImethods;

void SetTheGUImethods(GUImethods* gm)
{
   if (gm ==NULL)
   {
      curr_gui_methods = &DefaultGUImethods;
      DefaultGUIcanvas = NULL;
   }
   else
      curr_gui_methods = gm;
}

GUImethods* TheGUImethods(void)
{
   return curr_gui_methods;
}

//
// GCOMPOSE API
//

void GUIsetup(GUIcompose* c, Rect* area, GUIcomposeFlags flags, int canvtype)
{
   GUImethods* methods = TheGUImethods();
   c->area = *area;
   c->flags = flags;
   memset(&c->pal,0,sizeof(c->pal));
   if (methods->setup != NULL)
   {
      errtype err = methods->setup(c, canvtype);
      Assrt(err == OK);
   }
}

void GUIsetpal(GUIcompose* c, ulong pal)
{
   GUImethods* methods = TheGUImethods();
   c->pal = pal;
   if (methods->setpal != NULL)
      methods->setpal(c);
}

void GUIclear(GUIcompose* c, Rect *r)
{
   GUImethods* methods = TheGUImethods();
   if (methods->clear == NULL)
      return;
   if (r == NULL)
   {
      Rect area = { {0,0}, };
      area.lr = MakePoint(RectWidth(&c->area),RectHeight(&c->area));
      methods->clear(c,&area);
   }
   else
      methods->clear(c,r);
}


void GUIdone(GUIcompose* c)
{
   GUImethods* methods = TheGUImethods();
   if (methods->done != NULL)
      methods->done(c);
}


//
// Default GUI methods
//

errtype default_gui_setup(GUIcompose* c, int ctype);
void default_gui_clear(GUIcompose* c, Rect* r);
void default_gui_done(GUIcompose* c);
void default_gui_setpal(GUIcompose* c);

static errtype default_gui_setup(GUIcompose* c, int ctype)
{
   errtype retval = OK;
   Rect* r = &c->area;
   grs_canvas* canv = DefaultGUIcanvas;
   if (canv == NULL) 
   {
      canv = grd_canvas; // was canv = grd_visible_canvas;
   }
   // save the old default canvas
   c->data = DefaultGUIcanvas;
   DefaultGUIcanvas = canv;

   if (ctype == GUI_CANV_SCREENDEPTH && canv->bm.type != grd_visible_canvas->bm.type)
      retval = ERR_RANGE;

   // @TODO: Only do this if the canvas is on screen.
   {
      IDisplayDevice* dispdev = AppGetObj(IDisplayDevice);
      if (dispdev)
         IDisplayDevice_Lock(dispdev); 
      SafeRelease(dispdev); 
   }

   gr_init_sub_canvas(canv,&c->canv,r->ul.x,r->ul.y,RectWidth(r),RectHeight(r));
   gr_push_canvas(&c->canv);
   if (c->flags & ComposeFlagClear)
      GUIclear(c,NULL);

   return OK;
}

static void default_gui_clear(GUIcompose* c, Rect* r)
{
   grs_clip _clip = grd_gc.clip;
   if (r != NULL)
      gr_set_cliprect(r->ul.x,r->ul.y,r->lr.x,r->lr.y);
   else
      gr_set_cliprect(0,0,c->canv.bm.w,c->canv.bm.h);
   gr_clear(0);
   grd_gc.clip = _clip;
}

static void default_gui_done(GUIcompose* c)
{
   gr_pop_canvas();
   gr_close_sub_canvas(&c->canv);
   DefaultGUIcanvas = (grs_canvas*)c->data;

   // @TODO: Only do this if the canvas is on screen.
   {
      IDisplayDevice* dispdev = AppGetObj(IDisplayDevice);
      if (dispdev)
         IDisplayDevice_Unlock(dispdev); 
      SafeRelease(dispdev); 
   }
}

static void default_gui_setpal(GUIcompose* c)
{
   if (c->pal == 0)
      return;
   if (REFID(c->pal) == 0) // must be a resid
   {
      PallInfo* pal = ResLock(c->pal);
      gr_set_pal(pal->index,pal->numcols,pal->rgb);
      ResUnlock(c->pal);
   }
   else
   {
      gr_set_pal_imgref(c->pal);
   }
}

GUImethods DefaultGUImethods =
{
   default_gui_setup,
   default_gui_clear,
   default_gui_done,
   default_gui_setpal,
};

grs_canvas* DefaultGUIcanvas = NULL;
