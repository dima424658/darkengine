///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/gameshel/RCS/wgshell.cpp $
// $Author: TOML $
// $Date: 1997/09/10 14:51:38 $
// $Revision: 1.55 $
//

#include <windows.h>
#include <lg.h>
#include <comtools.h>

#include <appagg.h>

#include <gshelapi.h>
#include <dispapi.h>
#include <movieapi.h>
#include <wappapi.h>
#include <appapi.h>
#include <wdispapi.h>
#include <winshapi.h>

#include <wgshell.h>
#include <winput.h>

#include <filespec.h>

#ifdef DEBUG_KEYBOARD
#undef DEBUG_KEYBOARD
#define DEBUG_KEYBOARD 1
#pragma message("DEBUG_SETMODE")
#else
#define DEBUG_KEYBOARD 0
#endif

///////////////////////////////////////////////////////////////////////////////

#define WM_GAMESHELL_SWITCHTODESKTOP    WM_USER
#define WM_GAMESHELL_SHOWHELP           (WM_USER + 1)
#define WM_GAMESHELL_EXEC               (WM_USER + 2)
#define WM_GAMESHELL_BRINGTOFRONT       (WM_USER + 3)

struct sGSHelpInfo
{
    const char * pszHelpFile;
    uint         command;
    ulong        dwData;
};

///////////////////////////////////////////////////////////////////////////////
//
// Power management messages (missing from Watcom header set)
//
// This code fragement can be removed when compiler headers include this (toml 09-13-96)
//

#ifndef _INC_PBT
#define _INC_PBT

#ifndef WM_POWERBROADCAST
#define WM_POWERBROADCAST               0x218
#endif

#define PBT_APMQUERYSUSPEND             0x0000
#define PBT_APMQUERYSTANDBY             0x0001

#define PBT_APMQUERYSUSPENDFAILED       0x0002
#define PBT_APMQUERYSTANDBYFAILED       0x0003

#define PBT_APMSUSPEND                  0x0004
#define PBT_APMSTANDBY                  0x0005

#define PBT_APMRESUMECRITICAL           0x0006
#define PBT_APMRESUMESUSPEND            0x0007
#define PBT_APMRESUMESTANDBY            0x0008

#define PBTF_APMRESUMEFROMFAILURE       0x00000001

#define PBT_APMBATTERYLOW               0x0009
#define PBT_APMPOWERSTATUSCHANGE        0x000A

#define PBT_APMOEMEVENT                 0x000B

#endif // _INC_PBT

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cWinGameShell
//

//
// Pre-fab COM implementations
//
IMPLEMENT_COMPLEX_AGGREGATION_SELF_DELETE(cWinGameShell);

///////////////////////////////////////

cWinGameShell * cWinGameShell::gm_pWinGameShell;

///////////////////////////////////////
//
// Construct an instance...
//

cWinGameShell::cWinGameShell(int fFlags, IUnknown * pOuterUnknown)
  : m_WinInputDevices(this, pOuterUnknown),
    m_hWnd(NULL),
    m_fCreateFlags(fFlags),
    m_flags(0),
    m_ulStartTime(0),
    m_ulTimeOffset(0),
    m_WinAppAdviseSink(this),
    m_pfnCursorCallback(NULL)
{
    // Add internal components to outer aggregate...
    INIT_AGGREGATION_2( pOuterUnknown,
                        IID_IGameShell, this,
                        IID_IInputDevices, &m_WinInputDevices,
                        kPriorityLibrary,
                        NULL );

    AssertMsg(!gm_pWinGameShell, "Only one instance of game shell allowed");
    gm_pWinGameShell = this;

    if (GetPrivateProfileInt("WinShell", "ShowCursor", FALSE, "lg.ini"))
        m_fCreateFlags |= kShowNativeCursor;
}

///////////////////////////////////////
//
// Destroy an instance...
//

cWinGameShell::~cWinGameShell()
{
    gm_pWinGameShell = NULL;
}

///////////////////////////////////////
//
// Determine if we should block the display when inactive
// @Note (toml 09-10-97): This is only called when pumping messages and the window
// is not active.
//

BOOL cWinGameShell::DisplayIsExclusive()
{
    sGrModeInfoEx info;
    m_pDisplayDevice->GetMode(&info);
    return !(info.flags & (kGrModeIsWindowed | kGrModeNotSet));
}

///////////////////////////////////////
//
// Pump-input and lock display surfaces (if specified when created)
//

