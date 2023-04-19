/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/gadget/RCS/gadmenu.h 1.7 1996/11/08 14:23:13 xemu Exp $

#ifndef __GADMENU_H
#define __GADMENU_H

#include <gadbutt.h>

////////////////////////////////////////////////////////////
// MENU GADGETS
//
// A menu is pretty much a vertical array of buttons, usually text. 
// It can be transient, and will save under itself. 
// 

typedef struct _LGadMenu LGadMenu;

//------------------------------------------------------------
// MENU CALLBACKS
//

typedef bool (*LGadMenuCallback)(int which_sel,LGadMenu *vm);
/* When a menu item is chosen, the callback is called with the data of which item was chosen.  Returns 
TRUE if the selection was valid and the menu should be destroyed, otherwise FALSE. */

typedef bool (*LGadMenuRedraw)(short x, short y, short w, short h);
/* After the menu goes away, it wants to restore some information underneath it.  This callback gets called 
when the menu goes away, with the dimensions of the LGadMenu so that the right thing can get done. */

//------------------------------------------------------------
// STRUCTURE DEFINITION 
//

#define MENU_GUTS \
   BUTTON_GUTS;\
   short num_elems;\
   short vis_elems;\
   DrawElement *elems;\
   uint flags;\
   LGadMenuCallback vmenuc;\
   LGadMenuRedraw vmredraw;\
   short cur_sel;\
   int active_slot; \
   grs_bitmap saveunder;\
   uchar sel_info; \
   int mouse_sel; \
   short focus

struct _LGadMenu {
   MENU_GUTS;
};


//------------------------------------------------------------
// ACCESSORS
// 

#define LGadMenuNumElems(m)  (((LGadMenu*)m)->num_elems         )
#define LGadMenuVisibleElems(m)  (((LGadMenu*)m)->vis_elems         )
#define LGadMenuDrawElem(m,i)    (&((LGadMenu*)m->elems[i]      ))
#define LGadMenuFlags(m)        (((LGadMenu*)m)->flags  )
#define LGadMenuCallback(m)     (((LGadMenu*)m)->vmenuc )
#define LGadMenuRedrawCall(m)   (((LGadMenu*)m)->vmredraw       )
#define LGadMenuCurrentSelection(m)  (((LGadMenu*)m)->cur_sel   )

#define LGadMenuSaveUnder(m)  (&((LGadMenu*)m)->saveunder)


#define LGadMenuSetNumElems(m,v)  (((LGadMenu*)m)->num_elems        = (v))
#define LGadMenuSetVisibleElems(m,v)  (((LGadMenu*)m)->vis_elems        = (v))
#define LGadMenuSetDrawElem(m,i,v)    (&((LGadMenu*)m)->elems[i]     = *(v))
#define LGadMenuSetFlags(m,v)        (((LGadMenu*)m)->flags = (v))
#define LGadMenuSetCallback(m,v)     (((LGadMenu*)m)->vmenuc = (v))
#define LGadMenuSetRedrawCall(m,v)   (((LGadMenu*)m)->vmredraw      = (v))
#define LGadMenuSetCurrentSelection(m,v)  (((LGadMenu*)m)->cur_sel  = (v))

//------------------------------------------------------------
// MENU FLAGS
// 

// These flags are for user configuration of the menu
#define MENU_TRANSIENT		   0x001 // menu goes away if mouse leaves box
#define MENU_ALLOC_ELEMS	   0x002 // allocate local copy of elems and copy in info
#define MENU_GRAB_FOCUS       0x004 // grab all input focus while up
#define MENU_HORIZONTAL       0x008 // orient it horizontally, duh
#define MENU_MOUSEDOWNS       0x010 // are we interested in mouse down events?
#define MENU_SCROLLBAR        0x020 // add a scrollbar on the left/top
#define MENU_MOVEKEYS         0x040 // respond to the "standard" key sets for manipulation
#define MENU_ESC              0x080 // respond to ESC for termination
#define MENU_OUTER_DISMISS    0x100 // destroy if clicks out of region when not tracking
#define MENU_SELBOX           0x200 // selection box around current item
#define MENU_NOAUTODRAW       0x400 // do not automatically draw on creation
// useful combo
#define MENU_STDKEYS          (MENU_ESC|MENU_MOVEKEYS)

