//		Mouse.H		Mouse library header file
//		MAHK Leblanc 2/19/93
/*
 * $Source: x:/prj/tech/winsrc/input/RCS/mouse.h $
 * $Revision: 1.21 $
 * $Author: TOML $
 * $Date: 1996/11/07 14:39:47 $
 *
 * $Log: mouse.h $
 * Revision 1.21  1996/11/07  14:39:47  TOML
 * Improved thread support
 * 
 * Revision 1.20  1996/09/26  10:57:58  TOML
 * Added mutex accessor
 *
 * Revision 1.19  1996/09/23  16:59:03  TOML
 * Made C++ parser friendly
 *
 * Revision 1.18  1996/04/24  10:06:35  TONY
 * Obliterated mouse_event definition, use lgMouseEvent instead
 * where necessary.
 *
 * Revision 1.17  1996/04/18  16:43:12  TOML
 * *** empty log message ***
 *
 * Revision 1.16  1996/04/11  12:52:34  KEVIN
 * added mouse_event typedef when not in win32 for backwards compatability.
 *
 * Revision 1.15  1996/02/28  13:47:19  TOML
 * Timer adjustments
 *
 * Revision 1.14  1996/02/15  14:24:12  TOML
 * *** empty log message ***
 *
 * Revision 1.13  1996/01/25  14:20:39  DAVET
 * Added cplusplus stuff
 *
 * Revision 1.12  1996/01/19  11:21:55  JACOBSON
 * Initial revision
 *
 * Revision 1.11  1993/09/01  00:19:18  unknown
 * Changed left-handedness api
 *
 * Revision 1.10  1993/08/29  03:12:35  mahk
 * Added mousemask and lefty support.
 *
 * Revision 1.9  1993/08/27  14:05:06  mahk
 * Added shift factors
 *
 * Revision 1.8  1993/07/28  18:15:58  jak
 * Added mouse_extremes() function
 *
 * Revision 1.7  1993/06/28  02:04:59  mahk
 * Bug fixes for the new regime
 *
 * Revision 1.6  1993/06/27  22:17:32  mahk
 * Added timestamps and button state to the mouse event structure.
 *
 * Revision 1.5  1993/05/04  14:34:27  mahk
 * mouse_init no longer takes a screen mode argument.
 *
 * Revision 1.4  1993/04/14  12:08:46  mahk
 * Hey, I got my mouse ups and downs backwards.
 *
 * Revision 1.3  1993/03/19  18:46:57  mahk
 * Added RCS header
 *
 *
 */

#ifndef MOUSE_H
#define MOUSE_H

#include "lg.h"
#include "error.h"

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus


typedef struct _lgMouseEvent
{
   short x;    // position
   short y;
   uchar type; // Event mask, bits defined below
   ulong timestamp;
   uchar buttons;
   char pad[6];  // pad to sixteen bytes
} lgMouseEvent;

#if 0
// Note: For WIN32 compiles, this clashes with the mouse_event
//       definition in winuser.h, which is included by windows.h
//       (tony 04-24-96)

// For backward compatability (toml 04-18-96)
typedef struct _lgMouseEvent mouse_event;
#endif

#define MOUSE_MOTION    1
#define MOUSE_LDOWN     2
#define MOUSE_LUP       4
#define MOUSE_RDOWN     8
#define MOUSE_RUP      16
#define MOUSE_CDOWN    32
#define MOUSE_CUP      64


// Mask of events that are allowed into the queue.
extern ubyte mouseMask;

// type of mouse interrupt callback func
typedef void (*mouse_callfunc)(lgMouseEvent* e,void* data);


#define NUM_MOUSE_BTNS 3
#define MOUSE_LBUTTON 0
#define MOUSE_RBUTTON 1
#define MOUSE_CBUTTON 2

#define MouseMakeButtonBit(b) \
    (1 << b)

#define MouseMakeButtons(l, c, r) \
    (((!!l) << MOUSE_LBUTTON) | ((!!c) << MOUSE_CBUTTON) | ((!!r) << MOUSE_CBUTTON))

#define MouseIsButtonDown(buttons, which) \
    ((buttons >> which) & 0x01)

#define MOUSE_BTN2DOWN(num) (1 << (1+ 2*(num)))
#define MOUSE_BTN2UP(num) (1 << (2+ 2*(num)))

// Initialize the mouse, specifying screen size.
errtype mouse_init(short xsize, short ysize);

// shutdown mouse system
void mouse_shutdown(void);

// Tell the mouse library where to get timestamps from.
#ifndef _WIN32
errtype mouse_set_timestamp_register(ulong* tstamp);
#else
typedef ulong (*fn_mouse_ticks_t)();
ulong default_mouse_ticks();
errtype mouse_set_timestamp_register(fn_mouse_ticks_t tstamp);
#endif

// Get the current mouse timestamp
ulong mouse_get_time(void);

// Get the mouse position
errtype mouse_get_xy(short* x, short* y);

// Set the mouse position
errtype mouse_put_xy(short x, short y);

// Check the state of a mouse button
errtype mouse_check_btn(int button, bool* result);

// look at the next mouse event.
errtype mouse_look_next(lgMouseEvent* result);

// get & pop the next mouse event
errtype mouse_next(lgMouseEvent* result);

// Flush the mouse queue
errtype mouse_flush(void);

// Add an event to the back of the mouse queue
errtype mouse_generate(lgMouseEvent e);

// Set up an interrupt callback
errtype mouse_set_callback(mouse_callfunc f, void* data, int* id);

// Remove an interrupt callback
errtype mouse_unset_callback(int id);

// Constrain the mouse coordinates
errtype mouse_constrain_xy(short xl, short yl, short xh, short yh);

// Set the mouse rate and accelleration threshhold
errtype mouse_set_rate(short xr, short yr, short thold);

// Get the mouse rate and accelleration threshhold
errtype mouse_get_rate(short* xr, short* yr, short* thold);

// Sets the mouse coordinate bounds to (0,0) - (x-1,y-1),
// and scales the current values of the mouse sensitivity accordingly.
errtype mouse_set_screensize(short x, short y);

// Find the min and max "virtual" coordinates of the mouse position
errtype mouse_extremes( short *xmin, short *ymin, short *xmax, short *ymax );

// Sets mouse handedness (true for left-handed)
errtype mouse_set_lefty(bool lefty);

// Acquire/release mouse queue mutex
void LGAPI mouse_wait_for_queue_mutex(void);
void LGAPI mouse_release_queue_mutex(void);
HANDLE LGAPI mouse_get_queue_available_signal(void);

#ifdef __cplusplus
}
#endif  // cplusplus

#endif // _MOUSE_H


