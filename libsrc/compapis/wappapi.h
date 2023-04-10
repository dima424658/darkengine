///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/compapis/RCS/wappapi.h $
// $Author: TOML $
// $Date: 1996/07/17 11:35:00 $
// $Revision: 1.19 $
//
// Specification of generic "Windows Application" interface.  App could implement
// this.  Libraries provide a generic implementation.  Generic things common to
// all applications, game or not.
//
// No default creation function is provided, since the default application creation
// function in appapi.h implements this interface as well
//


#ifndef __WAPPAPI_H
#define __WAPPAPI_H

#ifdef _WINDOWS_

#if defined(__SC__) || defined(__RCC__)
#pragma once
#endif

///////////////////////////////////////////////////////////////////////////////

//
// Standard Resource IDs
//
#ifndef LGWINAPP_BASE_RC_ID

#define LGWINLIB_BASE_RC_ID         10000
#define LGWINAPP_BASE_RC_ID         (LGWINLIB_BASE_RC_ID + 5000)
#define LGWINAPP_BASE_COMMAND_ID    (LGWINAPP_BASE_RC_ID + 10000)

#define LGLIB_ID(n)                 (LGWINLIB_BASE_RC_ID + n)
#define LGAPP_ID(n)                 (LGAPPLIB_BASE_RC_ID + n)
#define LGCMD_ID(n)                 (LGWINAPP_BASE_COMMAND_ID + n)

#endif

#define IDI_APPICON                 LGLIB_ID(1)
#define IDM_APPMENU                 LGLIB_ID(2)
#define IDS_APPNAME                 LGLIB_ID(3)
#define IDS_APPTITLE                LGLIB_ID(4)

#define ID_EXIT                     LGCMD_ID(1)
#define ID_PRINTSCREEN              LGCMD_ID(2)

///////////////////////////////////////////////////////////////////////////////

#ifndef RC_INVOKED

///////////////////////////////////////

// Expected includes:
//  "windows.h"
//  "lg.h"
//  "comtools.h"

#include <wappguid.h>
#include <pumpenum.h>

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IWinAppAdviseSink);

///////////////////////////////////////////////////////////////////////////////
//
// Enumerations and constants
//

enum eWinPumpDuration
    {
    //
    // Pump just one message
    //
    kPumpOne,

    //
    // Pump until queue is depleted
    //
    kPumpUntilEmpty,

    //
    // Pump until WM_QUIT
    //
    kPumpUntilQuit

    };
typedef enum eWinPumpDuration eWinPumpDuration;

enum eWinPumpResult
    {
    //
    // Pump was successful
    //
    kPumpedOk,

    //
    // Pump failed because the queue is empty
    //
    kPumpedNothing,

    //
    // Pump was sucessful, and WM_QUIT was received
    //
    kPumpedQuit,

    //
    // Pump failed because received a quit sometime in the past
    //
    kPumpedPastQuit

    };
typedef enum eWinPumpResult eWinPumpResult;

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IWinApp
//
//

#undef  INTERFACE
#define INTERFACE IWinApp

