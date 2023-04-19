/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/gadget/RCS/gadbutt.h 1.1 1996/08/26 02:02:49 mahk Exp $

#ifndef __GADBUTT_H
#define __GADBUTT_H

#include <gadbase.h>
#include <gadbox.h>
#include <drawelem.h>

////////////////////////////////////////////////////////////
// BUTTON GADGETS
// 
// Button gadgets have more sophisticated behavior than boxes. 
// They fire when the mouse goes down and up within their area.
// They also know how to draw themseves.  
// 

//------------------------------------------------------------
// Button callback
//
// Called when a button fires.  Action is a bitmast describing 
// what happened. 
// 
// The data field is not used, by buttons, but is used by some gadgets that are derived
// from buttons.

typedef bool (*LGadButtonCallback)(short action, void *data, LGadBox *vb);

// basic "click" flags
#define LGADBUTT_LCLICK    MOUSE_LUP // left mouse click, possibly the first of a double click.
#define LGADBUTT_RCLICK    MOUSE_RUP // as above, but right button
#define LGADBUTT_CCLICK    MOUSE_CUP // as above, but center button
#define LGADBUTT_BTN2CLICK(i) MOUSE_BTN2UP(i)

// double click flags
#define LGADBUTT_LDOUBLE   UI_MOUSE_LDOUBLE // left mouse double-click. always follows _LCLICK
#define LGADBUTT_RDOUBLE   UI_MOUSE_RDOUBLE // as above, but right mousebutton
#define LGADBUTT_CDOUBLE   UI_MOUSE_CDOUBLE // as above, but center button
#define LGADBUTT_BTN2DOUBLE(i) UI_MOUSE_BTN2DOUBLE(i)

// "timeout" flags
#define LGADBUTT_LTIMEOUT  UI_MOUSE_LTIMEOUT  // signalled when the UI is sure that 
                                              // no _LDOUBLE can follow the last _LCLICK
#define LGADBUTT_RTIMEOUT  UI_MOUSE_RTIMEOUT // as above, but right button
#define LGADBUTT_CTIMEOUT  UI_MOUSE_CTIMEOUT // as above, but center button 

//------------------------------------------------------------
// Structure definition
// 

// internal structure, implementation dependent
typedef struct _ButtonGadg
{
   LGadBox box;
      
   DrawElement draw; // how we are displayed.
   ulong state; // Drawelem state
   short last_action; // what unresolved down events are there.
   bool focus; // do we have mouse move focus
   ulong flags;
} ButtonGadg;

// 
// actual LGadButton type
// 

#define BUTTON_GUTS \
   ButtonGadg gadg; \
   bool down;\
   LGadButtonCallback buttonfunc;\
   ulong back_color

typedef struct {
   BUTTON_GUTS;
} LGadButton;

//------------------------------------------------------------
// ACCESSORS
// 

// old accessor for back-compatibility
#define BUTTON_DRAWELEM(b)  (((LGadButton*)b)->gadg.draw)


#define LGadButtonDrawElem(b) BUTTON_DRAWELEM(b)
#define LGadButtonState(b)    (((LGadButton*)b)->gadg.state  )
#define LGadButtonCallback(b) (((LGadButton*)b)->buttonfunc  ) 
#define LGadButtonFlags(b)    (((LGadButton*)b)->gadg.flags ) 

#define LGadButtonSetDrawElem(b,v) (BUTTON_DRAWELEM(b) = (v))
#define LGadButtonSetState(b,v)    (((LGadButton*)b)->gadg.state = (v) )
#define LGadButtonSetCallback(b,v) (((LGadButton*)b)->buttonfunc = (v) ) 
#define LGadButtonSetFlags(b,v)    (((LGadButton*)b)->gadg.flags = (v)) 

//------------------------------------------------------------
// FLAGS
//

#define LGADBUTT_FLAG_STATELESS 0x1  // only draw dsNORMAL state
#define LGADBUTT_FLAG_GETDOWNS  0x2  // Get button down events, in addition to the others. 

//------------------------------------------------------------
// BUTTON GADGET FUNCTIONS
//

// Create a button
EXTERN LGadButton *LGadCreateButtonArgs(LGadButton *vb, LGadRoot *vr, short x, short y, short w, short h,  DrawElement *draw, LGadButtonCallback bfunc, char paltype);


////////////////////////////////////////////////////////////
// TOGGLE GADGETS
// Like buttons, but have multiple states.  They toggle an integer through values from 0..n
//
// Toggles have button callbacks, and pass the current state of the toggle through the 
// callback's "data" argument.
// 
//

//------------------------------------------------------------
// STRUCTURE DEFINITION
//

#define TOGGLE_GUTS \
   BUTTON_GUTS; \
   int *val_ptr;\
   short max_val;\
   short increm
   
typedef struct {
   TOGGLE_GUTS;
} LGadToggle;

//------------------------------------------------------------
// ACCESSORS
//

#define LGadToggleVal(b)        (*((LGadToggle*)b)->val_ptr  )
#define LGadToggleMax(b)        (((LGadToggle*)b)->max_val   )
#define LGadToggleIncrem(b)     (((LGadToggle*)b)->increm    )

#define LGadToggleSetVal(b,v)        (*((LGadToggle*)b)->val_ptr = (v))
#define LGadToggleSetMax(b,v)        (((LGadToggle*)b)->max_val  = (v))
#define LGadToggleSetIncrem(b,v)     (((LGadToggle*)b)->increm   = (v))

//------------------------------------------------------------
// TOGGLE GADGET FUNCTIONS 
//


// create a toggle
EXTERN LGadToggle *LGadCreateToggle(LGadToggle *vt, LGadRoot *vr, short x, short y, 
                                    short w, short h, char paltype); 	
EXTERN LGadToggle *LGadCreateToggleArgs(LGadToggle *vt, LGadRoot *vr, short x, short y, short w, short h, 	
   DrawElement *draw, LGadButtonCallback bfunc, int *val_ptr, short max_val, short increm, char paltype);



#endif // __GADBUTT_H








