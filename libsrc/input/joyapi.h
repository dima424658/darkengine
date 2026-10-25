/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/winsrc/input/RCS/joyapi.h $
// $Author: darrenl $
// $Date: 1999/02/11 10:49:19 $
// $Revision: 1.4 $
//
// Joystick device API
//

#ifndef __JOYAPI_H
#define __JOYAPI_H

#include <comtools.h>
#include <inpapi.h>

///////////////////////////////////////
//
// Forward declarations
//

F_DECLARE_INTERFACE(IJoystick);

///////////////////////////////////////////////////////////////////////////////
//
// Structures and constants
//

typedef int eJoystickObjs;

///////////////////////////////////////
//
// Joystick state
//

// hat positions
EXTERN const DWORD kJoyHatCenter;
EXTERN const DWORD kJoyHatUp;
EXTERN const DWORD kJoyHatRight;
EXTERN const DWORD kJoyHatDown;
EXTERN const DWORD kJoyHatLeft;

// for c, switch statements, etc
#define JOY_HAT_CENTER 65535

#define JOY_HAT_UP 0
#define JOY_HAT_RIGHT 9000
#define JOY_HAT_DOWN 18000
#define JOY_HAT_LEFT 27000

#define JOY_HAT_UPRIGHT 4500
#define JOY_HAT_DOWNRIGHT 13500
#define JOY_HAT_DOWNLEFT 22500
#define JOY_HAT_UPLEFT 31500


typedef struct sJoyState sJoyState;

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IJoystick
//

#undef INTERFACE
#define INTERFACE IJoystick

DECLARE_INTERFACE_(IJoystick, IInputDevice)
{
   //
   // IUnknown methods
   //
   DECLARE_UNKNOWN_PURE();

   // get info about this device
   STDMETHOD(GetInfo) (THIS_ sInputDeviceInfo *pInfo) PURE;
   // poll the joystick and get its current state
   STDMETHOD(GetState) (THIS_ sJoyState *pState) PURE;
   // get/set joystick axis range (default is -128 to 127)
   STDMETHOD(SetAxisRange) (THIS_ eJoystickObjs axis, long min, long max) PURE;
   STDMETHOD(GetAxisRange) (THIS_ eJoystickObjs axis, long *pMin, long *pMax) PURE;
   // set "dead zone" as a percentage of axis range * 10000 (default is 500 = 5%)
   STDMETHOD(SetAxisDeadZone) (THIS_ eJoystickObjs axis, DWORD deadZone) PURE;
   //exclusive/non-exclusive, foreground/background
   STDMETHOD(SetCooperativeLevel) (THIS_ BOOL excl, BOOL foreground) PURE;
};

#define IJoystick_GetInfo(p,a) COMCall1(p, GetInfo, a)
#define IJoystick_GetState(p,a) COMCall1(p, GetState, a)
#define IJoystick_SetAxisRange(p,a,b,c) COMCall3(p, SetAxisRange, a, b, c)
#define IJoystick_GetAxisRange(p,a,b,c) COMCall3(p, GetAxisRange, a, b, c)
#define IJoystick_SetAxisDeadZone(p,a,b) COMCall2(p, SetAxisDeadZone, a, b)

EXTERN void *GetGlobalJoyDevice( void );

///////////////////////////////////////////////////////////////////////////////

#endif /* !__JOYAPI_H */