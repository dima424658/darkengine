///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/gameshel/RCS/dgshell.cpp $
// $Author: KEVIN $
// $Date: 1997/09/03 17:06:39 $
// $Revision: 1.9 $
//

#include <lg.h>
#include <time.h>
#include <dgshell.h>

///////////////////////////////////////////////////////////////////////////////
//
// Game shell create method
//

tResult LGAPI _GameShellCreate(REFIID, IGameShell ** ppGameShell, IUnknown * pOuterUnknown, int fFlags, int msecBlockSignal)
{
    return (new cDosGameShell(fFlags, pOuterUnknown) != 0) ? S_OK : E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDosGameShell
//

//
// Pre-fab COM implementations
//
IMPLEMENT_SIMPLE_AGGREGATION_SELF_DELETE(cDosGameShell);

///////////////////////////////////////
//
// Construct an instance...
//

cDosGameShell::cDosGameShell(int fFlags, IUnknown * pOuterUnknown)
{
    // Add internal components to outer aggregate...
    INIT_AGGREGATION_1( pOuterUnknown,
                        IID_IGameShell, this,
                        kPriorityLibrary,
                        NULL );

}

///////////////////////////////////////
//
// Destroy an instance...
//

cDosGameShell::~cDosGameShell()
{
}

///////////////////////////////////////
//
// Pump-input and lock display surfaces (if specified when created)
//

STDMETHODIMP_(void) cDosGameShell::BeginFrame()
{
}

///////////////////////////////////////
//
// Release locks, ensure screen is up-to-date
//

STDMETHODIMP_(void) cDosGameShell::EndFrame()
{
}

///////////////////////////////////////
//
// Process pending events
//
// This model empties the queue rather than pumping only one event
//

STDMETHODIMP_(void) cDosGameShell::PumpEvents(int )
{
}

///////////////////////////////////////
//
// Try to lock video memory.
//

STDMETHODIMP_(int) cDosGameShell::DisplayLock()
{
    return 1;
}

///////////////////////////////////////
//
// Release a lock on memory
//

STDMETHODIMP_(int) cDosGameShell::DisplayUnlock()
{
    return 0;
}

///////////////////////////////////////
//
// Verify display reflects what is expected
//

STDMETHODIMP_(void) cDosGameShell::DisplayFlush()
{
}

///////////////////////////////////////
//
// Get the present tick count, in msec since initialization or timer restart
//

STDMETHODIMP_(unsigned long) cDosGameShell::GetTimerTick()
{
// @Note (toml 10-21-96): This is a totally fake time, stuck on ticks. Gotta fix
    return clock() / CLOCKS_PER_SEC;
}

///////////////////////////////////////
//
// Reset the timer (by default, starts at zero from initialization)
//

STDMETHODIMP_(void) cDosGameShell::SetTimer(unsigned long )
{
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cDosGameShell::ShowHelp(const char * , uint , ulong )
{
    return FALSE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cDosGameShell::SetOSCursor(sGrBitmap * , const Point * )
{
    return FALSE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cDosGameShell::ShowOSCursor(BOOL)
{
    return FALSE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cDosGameShell::SetCursorCallback(tGSCursorCallbackFunc)
{
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cDosGameShell::Exec(const char * pszCommandLine )
{
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL) cDosGameShell::SetFlags(int )
{
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL) cDosGameShell::GetFlags(int * )
{
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
