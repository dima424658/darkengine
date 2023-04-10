/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Source: r:/t2repos/thief2/src/framewrk/dappsink.cpp,v $
// $Author: mahk $
// $Date: 1998/11/03 06:35:03 $
// $Revision: 1.7 $
//

#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <appapi.h>
#include <loopapi.h>

#include <dappsink.h>
#include <menus.h>


// must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDarkAppAdviseSink
//

class cDarkAppAdviseSink : public IAppAdviseSink
{
public:
    cDarkAppAdviseSink() {}

    void Advise();

private:
    // IUnknown methods
    DECLARE_UNAGGREGATABLE();

private:
    // Determine the revision of the sink interface
    STDMETHOD_(short, GetVersion)();

    // Application will exit (return non-zero to block)
    STDMETHOD (OnQueryQuit)();

    // Application exititing
    STDMETHOD_(void, OnQuit)();

    // Called when making an emergency exit
    STDMETHOD_(void, OnCriticalError)(int errorCode);
    
    // This is called when a command is issued (menus, for example)
    STDMETHOD_(void, OnCommand)(unsigned nCmdId);
};

///////////////////////////////////////
//
// cDarkAppAdviseSink implementation
//

//
// Pre-fab COM implementations
//

IMPLEMENT_UNAGGREGATABLE_NO_FINAL_RELEASE(cDarkAppAdviseSink, IAppAdviseSink);

///////////////////////////////////////

void cDarkAppAdviseSink::Advise()
{
    DWORD ignoredCookie;
    AutoAppIPtr(Application);
    if (!!pApplication)
    {
        pApplication->Advise(this, &ignoredCookie);
    }
}

///////////////////////////////////////
//
// Determine the revision of the sink interface
//

STDMETHODIMP_(short) cDarkAppAdviseSink::GetVersion()
{
    return kVerAppAdvise;
}

///////////////////////////////////////
//
// Application will exit (return non-zero to block)
//

STDMETHODIMP cDarkAppAdviseSink::OnQueryQuit()
{
#ifdef SHIP
   return S_FALSE; // don't quit 
#else
   return S_OK; 
#endif 
}

///////////////////////////////////////
//
// Application exititing
//

STDMETHODIMP_(void) cDarkAppAdviseSink::OnQuit()
{
   AutoAppIPtr(Loop);
   // Here's where we do the actual quitting.
   pLoop->EndAllModes(0);   
   pLoop->Terminate();
}

///////////////////////////////////////

STDMETHODIMP_(void) cDarkAppAdviseSink::OnCommand(unsigned nCmdId) 
{
   MenuCommand(nCmdId);
}

///////////////////////////////////////
//
// Called when making an emergency exit (not implemented as of 11-05-96)
//

#pragma off(unreferenced);
STDMETHODIMP_(void) cDarkAppAdviseSink::OnCriticalError(int errorCode)
{
}
#pragma on(unreferenced);

///////////////////////////////////////////////////////////////////////////////

static cDarkAppAdviseSink g_DarkAppAdviseSink;

///////////////////////////////////////

void AppAdvise()
{
    g_DarkAppAdviseSink.Advise();
}

///////////////////////////////////////////////////////////////////////////////
