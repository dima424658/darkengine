/*
 * $Source: x:/prj/tech/libsrc/ui/RCS/curdrw.c $
 * $Revision: 1.20 $
 * $Author: mahk $
 * $Date: 1997/01/23 02:50:31 $
 *
 * Cursor drawing routines called from the interrupt handler.
 */

#include <stdlib.h>
#include <lg.h>
#include <2d.h>
#include <cursors.h>
#include <curdat.h>
#include <slab.h>
#include <string.h>

#ifdef WIN32

//-----------------------------------------------------------------------------
// Jacobson, 1-30-96
// This include helps the UI library keep track of where it's writing to
//-----------------------------------------------------------------------------

#include <comtools.h>
#include <dispapi.h>
extern IDisplayDevice *g_pUiDisplayDevice;

#endif

void cursor_draw_callback(lgMouseEvent* e, void* data)
{
   Point pos;
#ifndef NO_DUMMIES
   void *dummy; dummy = (void *)e; dummy = data;
#endif

   if (MouseLock > 0) return;
   MouseLock++;
   pos.x = e->x;
   pos.y = e->y;
   if (CurrentCursor != NULL)
   {
      pos.x -= CurrentCursor->hotspot.x;
      pos.y -= CurrentCursor->hotspot.y;
   }
   if (LastCursor != NULL)
   {
      if (abs(LastCursorPos.x - pos.x) + abs(LastCursorPos.y - pos.y) <= CursorMoveTolerance)
         goto out;
      LastCursor->func(CURSOR_UNDRAW,LastCursorRegion,LastCursor,LastCursorPos);
      LastCursor = NULL;
   }
   if (CurrentCursor != NULL)
   {
      Rect cr;
      cr.ul = pos;
      cr.lr.x = pos.x + CurrentCursor->w;
      cr.lr.y = pos.y + CurrentCursor->h;
      if (RECT_TEST_SECT(&HideRect[curhiderect],&cr))
      {
         goto out;
      }
      CurrentCursor->func(CURSOR_DRAW,CursorRegion,CurrentCursor,pos);
      LastCursor = CurrentCursor;
      LastCursorPos = pos;
      LastCursorRegion = CursorRegion;
   }
 out:
   MouseLock--;
}

// ---------------------
// BITMAP CURSOR SUPPORT
// ---------------------

static void init_saveunder(uchar bm_type, grs_bitmap* bm)
{
   static bool fSaveUnderInited;
   if (fSaveUnderInited)
      gr_close_bitmap(&SaveUnder.bm);
   gr_init_bitmap(&SaveUnder.bm,SaveUnder.bm.bits,bm_type,0,bm->w,bm->h);
   fSaveUnderInited = TRUE;
}

#define GR_BITMAP gr_bitmap
#define GR_GET_BITMAP gr_get_bitmap
#define GR_HFLIP_BITMAP_IN_PLACE(x)  gr_hflip_in_place(x)

static grs_canvas* old_canvas = NULL;

#pragma off(unreferenced)
void _bitmap_cursor_drawfunc(int cmd, Region* r, Cursor* c, Point pos)
{
   grs_bitmap* bm = (grs_bitmap*)(c->state);
   uchar bm_type = gr_bm_type_to_flat(CursorCanvas->bm.type);

   // set up screen canvas
   old_canvas = grd_canvas;
   gr_set_canvas(CursorCanvas);
   switch(cmd)
   {
      case CURSOR_UNDRAW:
        {
         GR_BITMAP(&SaveUnder.bm,pos.x,pos.y);
        }
        break;
      case CURSOR_REDRAW:
// what purpose does this serve??
       bm_type = gr_bm_type_to_flat(SaveUnder.bm.type);
      case CURSOR_DRAW:
       // Get saveunder
       init_saveunder(bm_type,bm);
       {
         GR_GET_BITMAP(&SaveUnder.bm,pos.x,pos.y);
       }
       // Blit over the save under
       GR_BITMAP(bm,pos.x,pos.y);
       break;
      case CURSOR_DRAW_HFLIP:
       // Get saveunder
       pos.x -= bm->w-1;
       init_saveunder(bm_type,bm);
       GR_HFLIP_BITMAP_IN_PLACE(&SaveUnder.bm);
       GR_GET_BITMAP(&SaveUnder.bm,pos.x,pos.y);
       GR_HFLIP_BITMAP_IN_PLACE(&SaveUnder.bm);
       // Blit over the save under
       GR_HFLIP_BITMAP_IN_PLACE(bm);
       GR_BITMAP(bm,pos.x,pos.y);
       GR_HFLIP_BITMAP_IN_PLACE(bm);
       break;

   }
   gr_set_canvas(old_canvas);
}
#pragma on(unreferenced)

#pragma off(unreferenced)
void bitmap_cursor_drawfunc(int cmd, Region* r, Cursor* c, Point pos)
{
   grs_bitmap* bm = (grs_bitmap*)(c->state);
#ifdef WIN32
   IDisplayDevice_Lock(g_pUiDisplayDevice);
#endif
   _bitmap_cursor_drawfunc(cmd,r,c,pos);
   if (SecondCursorCanvas != NULL)
   {
      grs_canvas* tmp_canvas = CursorCanvas;
      char* saveunder = SaveUnder.bm.bits;

      CursorCanvas = SecondCursorCanvas;
      // point bits at second area, that's why you allocate twice as much
      // as you need
      SaveUnder.bm.bits += SaveUnder.bm.row*SaveUnder.bm.h;
      _bitmap_cursor_drawfunc(cmd,r,c,pos);
      CursorCanvas = tmp_canvas;
      SaveUnder.bm.bits = saveunder;
   }
#ifdef WIN32
   IDisplayDevice_Unlock(g_pUiDisplayDevice);
   IDisplayDevice_FlushRect(g_pUiDisplayDevice, pos.x, pos.y, pos.x + bm->w, pos.y + bm->h);
#endif
}
#pragma on(unreferenced)
