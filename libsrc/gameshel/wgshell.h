///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/gameshel/RCS/wgshell.h $
// $Author: TOML $
// $Date: 1997/09/10 14:01:40 $
// $Revision: 1.31 $
//

#ifndef __WGSHELL_H
#define __WGSHELL_H

#include <comtools.h>
#include <objcoll.h>
#include <aggmemb.h>
#include <gshelapi.h>
#include <wappapi.h>

#include <wdispapi.h>

#include <winshapi.h>
#include <winput.h>

#include <comconn.h>

F_DECLARE_INTERFACE(IDisplayDevice);
F_DECLARE_INTERFACE(IWinApp);
F_DECLARE_INTERFACE(IWinDisplayDevice);

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cWinGameShell
//

class cWinGameShell : public IGameShell
{
public:
    // Construct an instance...
    cWinGameShell(int fFlags, IUnknown * pOuterUnknown);

    // Accessors for contained instance of cWinInputDevices
    HWND GetHwnd();
    BOOL CaptureMouseWhileDown();
    void ClientToGamePoint(int & x, int & y);
    void GameToClientPoint(int & x, int & y);

    // Convert a Windows millisecond clock value to one the game likes
    unsigned long WindowsTimeToGameTime(unsigned long ulWindowsTime) const;


protected:
    // Destroy an instance...
    virtual ~cWinGameShell();

    ///////////////////////////////////
    //
    // IGameShell implementation functions
    //

    // IUnknown methods
    DECLARE_COMPLEX_AGGREGATION(cWinGameShell);

    // Pump-input and lock display surfaces (if specified when created)
    STDMETHOD_(void, BeginFrame)();

    // Release locks, ensure screen is up-to-date
    STDMETHOD_(void, EndFrame)();

    // Make sure the game is the user focus
    STDMETHOD (BringToFront)();

    // Obsolete Holder/Sink methods
    STDMETHOD (Obsolete1)() { return 0; }

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

    ///////////////////////////////////
    //
    // Functions to define specific behavior, intended for optional derived overriding
    //

    virtual void CreateGameWindow();
    virtual void DestroyGameWindow();
    virtual void NotifyDisplayOnActivateApp(BOOL fActive);
    virtual void OnQuit();
    virtual void OnActivateApp(BOOL fActive);

    ///////////////////////////////////

    BOOL IsActive()                 { return (m_flags & kActive);                                       }
    BOOL PumpAllowed()              { return (m_flags & kAllowPump);                                    }
    BOOL BlockInactive()            { return (m_flags & kBlockInactive);                                }
    void SetBlockInactive(BOOL b)   { if (b) m_flags |= kBlockInactive; else m_flags &= ~kBlockInactive;}
    BOOL IsQuitSignaled()           { return (m_flags & kQuit);                                         }
    void SetQuitRequest(BOOL b)     { if (b) m_flags |= kQuit; else m_flags &= ~kQuit;                  }
    BOOL Quitting()                 { return (m_flags & kQuitting);                                     }
    void SetQuitting(BOOL b)        { if (b) m_flags |= kQuitting; else m_flags &= ~kQuitting;          }
    
    BOOL DisplayIsExclusive();

    ///////////////////////////////////
    //
    // Message routing and handling
    //

    // Actual message pump
    void DoPumpEvents(int fPumpFlags = kPumpAll, eWinPumpDuration fDuration = kPumpUntilEmpty);

    // Handle a message before dispatch
    virtual BOOL PreTranslateMessage(MSG * pMsg);

    // Handle a message dispatched to our window
    virtual long WndProc(UINT msg, WPARAM wParam, LPARAM lParam);

    ///////////////////////////////////
    //
    // Protected Data
    //

    // Interfaces used
    DeclMemberIPtr(DisplayDevice);
    DeclMemberIPtr(WinApp);
    DeclMemberIPtr(WinDisplayDevice);


private:

    ///////////////////////////////////
    //
    // Aggregate member protocol
    //

    HRESULT Connect();
    HRESULT PostConnect();
    HRESULT Init();
    HRESULT End();
    HRESULT Disconnect();

    ///////////////////////////////////

    // Because DirectDraw reserves the space in the window structure we could
    // normally use to store our context, we place it in gm_pWinGameShell
    static cWinGameShell * gm_pWinGameShell;
    static long FAR PASCAL StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    ///////////////////////////////////
    //
    // class cWinAppAdviseSink: router for PreTranslateMessage()
    //

    class cWinAppAdviseSink : public IWinAppAdviseSink
    {
    public:
        cWinAppAdviseSink(cWinGameShell * pOuter)
          : m_pOuter(pOuter)
        {
        }
    private:
        DECLARE_UNAGGREGATABLE();
        STDMETHOD_(short, GetVersion)();
        STDMETHOD_(BOOL, PreTranslateMessage)(MSG * pMsg);
        STDMETHOD_(BOOL, OnIdle)(long lCount);

        cWinGameShell * m_pOuter;
    };

    friend class cWinAppAdviseSink;

    ///////////////////////////////////
    //
    // Private Data
    //

    // Input devices
    cWinInputDevices    m_WinInputDevices;

    // Instance data
    HWND                m_hWnd;
    int                 m_fCreateFlags;

    unsigned long       m_ulStartTime;
    unsigned long       m_ulTimeOffset;

    enum
    {
        kActive         = 0x01,
        kBlockInactive  = 0x02,
        kAllowPump      = 0x04,
        kQuit           = 0x08,
        kDisplayActive  = 0x10,
        kQuitting       = 0x20
    };

    int                 m_flags;

    // Sink for receiving PreTranslateMessage();
    cWinAppAdviseSink   m_WinAppAdviseSink;
    unsigned long       m_WinAppAdviseCookie;
    
    tGSCursorCallbackFunc m_pfnCursorCallback;

};

///////////////////////////////////////

inline HWND cWinGameShell::GetHwnd()
{
    return m_hWnd;
}

///////////////////////////////////////

inline BOOL cWinGameShell::CaptureMouseWhileDown()
{
    return !!(m_fCreateFlags & kCaptureMouseWhileDown);
}

///////////////////////////////////////
//
// Convert Windows client coordinates to game display device coordinates
//

inline void cWinGameShell::ClientToGamePoint(int & x, int & y)
{
    RECT rect;
    unsigned nDisplayWidth, nDisplayHeight;

    m_pWinDisplayDevice->GetWidthHeight(&nDisplayWidth, &nDisplayHeight);
    GetClientRect(m_hWnd, &rect); // left and top will be zero

    if (rect.right && rect.bottom)
    {
        x = (x * nDisplayWidth) / rect.right;
        y = (y * nDisplayHeight) / rect.bottom;
    }
}

///////////////////////////////////////
//
// Convert game display device coordinates to Windows client coordinates
//

inline void cWinGameShell::GameToClientPoint(int & x, int & y)
{
    RECT rect;
    unsigned nDisplayWidth, nDisplayHeight;

    m_pWinDisplayDevice->GetWidthHeight(&nDisplayWidth, &nDisplayHeight);
    GetClientRect(m_hWnd, &rect); // left and top will be zero

    if (nDisplayWidth && nDisplayHeight)
    {
        x = (x * rect.right) / nDisplayWidth;
        y = (y * rect.bottom) / nDisplayHeight;
    }
}

///////////////////////////////////////

inline unsigned long cWinGameShell::WindowsTimeToGameTime(unsigned long ulWindowsTime) const
{
    return (ulWindowsTime - m_ulStartTime) + m_ulTimeOffset;
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__WGSHELL_H */
