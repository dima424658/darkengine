// $Header: x:/prj/tech/winsrc/input/RCS/inpbase.h 1.3 1998/02/04 14:00:05 PATMAC Exp $

#ifndef __INPBASE_H
#define __INPBASE_H

struct sInputDeviceIter
{
   void *m_pDev;
};

struct sInputDeviceInfo
{
   GUID interfaceID;
};

// max buttons we support
#define kJoyButtonsMax 32
// max sliders we support
#define kJoySlidersMax 2
// max POV hats we support
#define kJoyPOVsMax 4

//
// Joystick state returned byt Joystick::GetState
//
// NOTE: make this look just like a DIJOYSTATE, or else some DirectInput
//  joystick drivers (like CH ForceFX) don't work - patmc
//
struct sJoyState
{
    long   x;
    long   y;
    long   z;
    long   rx;
    long   ry;
    long   rz;  // rudder
    long   u;   // sliders
    long   v;
    // hat posn in degrees from up (clockwise) * 100, -1 = centered
    // eg, 0 = up, 9000 = right, 18000 = down, 27000 = left
    DWORD POV[kJoyPOVsMax];
    BYTE buttons[kJoyButtonsMax];  // high bit set when down
};

// joystick components - should match sJoyState
enum eJoystickObjs_
{
   kJoystickXAxis = 0,
   kJoystickYAxis = 1, 
   kJoystickZAxis = 2, 
   kJoystickXRot = 3,
   kJoystickYRot = 4,
   kJoystickZRot = 5, 
   kJoystickUSlider = 6, 
   kJoystickVSlider = 7, 
   kJoystickPOV1 = 8,
   kJoystickPOV2 = 9,
   kJoystickPOV3 = 10,
   kJoystickPOV4 = 11,
   kJoystickButtonBase = 12,
};

#define kJoystickRudder kJoystickZRot

#endif
