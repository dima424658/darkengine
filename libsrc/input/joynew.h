/* 
 * $Source: x:/prj/tech/winsrc/input/RCS/joynew.h $
 * $Revision: 1.3 $
 * $Author: PATMAC $
 * $Date: 1998/01/28 15:36:59 $
 *
 *	New joystick device 
 *
 */

#ifndef __JOYNEW_H
#define __JOYNEW_H

#include <windows.h>
#include <dinput.h>

// continue to support old API
#include <joystick.h>
// new API
#include <joyapi.h>

class cJoystick: public IJoystick
{
public:
   // IUnknown methods
   DECLARE_UNAGGREGATABLE();

   STDMETHOD(GetInfo) (THIS_ sInputDeviceInfo * );
   STDMETHOD(GetState) (sJoyState *pState);
   STDMETHOD(SetAxisRange) (eJoystickObjs axis, long min, long max);
   STDMETHOD(GetAxisRange) (eJoystickObjs axis, long *pMin, long *pMmax);
   STDMETHOD(SetAxisDeadZone) (eJoystickObjs axis, DWORD deadZone);
   STDMETHOD(SetCooperativeLevel) (THIS_ BOOL excl, BOOL foreground);

   cJoystick();
   virtual ~cJoystick(void);
   STDMETHOD(Init) (LPDIRECTINPUTDEVICE pDev);

private:
   LPDIRECTINPUTDEVICE2 m_pDev;        // actual device
   DIDEVCAPS            mDevCaps;      // device capabilities
   bool                 mHasZAxis;     // does device have a Z Axis?
   bool                 mHasRZAxis;    // does device have rotation about Z Axis?
   int                  mNumSliders;   // number of sliders supported

   HRESULT ReacquireInput(void);
   long FieldOffset(eJoystickObjs axis);
};

#endif /* !__JOYNEW_H */
