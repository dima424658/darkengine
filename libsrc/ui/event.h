/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/ui/RCS/event.h 1.28 1999/12/28 11:59:40 JAEMZ Exp $

#ifndef _EVENT_H
#define _EVENT_H
#pragma once

#include <lg.h>
#include <error.h>
#include <slab.h>
#include <region.h>
#include <mouse.h>


#define UIEV_DATASIZE 6


#define UIEVFRONT  Point pos;   /* all ui events have a "screen" position */  \
ulong type;         /* An event type, 32 possible.   */

#define UIEVBACK(sz) char pad[UIEV_DATASIZE-(sz)]

// ---------------
// INPUT EVENTS
// ---------------

// Generalized input event struct
typedef struct _ui_event
{
  UIEVFRONT
  short subtype;                 /* type specific */
  char data[UIEV_DATASIZE];      /* type specific */
} uiEvent;


// Type field values
#define UI_EVENT_NULL           0x00000000
#define UI_EVENT_KBD_RAW        0x00000001
#define UI_EVENT_KBD_COOKED     0x00000002
#define UI_EVENT_KBD_POLL       0x00000020
#define UI_EVENT_MOUSE          0x00000004
#define UI_EVENT_MOUSE_MOVE     0x00000008
#define UI_EVENT_JOY            0x00000010
#define UI_EVENT_MIDI           0x10000000  // Hey, gotta be ready for the future.
#define UI_EVENT_USER_DEFINED   0x80000000
#define ALL_EVENTS              0xFFFFFFFF


// Type-specific versions of event structs
// ---------------------------------------


// Raw key events
typedef struct _ui_raw_key_event
{
  UIEVFRONT
  short scancode;    /* subtype */
  uchar action;      /* KBS_UP or _DOWN */
  UIEVBACK(sizeof(uchar));
} uiRawKeyEvent;

typedef uiRawKeyEvent uiPollKeyEvent;

// Cooked key events
typedef struct _ui_cooked_key_event
{
  UIEVFRONT
  short code;        /* cooked keycode, chock full o' stuff */
  UIEVBACK(0);
} uiCookedKeyEvent;

// mouse events
typedef struct _ui_mouse_event
{
  UIEVFRONT
  short action;        /* mouse event type, as per mouse library */
  ulong tstamp;
  ubyte buttons;
  char wheel;   // would rather make it short, but we're out of space!
  // Full house!   UIEVBACK(sizeof(ulong)+sizeof(ubyte)+sizeof(char));
} uiMouseEvent;

// joystick events
typedef struct _ui_joy_event
{
  UIEVFRONT
  short action;        /* joystick event subtype, as defined below */
  uchar joynum;        /* joystick number */
  Point joypos;     /* joystick position */
  UIEVBACK(sizeof(uchar)+sizeof(Point));
} uiJoyEvent;

// user-defined events
typedef struct _ui_user_defined_event
{
  UIEVFRONT
  short action;        /* event subtype, as defined by application */
  UIEVBACK(0);
} uiUserDefinedEvent;

// extended mouse event types (double clicks)
#define UI_MOUSE_LDOUBLE   (1 << 7)
#define UI_MOUSE_RDOUBLE   (1 << 8)
#define UI_MOUSE_CDOUBLE   (1 << 9)
#define UI_MOUSE_BTN2DOUBLE(i) (128 << (i))

// These events are signaled when a down event "times out"
// and is no longer qualified to be a double click.

#define UI_MOUSE_LTIMEOUT (1 << 10)
#define UI_MOUSE_RTIMEOUT (1 << 11)
#define UI_MOUSE_CTIMEOUT (1 << 12)
#define UI_MOUSE_BTN2TIMEOUT(i) (UI_MOUSE_LTIMEOUT << (i))

#define UI_JOY_MOTION         0
#define UI_JOY_BUTTON1UP      1
#define UI_JOY_BUTTON2UP      2
#define UI_JOY_BUTTON1DOWN    3
#define UI_JOY_BUTTON2DOWN    4


// ----------------
//  EVENT HANDLERS
// ----------------

/* Event handlers are installed by the client to receive callbacks when events
   happen.  Event handlers are called when the ui toolkit is
   polled.  Interrupt-driven phenomena such as mouse cursors will, in
   general, be internal to the ui-toolkit.  An event handler is installed
   on a region, and will receive events when the mouse is in that region.
   It is possible to chain event handlers within a region.  In
   this case, an event is "offered" to each event handler, in order,
   one at a time until an event handler chooses to accept it.   */


typedef BOOL (*uiHandlerProc)(uiEvent* e, Region* r, void* state);

// If an event-handler's proc returns true, it has accepted the event, and no
// other event handler will see the event.  An event handler will only be
// offered those events specified by its typemask; it automatically rejects
// any other events.

EXTERN errtype uiInstallRegionHandler(Region* v, ulong evmask, uiHandlerProc proc, void* state, int* id);
// installs an event handler at the front of r's handler chain.  The event handler
// will call "proc" with the event, the region "v", and the value of "state"
// whenever "v" receives any event whose type bit is set in evmask.
// sets *id to an id for that event handler.


EXTERN errtype uiRemoveRegionHandler(Region* v, int id);
// Removes the event handler with the specified id from a region's handler chain

EXTERN errtype uiSetRegionHandlerMask(Region* r, int id, int evmask);
// Changes the event mask for handler #id in region r.

EXTERN errtype uiShutdownRegionHandlers(Region* r);
// Shut down and destroy all handlers for a region.

