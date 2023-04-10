// $Header: x:/prj/tech/libsrc/ui/RCS/cursors.h 1.12 1998/06/18 13:28:18 JAEMZ Exp $

#ifndef __CURSORS_H
#define __CURSORS_H
#pragma once

struct _cursor;
struct _ui_slab;

// Includes
#include <lg.h>  // every file should have this
#include <2d.h>
//#include <base.h> 
#include <rect.h>
#include <region.h> 
#include <mouse.h> 
#include <error.h>

// Defines


typedef void (*CursorDrawFunc)(int cmd, Region* r, struct _cursor* c, Point pos);
//  A cursor drawfunc executes the command specified by cmd to draw and undraw cursor
//  c at point pos.  R is the region in which c was found. 

//  The commands are as follows:
#define CURSOR_DRAW        0
#define CURSOR_UNDRAW      1
#define CURSOR_DRAW_HFLIP  2 // draw horizontally flipped.  Go figure. 
#define CURSOR_REDRAW      3 // for second and subsequent draws before undraw

typedef struct _cursor
{
  CursorDrawFunc func;
  void* state;
  Point hotspot;
  short w,h;
} Cursor;

// Every region has a cursor stack.  
typedef struct _cursorstack
{
   int size;
   int fullness;
   Cursor** stack;
} cursor_stack;


typedef cursor_stack uiCursorStack; 



// Prototypes
EXTERN errtype uiMakeBitmapCursor(Cursor* c, grs_bitmap* bm, Point hotspot);
// Initializez *c to a bitmap cursor whose bitmap is bm, with the specified hotspot

EXTERN errtype uiSetRegionDefaultCursor(Region* r, Cursor* c);
// Sets the default cursor to be used when the cursor is in region r and no cursor has been
// pushed to r's cursor stack

EXTERN errtype uiPushRegionCursor(Region* r, Cursor* c);
// Pushes c to r's regional cursor stack.  When the mouse is in region r,
// the top of r's cursor stack will be displayed.  

EXTERN errtype uiPopRegionCursor(Region* r);
// Pops the top cursor off of r's cursor stack

EXTERN errtype uiGetRegionCursor(Region* r, Cursor** c);
// Gets the current cursor for region r.  *c will be NULL if 
// there is no default cursor for r, and no cursors on the 
// r's cursor stack.

EXTERN errtype uiShutdownRegionCursors(Region* r);
// Deletes the cursor stack and default cursor for region r.

EXTERN errtype uiSetGlobalDefaultCursor(Cursor* c);
// Sets the default cursor for the currently active slab.

EXTERN errtype uiPushGlobalCursor(Cursor* c);
// Pushes a cursor to the active slab's global cursor stack.  

EXTERN errtype uiPopGlobalCursor(void);
// Pops the top cursor off of the active slab's global cursor stack. 

EXTERN errtype uiGetGlobalCursor(Cursor** c);
// Gets the cursor on top of the active slab's global cursor stack,
// or the global default cursor if the stack is empty.

EXTERN errtype uiSetSlabDefaultCursor(struct _ui_slab* slab, Cursor* c);
// Sets the default cursor for the specified slab.

EXTERN errtype uiPushSlabCursor(struct _ui_slab* slab, Cursor* c);
// Pushes a cursor to the specified slab's global cursor stack.  

EXTERN errtype uiPopSlabCursor(struct _ui_slab* slab);
// Pops the top cursor off of the specified slab's global cursor stack. 

EXTERN errtype uiGetSlabCursor(struct _ui_slab* slab, Cursor** c);
// Gets the cursor on top of the specified slab's global cursor stack,
// or the global default cursor if the stack is empty.

EXTERN errtype uiHideMouse(Rect* r);
// Hides the mouse if it intersects r.

EXTERN errtype uiShowMouse(Rect* r);
// Shows the mouse if it intersects r.  

EXTERN errtype uiSetCursor(void);
// Recomputes and redraws the current cursor based on the position of the
// mouse. 

EXTERN void uiComposedBlit(grs_bitmap* bm, Point xy);
// Blits bm to point xy on the visible canvas, 
// composing the cursor as necessary.



// ------------------
// SCREEN_MODE_UPDATE
// ------------------

#define UI_DETECT_SCREEN_SIZE (MakePoint(-1,-1))


// resizes the ui coordinate space.
// if UI_DETECT_SCREEN_SIZE, detect the screen size
EXTERN errtype uiUpdateScreenSize(Point size);


// Globals

EXTERN int CursorMoveTolerance;
// Number of pixels of movement the interrupt handler will "tolerate" before
// redrawing the cursor. 





// ----------------------
// CURSOR-STACK-BASED API 
// 

/*
   Routines for manipulating cursor stacks.  This is kind of an
   afterthought, but it's a good one, and in the future, when we have
   light without heat and travel to the stars, all good ui clients will use it.
   
   The idea here is to *expose* the notion of cursor stack to the client, so it can do
   clever things like have regions/slabs share cursor stacks.  In addition to this, we
   implement the push-one and pop-every operations, which will only operate on cursor stacks
   so that the API does not explode with element of { slab, region, stack} x { once, always} etc.

*/

EXTERN errtype uiMakeCursorStack(uiCursorStack* cs);
// initializes cs to an empty cursor stack  

EXTERN errtype uiDestroyCursorStack(uiCursorStack* cs);
// destroys a cursor stack.

EXTERN errtype uiGetRegionCursorStack(Region* reg, uiCursorStack** cs);
// points *cs to reg's cursor stack.  If reg has no cursor stack, 
// creates one.  

EXTERN errtype uiSetRegionCursorStack(Region* reg, uiCursorStack* cs);
// Sets reg's cursor stack to cs. 

EXTERN errtype uiGetSlabCursorStack(struct _ui_slab* slab, uiCursorStack** cs);
// points *cs to slab's cursor stack.  If slab has no cursor stack, sets *cs to NULL
// and returns ERR_NULL;

// note that there is not currently a uiSetSlabCursorStack

EXTERN errtype uiSetDefaultCursor(uiCursorStack* cs, Cursor* c);
// sets cs' default cursor to c.  if cs is the cursor stack of a slab, 
// c will become the default global cursor when that slab is the current slab.  If
// cs is the cursor stack of a region, c will become the default cursor for the region. 

EXTERN errtype uiGetDefaultCursor(uiCursorStack* cs, Cursor** c);
// sets *c to the default cursor for cs, or NULL if there is none.

EXTERN errtype uiPushCursor(uiCursorStack* cs, Cursor* c);
// pushes cursor c onto cursor stack cs

EXTERN errtype uiPopCursor(uiCursorStack* cs);
// pops the top cursor off of cs. 

EXTERN errtype uiGetTopCursor(uiCursorStack* cs, Cursor** c);
// Points *c to the top cursor on cs; or NULL if there is no 
// top cursor.  will NOT set *c to the default cursor.

EXTERN errtype uiPushCursorOnce(uiCursorStack* cs, Cursor* c);
// pushes cursor c to the top of cs ONLY IF c is not already 
// on the top of cs.  

EXTERN errtype uiPopCursorEvery(uiCursorStack* cs, Cursor* c);
// deletes every instance of c from cs.


#endif // __CURSORS_H
                                                                                                                                               
