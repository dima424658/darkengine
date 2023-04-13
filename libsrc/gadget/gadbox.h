/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/gadget/RCS/gadbox.h 1.8 1999/12/28 12:13:07 JAEMZ Exp $

#ifndef __GADBOX_H
#define __GADBOX_H

#include <gadbase.h>

////////////////////////////////////////////////////////////
// GADGET BOXES AND ROOTS 
// 
// The basic gadget types.
// 


//------------------------------------------------------------
// ROOT GADGET
// 
// A root is the backing surface to which all other gadgets are nailed.  There is 
// one big root for the entire screen, to which all gadgets on the screen are 
// attached.  The screen root is also responsible for determining what hte cursor 
// looks like.
// It is also possible to create a "subroot" in a region of the screen. 
//  

typedef struct _LGadRoot LGadRoot;

//
// Initialization
// 

// Create the screen root
EXTERN LGadRoot *LGadSetupRoot(LGadRoot *vr, short w, short h, IDataSource *pCurs, Point anchor, int paltype);

// Change which root is the "current" screen root.  Only gadgets on the "current"
// root will receive input. 
EXTERN int LGadSwitchRoot(LGadRoot *newroot);

// Get the current root
EXTERN LGadRoot* LGadCurrentRoot(void);

// Create a "subroot" in a region of the screen.  SubRoots are useful for creating  
// complicated aggregate gadgets.  Note that subroots have their own coordinate space;
// when adding a gadget to a subroot, 0,0 is the upper-left of the subroot. 

EXTERN LGadRoot* LGadSetupSubRoot(LGadRoot* subroot, LGadRoot* parent, short x, short y, short w, short h);

// Destroy a root
EXTERN int LGadDestroyRoot(LGadRoot *vr);

// Change the root cursor 
EXTERN void LGadSetRootCursor(LGadRoot* root, IDataSource* bm, Point hotspot); 

////////////////////////////////////////////////////////////
// BOX GADGETS
// 
// All gadgets are derived from box gadgets.  Every gadget is a box structure with
// other fields appended at the end.  Thus, all box operations will work on 
// all gadgets.  Boxes have callbacks for reading input and for drawing themselves.
//
// Note that roots are also boxes.  Calling LGadDrawBox on a root will draw all 
// gadgets attached to it. 
//

typedef struct _LGadBox LGadBox;

//------------------------------------------------------------
// BOX CALLBACKS
// 

typedef bool (*LGadMouseCallback)(short x, short y, short action, short wheel, LGadBox *vb);
typedef bool (*LGadMotionCallback)(short x, short y, LGadBox *vb);
typedef bool (*LGadKeyCallback)(short keycode, LGadBox *vb);
typedef void (*DrawCallback)(void *data, LGadBox *vb);

//------------------------------------------------------------
// STRUCTURE DEFINITION
// 

#define BOX_GUTS \
   Region r; \
   LGadMouseCallback vmc; \
   LGadKeyCallback vkc;\
   LGadMotionCallback vmotc; \
   ushort box_flags; \
   DrawCallback drawcall; \
   char paltype; \
   guiStyle* style  // no semi

struct _LGadBox {
   BOX_GUTS;
};

//------------------------------------------------------------
// BOX FLAGS
//

#define BOXFLAG_BUFFERED   0x0 // Use normal compose methods
#define BOXFLAG_DIRECT     0x1 // draw straight to screen
#define BOXFLAG_ACTIVE     0x2 // (internal) box has been initialized
#define BOXFLAG_FLIP       0x4 // draw this box twice and flip in between
#define BOXFLAG_CLEAR      0x8 // gui clear at start of draw 

//------------------------------------------------------------
// ACCESSORS
//

// Convert gadget to box
#define VB(x) ((LGadBox *)x)

// back compatible box rect
#define BOX_RECT(vb) ((vb)->r.r)

#define LGadBoxRect(b)          (VB(b)->r.r            )
#define LGadBoxRegion(b)        (&VB(b)->r              )
#define LGadBoxMouseCall(b)     (VB(b)->vmc             )
#define LGadBoxKeyCall(b)       (VB(b)->vkc             )
#define LGadBoxMotionCall(b)    (VB(b)->vmotc           )
#define LGadBoxFlags(b)         (VB(b)->box_flags       )
#define LGadBoxPaltype(b)       (VB(b)->paltype         )
#define LGadBoxStyle(b)         (VB(b)->style           )

EXTERN void LGadBoxMouseMotion(LGadBox *vbox, LGadMotionCallback vmotc);

#define LGadBoxSetMouseCall(b,v)     (VB(b)->vmc            = (v))
#define LGadBoxSetKeyCall(b,v)       (VB(b)->vkc            = (v))
#define LGadBoxSetMotionCall(b,v)    LGadBoxMouseMotion(VB(b),v)
#define LGadBoxSetFlags(b,v)         (VB(b)->box_flags      = (v))
#define LGadBoxSetPaltype(b,v)       (VB(b)->paltype        = (v))
#define LGadBoxSetStyle(b,v)         (VB(b)->style          = (v))

// Add satellite data to a box 
EXTERN void LGadBoxSetUserData(LGadBox* box, void* data);
EXTERN void* LGadBoxGetUserData(LGadBox* box);

// Set the masking function of the box. 
// the masking function determines the shape of a non-rectangular box.
typedef bool (*LGadMaskFunc)(LGadBox* box, Rect* r, void* data);

EXTERN void LGadBoxSetMask(LGadBox* box, LGadMaskFunc func, void* data); 

//------------------------------------------------------------
// BOX GADGET INTERFACE FUNCTIONS
//

//
// Creation/destruction
// 

// Create a new box
// If vbox is null, allocates a new box and returns it, otherwise just initiallizes vbox in place.
// if vr is null, uses LGadCurrentRoot().  
EXTERN LGadBox *LGadCreateBox(LGadBox *vbox, LGadRoot *vr, short x, short y, short w, short h,  
   LGadMouseCallback vmc, LGadKeyCallback vkc, DrawCallback drawcall, char paltype);

// Destroy a box
EXTERN int LGadDestroyBox(LGadBox *vb, bool free_self);

// Erase a box and then destroy it
EXTERN int LGadEraseBox(LGadBox *vb, bool free_self);


// 
// Drawing
//

// Force a box to draw.  Data gets passed on to its draw callback. 
// Will force a root to draw all its subordinate gadgets. 
EXTERN int LGadDrawBox(LGadBox *vb, void *data);

// Register an LGadBox as needing to be checked for redraw in certain cases.
EXTERN int LGadAddOverlap(LGadBox *vb);
EXTERN int LGadRemoveOverlap(LGadBox *vb);

// erase a box, and hide it so that it will not receive mouse events, and will
// not be drawn unless LGadDrawBox is explicitly called.

// if hide is true then hide, else show
EXTERN int LGadHideBox(LGadBox *vb, bool hide);



////////////////////////////////////////////////////////////
// GADGET ROOT STRUCTURE DEF
// 

struct _LGadRoot {
   struct _LGadBox box;
   uiSlab* root_slab;
   Cursor* curs;
   bool subroot;
   IDataSource *pCursor;
   bool free_self;
};


#endif // __GADBOX_H