///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/gameshel/RCS/wgshelmt.cpp $
// $Author: TOML $
// $Date: 1997/09/10 14:01:42 $
// $Revision: 1.18 $
//
// - Because the priority is so high in this thread, it is important
//   that any code in here be coded with an eye on performance (toml 10-29-96)
//
// - When activation switches away from the app, the shell blocks
//   the app from using the display.  Because Full screen DD, GDI and
//   windowed DD all have slightly different interactions with
//   activation change, the blocking code is repeated both in the
//   pretranslate hook and the window proc hook to cover all cases.
//

#include <windows.h>
#include <lg.h>
#include <coremutx.h>
#include <comtools.h>

#include <dispapi.h>

#include <wgshell.h>
#include <wgshelmt.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cMTWinGameShell
//

cMTWinGameShell::cMTWinGameShell(int fFlags, IUnknown * pOuterUnknown)
  : cWinGameShell(fFlags, pOuterUnknown),
    m_fMasterBlockedFromDisplay(FALSE),
    m_fState(kStartup),
    m_fInGame(FALSE),
    m_WorkerThreadId(0),
    m_MasterThreadId(0)
{
    Verify(DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &m_hMasterThread, 0, FALSE, DUPLICATE_SAME_ACCESS));
    SetBlockInactive(FALSE);
    gm_pfnPreviousCritMsgNotificationHandler = CritMsgSetHandler(CritMsgNotificationHandler);
}

////////////////////////////////////////

cMTWinGameShell::~cMTWinGameShell()
{
    CritMsgSetHandler(gm_pfnPreviousCritMsgNotificationHandler);
    AssertInMaster();
    if (ThreadExists())
    {
        CallWorker(kGSWTExit);
        WaitForClose();
    }

    if (m_hMasterThread)
        CloseHandle(m_hMasterThread);
}

////////////////////////////////////////

inline BOOL cMTWinGameShell::OkToBlock()
{
    return m_fInGame && !gm_fInAssert && DisplayIsExclusive();
}

////////////////////////////////////////

inline void cMTWinGameShell::BlockDisplay()
{
    if (!m_fMasterBlockedFromDisplay)
    {
        m_fMasterBlockedFromDisplay = TRUE;
        m_pWinDisplayDevice->WaitForMutex();
    }
}

////////////////////////////////////////

inline void cMTWinGameShell::UnblockDisplay()
{
    if (m_fMasterBlockedFromDisplay)
    {
        m_fMasterBlockedFromDisplay = FALSE;
        m_pWinDisplayDevice->ReleaseMutex();
    }
}

////////////////////////////////////////

STDMETHODIMP_(void) cMTWinGameShell::BeginFrame()
{
    // If this is the first frame, set properties of normal operation
    if (!m_fInGame)
    {
        m_fInGame = TRUE;
        SetPriority(THREAD_PRIORITY_HIGHEST);
    }
    cWinGameShell::BeginFrame();
}

////////////////////////////////////////
//
// Message pump
//
// Only called from master, this function handles requests to exit
// from the worker
//

STDMETHODIMP_(void) cMTWinGameShell::PumpEvents(int fPumpFlags)
{
    AssertInMaster();

    switch (m_fState)
    {
        case kStartup:
        case kActive:
        {
            cWinGameShell::PumpEvents(fPumpFlags);
            break;
        }

        case kClosed:
            break;

        default:
            CriticalMsg("Unknown state");
    }
}

////////////////////////////////////////
//
// Window procedure
//

long cMTWinGameShell::WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND hWnd = GetHwnd();
    long retVal;

    switch (msg)
    {
        case WM_NCACTIVATE:
        {
            if (!wParam && !IsWindowVisible(hWnd))
                return FALSE;
            break;
        }

        case WM_DESTROY:
            m_fState = kClosed;
            break;
    }


    retVal = cWinGameShell::WndProc(msg, wParam, lParam);

    // We yield our timeslice to feed any starving threads
    Sleep(0);

    return retVal;
}

////////////////////////////////////////

void cMTWinGameShell::CreateGameWindow()
{
    m_MasterThreadId = GetCurrentThreadId();

    Create();
    CallWorker(kGSWTCreateWindow);
    SetForegroundWindow(GetHwnd());
}

////////////////////////////////////////

void cMTWinGameShell::DestroyGameWindow()
{
    AssertInMaster();
    if (ThreadExists())
    {
        CallWorker(kGSWTExit);
        WaitForClose();
    }
    cWinGameShell::DestroyGameWindow();

    m_MasterThreadId = 0;
}

////////////////////////////////////////

void cMTWinGameShell::NotifyDisplayOnActivateApp(BOOL fActive)
{
    AssertInWorker();
    if (!fActive)
    {
        SetThreadPriority(m_hMasterThread, THREAD_PRIORITY_NORMAL);
        SetPriority(THREAD_PRIORITY_NORMAL);
        if (OkToBlock())
        {
            BlockDisplay();
        }
    }

    cWinGameShell::NotifyDisplayOnActivateApp(fActive);

    if (fActive)
    {
        SetThreadPriority(m_hMasterThread, THREAD_PRIORITY_ABOVE_NORMAL);
        SetPriority(THREAD_PRIORITY_HIGHEST);
        UnblockDisplay();
    }
}