DECLARE_INTERFACE_(IWinApp, IUnknown)
    {
    //
    // IUnknown methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    // Get/Set the main window (Set() returns previous)
    //
    STDMETHOD_(HWND, GetMainWnd)(THIS) PURE;
    STDMETHOD_(HWND, SetMainWnd)(THIS_ HWND) PURE;

    //
    // Access important instance handles
    //
    STDMETHOD_(HINSTANCE, GetInstance)(THIS) PURE;
    STDMETHOD_(HINSTANCE, GetResourceInstance)(THIS) PURE;

    //
    // Get startup info
    //
    STDMETHOD_(const char *, GetCommandLine)(THIS) PURE;
    STDMETHOD_(int, GetCommandShow)(THIS) PURE;

    //
    // Get/Set application's registry key
    //
    STDMETHOD_(void, SetRegistryKey)(THIS_ const char *) PURE;
    STDMETHOD_(const char *, GetRegistryKey)(THIS) PURE;

    //
    // default based on app name
    //
    STDMETHOD_(void, SetProfileName)(THIS_ const char *) PURE;
    STDMETHOD_(const char *, GetProfileName)(THIS) PURE;

    //
    // set dialog box and message box background color
    //
    STDMETHOD_(void, SetDialogBkColor)(THIS_ COLORREF clrCtlBk /* = RGB(192, 192, 192)*/,
                                       COLORREF clrCtlText /* = RGB(0, 0, 0)*/ ) PURE;

    //
    // Process pending events
    //
    STDMETHOD_(eWinPumpResult, PumpEvents)(THIS_ int fPumpFlags,
                                          eWinPumpDuration fDuration) PURE;

    //
    // Create a modal dialog box
    //
    STDMETHOD (ModalDialogBox)(THIS_ LPCTSTR lpTemplate, DLGPROC lpDialogFunc) PURE;

    //
    // Advise/Unadvise for sink/holder protocol
    //
    STDMETHOD (Advise)(THIS_ IWinAppAdviseSink * pSink, DWORD * pCookie) PURE;
    STDMETHOD (Unadvise)(THIS_ DWORD cookie) PURE;

    };

///////////////////////////////////////

#define IWinApp_QueryInterface(p, a, b)     COMQueryInterface(p, a, b)
#define IWinApp_AddRef(p)                   COMAddRef(p)
#define IWinApp_Release(p)                  COMRelease(p)
#define IWinApp_GetMainWnd(p)               COMCall0(p, GetMainWnd)
#define IWinApp_SetMainWnd(p, a)            COMCall1(p, SetMainWnd, a)
#define IWinApp_GetInstance(p)              COMCall0(p, GetInstance)
#define IWinApp_GetResourceInstance(p)      COMCall0(p, GetResourceInstance)
#define IWinApp_GetCommandLine(p)           COMCall0(p, GetCommandLine)
#define IWinApp_GetCommandShow(p)           COMCall0(p, GetCommandShow)
#define IWinApp_SetRegistryKey(p, a)        COMCall1(p, SetRegistryKey, a)
#define IWinApp_GetRegistryKey(p)           COMCall0(p, GetRegistryKey)
#define IWinApp_SetProfileName(p, a)        COMCall1(p, SetProfileName, a)
#define IWinApp_GetProfileName(p)           COMCall0(p, GetProfileName)
#define IWinApp_SetDialogBkColor(p, a, b)   COMCall2(p, SetDialogBkColor, a, b)
#define IWinApp_PumpEvents(p, a, b)         COMCall2(p, PumpEvents, a, b)
#define IWinApp_ModalDialogBox(p, a, b)     COMCall2(p, ModalDialogBox, a, b)
#define IWinApp_Advise(p, a, b)             COMAdvise(p, a, b)
#define IWinApp_Unadvise(p, a)              COMUnadvise(p, a)

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IWinAppAdviseSink
//
//

#define kVerWinAppAdvise 1
// Version 1:
//      Original interface

///////////////////////////////////////

#undef  INTERFACE
#define INTERFACE IWinAppAdviseSink

DECLARE_INTERFACE_(IWinAppAdviseSink, IUnknown)
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
    // Pre-filter all application messages
    //
    STDMETHOD_(BOOL, PreTranslateMessage)(THIS_ MSG* pMsg) PURE;

    //
    // Called when idle message received
    //
    STDMETHOD_(BOOL, OnIdle)(THIS_ long lCount) PURE; // return TRUE if more idle processing

    };

#define IWinAppAdviseSink_QueryInterface(p, a, b) COMQueryInterface(p, a, b)
#define IWinAppAdviseSink_AddRef(p) COMAddRef(p)
#define IWinAppAdviseSink_Release(p) COMRelease(p)

#endif /* RC_INVOKED */

///////////////////////////////////////////////////////////////////////////////

#endif /* _WINDOWS */

#endif /* __WAPPAPI_H */

