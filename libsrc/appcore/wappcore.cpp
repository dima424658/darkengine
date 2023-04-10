///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/appcore/RCS/wappcore.cpp $
// $Author: TOML $
// $Date: 1997/10/16 13:18:53 $
// $Revision: 1.27 $
//

#ifdef _WIN32

#include <windows.h>
#include <lg.h>
#include <aggmemb.h>
#include <wappcore.h>
#include <appagg.h>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

#if defined(__WATCOMC__)
#pragma off (unreferenced)
#endif

///////////////////////////////////////////////////////////////////////////////

EXTERN tResult LGAPI _GenericApplicationCreate(REFIID,
                                               IApplication ** ppApplication,
                                               IUnknown * pOuterUnknown,
                                               int argc, const char *argv[],
                                               const char * pszName,
                                               const char * pszDefaultPath)
{
    USE_EXTENDED_START_UP();
    // Create the application.  Instance adds self to pOuterUnknown
    return (new cWindowsApplication(argc, argv, pszName, pOuterUnknown) != 0);
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cWindowsApplication
//

//
// Pre-fab COM implementations
//
IMPLEMENT_SIMPLE_AGGREGATE_CONTROL_DELETE_CLIENT(cWindowsApplication);

///////////////////////////////////////
//
// Create an instance...
//

cWindowsApplication::cWindowsApplication(int argc, const char *argv[],
                                         const char * pszName,
                                         IUnknown * pOuterUnknown)
  : m_argc(argc), m_argv(argv),
    m_NameStr(pszName),
    m_WinAppOperations(this, pOuterUnknown),
    m_ApplicationOperations(this, pOuterUnknown)
{
    // Add internal components to outer aggregate...
    ADD_TO_AGGREGATE_2( pOuterUnknown,
                        IID_IWinApp, &m_WinAppOperations,
                        IID_IApplication, &m_ApplicationOperations,
                        kPriorityReserved,
                        NULL );
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cWindowsApplication::cWinAppOperations
//

//
// Pre-fab COM implementations
//

IMPLEMENT_DELEGATION(cWindowsApplication::cWinAppOperations);
IMPLEMENT_CONNECTION_POINT(cWindowsApplication::cWinAppOperations, IWinAppAdviseSink);

///////////////////////////////////////
//
// Get the main window
//

STDMETHODIMP_(HWND) cWindowsApplication::cWinAppOperations::GetMainWnd()
{
    return m_hMainWnd;
}

///////////////////////////////////////
//
// Set the main window
//

STDMETHODIMP_(HWND) cWindowsApplication::cWinAppOperations::SetMainWnd(HWND hNewMainWnd)
{
    HWND hOldMainWnd = m_hMainWnd;
    m_hMainWnd = hNewMainWnd;
    return hOldMainWnd;
}

///////////////////////////////////////
//
// Get the current instance handle
//

STDMETHODIMP_(HINSTANCE) cWindowsApplication::cWinAppOperations::GetInstance()
{
    return GetModuleHandle(NULL);
}

///////////////////////////////////////
//
// Get the handle for Windows resources
//

STDMETHODIMP_(HINSTANCE) cWindowsApplication::cWinAppOperations::GetResourceInstance()
{
    return GetModuleHandle(NULL);
}

///////////////////////////////////////
//
// Get the command line
//

STDMETHODIMP_(const char *) cWindowsApplication::cWinAppOperations::GetCommandLine()
{
    CriticalMsg("Unimplemented call");
    return 0;
}

///////////////////////////////////////
//
// Get the intially requested "show state"
//

STDMETHODIMP_(int) cWindowsApplication::cWinAppOperations::GetCommandShow()
{
    CriticalMsg("Unimplemented call");
    return 0;
}

///////////////////////////////////////
//
// Set the application registry key
//

STDMETHODIMP_(void) cWindowsApplication::cWinAppOperations::SetRegistryKey(const char *)
{
    CriticalMsg("Unimplemented call");
}

///////////////////////////////////////
//
// Get the application registry key
//

STDMETHODIMP_(const char *) cWindowsApplication::cWinAppOperations::GetRegistryKey()
{
    CriticalMsg("Unimplemented call");
    return 0;
}

///////////////////////////////////////
//
// Set the application .ini file name
//

STDMETHODIMP_(void) cWindowsApplication::cWinAppOperations::SetProfileName(const char *)
{
    CriticalMsg("Unimplemented call");
}

///////////////////////////////////////
//
// Get the application .ini file name
//

STDMETHODIMP_(const char *) cWindowsApplication::cWinAppOperations::GetProfileName()
{
    CriticalMsg("Unimplemented call");
    return 0;
}

///////////////////////////////////////
//
// Set the default background color of application's dialogs
//

STDMETHODIMP_(void) cWindowsApplication::cWinAppOperations::SetDialogBkColor(COLORREF clrCtlBk, COLORREF clrCtlText)
{
    CriticalMsg("Unimplemented call");
}

///////////////////////////////////////
//
// Message pumping
//

eWinPumpResult cWindowsApplication::cWinAppOperations::DispatchOneMessage(MSG * pMsg, unsigned nFilterMin, unsigned nFilterMax)
{
    const int fGetResult = GetMessage(pMsg, NULL, nFilterMin, nFilterMax);

    AssertMsg(fGetResult >= 0, "Error calling GetMessage()");
    DebugMsgTrue(retVal == 0, "Recevied WM_QUIT");

    if (fGetResult && !PreTranslateMessage(pMsg))
    {
        TranslateMessage(pMsg);
        DispatchMessage(pMsg);
    }
    return (fGetResult > 0) ? kPumpedOk : kPumpedQuit;
}

STDMETHODIMP_(eWinPumpResult) cWindowsApplication::cWinAppOperations::PumpEvents(int fPumpKind, eWinPumpDuration fDuration)
{
    // If we're not in the midst of exiting...
    if (!(m_fFlags & kReceivedQuit))
    {
        MSG msg;
        unsigned nFilterMin;
        unsigned nFilterMax;

        // If want to pump until quit, must choose kPumpAll...
        AssertMsg(fDuration != kPumpUntilQuit || fPumpKind == kPumpAll, "PumpEvents() parameters too dangerous");

        // First pick filtering based on kind of pump desired...
        switch (fPumpKind)
        {
            case kPumpAll:
                nFilterMin = 0;
                nFilterMax = 0;
                break;

            case kPumpKeyboard:
                nFilterMin = WM_KEYFIRST;
                nFilterMax = WM_KEYLAST;
                break;

            case kPumpMouse:
                nFilterMin = WM_MOUSEFIRST;
                nFilterMax = WM_MOUSELAST;
                break;

            case kPumpPaint:
                nFilterMin = WM_PAINT;
                nFilterMax = WM_PAINT;
                break;

            default:
                CriticalMsg2("Don't know how to handle requested pump kind in PumpEvents(%x, %d)", fPumpKind, fDuration);
        }

        // Now start pumping according to desired duration...
        eWinPumpResult retVal;

        switch (fDuration)
        {
            case kPumpOne:
                if (PeekMessage(&msg, NULL, nFilterMin, nFilterMax, PM_NOREMOVE))
                    retVal = DispatchOneMessage(&msg, nFilterMin, nFilterMax);
                else
                    retVal = kPumpedNothing;

                break;

            case kPumpUntilEmpty:
                retVal = kPumpedNothing;
                while (PeekMessage(&msg, NULL, nFilterMin, nFilterMax, PM_NOREMOVE))
                    retVal = DispatchOneMessage(&msg, nFilterMin, nFilterMax);

                break;

            case kPumpUntilQuit:
                while (DispatchOneMessage(&msg, nFilterMin, nFilterMax) == kPumpedOk)
                    ;
                retVal = kPumpedQuit;

            default:
                CriticalMsg2("Don't know how to handle requested pump duration in PumpEvents(%x, %d)", fPumpKind, fDuration);
        }

        // If we received quit, set flag to shut down all subsequent PumpMessage() calls
        if (retVal == kPumpedQuit)
            m_fFlags |= kReceivedQuit;
        return retVal;
    }
    return kPumpedPastQuit;
}

///////////////////////////////////////
//
// Give clients a chance to handle our message
//

int cWindowsApplication::cWinAppOperations::PreTranslateMessage(MSG * pMsg)
{
    CONNECTION_POINT_ITERATE()
    {
        if (pSink->PreTranslateMessage(pMsg))
            return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////
//
// Show a modal dialog box
//

STDMETHODIMP cWindowsApplication::cWinAppOperations::ModalDialogBox(LPCTSTR lpTemplate, DLGPROC lpDialogFunc)
{
    CriticalMsg("Unimplemented call");
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cWindowsApplication::cApplicationOperations
//

//
// Pre-fab COM implementations
//
IMPLEMENT_DELEGATION(cWindowsApplication::cApplicationOperations);
IMPLEMENT_CONNECTION_POINT(cWindowsApplication::cApplicationOperations, IAppAdviseSink);

///////////////////////////////////////

STDMETHODIMP cWindowsApplication::cApplicationOperations::QueryQuit()
{
    CONNECTION_POINT_ITERATE()
    {
        if (pSink->OnQueryQuit() != S_OK)
        {
            return S_FALSE;
        }
    }

    Quit();

    return S_OK; // This will never execute
}

///////////////////////////////////////

STDMETHODIMP_(void) cWindowsApplication::cApplicationOperations::Quit()
{
    CONNECTION_POINT_ITERATE()
    {
        pSink->OnQuit();
    }
    exit(0); // This may never execute
}

///////////////////////////////////////

STDMETHODIMP_(void) cWindowsApplication::cApplicationOperations::Abort(const char * pszReason)
{
    CriticalMsg("Unimplemented call");
}

///////////////////////////////////////

STDMETHODIMP_(void) cWindowsApplication::cApplicationOperations::SetCaption(const char *)
{
    CriticalMsg("Unimplemented call");
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cWindowsApplication::cApplicationOperations::GetCaption()
{
    return m_pWindowsApplication->m_NameStr;
}

///////////////////////////////////////

STDMETHODIMP_(void) cWindowsApplication::cApplicationOperations::SetDefaultFilePath(const char *)
{
    CriticalMsg("Unimplemented call");
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cWindowsApplication::cApplicationOperations::GetDefaultFilePath()
{
    CriticalMsg("Unimplemented call");
    return 0;
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cWindowsApplication::cApplicationOperations::GetFullName()
{
    CriticalMsg("Unimplemented call");
    return 0;
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cWindowsApplication::cApplicationOperations::GetPath()
{
    CriticalMsg("Unimplemented call");
    return 0;
}

///////////////////////////////////////

STDMETHODIMP_(int) cWindowsApplication::cApplicationOperations::MessageBox(const char * pszMessage, const char * pszCaption, int fFlags)
{
    CriticalMsg("Unimplemented call");
    return 0;
}

///////////////////////////////////////

STDMETHODIMP_(int) cWindowsApplication::cApplicationOperations::CriticalMessageBox(const char * pszMessage, const char * pszCaption, int errorCode)
{
    CriticalMsg("Unimplemented call");
    return 0;
}

///////////////////////////////////////

STDMETHODIMP_(void) cWindowsApplication::cApplicationOperations::AppCommand(unsigned nCmdId)
{
    CONNECTION_POINT_ITERATE()
    {
        if (pSink->GetVersion() >= kVerAppAdvise_CommandSupport)
        {
            pSink->OnCommand(nCmdId);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

#endif /* _WIN32 */

