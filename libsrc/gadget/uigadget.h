/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifndef __UIGADG_H
#define __UIGADG_H
/*
 * $Source: r:/t2repos/thief2/libsrc/gadget/uigadget.h,v $
 * $Revision: 1.1 $
 * $Author: mahk $
 * $Date: 1996/07/25 03:32:39 $
 *
 */
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <event.h>   

//**************************************
// REVISED GADGET SYSTEM INFRASTRUCTURE
// MAHK 1/95
// TO GO INTO THE UI LIBRARY SOMEDAY
//--------------------------------------
// Gadgets are high-level abstractions that implement 
// elements of a user-interface. 
// A typical gadget implementation will not only assist in
// displaying a user-interface element, but will help with 
// high-level input processing and feedback as well.  
// Each gadget lives in a region, and sends a USER_DEFINED
// event to that region whenever important status changes 
// occur.  The basic format for such an event is this: 

typedef struct _GadgEvent
{
   UIEVFRONT
   ushort signaller; // what kind of gadget signalled this.
   ushort action; // gadget-specific data here. 
   UIEVBACK(sizeof(ushort));
} GadgEvent;

// Every type of gadget has a unique "signaller" code.  These are
// defined here: 

enum
{
   BUTTONGADG_SIGNALLER,
   BUTTONTOGGLE_SIGNALLER,
   BUTTONLIST_SIGNALLER,
   TEXTED_SIGNALLER,
};

// A client of a ui gadget can install a USER_DEFINED event callback
// on the gadget's region in order to process status changes signalled by
// the gadget.  In general, such a callback should always return FALSE, 
// and should always check the signaller of any event received to be sure 
// that it matches the expected gadget type.  This way, new gadgets 
// can easily be implemented on top of old ones.  






#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __UIGADG_H

