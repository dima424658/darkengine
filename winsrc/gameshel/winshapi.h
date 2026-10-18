///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/winshell/RCS/winshapi.h $
// $Author: TOML $
// $Date: 1996/09/18 16:45:39 $
// $Revision: 1.5 $
//
// Extended interfaces for the Windows implementation of the game shell
//

#ifndef __WINSHAPI_H
#define __WINSHAPI_H

#include <wnshguid.h>

F_DECLARE_INTERFACE(IWinShellSink);

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IWinShell
//
// Interface for Windows-specific elements of the IGameShell implementation.
// These are the things that are generally not portable, and more likely to
// have problems between releases of Windows and/or the game shell.
//
// @Note (toml 08-12-96): This interface is currently "under construction" and
// not presently in use
//

DECLARE_INTERFACE_(IWinShell, IUnknown)
{
    //
    // IUnknown methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    // Cursor manipulation
    //
    STDMETHOD_(HCURSOR, SetCursor)(THIS_ HCURSOR) PURE;
    STDMETHOD_(HCURSOR, GetCursor)(THIS) PURE;


    //
    // Advise/Unadvise for sink/holder protocol
    //
    STDMETHOD (Advise)(THIS_ IWinShellSink * pSink, int priority, DWORD * pCookie) PURE;
    STDMETHOD (Unadvise)(THIS_ DWORD cookie) PURE;

};


///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IWinShellSink
//
// @Note (toml 08-12-96): This interface is currently "under construction" and
// not presently in use
//

DECLARE_INTERFACE_(IWinShellSink, IUnknown)
{
    //
    // IUnknown methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    // Handle a message from IGameShell instance.  Return TRUE indicate message
    // is entirely handled. pRetVal to specify return value from window procedure,
    // if handled.
    //
    STDMETHOD_(BOOL, PreProcessMessage)(THIS_ UINT msg, WPARAM wParam, LPARAM lParam, long * pRetVal) PURE;
    STDMETHOD_(BOOL, PostProcessMessage)(THIS_ UINT msg, WPARAM wParam, LPARAM lParam, long * pRetVal) PURE;

    //
    // Signal a task switch
    //
    STDMETHOD_(void, OnTaskSwitch)(THIS_ BOOL) PURE;

};

#endif /* !__WINSHAPI_H */