#ifdef FRAME_TIME
#pragma message ("Game shell frame timing enabled")
#include <mprintf.h>
static int g_iTimeLastFrame;
#define FrameTimeBegin()    g_iTimeLastFrame = timeGetTime()
#define FrameTimeEnd()      mprintf(" [%d]\n", timeGetTime() - g_iTimeLastFrame)
#else
#define FrameTimeBegin()
#define FrameTimeEnd()
#endif

//
// determine whether a window owns another, recursively.
//
BOOL WindowBelongsTo(HWND parent, HWND child)
{
   if (parent == child)
      return TRUE;
   HWND owner = GetWindow(child,GW_OWNER);
   if (owner != NULL && WindowBelongsTo(parent,owner))
      return TRUE;
   HWND mom = GetParent(child);
   if (mom != NULL && WindowBelongsTo(parent,mom))
      return TRUE;
   return FALSE;
}


STDMETHODIMP_(void) cWinGameShell::BeginFrame()
{
    DebugMsgEx(VERBOSE, "cWinGameShell::BeginFrame()");
    FrameTimeBegin();

    // If the game is calling this, our window or one of its children had
    // better have focus...
    HWND hForeWnd = GetForegroundWindow();
    if ((hForeWnd != m_hWnd) && !WindowBelongsTo(m_hWnd, hForeWnd) && DisplayIsExclusive())
    {
        DebugMsg("Uncertain activation state.");
        SetForegroundWindow(m_hWnd);
    }

    // Move events from the Windows queue
    PumpEvents();

    // Lock the display device, if appropriate
    if (m_fCreateFlags & kLockFrame)
        m_pDisplayDevice->Lock();
}

///////////////////////////////////////
//
// Release locks, ensure screen is up-to-date
//

STDMETHODIMP_(void) cWinGameShell::EndFrame()
{
    // Unlock the display device, if appropriate
    if (m_fCreateFlags & kLockFrame)
        m_pDisplayDevice->Unlock();

    // Flush & optionally flip
    if (m_fCreateFlags & kFlushOnEndFrame)
        m_pDisplayDevice->Flush();

    if (m_fCreateFlags & kFlipOnEndFrame)
        m_pDisplayDevice->PageFlip();

    PumpEvents();

    FrameTimeEnd();
    DebugMsgEx(VERBOSE, "cWinShell::EndFrame()");
}

///////////////////////////////////////

STDMETHODIMP cWinGameShell::BringToFront()
{
    PostMessage(m_hWnd, WM_GAMESHELL_BRINGTOFRONT, 0, 0);
    return S_OK;
}

///////////////////////////////////////
//
// Process pending events
//
// This model empties the queue rather than pumping only one event
//

STDMETHODIMP_(void) cWinGameShell::PumpEvents(int /*fPumpFlags*/)
{
    int lockCount = m_pDisplayDevice->BreakLock();

    DoPumpEvents();

    // If a WM_CLOSE event was received...
    if (IsQuitSignaled())
    {
        // And we're not already handling it (i.e., someone is pumping events during it)
        if (!Quitting())
        {
            // Ask the application to exit
            SetQuitting(TRUE);

            AutoAppIPtr(Application);
            if (pApplication)
                pApplication->QueryQuit();

            // If we reach here, we didn't quit...
            SetQuitRequest(FALSE);
            SetQuitting(FALSE);
        }
    }

    m_pDisplayDevice->RestoreLock(lockCount);
    return;
}

///////////////////////////////////////
//
// Try to lock video memory.
//

STDMETHODIMP_(int) cWinGameShell::DisplayLock()
{
    return m_pDisplayDevice->Lock();
}

///////////////////////////////////////
//
// Release a lock on memory
//

STDMETHODIMP_(int) cWinGameShell::DisplayUnlock()
{
    return m_pDisplayDevice->Unlock();
}

///////////////////////////////////////
//
// Verify display reflects what is expected
//

STDMETHODIMP_(void) cWinGameShell::DisplayFlush()
{
    m_pDisplayDevice->Flush();
}

///////////////////////////////////////
//
// Get the present tick count, in msec since initialization or timer restart
//

STDMETHODIMP_(unsigned long) cWinGameShell::GetTimerTick()
{
    // @TBD (toml 05-06-96): This should be moved to a proper timer library thing
    return WindowsTimeToGameTime(timeGetTime());
}

///////////////////////////////////////
//
// Reset the timer (by default, starts at zero from initialization)
//

