// Mousesys.h
// internal header stuff for the mouse

#ifndef __MOUSESYS_H
#define __MOUSESYS_H

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus


typedef struct _mouse_state
{
  short x,y;
  short butts;
} mouse_state;

#define NUM_MOUSEEVENTS 32
#define NUM_MOUSE_CALLBACKS 16

//	These are global for fast access from interrupt routine & others

extern short gMouseCritical;				// in critical region?

extern short mouseQueueSize;
extern volatile short mouseQueueIn;	     // back of event queue
extern volatile short mouseQueueOut;      // front of event queue
extern lgMouseEvent mouseQueue[];	// array of events
extern short mouseInstantX;					// instantaneous mouse xpos (int-based)
extern short mouseInstantY;					// instantaneous mouse ypos (int-based)
extern short mouseInstantButts;
extern short mouseButtMask;   // amt to mask to get buttons.
extern ubyte mouseXshift;  // Extra bits of mouse resolution
extern ubyte mouseYshift;

extern ubyte mouseMask; // mask of events to put in the queue.
extern bool  mouseLefty; // is the user left-handed?

extern mouse_callfunc mouseCall[];
extern void* mouseCallData[];
extern short mouseCalls;  // current number of mouse calls.
extern short mouseCallSize;

extern bool mouse_installed;					// was mouse found?

#ifndef _WIN32
extern ulong default_mouse_ticks;
extern ulong volatile *mouse_ticks;  // Place to get mouse timestamps.
#else
typedef ulong (*fn_mouse_ticks_t)();
extern fn_mouse_ticks_t pfn_mouse_ticks;  // Place to get mouse timestamps.
#endif

// MOUSE VELOCITY STUFF

extern int mouseVelX, mouseVelY;
extern int mouseVelXmax;
extern int mouseVelYmax;
extern int mouseVelXmin;
extern int mouseVelYmin;

//	Macros & defines

#define MOUSECRITON() (gMouseCritical++)
#define MOUSECRITOFF() (gMouseCritical--)

#ifdef __cplusplus
}
#endif  // cplusplus

#endif // __MOUSESYS_H