// This is some selection-specific information flags
#define MENU_SELINFO_LUP      0x01
#define MENU_SELINFO_RUP      0x02
#define MENU_SELINFO_LDN      0x04
#define MENU_SELINFO_RDN      0x08

// 
// INTERNAL FLAGS
//

// These flags are secretly used by the menu itself
#define MENU_ALLOC_BASE          0x010000 // did we allocate the basic memory ourselves?
#define MENU_SCROLLBARS_USED     0x020000 // did we actually make scrollbuttons?
#define MENU_SAVEUNDER_DISABLE   0x040000 // the menu has decided to disable saveunders, typically because of
                                     // rendered area overlap
#define MENU_SAVEUNDER_TAKEN     0x080000 // have we grabbed a saveunder yet?
#define MENU_SCROLL_TRACK        0x100000 // are we currently tracking mouse motion for scrollbars

//------------------------------------------------------------
// MENU GADGET FUNCTIONS 
// 

//
// Creation/Destruction
//

EXTERN LGadMenu *LGadCreateMenu(LGadMenu *vm, LGadRoot *vr, short x, short y, short w, short h, char paltype);
EXTERN LGadMenu *LGadCreateMenuArgs(LGadMenu *vm, LGadRoot *vr, short x, short y, short w, 
	short h, short num_elems, short vis_elems, DrawElement *elems, LGadMenuCallback vmc, 
	LGadMenuRedraw vmredraw, ushort flags, ushort draw_flags, DrawElement *inner, char paltype);
EXTERN int LGadDestroyMenu(LGadMenu *vm);

// Change the "focus" (first visible element) of the menu
EXTERN int LGadFocusMenu(LGadMenu *vm, int new_f);

// Compute the size of a menu
EXTERN void LGadMenuComputeSize(short *w, short *h, short num_elems, short vis_elems, uint flags, 
   DrawElement *elems, uint draw_flags, short extra_w, short extra_h);

////////////////////////////////////////////////////////////
// EDIT MENU GADGET
// 
// An edit menu is an array of editable fields.  Each field edits a different variable. 
// 

//------------------------------------------------------------
// VARELEM DEFINITION
//

// A VarElem is a description of a variable, combined with all the state necessary to edit it.
#define VARELEM_STRING_LEN    64

typedef struct {
   void *vdata;  // the variable being edited
   char vtype; // the type of the variable (see below)
   uchar flags; // some flags
   short cursor_pos; 
   char edit[VARELEM_STRING_LEN];
} VarElem;

//------------------------------------------------------------
// VARELEM TYPES 
// 

#define EDITTYPE_INT			0
#define EDITTYPE_SHORT		1
#define EDITTYPE_CHAR		2
#define EDITTYPE_STRING		3
#define EDITTYPE_FIX       4
#define EDITTYPE_FLOAT     5
/* These 4 all interpret the vdata pointer as a pointer to that specific kind of data type. */
#define EDITTYPE_CALLBACK	6
/* This vartype will just make a button in the editable area when can be pushed to trigger the 
callback.  The vdata will be interpreted as a LGadButtonCallback. */
#define EDITTYPE_CLOSE		7
/* Really a special case of EDITTYPE_CALLBACK.  Makes a "close" button which when pressed will 
go process all the text in the various edit boxes and interpret them, storing the final values 
into the vdata for that VarElem.  The menu is then destroyed. */
#define EDITTYPE_CANCEL    8
/* A button that is like EDITTYPE_CLOSE but it doesn't process any of the edit information, just 
terminates the menu without affecting the vdata variables. */
#define EDITTYPE_APPLY     9
/* like EDITTYPE_CLOSE but doesn't actually destroy it (just reprocesses the
values) . */
#define EDITTYPE_TOGGLE    10
// when clicked, will cycle through values and pass those off to the drawelement with the value of the
// variable passed off to the drawelem as draw_data2.  This is most useful with
// DRAWTYPE_VARRES and DRAWTYPE_VARSTRING.   Note that vdata actually points to an EditToggleInfo struct!!