typedef void (*deferfunc)(void* ); 

EXTERN errtype uiDefer(deferfunc func, void *data); 
// call func(data) when polling is done. 

// --------------
// REGION OPACITY
// --------------

// The opacity of a region is the mask of event types that cannot pass through the
// region.  Set bits in the opacity mask indicate that events of that type will be
// automatically rejected if they reach that region, and will not be offered to any other region.

// Se uiDefaultRegionOpacity  in the globals section

EXTERN errtype uiSetRegionOpacity(Region* r, ulong opacity);
// Sets the opacity of a region.

EXTERN ulong uiGetRegionOpacity(Region* r);
// Gets the opacity mask of the region.


// -----------
// INPUT FOCUS
// -----------

EXTERN errtype uiGrabFocus(Region* r, ulong evmask);
// grabs input focus on the active slab for region r for events specified by evmask

EXTERN errtype uiReleaseFocus(Region* r, ulong evmask);
// If r has the current input focus in the active slab, then releases r's
// focus on the events specified by  evmask, and restores the previous focus.  Else does nothing.

EXTERN errtype uiGrabSlabFocus(uiSlab* slab, Region* r, ulong evmask);
// Grabs focus for region r on the specified slab.

EXTERN errtype uiReleaseSlabFocus(uiSlab* slab, Region* r, ulong evmask);
// If r has the current input focus in the specified slab, then releases r's
// focus on the events specified by  evmask, and restores the previous focus.
// Else does nothing.


// -----------------------
// POLLING AND DISPATCHING
// -----------------------

EXTERN errtype uiPoll(void);
// polls the ui toolkit, dispatching all events.


EXTERN errtype uiQueueEvent(uiEvent* ev);
// adds an event to the ui event queue.  The event will be dispatched at the next uiPoll() call

EXTERN bool uiDispatchEvent(uiEvent* ev);
// Dispatches an event right away, without queueing. Returns
// Whether or not the event was accepted by a handler.

EXTERN bool uiDispatchEventToRegion(uiEvent* ev, Region* r);
// Like uiDispatchEvent, but dispatches an event to a
// specific region's event handlers.

EXTERN errtype uiSetMouseMotionPolling(bool poll);
// Iff poll is true, exactly one mouse motion event will be  generated
// per call to uiPoll, the motion event will be generated by polling the
// mouse position.  Otherwise, all motion events generated by the interrupt handler will
// be dispatched, and thus no motion event will be dispatched if the mouse has not moved.
// Defaults to FALSE

EXTERN errtype uiMakeMotionEvent(uiMouseEvent* ev);
// Fills *ev with a mouse motion event reflecting the current mouse position.

EXTERN errtype uiSetKeyboardPolling(uchar* codes);
// Codes is a KBC_NONE terminated array of scancodes to be polled by the system.
// if  a code is in the list, the specified key will generate one keyboard polling event
// (type UI_EVENT_KBD_POLL) per call to uiPoll.  otherwise, no such event will be generated
// for this key.
// defaults to NULL.

EXTERN errtype uiFlush(void);
// Flushes all ui system input events.

EXTERN bool uiCheckInput(void);
// reads through the input queue, returning true if there
// is a key or mouse button up event, false otherwise.

typedef void (*key_handler)(ushort keycode);

EXTERN bool uiCheckKeys(key_handler handler);
// like uiCheckInput, but takes calls key_handler
// with the cooked keycode of any pending keyboard events.

// ---------------------------
// INITIALIZATION AND SHUTDOWN
// ---------------------------

EXTERN errtype uiInit(uiSlab* slab);
// Initialize the ui toolkit.
// Sets the current slab.

EXTERN void uiShutdown(void);
// shuts down the ui toolkit.



// ----------------
//     GLOBALS
// ----------------

EXTERN ushort uiDoubleClickTime;
// The maximum time separation between individual clicks of a double click

EXTERN ushort uiDoubleClickDelay;
// The maximum allowed time between the first down and up event in a double click

EXTERN bool uiDoubleClicksOn[NUM_MOUSE_BTNS];
// are double clicks allowed for the specified button?
// defaults to FALSE

EXTERN bool uiAltDoubleClick;
// Whether alt-click should emulate double click.
// Defaults to FALSE;

EXTERN Region* uiLastMouseRegion[NUM_MOUSE_BTNS];
// Stores a pointer to the region that accepted the
// last down event for each button.

EXTERN ushort uiDoubleClickTolerance;
// How much mouse motion will we tolerate before discarding
// a potiential double click.  Defaults to 5.

EXTERN ulong uiGlobalEventMask;
// Global mask of what events are to be dispatched.
// initially set to ALL_EVENTS

EXTERN ulong uiDefaultRegionOpacity;
// The initial value of the opacity of a region, used until
// an opacity is set for the region.
// Defaults to zero.
// When a handler is added to a region, the opacity is set to the
// value of uiDefaultRegionOpacity.

EXTERN bool uiJoystickMouseEmulation;
// Are we emulating the mouse with the joystick?

EXTERN Point uiJoystickMouseGain;
// Joystick-emulated mouse sensetivity

// Recording Support
typedef bool (*uiEventRecordFunc)(uiEvent *ev);

// By default you can't have two overlapping buttons and expect
// one to get the event if the other doesn't.  This is done as an
// optimization, I guess, but now you can control it.
// This defaults to FALSE -JF
EXTERN bool uiAllowOverlappingRegions;


EXTERN uiEventRecordFunc ui_recording_func;



#endif // _EVENT_H