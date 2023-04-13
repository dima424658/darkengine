///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/winsrc/input/RCS/inpapi.h $
// $Author: JON $
// $Date: 1997/10/06 20:42:21 $
// $Revision: 1.1 $
//
// New DirectInput based input manager
// 

#ifndef __INPUTAPI_H
#define __INPUTAPI_H

#include <comtools.h>
#include <inpguid.h>

///////////////////////////////////////
//
// Forward declarations
//

F_DECLARE_INTERFACE(IInputManager);
F_DECLARE_INTERFACE(IInputDevice);

///////////////////////////////////////////////////////////////////////////////
//
// Structures and constants
//

typedef struct sInputDeviceIter sInputDeviceIter;

typedef struct sInputDeviceInfo sInputDeviceInfo;

///////////////////////////////////////////////////////////////////////////////
//
// Create an input manager and add it to the global app-object
//
#define InputManagerCreate() \
{ \
    IUnknown *pAppUnknown = AppGetObj(IUnknown); \
    _InputManagerCreate(pAppUnknown); \
    COMRelease(pAppUnknown); \
}

EXTERN tResult LGAPI _InputManagerCreate(IUnknown * pOuterUnknown);

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IInputManager
//
// This interface presents the methods needed to enumerate and access all
// installed input devices.
//
#undef INTERFACE
#define INTERFACE IInputManager

DECLARE_INTERFACE_(IInputManager, IUnknown)
{
    //
    // IUnknown methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    // Iterate over input devices of a provided type
    // Returned interface can be downcast to the correct type
    //
    STDMETHOD_(void, IterStart) (THIS_ sInputDeviceIter *pIter, GUID interfaceID) PURE; 
    STDMETHOD_(void, IterNext) (THIS_ sInputDeviceIter *pIter) PURE;
    STDMETHOD_(IInputDevice*, IterGet) (THIS_ sInputDeviceIter *pIter) PURE;
    STDMETHOD_(BOOL, IterFinished) (THIS_ sInputDeviceIter *pIter) PURE;
};

#define IInputManager_IterStart(p,a,b) COMCall2(p, IterStart, a, b)
#define IInputManager_IterNext(p,a) COMCall1(p, IterNext, a)
#define IInputManager_IterGet(p,a) COMCall1(p, IterGet, a)
#define IInputManager_IterFinished(p,a) COMCall1(p, IterFinished, a)

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IInputDevice
//
// Lower-level interface used to represent device "drivers"
//

typedef struct sInputDeviceIter sInputDeviceIter;

#undef INTERFACE
#define INTERFACE IInputDevice

DECLARE_INTERFACE_(IInputDevice, IUnknown)
{
    //
    // IUnknown methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    // Query details about the device
    //
    STDMETHOD(GetInfo) (THIS_ sInputDeviceInfo *pInfo) PURE;

};

#define IInputDevice_GetInfo(p,a) COMCall1(p, GetInfo, a)

///////////////////////////////////////////////////////////////////////////////

#endif /* !__INPUTAPI_H */




