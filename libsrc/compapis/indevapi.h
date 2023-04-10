///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/compapis/RCS/indevapi.h $
// $Author: TOML $
// $Date: 1996/07/17 11:34:59 $
// $Revision: 1.7 $
//
// Low-level Input API
//

#ifndef __INDEVAPI_H
#define __INDEVAPI_H

#if defined(__SC__) || defined(__RCC__)
#pragma once
#endif

///////////////////////////////////////

// Expected includes:
//  "lg.h"
//  "comtools.h"

#include <indvguid.h>

///////////////////////////////////////////////////////////////////////////////
//
// Forward Declarations
//

F_DECLARE_INTERFACE(IInputDevices);
F_DECLARE_INTERFACE(IPrimaryInputDevicesSink);

typedef struct kbs_event        sInpKeyEvent;
typedef struct _lgMouseEvent    sInpMouseEvent;

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IInputDevices
//
// Primitive conduit for input devices, right now limited to mouse and keyboard
//
// Low-level keyboard and positioning devices will eventually be hooked in here
//

#undef  INTERFACE
#define INTERFACE IInputDevices

DECLARE_INTERFACE_(IInputDevices, IUnknown)
    {
    //
    // IUnknown methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    // Advise/Unadvise for sink/holder protocol
    //
    STDMETHOD (Advise)(THIS_ IPrimaryInputDevicesSink * pSink, DWORD * pCookie) PURE;
    STDMETHOD (Unadvise)(THIS_ DWORD cookie) PURE;

    //
    // Query/Set mouse position, relative to game display surface
    //
    STDMETHOD_(BOOL, GetMouseState)(THIS_ int * x, int * y, int * pButtons) PURE;
    STDMETHOD_(BOOL, SetMousePos)(THIS_ int x, int y) PURE;
    };

#define IInputDevices_QueryInterface(p, a, b)   COMQueryInterface(p, a, b)
#define IInputDevices_AddRef(p)                 COMAddRef(p)
#define IInputDevices_Release(p)                COMRelease(p)
#define IInputDevices_Advise(p, a, b)           COMAdvise(p, a, b)
#define IInputDevices_Unadvise(p, a)            COMUnadvise(p, a)
#define IInputDevices_GetMouseState(p, a, b, c) COMCall3(p, GetMouseState, a, b, c)
#define IInputDevices_SetMousePos(p, a, b)      COMCall2(p, SetMousePos, a, b)

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IPrimaryInputDevicesSink
//
// A non-zero return from any handler function blocks the event from remaining
// advised handlers.
//

#define kVerInputDeviceAdvise 1
// Version 1:
//      Original interface

#undef  INTERFACE
#define INTERFACE IPrimaryInputDevicesSink

DECLARE_INTERFACE_(IPrimaryInputDevicesSink, IUnknown)
    {
    //
    // IUnknown methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    // This is called to determine the revision of the sink interface
    //
    STDMETHOD_(short, GetVersion)(THIS) PURE;

    //
    // A key was pressed/lifted
    //
    STDMETHOD (OnKey)(THIS_ const sInpKeyEvent * pEvent) PURE;

    //
    // The user is moving/clicking the mouse.  The time stamp in the event
    // structure will be automatically filled with IGameShell::GetTimerTick()
    // by default
    //
    STDMETHOD (OnMouse)(THIS_ const sInpMouseEvent * pEvent) PURE;
    };

#define IPrimaryInputDevicesSink_QueryInterface(p, a, b)    COMQueryInterface(p, a, b)
#define IPrimaryInputDevicesSink_AddRef(p)                  COMAddRef(p)
#define IPrimaryInputDevicesSink_Release(p)                 COMRelease(p)

///////////////////////////////////////////////////////////////////////////////

#endif /* !__INDEVAPI_H */
