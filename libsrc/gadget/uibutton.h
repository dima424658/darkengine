/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifndef __UIBUTTON_H
#define __UIBUTTON_H
/*
 * $Source: r:/t2repos/thief2/libsrc/gadget/uibutton.h,v $
 * $Revision: 1.4 $
 * $Author: mahk $
 * $Date: 1996/08/26 02:03:03 $
 *
 */

#ifdef __cplusplus
extern "C"
{
struct _ButtonCallback;
struct _ButtonGadg;
#endif // __cplusplus

#include <rect.h>
#include <2d.h>
#include <region.h>
#include <event.h>
#include <gadget.h>
#include <drawelem.h>

//***********************************************
// PUSHBUTTON GADGETS FOR FLIGHT
// MAHK 11/94
// TO GO INTO THE UI LIBRARY AT SOME POINT
// Ported to LGad by MAHK 7/96
// ---------------------------------------

typedef DrawElement ButtonDisplay;

// ===============================================
// BASE BUTTON GADGET TYPE
// -----------------------
//
// A basic button gadget provides some simple preprocessing of 
// mouse click events, based on a model in which click-and-drag
// has no meaning.  
// The client may install a USER_DEFINED event handler on the 
// button's region, whose "action" fields are described below.
// In general, handlers installed by the client should always 
// return FALSE, to allow other clients and standard implementations
// to receive this data also. 
// Buttons keep track of, and optionally display, their "depressedness."
// A button is "depressed" when a mouse button has gone down over the button gadget,
// but has not come up, and the mouse has not left the button gadget's rect.

// action bits
#define BUTTONGADG_LCLICK    MOUSE_LUP // left mouse click, possibly the first of a double click.
#define BUTTONGADG_RCLICK    MOUSE_RUP // as above, but right button
#define BUTTONGADG_CCLICK    MOUSE_CUP // as above, but center button

#define BGADG_BTN2CLICK(i) MOUSE_BTN2UP(i)

#define BUTTONGADG_LDOUBLE   UI_MOUSE_LDOUBLE // left mouse double-click. always follows _LCLICK
#define BUTTONGADG_RDOUBLE   UI_MOUSE_RDOUBLE // as above, but right mousebutton
#define BUTTONGADG_CDOUBLE   UI_MOUSE_CDOUBLE // as above, but center button

#define BGADG_BTN2DOUBLE(i) UI_MOUSE_BTN2DOUBLE(i)

#define BUTTONGADG_LTIMEOUT  UI_MOUSE_LTIMEOUT  // signalled when the UI is sure that 
                                                // no _LDOUBLE can follow the last _LCLICK
#define BUTTONGADG_RTIMEOUT  UI_MOUSE_RTIMEOUT // as above, but right button
#define BUTTONGADG_CTIMEOUT  UI_MOUSE_CTIMEOUT // as above, but center button 

#define BGADG_BTN2TIMEOUT(i) UI_MOUSE_BTN2TIMEOUT(i)

//------------------------------------------------------
// ButtonGadgInit()
//
// Initializes a basic button gadget.  
// parent: region the button lives in.
// btn:    button structure to be initialized
// area:   the coordinates of the buttons bounding rectangle.
// z:      the button's region z coordinate.
// dp:     A pointer to display data, need not persist.
extern errtype ButtonGadgInit(Region* parent, ButtonGadg* btn, Rect* area, int z, ButtonDisplay* dp);

//----------------------------------------------------
// ButtonGadgRegion()
//
// Accessors for button data
#define ButtonGadgRegion(btn) ((btn)->box.r)

//----------------------------------------------------
// ButtonGadgDestroy()
//
// Performs any necessary shutdown on a button gadget

extern errtype ButtonGadgDestroy(ButtonGadg* bttn);

//******************************************************
// BUTTON GADGET VARIANTS AND STANDARD IMPLEMENTATIONS
// -----------------------------------------------------

//======================================================
// Toggle buttons.
//------------------------------------------------------
// An Toggle button has an integer state variable 
// which varies from 0 to N-1.  
// The state increments whenever the user left-clicks 
// on the button, and decrements whenever the 
// user right-clicks on the button.  
// toggle buttons signal a special gadget event whenever the
// button's state changes.

typedef struct _ToggleEvent
{
   UIEVFRONT
   ushort signaller; // what kind of gadget signalled this. 
   ushort action; // same bits as normal button gadget
   int new_state;  // new state of the button
//   UIEVBACK(sizeof(ushort)+sizeof(int));
} ToggleEvent;


typedef struct _ButtonToggle
{
   ButtonGadg button;
   int* statevar;
   ButtonDisplay* dispvec;
   int n;
} ButtonToggle;

//------------------------------------------
// ButtonToggleInit()
//
// inits a basic n-state button gadget.  To create a toggle button, one must first initialize
// a button as a regular ButtonGadg, giving NULL as its button display.
// Then call ButtonToggleInit to fill in extra state data.
// bttn:    A pointer to a ButtonToggle struct that has already been initialized as a ButtonGadg.
// n   :    the number of states
// statevar: a pointer to the integer value that holds the button's state
// dispvec:  an array of N buttondisplays

extern errtype ButtonToggleInit(ButtonToggle* bttn, int n, int* statevar, ButtonDisplay dispvec[]);


//------------------------------------------
// ButtonToggleState()
// ButtonToggleStateSet()
//
// gets and sets a toggle button's state, for those who 

#define ButtonToggleState(bt) (*(bt)->statevar)
#define ButtonToggleStateSet(bt,val) (*(bt)->statevar = (val))



//------------------------------------------
// ButtonToggleDestroy()
// 

extern errtype ButtonToggleDestroy(ButtonToggle* bttn);

//==============================================
// BUTTON LISTS
//
// A button list gadget is a sparse aggregate list of pushbuttons 
// that all live in the same region.  
// Whenever a button in a button list is clicked, the following
// event is sent to the buttonlist's region: 

struct _ButtonListEvent;
typedef struct _ButtonListEvent
{
   UIEVFRONT
   ushort signaller; // what kind of gadget signalled this. 
   ushort action; // same bits as normal button gadget
   int button;  // the number of the clicked button
//   UIEVBACK(sizeof(ushort)+sizeof(int));
} ButtonListEvent;


//------------------------------------------
// The buttonlist type
// 
// 

struct _ButtonList;
typedef struct _ButtonList
{
   ButtonGadg gadg;
   ButtonDisplay* dvec;
   Rect* rvec;
   ulong* statevec;
   bool focused;
   int num_buttons;
   int curr_button;
   int key_button;      
   ulong flags;
   int last_button;
} ButtonList;


//
// FLAGS
//

#define BLIST_RADIO_FLAG 0x1

//------------------------------------------
// ButtonListInit()
//
// Inits a button list:
//
// parent   :  the parent region for the gadget.
// list     :  the list to be initialized.
// z        :  the z-coordinate (parent's frame) 
//             that all buttons on the list will occupy
// n        :  the number of buttons in the list
// rvec     :  a vector of the N rects for the buttons.
// dispvec  :  an array of the N displays for the buttons

EXTERN errtype ButtonListInit(Region* parent, ButtonList* list, int z, int n, Rect rvec[], ButtonDisplay dispvec[], ulong flags);


//-------------------------------------------
// ButtonList Accessors

#define ButtonListRegion(bl) (ButtonGadgRegion(&(bl)->gadg))

#define BLIST_DOWN_BIT 0x100 
#define BLIST_DSTATE_MASK 0xFF


#define ButtonListButtonDown(bl,n) (((bl)->statevec[n] & BLIST_DOWN_BIT) != 0)
#define ButtonListDrawState(bl,n)  ((bl)->statevec[n] & ~BLIST_DOWN_BIT)

//
// Radio-button specific accessors
// 

// which radio button is in.
#define RadioButtonSelection(bl) ((bl)->last_button)

EXTERN errtype RadioButtonSetSelection(ButtonList* bl, int new_sel);



//-------------------------------------------
// ButtonListDestroy

extern errtype ButtonListDestroy(ButtonList* button);

//--------------------------------------------
// ButtonListEnableKeyboard
//
// enables the keyboard interface for a buttonlist, grabbing key
// focus.  def specifies the default value for the button. 
// if warp is true, the mouse warps to the default button.

extern errtype ButtonListEnableKeyboard(ButtonList* list, uint def, bool warp);


//-------------------------------------------
// BuildButtonRects
//
// Given an array of buttondisplays and a bounding rect,
// generate a rectangle for each display within that bounding
// rect, spacing appropriately.
// Fill an array of rectangles with these rects.
//

extern void BuildButtonRects(ButtonDisplay* dvec, int num_displays, Rect bounds, Rect rvec[]);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __UIBUTTON_H

