///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/compapis/RCS/appapi.h $
// $Author: TOML $
// $Date: 1997/08/05 13:12:03 $
// $Revision: 1.20 $
//
// Specification of generic "Application" interface.  App could implement
// this.  Libraries provide a generic implementation.  Generic things common to
// all applications, game or not.
//

#ifndef __APPAPI_H
#define __APPAPI_H

#if defined(__SC__) || defined(__RCC__)
#pragma once
#endif

///////////////////////////////////////

// Expected includes:
//  "lg.h"
//  "comtools.h"

#include "appguid.h"

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IApplication);
F_DECLARE_INTERFACE(IAppAdviseSink);

///////////////////////////////////////////////////////////////////////////////
//
// Create a generic application and add it to the global app-object
//

#define GenericApplicationCreate(argc, argv, name, defaultPath) \
{ \
    IUnknown * pAppUnknown = AppGetObj(IUnknown); \
    _GenericApplicationCreate(IID_TO_REFIID(IID_IApplication), NULL, pAppUnknown, argc, argv, name, defaultPath);\
    COMRelease(pAppUnknown); \
}

//
// Creates a generic application, aggregating it with specfied pOuter,
// use IAggregate protocol if ppApplication is NULL, else self-init
//

EXTERN tResult LGAPI _GenericApplicationCreate(REFIID, IApplication ** ppApplication, IUnknown * pOuter, int argc, const char *argv[], const char * pszName, const char * pszDefaultPath);

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IApplication
//
//

#undef  INTERFACE
#define INTERFACE IApplication

DECLARE_INTERFACE_(IApplication, IUnknown)
{
    //
    // IUnknown methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    // Quit the application (conditional, non-error), return non-zero to not quit
    //
    STDMETHOD (QueryQuit)(THIS) PURE;

    //
    // Quit the application (unconditional, non-error)
    //
    STDMETHOD_(void, Quit)(THIS) PURE;

    //
    // Quit the application (unconditional, error)
    //
    STDMETHOD_(void, Abort)(THIS_ const char * pszReason) PURE;

    //
    // Get/Set application name, as displayed in title bar
    //
    STDMETHOD_(void, SetCaption)(THIS_ const char *) PURE;
    STDMETHOD_(const char *, GetCaption)(THIS) PURE;

    //
    // Get/Set default path for file access
    //
    STDMETHOD_(void, SetDefaultFilePath)(THIS_ const char *) PURE;
    STDMETHOD_(const char *, GetDefaultFilePath)(THIS) PURE;

    //
    // Get the full path of the application .EXE
    // i.e., c:\foo\bar.exe
    //
    STDMETHOD_(const char *, GetFullName)(THIS) PURE;

    //
    // Get the full path of the application .EXE
    // i.e., c:\foo
    //
    STDMETHOD_(const char *, GetPath)(THIS) PURE;

    //
    // Display a message box
    //
    STDMETHOD_(int, MessageBox)(THIS_ const char * pszMessage, const char * pszCaption, int fFlags) PURE;

    //
    // Advise/Unadvise for sink/holder protocol
    //
    STDMETHOD (Advise)(THIS_ IAppAdviseSink * pSink, DWORD * pCookie) PURE;
    STDMETHOD (Unadvise)(THIS_ DWORD cookie) PURE;
    
    //
    // Fire a command
    //
    STDMETHOD_(void, AppCommand)(THIS_ unsigned nCmdId) PURE;

};

///////////////////////////////////////

#define IApplication_QueryInterface(p, a, b)    COMQueryInterface(p, a, b)
#define IApplication_AddRef(p)                  COMAddRef(p)
#define IApplication_Release(p)                 COMRelease(p)
#define IApplication_QueryQuit(p)               COMCall0(p, QueryQuit)
#define IApplication_Quit(p)                    COMCall0(p, Quit)
#define IApplication_Abort(p, a)                COMCall1(p, Abort, a)
#define IApplication_SetCaption(p, a)           COMCall1(p, SetCaption, a)
#define IApplication_GetCaption(p)              COMCall0(p, GetCaption)
#define IApplication_SetDefaultFilePath(p, a)   COMCall1(p, SetDefaultFilePath, a)
#define IApplication_GetDefaultFilePath(p)      COMCall0(p, GetDefaultFilePath)
#define IApplication_GetFullName(p)             COMCall0(p, GetFullName)
#define IApplication_GetPath(p)                 COMCall0(p, GetPath)
#define IApplication_MessageBox(p, a, b, c)     COMCall3(p, MessageBox, a, b, c)
#define IApplication_Advise(p, a, b)            COMAdvise(p, a, b)
#define IApplication_Unadvise(p, a)             COMUnadvise(p, a)

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAppAdviseSink
//
//

#define kVerAppAdvise 2
// Version 1:
//      Original interface
#define kVerAppAdvise_Original 1
// Version 2:
//      Command support
#define kVerAppAdvise_CommandSupport 2

///////////////////////////////////////

#undef  INTERFACE
#define INTERFACE IAppAdviseSink

DECLARE_INTERFACE_(IAppAdviseSink, IUnknown)
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
    // This is called when the application exits (return non-zero to block)
    //
    STDMETHOD (OnQueryQuit)(THIS) PURE;

    //
    // This is called when the application exits
    //
    STDMETHOD_(void, OnQuit)(THIS) PURE;

    //
    // This is called when making an emergency exit
    //
    STDMETHOD_(void, OnCriticalError)(THIS_ int errorCode) PURE;

    //
    // This is called when a command is issued (menus, for example)
    //
    STDMETHOD_(void, OnCommand)(THIS_ unsigned nCmdId) PURE;
};

///////////////////////////////////////

#define IAppAdviseSink_QueryInterface(p, a, b)  COMQueryInterface(p, a, b)
#define IAppAdviseSink_AddRef(p)                COMAddRef(p)
#define IAppAdviseSink_Release(p)               COMRelease(p)
#define IAppAdviseSink_GetVersion(p)            COMCall0(p, GetVersion)
#define IAppAdviseSink_OnQueryQuit(p)           COMCall0(p, OnQueryQuit)
#define IAppAdviseSink_OnQuit(p)                COMCall0(p, OnQuit)
#define IAppAdviseSink_OnCriticalError(p, a)    COMCall1(p, OnCriticalError, a)

///////////////////////////////////////////////////////////////////////////////

#endif /* __APPAPI_H */
