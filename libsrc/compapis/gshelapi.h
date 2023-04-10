///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/compapis/RCS/gshelapi.h $
// $Author: TOML $
// $Date: 1997/09/10 13:57:12 $
// $Revision: 1.33 $
//
// Interfaces for use with the Looking Glass Generic Game Shell.  Game shell
// is responsible for platform dependent synchronization.
//

#ifndef __GSHELAPI_H
#define __GSHELAPI_H

#if defined(__SC__) || defined(__RCC__)
#pragma once
#endif

///////////////////////////////////////

// Expected includes:
//  "lg.h"
//  "comtools.h"

#include <gshlguid.h>
#include <pumpenum.h>

///////////////////////////////////////////////////////////////////////////////
//
// Forward Declarations
//
typedef struct grs_bitmap grs_bitmap;
typedef grs_bitmap        sGrBitmap;
typedef struct Point      Point;

F_DECLARE_INTERFACE(IGameShell);
F_DECLARE_INTERFACE(IGameShellAdviseSink);

///////////////////////////////////////////////////////////////////////////////
//
// Game shell API
//

//
// Create a game shell and add it to the global app-object
//
#define GameShellCreate(flags) \
{ \
    IUnknown * pAppUnknown = AppGetObj(IUnknown); \
    _GameShellCreate(IID_TO_REFIID(IID_IGameShell), NULL, pAppUnknown, flags, 0); \
    COMRelease(pAppUnknown); \
}

#define GameShellCreate2(flags, msecBlockSignal) \
{ \
    IUnknown * pAppUnknown = AppGetObj(IUnknown); \
    _GameShellCreate(IID_TO_REFIID(IID_IGameShell), NULL, pAppUnknown, flags, msecBlockSignal); \
    COMRelease(pAppUnknown); \
}
//
// Creates a game shell, aggregating it with specfied pOuter,
// use IAggregate protocol if ppGameShell is NULL
//
EXTERN tResult LGAPI _GameShellCreate(REFIID, IGameShell ** ppGameShell, IUnknown * pOuter, int fFlags, int msecBlockSignal);

///////////////////////////////////////

enum eGameShellCreateFlags
{
    //
    // Automatically lock display device within Begin/EndFrame() pair
    //
    kLockFrame = 0x01,

    //
    // Automatically flip display device on EndFrame(), if flippable
    //
    kFlipOnEndFrame = 0x02,

    //
    // Call exit() if OS-specific quit event is detected
    //
    kExitOnQuitEvent = 0x04,

    //
    // Automatically capture mouse while button is down
    //
    kCaptureMouseWhileDown = 0x08,

    //
    // Show cursor
    //
    kShowNativeCursor = 0x10,

    //
    // Attempt to run the game shell in multi-threaded mode
    //
    kMultithreadedShell = 0x20,

    //
    // Automatically flush display device on EndFrame()
    //
    kFlushOnEndFrame = 0x40,

    //
    // Create default
    //
    kGameShellDefault = (kLockFrame + kFlipOnEndFrame + kExitOnQuitEvent + kCaptureMouseWhileDown + kFlushOnEndFrame)
};

///////////////////////////////////////////////////////////////////////////////

typedef enum eGSCursorEvent
{
    kGSShowCursor,
    kGSHideCursor
} eGSCursorEvent;

typedef void (LGAPI * tGSCursorCallbackFunc)(eGSCursorEvent);

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IGameShell
//
//

#undef  INTERFACE
#define INTERFACE IGameShell

