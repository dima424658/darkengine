#ifndef _MOUSEVEL_H
#define _MOUSEVEL_H

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus

/*
 * $Source: x:/prj/tech/libsrc/input/RCS/mousevel.h $
 * $Revision: 1.2 $
 * $Author: DAVET $
 * $Date: 1996/01/25 14:20:42 $
 *
 */

// --------------------------------------------------------------
//                MOUSE-LIBRARY SUPPORT FOR MOUSE 
//              EMULATION BY VELOCITY-BASED DEVICES 
// --------------------------------------------------------------




/* ------------------------------------------------------
   These routines allow the client to set a _velocity_ for the mouse
   pointer.   The mouse pointer will move at the specified velicity 
   whenever the mouse's current position is queried.   (Queries include
   mouse_get_xy (always) and queue checks (when the queue is empty).)
   Mouse velocity is specified in units of pixels per MOUSE_VEL_UNIT ticks,
   where ticks are the unit of the mouse library timestamp register.
   These routines are intended for use in emulating the mouse with other
   non-positional devices.  (keyboard, joystick, cyberbat, etc.) 
   ------------------------------------------------------ */


#define MOUSE_VEL_UNIT_SHF 16
#define MOUSE_VEL_UNIT (1 << MOUSE_VEL_UNIT_SHF)


errtype mouse_set_velocity_range(int xl, int yl, int xh, int yh);
// Specifies the range of the mouse pointer velocity.  
// (xl,yl) is the low end of the range, whereas (xh,yh) 
// is the high end.  For most applications xl and xh will have the same 
// absolute value, as with yl and yh. 

errtype mouse_set_velocity(int x, int y);
// Sets the velocity of the mouse pointer, 
// in units of pixels per MOUSE_VEL_UNIT ticks. 

errtype mouse_add_velocity(int x, int y);
// Adds x and y to the mouse pointer velocity, constraining it 
// to remain withing the range specified by mouse_set_velocity_range() 

errtype mouse_get_velocity(int *x, int *y);
// Gets the current value of the mouse pointer velocity, as 
// set by mouse_set_velocity and mouse_add_velocity.


#ifdef __cplusplus
}
#endif  // cplusplus

#endif // _MOUSEVEL_H