// Note that for EDITTYPE_CLOSE, CANCEL, and APPLY, a non-NULL vdata is interpreted as like
// EDITTYPE_CALLBACK.

typedef struct {
   int *togval;
   int max_val;
} EditToggleInfo;


//------------------------------------------------------------
// VARELEM FLAGS
// 

#define EDITFLAG_NORMAL    0

#define EDITFLAG_HEX       0x01   
#define EDITFLAG_OCTAL     0x02
#define EDITFLAG_BINARY    0x04
#define EDITFLAG_READONLY  0x08
#define EDITFLAG_UNSIGNED  0x10
#define EDITFLAG_INITCLEAR 0x20
#define EDITFLAG_PRISTINE  0x40

//------------------------------------------------------------
// EDIT MENU STRUCTURE DEFINITION 
//

typedef struct {
   MENU_GUTS; 
   VarElem *varlist; 
} LGadEditMenu;

//------------------------------------------------------------
// ACCESSORS 
// 

#define LGadEditMenuNumElems(m)          LGadMenuNumElems(m)            
#define LGadEditMenuVisibleElems(m)      LGadMenuVisibleElems(m)       
#define LGadEditMenuDrawElem(m,i)        LGadMenuDrawElem(m,i)         
#define LGadEditMenuFlags(m)             LGadMenuFlags(m)              
#define LGadEditMenuCallback(m)          LGadMenuCallback(m)           
#define LGadEditMenuRedrawCall(m)        LGadMenuRedrawCall(m)         
#define LGadEditMenuCurrentSelection(m)  LGadMenuCurrentSelection(m) 

#define LGadEditMenuVarElem(m,i)         (&((LGadEditMenu*)m)->varlist[i])


#define LGadEditMenuSaveUnder(m)  LGadMenuSaveUnder(m)  


#define LGadEditMenuSetNumElems(m,v)         LGadMenuSetNumElems(m,v)         
#define LGadEditMenuSetVisibleElems(m,v)     LGadMenuSetVisibleElems(m,v)     
#define LGadEditMenuSetDrawElem(m,i,v)       LGadMenuSetDrawElem(m,i,v)       
#define LGadEditMenuSetFlags(m,v)            LGadMenuSetFlags(m,v)            
#define LGadEditMenuSetCallback(m,v)         LGadMenuSetCallback(m,v)         
#define LGadEditMenuSetRedrawCall(m,v)       LGadMenuSetRedrawCall(m,v)       
#define LGadEditMenuSetCurrentSelection(m,v) LGadMenuSetCurrentSelection(m,v) 

//------------------------------------------------------------
// EDIT MENU FUNCTIONS
// 

//
// Creation
// 

EXTERN LGadEditMenu *LGadCreateEditMenuArgs(LGadEditMenu *vm, LGadRoot *vr, short x, short y, short w, 
	short h, short num_elems, short vis_elems, DrawElement *elems, ushort flags, ushort draw_flags, 
	DrawElement *inner, VarElem *varlist, char paltype);
EXTERN LGadEditMenu *LGadCreateEditMenu(LGadEditMenu *vm, LGadRoot *vr, short x, short y, short w, short h, char paltype);

// Force an editmenu's variables to update
EXTERN void LGadEditMenuProcess(LGadEditMenu *vem);

// Set the text fields for a menu from the data
EXTERN void LGadEditMenuSetText(LGadEditMenu *vm);

#endif // __GADMENU_H