DECLARE_INTERFACE_(IGameShell, IUnknown)
{
    //
    // IUnknown methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    // Pump-input and lock display surfaces (if specified when created)
    //
    STDMETHOD_(void, BeginFrame)(THIS) PURE;

    //
    // Release locks, ensure screen is up-to-date
    //
    STDMETHOD_(void, EndFrame)(THIS) PURE;

    //
    // Make sure the game is the user focus
    //
    STDMETHOD (BringToFront)(THIS) PURE;

    //
    // Obsolete functon
    // @TBD (toml 08-03-97): next time a non-backtouch game shell change goes in, remove this
    //
    STDMETHOD (Obsolete1)(THIS) PURE;

    //
    // Process pending events (on event-driven OSs)
    //
    // This is done automatically in BeginFrame()
    //
    // This will break all locks, if there are any, then re-lock.  Messages
    // will be passed into the event aggregation event handler, if any.
    //
    // Sprinkle this around if things seem choppy, but not too many places,
    // as it can be relatively expensive
    //
    STDMETHOD_(void, PumpEvents)(THIS_ int fPumpFlags /* = kPumpAll */) PURE;

    ///////////////////////////////////
    //
    // Convenience functions that provide a facade for common operations on
    // other components without forcing game shell clients to include headers
    // with a lot of irrelevant dependencies
    //

    //
    // Try to lock video memory.  Locked pointers will be placed in
    // structure provided in SetMode(). Returns lock count.
    //
    STDMETHOD_(int, DisplayLock)(THIS) PURE;

    //
    // Release a lock on memory
    //
    STDMETHOD_(int, DisplayUnlock)(THIS) PURE;

    //
    // Verify display reflects what is expected.  Useful if not particularly
    // frame based & want to ensure screen is up-to-date within/without a
    // BeginFrame()/EndFrame() pair. Meaningful in "artificial" modes, no-op
    // otherwise.
    //
    STDMETHOD_(void, DisplayFlush)(THIS) PURE;

    //
    // Get the present tick count, in msec since initialization or timer restart
    //
    STDMETHOD_(unsigned long, GetTimerTick)(THIS) PURE;

    //
    // Reset the timer (by default, starts at zero from initialization)
    //
    STDMETHOD_(void , SetTimer)(THIS_ unsigned long) PURE;

    //
    // Show a help topic
    //
    STDMETHOD_(BOOL, ShowHelp)(THIS_ const char * pszHelpFile, uint command, ulong dwData) PURE;

    //
    // Show/Hide the OS cursor.
    //
    STDMETHOD_(BOOL, SetOSCursor)(THIS_ sGrBitmap * pCursorBitmap, const Point * pHotSpot) PURE;
    STDMETHOD_(BOOL, ShowOSCursor)(THIS_ BOOL) PURE;
    STDMETHOD_(void, SetCursorCallback)(THIS_ tGSCursorCallbackFunc) PURE;

    //
    // Execute a command line
    //
    STDMETHOD_(BOOL, Exec)(THIS_ const char * pszCommandLine ) PURE;

    //
    // Set flags.  Some flags may be unaffected (e.g., multithreaded)
    //
    STDMETHOD_(BOOL, SetFlags)(THIS_ int fFlags ) PURE;

    //
    // Get flags
    //
    STDMETHOD_(BOOL, GetFlags)(THIS_ int * pfFlags ) PURE;
};

#define IGameShell_QueryInterface(p, a, b)  COMQueryInterface(p, a, b)
#define IGameShell_AddRef(p)                COMAddRef(p)
#define IGameShell_Release(p)               COMRelease(p)
#define IGameShell_BeginFrame(p)            COMCall0(p, BeginFrame)
#define IGameShell_EndFrame(p)              COMCall0(p, EndFrame)
#define IGameShell_PumpEvents(p, a)         COMCall1(p, PumpEvents, a)
#define IGameShell_DisplayLock(p)           COMCall0(p, DisplayLock)
#define IGameShell_DisplayUnlock(p)         COMCall0(p, DisplayUnlock)
#define IGameShell_DisplayFlush(p)          COMCall0(p, DisplayFlush)
#define IGameShell_SetTimer(p, a)           COMCall1(p, SetTimer, a)
#define IGameShell_GetTimerTick(p)          COMCall0(p, GetTimerTick)
#define IGameShell_ShowHelp(p, a, b, c)     COMCall3(p, ShowHelp, a, b, c)
#define IGameShell_SetOSCursor(p, a, b)     COMCall2(p, SetOSCursor, a, b)
#define IGameShell_ShowOSCursor(p, a)       COMCall1(p, ShowOSCursor, a)
#define IGameShell_SetCursorCallback(p, a)  COMCall1(p, SetCursorCallback, a)
#define IGameShell_Exec(p, a)               COMCall1(p, Exec, a)
#define IGameShell_SetFlags(p, a)           COMCall1(p, SetFlags, a)
#define IGameShell_GetFlags(p, a)           COMCall1(p, GetFlags, a)

///////////////////////////////////////////////////////////////////////////////

#endif /* !__GSHELAPI_H */
