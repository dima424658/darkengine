///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/gameshel/RCS/dgshell.h $
// $Author: KEVIN $
// $Date: 1997/09/03 17:08:57 $
// $Revision: 1.8 $
//
// @TBD (toml 10-21-96): the common elements of the DOS & windows game shells
// can be consolodated into a cGameShell

#ifndef __DGSHELL_H
#define __DGSHELL_H

#include <comtools.h>
#include <objcoll.h>
#include <aggmemb.h>
#include <gshelapi.h>

#include <comconn.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDosGameShell
//

class cDosGameShell : public IGameShell
{
public:
    // Construct an instance...
    cDosGameShell(int fFlags, IUnknown * pOuterUnknown);

protected:
    // Destroy an instance...
    virtual ~cDosGameShell();

    ///////////////////////////////////
    //
    // IGameShell implementation functions
    //

    // IUnknown methods
    DECLARE_SIMPLE_AGGREGATION(cDosGameShell);

    // Pump-input and lock display surfaces (if specified when created)
    STDMETHOD_(void, BeginFrame)();

    // Release locks, ensure screen is up-to-date
    STDMETHOD_(void, EndFrame)();

    // Holder/Sink methods
    STDMETHOD (Obsolete1)() { return 0; }
    STDMETHOD (Obsolete2)() { return 0; }

    // Process pending events
    STDMETHOD_(void, PumpEvents)(int fPumpFlags = kPumpAll);

    // Try to lock video memory.
    STDMETHOD_(int, DisplayLock)();

    // Release a lock on memory
    STDMETHOD_(int, DisplayUnlock)();

    // Verify display reflects what is expected
    STDMETHOD_(void, DisplayFlush)();

    // Get the present tick count, in msec since initialization or timer restart
    STDMETHOD_(unsigned long, GetTimerTick)();

    // Reset the timer (by default, starts at zero from initialization)
    STDMETHOD_(void , SetTimer)(unsigned long);

    // Show a help topic
    STDMETHOD_(BOOL, ShowHelp)(const char * pszHelpFile, uint command, ulong dwData);

    //
    // Show/Hide the OS cursor.
    //
    STDMETHOD_(BOOL, SetOSCursor)(sGrBitmap * pCursorBitmap, const Point * pHotSpot);
    STDMETHOD_(BOOL, ShowOSCursor)(BOOL);
    STDMETHOD_(void, SetCursorCallback)(tGSCursorCallbackFunc);

    //
    // Execute a command line
    //
    STDMETHOD_(BOOL, Exec)(const char * pszCommandLine );

    //
    // Set flags
    //
    STDMETHOD_(BOOL, SetFlags)(int fFlags);

    //
    // Get flags
    //
    STDMETHOD_(BOOL, GetFlags)(int * pfFlags);
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__DGSHELL_H */