STDMETHODIMP_(void) cWinGameShell::SetTimer(unsigned long ulTimeOffset)
{
    m_ulTimeOffset = ulTimeOffset;
    m_ulStartTime = timeGetTime();
}

///////////////////////////////////////
//
// Show Windows Help File
//

STDMETHODIMP_(BOOL) cWinGameShell::ShowHelp(const char * pszHelpFile, uint command, ulong dwData)
{
    sGSHelpInfo * pHelpInfo =  new sGSHelpInfo;

    pHelpInfo->pszHelpFile = pszHelpFile;
    pHelpInfo->command = command;
    pHelpInfo->dwData = dwData;

    PostMessage(m_hWnd, WM_GAMESHELL_SHOWHELP, (ulong)pHelpInfo, 0);

    return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cWinGameShell::SetOSCursor(sGrBitmap * /*pCursorBitmap*/, const Point * /*pHotSpot*/)
{
    return FALSE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cWinGameShell::ShowOSCursor(BOOL)
{
    return FALSE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cWinGameShell::SetCursorCallback(tGSCursorCallbackFunc pfnCursorCallback)
{
    m_pfnCursorCallback = pfnCursorCallback;
}

///////////////////////////////////////
//
// Execute a command line
//

STDMETHODIMP_(BOOL) cWinGameShell::Exec(const char * pszCommandLine )
{
    PostMessage(m_hWnd, WM_GAMESHELL_EXEC, (ulong)pszCommandLine, 0);

    return TRUE;
}

STDMETHODIMP_(BOOL) cWinGameShell::SetFlags(int fFlags )
{
    m_fCreateFlags = fFlags;

    return TRUE;
}

STDMETHODIMP_(BOOL) cWinGameShell::GetFlags(int * pfFlags )
{
    *pfFlags = m_fCreateFlags;

    return TRUE;
}

///////////////////////////////////////
//
// Aggregate member protocol: Called by contained aggregate control, it being
// called by outer aggregate
//

HRESULT cWinGameShell::Connect()
{
    SelfConnectRequired(DisplayDevice);
    SelfConnectRequired(WinApp);
    SelfConnectRequired(WinDisplayDevice);

    return NOERROR;
}

///////////////////////////////////////

HRESULT cWinGameShell::PostConnect()
{
    return NOERROR;
}

///////////////////////////////////////

HRESULT cWinGameShell::Init()
{
    BEGIN_DEBUG_MSG("cWinGameShell::Init()");

    // Hook up for PreTranslateMessage()
    AutoConIPtr(Aggregate, this);
    if (pAggregate)
        pAggregate->BeginSelfReference();

    m_pWinApp->Advise(&m_WinAppAdviseSink, &m_WinAppAdviseCookie);

    if (pAggregate)
        pAggregate->EndSelfReference();

    CreateGameWindow();

    if (!m_hWnd)
    {
        CriticalMsg("Failed to create main window");
        return E_FAIL;
    }

    m_flags |= kAllowPump;

    m_pWinApp->SetMainWnd(m_hWnd);

    ShowWindow(m_hWnd, SW_HIDE);
    UpdateWindow(m_hWnd);

    PumpEvents();

    // Start by assuming we're active
    m_flags |= kActive;

    if (!GetPrivateProfileInt("WinShell", "DisableInactiveBlock", FALSE, "lg.ini"))
        m_flags |= kBlockInactive;

    m_ulStartTime = timeGetTime();

    return NOERROR;

    END_DEBUG;
}

///////////////////////////////////////

HRESULT cWinGameShell::End()
{
    DestroyGameWindow();

    m_flags &= ~kAllowPump;

    AutoConIPtr(Aggregate, this);
    if (pAggregate)
        pAggregate->BeginSelfReference();

    m_pWinApp->Unadvise(m_WinAppAdviseCookie);

    if (pAggregate)
        pAggregate->EndSelfReference();

    return NOERROR;
}

///////////////////////////////////////

HRESULT cWinGameShell::Disconnect()
{
    SafeRelease(m_pDisplayDevice);
    SafeRelease(m_pWinApp);
    SafeRelease(m_pWinDisplayDevice);

    return NOERROR;
}

///////////////////////////////////////

void cWinGameShell::CreateGameWindow()
{
    const char * pszWndClass = "LookingGlassTechnologies_WindowsGameShell_v2";

    // Register a class for the game window
    HINSTANCE hInst = m_pWinApp->GetInstance();
    WNDCLASS wndClass;

    wndClass.style          = CS_BYTEALIGNCLIENT |  CS_VREDRAW | CS_HREDRAW /*| CS_DBLCLKS*/;
    wndClass.lpfnWndProc    = StaticWndProc;
    wndClass.cbClsExtra     = 0;
    wndClass.cbWndExtra     = 0;
    wndClass.hInstance      = hInst;
    wndClass.hIcon          = LoadIcon(hInst, MAKEINTRESOURCE(IDI_APPICON));
    wndClass.hCursor        = (m_fCreateFlags & kShowNativeCursor) ? LoadCursor(NULL, IDC_ARROW) : NULL;
    wndClass.hbrBackground  = (HBRUSH) NULL;
    wndClass.lpszClassName  = pszWndClass;
    wndClass.lpszMenuName   = NULL;

    if (!RegisterClass(&wndClass))
    {
        CriticalMsg("Failed to register class");
        return;
    }

    //
    // Create the window
    //
    // There's always only one instance of the shell. The m_hWnd
    // member will be set when the WM_CREATE message is received
    // (before CreateGameWindow() returns).  It is set to blow up if a second
    // cWinShell::Init() is called (toml 01-29-96)

    AutoAppIPtr(Application);

    const DWORD kDefaultStyle = WS_OVERLAPPED | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    CreateWindowEx(0,
                   pszWndClass,
                   pApplication->GetCaption(),
                   kDefaultStyle,
                   0, 0,
                   GetSystemMetrics( SM_CXSCREEN ) / 2,
                   GetSystemMetrics( SM_CYSCREEN ) / 2,
                   NULL,
                   NULL,
                   hInst,
                   NULL);

    SetForegroundWindow(m_hWnd);
}

///////////////////////////////////////

void cWinGameShell::DestroyGameWindow()
{
    if (m_hWnd)
        DestroyWindow(m_hWnd);
}

///////////////////////////////////////

void cWinGameShell::OnQuit()
{
    // If we pumped WM_QUIT and client has requested a exit on quit...
    if (m_fCreateFlags & kExitOnQuitEvent)
        exit(0);
}

///////////////////////////////////////

void cWinGameShell::NotifyDisplayOnActivateApp(BOOL fActive)
{
    const BOOL fDisplayActive = !!(m_flags & kDisplayActive);

    if (fActive != fDisplayActive)
    {
        if (m_pfnCursorCallback && !fActive)
            (*m_pfnCursorCallback)(kGSHideCursor);

        m_pWinDisplayDevice->OnTaskSwitch(fActive);

        if (m_pfnCursorCallback && fActive)
            (*m_pfnCursorCallback)(kGSShowCursor);

        m_flags = (fActive) ? (m_flags | kDisplayActive) : (m_flags & ~kDisplayActive);
    }
}

///////////////////////////////////////

void cWinGameShell::OnActivateApp(BOOL fActive)
{
    if (IsActive() == fActive)
        return;

    NotifyDisplayOnActivateApp(fActive);

    if (fActive)
        m_flags |= kActive;
    else
        m_flags &= ~kActive;

    // If we're becoming active...
    if (IsActive())
    {
        // Then flush the queue of all keyboard input, reset any saved
        // alt/ctrl/shift states by posting an aritifical key-up
        // event
        MSG msg;
        DebugMsgEx(KEYBOARD, "Flushing out keyboard states...");
        while (PeekMessage(&msg, m_hWnd, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
            ;

        const ulong kExtendedBit = 0x01000000;
        const ulong kUpBit = 0x80000000;
        PostMessage(m_hWnd, WM_SYSKEYUP, VK_MENU,    (kUpBit | (MapVirtualKey(VK_MENU, 0)    << 16) | 1));
        PostMessage(m_hWnd, WM_KEYUP,    VK_CONTROL, (kUpBit | (MapVirtualKey(VK_CONTROL, 0) << 16) | 1));
        PostMessage(m_hWnd, WM_KEYUP,    VK_SHIFT,   (kUpBit | (MapVirtualKey(VK_SHIFT, 0)   << 16) | 1));

        PostMessage(m_hWnd, WM_SYSKEYUP, VK_MENU,    (kExtendedBit | kUpBit | (MapVirtualKey(VK_MENU, 0)    << 16) | 1));
        PostMessage(m_hWnd, WM_KEYUP,    VK_CONTROL, (kExtendedBit | kUpBit | (MapVirtualKey(VK_CONTROL, 0) << 16) | 1));
        PostMessage(m_hWnd, WM_KEYUP,    VK_SHIFT,   (kExtendedBit | kUpBit | (MapVirtualKey(VK_SHIFT, 0)   << 16) | 1));
    }
}

///////////////////////////////////////

long FAR PASCAL cWinGameShell::StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // If this is the first meaningful message sent to the window...
    if (msg == WM_CREATE)
    {
        // ... then set the cWinShell m_hWnd
        AssertMsg(!gm_pWinGameShell->m_hWnd, "Cannot create two instances of cWinShell, or Init() called twice!");
        gm_pWinGameShell->m_hWnd = hwnd;
    }

    // Let the shell instance handle the message...
    if (hwnd == gm_pWinGameShell->m_hWnd)
        return gm_pWinGameShell->WndProc(msg, wParam, lParam);
    else
        return DefWindowProc(hwnd, msg, wParam, lParam);
}

///////////////////////////////////////

void cWinGameShell::DoPumpEvents(int fPumpFlags, eWinPumpDuration fDuration)
{
    BEGIN_DEBUG_MSG_EX(VERBOSE, "cWinGameShell::PumpEvents()");

    // Pump is allowed after game shell window is created until
    // the destroy message is received
    if (!PumpAllowed())
        return;

    // Pump messages once.  If we're not active, pump until we are (thus
    // blocking the game while it doesn't have focus)
    eWinPumpResult pumpResult;
    do
    {
        pumpResult = m_pWinApp->PumpEvents(fPumpFlags, fDuration);

        // If we're exiting, shut down the message pump...
        if (pumpResult == kPumpedQuit || pumpResult == kPumpedPastQuit)
            m_flags &= ~kAllowPump;

        if (pumpResult == kPumpedQuit)
            OnQuit();

    } while ( PumpAllowed() && (!IsActive() && BlockInactive() && DisplayIsExclusive()) );

    END_DEBUG;
}

///////////////////////////////////////
//
// Handle messages from pump before dispatch
//
// PreTranslateMessage is used for very powerful message handling.  In
// general, all messages should be handled by the WndProc.  This
// facility is for more advancced control of behavior. (toml 05-08-96)
//

BOOL cWinGameShell::PreTranslateMessage(MSG * pMsg)
{
    // If it's not us...
    if (pMsg->hwnd != m_hWnd)
        return FALSE;

    // ... else handle critical messages
    switch (pMsg->message)
    {
        case WM_ACTIVATEAPP:
             if (!pMsg->wParam)
                NotifyDisplayOnActivateApp(FALSE);
             break;

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYUP:
        {
            // Check if it's a special key key...
            switch (pMsg->wParam)
            {
                // If it's a pause, set focus to the desktop window...
                case VK_PAUSE:
                    switch (pMsg->message)
                    {
                        case WM_SYSKEYDOWN:
                        case WM_KEYDOWN:
                        {
                            AutoAppIPtr(MoviePlayer1);
                            if (!!pMoviePlayer1 && pMoviePlayer1->GetState() == kMP1Playing)
                                return FALSE;

                            NotifyDisplayOnActivateApp(FALSE);
                            SetForegroundWindow(GetDesktopWindow());
                            ShowWindow(m_hWnd, SW_MINIMIZE);
                        }
                    }
                    return TRUE;  // Eat it

                // If it's a Print Screen, post the Print Screen command...
                case VK_SNAPSHOT:
                    switch (pMsg->message)
                    {
                        // @Note (toml 03-06-96): It appears that Win95 VK_SNAPSHOT
                        // inside the low-level keyboard driver, before we
                        // can intercept it.  But we do get the up event So...
                        case WM_SYSKEYUP:
                        case WM_KEYUP:
                        {
                            // Clear the clipboard of the BMP that Windows has
                            // "kindly" shoved into it (we need that memory!)

                            // @TBD (toml 05-07-96): Golf wants windows clipboard, flight doesn't -- must resolve
                            if (GetPrivateProfileInt("WinShell", "DisableWindowsPrintScreen", TRUE, "lg.ini") && OpenClipboard(NULL))
                            {
                                EmptyClipboard();
                                CloseClipboard();
                            }
                            DebugMsg("Posting (WM_COMMAND, ID_PRINTSCREEN)");
                            PostMessage(m_hWnd, WM_COMMAND, ID_PRINTSCREEN, 0);
                        }
                    }
                    DebugMsg("Eating VK_SNAPSHOT");
                    return TRUE;  // Eat it
            }
        }
    }
    return FALSE;
}

///////////////////////////////////////

long cWinGameShell::WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Begin Scope
    {
    long retVal = 0;

    // Allow primitive input device manager first dibs to message if we're active
    if (IsActive() && m_WinInputDevices.ProcessMessage(msg, wParam, lParam, retVal))
        return retVal;

    // Then give the display device a shot
    if (m_pWinDisplayDevice->ProcessMessage(msg, wParam, lParam, &retVal))
        return retVal;
    }
    // End scope

    switch (msg)
    {
        case WM_SETCURSOR:
            // If we're in the client area, hide the mouse pointer if set
            if (!(m_fCreateFlags & kShowNativeCursor) && LOWORD(lParam) == HTCLIENT)
            {
                SetCursor(NULL);
                return TRUE;
            }
            break;

        case WM_ACTIVATEAPP:
        {
            OnActivateApp(!!wParam);

            DebugMsg1("Receved WM_ACTIVATEAPP (%d)", !!IsActive());
            break;
        }

        case WM_POWERBROADCAST:
            switch(wParam)
            {
                case PBT_APMSUSPEND:
                case PBT_APMQUERYSUSPEND:
                    PostMessage(m_hWnd, WM_GAMESHELL_SWITCHTODESKTOP, 0, 0);
            }
            break;

        case WM_GAMESHELL_SWITCHTODESKTOP:
            if (IsActive() || GetForegroundWindow() == m_hWnd)
            {
                ShowWindow(m_hWnd, SW_MINIMIZE);
                SetForegroundWindow(GetDesktopWindow());
                SetFocus(GetDesktopWindow());
            }

            return TRUE;

        case WM_GAMESHELL_SHOWHELP:
        {
            sGSHelpInfo * pHelpInfo = (sGSHelpInfo *) wParam;

            NotifyDisplayOnActivateApp(FALSE);
            SetForegroundWindow(GetDesktopWindow());
            WinHelp(m_hWnd, pHelpInfo->pszHelpFile, pHelpInfo->command, pHelpInfo->dwData);

            delete pHelpInfo;

            return TRUE;
        }

        case WM_GAMESHELL_EXEC:
        {
            // pop us out of full screen mode if needed
            NotifyDisplayOnActivateApp(FALSE);
            SetForegroundWindow(GetDesktopWindow());

            char *pCmdLine = (char *) wParam;
            
            WinExec( pCmdLine, SW_SHOW );
            
            return TRUE;
        }
        
        case WM_GAMESHELL_BRINGTOFRONT:
        {
            ShowWindow(m_hWnd, SW_RESTORE);
            SetForegroundWindow(m_hWnd);
            SetFocus(m_hWnd);
            
            return TRUE;
        }


        case WM_CLOSE:
        {
            if (GetForegroundWindow() != m_hWnd)
                SetForegroundWindow(m_hWnd);
            SetQuitRequest(TRUE);
            return FALSE;
        }

        case WM_DESTROY:
            m_flags &= ~kAllowPump;
            m_pWinApp->SetMainWnd(NULL);
            m_hWnd = NULL;
            PostQuitMessage(0);

            break;

        case WM_COMMAND:
        {
            // Handle standard menus
            DebugMsg("Received WM_COMMAND...");
            switch (wParam)
            {
                case ID_EXIT:
                    PostMessage(m_hWnd, WM_CLOSE, 0, 0);
                    break;

                default:
                {
                    AutoAppIPtr(Application);
                    if (pApplication)
                        pApplication->AppCommand(wParam);
                }
            }
            break;
        }

        case WM_PAINT:
        {
            // Validate the invalid region.  It's up to m_pWinDisplayDevice to
            // do any painting
            PAINTSTRUCT ps;
            BeginPaint(m_hWnd, &ps);
            EndPaint(m_hWnd, &ps);

            return FALSE;
        }
    }

    return DefWindowProc(m_hWnd, msg, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////////
//
// class cWinAppAdviseSink
//

IMPLEMENT_UNAGGREGATABLE_NO_FINAL_RELEASE(cWinGameShell::cWinAppAdviseSink, IWinAppAdviseSink);

///////////////////////////////////////

STDMETHODIMP_(short) cWinGameShell::cWinAppAdviseSink::GetVersion()
{
    return kVerWinAppAdvise;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cWinGameShell::cWinAppAdviseSink::PreTranslateMessage(MSG * pMsg)
{
    return m_pOuter->PreTranslateMessage(pMsg);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cWinGameShell::cWinAppAdviseSink::OnIdle(long /*lCount*/)
{
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