////////////////////////////////////////

void cMTWinGameShell::OnQuit()
{
    AssertInWorker();
    CriticalMsg("Abnormal termination -- no WM_CLOSE received!");
    ExitProcess(1);
}

////////////////////////////////////////

void cMTWinGameShell::OnActivateApp(BOOL fActive)
{
    AssertInWorker();

    if (!fActive && OkToBlock())
        BlockDisplay();

    cWinGameShell::OnActivateApp(fActive);

    if (fActive)
        UnblockDisplay();
}

////////////////////////////////////////

#define kSleepLimit 500

DWORD cMTWinGameShell::ThreadProc()
{
    m_WorkerThreadId = GetCurrentThreadId();

    enum eMTGameShellEvent
    {
        kCallFromMaster,
        kWindowMessage
    };

    // Wait for either a call from the master thread, or an item in the queue...
    DWORD   fWaitResult;
    BOOL    fFailedLastWait;
    HANDLE  waitHandles[1];

    m_fState = kActive;

    waitHandles[kCallFromMaster] = GetCallHandle();

    while (m_fState != kClosed)
    {
        // Wait for a message either from the master thread or in the queue...
        fWaitResult = MsgWaitForMultipleObjects(1, waitHandles, FALSE, kSleepLimit, QS_ALLINPUT);

        if (gm_fInAssert)
        {
            // Wait until assertion dialog is dealt with
            Sleep(50);
            continue;
        }

        // It apperars MsgWaitForMultipleObjects() fails, seemingly harmlessly, immediately
        // following a coninuance after a breakpoint is hit.  Everything appears fine
        // on the next iteration.  So, what we do is ignore the failure, and blow up
        // only if the exception repeats on the next wait. (toml 09-23-96)
        if (fWaitResult == WAIT_FAILED)
        {
            AssertMsg1(!fFailedLastWait, "Error in multi-threaded game shell message pump: 0x%x", GetLastError());
            Sleep(0);
            fFailedLastWait = TRUE;
            continue;
        }
        else
            fFailedLastWait = FALSE;

        // If we hit the sleep limit, peek for a message to inhibit non-critical OS cleanup code from activating
        if (fWaitResult == WAIT_TIMEOUT)
        {
            MSG msg;
            PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
            continue;
        }

        // Handle the message...
        switch (fWaitResult - WAIT_OBJECT_0)
        {
            // It's a call from the master thread...
            case kCallFromMaster:
            {
                switch (GetCallParam())
                {
                    case kGSWTExit:
                        m_fState = kClosed;
                        Reply(S_OK);
                        break;
#if 0
                    case kGSWTSuspend:
                        Reply(S_OK);
                        Suspend();
                        break;
#endif

                    case kGSWTCreateWindow:
                        cWinGameShell::CreateGameWindow();
                        SetThreadPriority(m_hMasterThread, THREAD_PRIORITY_ABOVE_NORMAL);
                        Reply(S_OK);
                        break;

                    default:
                        CriticalMsg("Unknown call to game shell worker thread");
                        Reply((DWORD)E_FAIL);
                        break;
                }
                break;
            }

            // Otherwise, if there's a someting in the message queue...
            case kWindowMessage:
            {
                m_pWinDisplayDevice->WaitForMutex();
                CoreThreadLock();
                DoPumpEvents();
                CoreThreadUnlock();
                m_pWinDisplayDevice->ReleaseMutex();
                break;
            }

            default:
                CriticalMsg("There was nothing to do!");
        }

        // We yield our timeslice to feed any starving threads
        Sleep(0);
    }

    m_WorkerThreadId = 0;

    return S_OK;
}

////////////////////////////////////////
//
// Critical message handling -- don't block the display if we're trying to assert
//

BOOL                        cMTWinGameShell::gm_fInAssert;
tCritMsgNotificationHandler cMTWinGameShell::gm_pfnPreviousCritMsgNotificationHandler;

void LGAPI cMTWinGameShell::CritMsgNotificationHandler(eCritMsgNotification notification)
{
    switch (notification)
    {
        case kCritMsgEnter:
            gm_fInAssert = TRUE;
            if (gm_pfnPreviousCritMsgNotificationHandler)
                (*gm_pfnPreviousCritMsgNotificationHandler)(notification);
            break;

        case kCritMsgExit:
            if (gm_pfnPreviousCritMsgNotificationHandler)
                (*gm_pfnPreviousCritMsgNotificationHandler)(notification);
            gm_fInAssert = FALSE;
            break;

        default:
            if (gm_pfnPreviousCritMsgNotificationHandler)
                (*gm_pfnPreviousCritMsgNotificationHandler)(notification);
    }

}

///////////////////////////////////////////////////////////////////////////////

