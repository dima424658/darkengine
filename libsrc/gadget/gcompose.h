/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifndef __GCOMPOSE_H
#define __GCOMPOSE_H
/*
 * $Source: x:/prj/tech/libsrc/gadget/RCS/gcompose.h $
 * $Revision: 1.6 $
 * $Author: JAEMZ $
 * $Date: 1998/08/04 14:09:38 $
 *
 */

#include <2d.h>
#include <rect.h>
#include <error.h>


//////////////////////////////////////////////////////////////
// GUI Compose
// 
// A composition abstraction
/////////////////////////////////////////////////////////////

typedef enum GUIcomposeFlags
{
   ComposeFlagClear = 1,  // should we clear on setup
   ComposeFlagRead = 2,   // am I going to read from the canvas
   ComposeFlagReadOnly = 4, // am I ONLY going to read from the canvas
   ComposeFlagSaveUnder = 8, // I am reading or writing a save under
} GUIcomposeFlags;

typedef struct _GUIcompose 
{
   Rect area;           // draw area, set up by GUIsetup
   GUIcomposeFlags flags;         // flags, set up by GUIsetup

   ulong pal;                  // palette, set by GUIsetpal, cleared by GUIsetup

   grs_canvas canv;     // draw canvas, set up by methods
   void *data;  // set up by methods
} GUIcompose;

//////////////////////////////////////////
// GUIsetup()
// Initializes the specified GUIcompose
// Sets up to draw in a specified rectangle
// pushes a canvas whose origin is the top left 
// of the rectangle.
// canvtype specified the kind of canvas that is acceptable



enum _GUI_CANV
{
   GUI_CANV_ANY,             // I don't care what type of canvas
   GUI_CANV_SCREENDEPTH,     // Canvas' bit depth MUST be the same as the screen
};


EXTERN void GUIsetup(GUIcompose* c, Rect* area, GUIcomposeFlags Flags, int canvtype);

////////////////////////////////////////
// GUIclear()
//
// Clears the specified rectangle, in canvas-relative coordinates
// NULL means clear the whole canvas

EXTERN void GUIclear(GUIcompose* c, Rect* r);


////////////////////////////////////////
// GUIsetpal()
//
// Set the palette in the current gui context
// 

EXTERN void GUIsetpal(GUIcompose* c, ulong pal);

////////////////////////////////////////
// GUIdone()
//
// Finished drawing to the canvas setup by GUISetup()
//
//

EXTERN void GUIdone(GUIcompose* c);


//
// Useful "clear this rectangle" macro
//

#define GUIErase(rect) \
do { \
  GUIcompose c; \
  GUIsetup(&c,rect,ComposeFlagClear,GUI_CANV_ANY); \
  GUIdone(&c); \
} while(0)                                                   


////////////////////////////////////////////////////////////// 
// GUI METHODS
//
// These are "hooks" to allow the app to implement its own 
// setup, clear, and done functions
//


typedef struct _GUImethods
{
   errtype (*setup)(GUIcompose* c, int canvtype); // setup to draw to an absolute rect
                                          // including setting the canvas                   
   void (*clear)(GUIcompose* c, Rect* r); // erase a rect to look like the background
   void (*done)(GUIcompose* c);         // done drawing.
   void (*setpal)(GUIcompose* c);       // called after palette set

} GUImethods;


//----------------------------------------------
// TheGUImethods
//
// pointer to the current methods, initially set to 
// DefaultGUImethods
//
EXTERN GUImethods* TheGUImethods(void);
EXTERN void SetTheGUImethods(GUImethods*);


//----------------------------------------------------------------------------
// Default gui methods: draw straight to DefaultGUIcanvas, clear to color zero
// (if DefaultGUIcanvas = NULL, go to grd_canvas)
//

EXTERN GUImethods DefaultGUImethods;
EXTERN grs_canvas* DefaultGUIcanvas;


#endif // __GCOMPOSE_H