///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/gameshel/RCS/winput.cpp $
// $Author: DAVET $
// $Date: 1997/10/29 09:46:06 $
// $Revision: 1.19 $
//
// Low-level Input
//

#include <windows.h>
#include <lg.h>
#include <comtools.h>

#include <indevapi.h>
#include <winput.h>
#include <wgshell.h>

#include <mouse.h>
#include <kb.h>
#include <nec98.h>

#ifndef DEBUG_MOUSE
#define DEBUG_MOUSE 0
#else
#pragma message("DEBUG_MOUSE is enabled")
#endif

#ifndef DEBUG_KEYBOARD
#define DEBUG_KEYBOARD 0
#else
#pragma message("DEBUG_KEYBOARD is enabled")
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cWinInputDevices
//

//
// Pre-fab COM implementations
//
IMPLEMENT_DELEGATION(cWinInputDevices);
IMPLEMENT_CONNECTION_POINT(cWinInputDevices, IPrimaryInputDevicesSink);

///////////////////////////////////////

cWinInputDevices::cWinInputDevices(cWinGameShell * pOuter, IUnknown * pOuterUnknown)
 : m_pOuter(pOuter),
   m_fNEC98KeyBoard(IsJapaneseNec98())
{
    INIT_DELEGATION(pOuterUnknown);
}

///////////////////////////////////////
//
// This routine figures out if we are running on a NEC-PC98 keyboard. 
//

BOOL cWinInputDevices::IsJapaneseNec98()
{
        int  ktype,stype;

        ktype=GetKeyboardType(0);  
        stype=GetKeyboardType(1);
        if(ktype==7)   // actually, 7 here refers to an OEM keyboard  
                if((0x0d01<=stype)&&(stype<=0x0d04)){  // PC-98 Series
                   return TRUE;                 // Japanese Keyboard 
                }
        return FALSE;

}

///////////////////////////////////////
//
// Handle a message from owning cWinGameShell instance
//

BOOL cWinInputDevices::ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam, long & RetVal)
{
    if (!m_Sinks.Count())
        return FALSE;

    // If it's a keyboard event...
    if (msg >= WM_KEYFIRST && msg <= WM_KEYLAST)
    {
        kbs_event event;
        BOOL bPassKeyToGame = TRUE;

        // And it's interesting...
        switch (msg)
        {
            case WM_SYSKEYDOWN:
            case WM_KEYDOWN:
            {
                // Check if it's a key to block...
                switch (wParam)
                {
                    // Don't pass these...
                    case VK_SCROLL:
                    case VK_NUMLOCK:
                        bPassKeyToGame = FALSE;
                        break; // To fall through...
                }
            }
                // Fall through...
            case WM_SYSKEYUP:
            case WM_KEYUP:
                if (bPassKeyToGame && WinKeyToLGKey(msg, wParam, lParam, event))
                {
                    DebugMsgEx3(KEYBOARD, "Received keyboard event (%s). Sending to game as (0x%x, 0x%x)...", LogStrWinMsg(m_pOuter->GetHwnd(), msg, wParam, lParam), event.code, event.state);

                    // Dispatch event...
                    CONNECTION_POINT_ITERATE()
                    {
                        if (pSink->OnKey(&event) != NOERROR)
                            break;
                    }
                }
                else
                    DebugMsgEx1(KEYBOARD, "Received keyboard event (%s)", LogStrWinMsg(m_pOuter->GetHwnd(), msg, wParam, lParam));
        }
    }
    else if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST)
    {
        DispatchMouseEvent(msg, wParam, lParam);
        
        if (m_pOuter->CaptureMouseWhileDown())
        {
            switch (msg)
            {
                case WM_LBUTTONDOWN:
                case WM_RBUTTONDOWN:
                case WM_MBUTTONDOWN:
                {
                    if (GetCapture() != m_pOuter->GetHwnd())
                    {
                        SetCapture(m_pOuter->GetHwnd());
                    }
                    break;
                }

                case WM_LBUTTONUP:
                case WM_LBUTTONDBLCLK:
                case WM_RBUTTONUP:
                case WM_RBUTTONDBLCLK:
                case WM_MBUTTONUP:
                case WM_MBUTTONDBLCLK:
                {
                    if (GetCapture() == m_pOuter->GetHwnd())
                    {
                        ReleaseCapture();
                    }
                    break;
                }
            }
        }

    }
    // It's not a key or mouse, but is it still interesting?
    else
    {
        switch (msg)
        {
            case WM_SYSCOMMAND:
            {
                // This prevents Windows from raising menus by the
                // keyboard during the game.  This may be bad if we
                // ever really use Windows menus
                if (wParam == SC_KEYMENU)
                {
                    RetVal = 0;
                    return TRUE;
                }
                break;
            }
            
            case WM_SETCURSOR:
            {
                // If we're not in the client area...
                if (LOWORD(lParam) != HTCLIENT && GetCapture() != m_pOuter->GetHwnd())
                {
                    // ...fake the game into hiding the cursor
                    // @TBD (toml 01-10-97): when input library is reworked, cursor hiding should be formalized
                    DispatchMouseEvent(WM_MOUSEMOVE, 0, ((4000 << 16) | 4000));
                }
                break;
            }
        }
    }

    return FALSE;
}

///////////////////////////////////////

inline int cWinInputDevices::GetAsyncMouseButtonState()
{
    return MouseMakeButtons((GetAsyncKeyState(VK_LBUTTON) & 0x8000),
                            (GetAsyncKeyState(VK_MBUTTON) & 0x8000),
                            (GetAsyncKeyState(VK_RBUTTON) & 0x8000));
}

///////////////////////////////////////
//
// Query mouse state, relative to game display surface
//

STDMETHODIMP_(BOOL) cWinInputDevices::GetMouseState(int * x, int * y, int * pButtons)
{
    BEGIN_DEBUG_MSG_EX(MOUSE, "cWinInputDevices::GetMouseState()");
    POINT point;

    if (GetForegroundWindow() == m_pOuter->GetHwnd() && GetCursorPos(&point) && ScreenToClient(m_pOuter->GetHwnd(), &point))
    {
        *x = (int) point.x;
        *y = (int) point.y;
        m_pOuter->ClientToGamePoint(*x, *y);
        *pButtons = GetAsyncMouseButtonState();
        return TRUE;
    }

    return FALSE;
    END_DEBUG;
}

///////////////////////////////////////
//
// Set mouse position, relative to game display surface
//

STDMETHODIMP_(BOOL) cWinInputDevices::SetMousePos(int x, int y)
{
    BEGIN_DEBUG_MSG_EX(MOUSE, "cWinInputDevices::SetMousePos()");
    POINT point;
    m_pOuter->GameToClientPoint(x, y);
    point.x = x;
    point.y = y;

    if (GetForegroundWindow() == m_pOuter->GetHwnd() && ClientToScreen(m_pOuter->GetHwnd(), &point))
        return SetCursorPos(point.x, point.y);
    return FALSE;
    END_DEBUG;
}

///////////////////////////////////////

BOOL cWinInputDevices::DispatchMouseEvent(UINT msg, WPARAM wParam, LPARAM lParam)
{
    BEGIN_DEBUG_MSG_EX(MOUSE, "Posting mouse event");
    lgMouseEvent mouseEvent;
    if (WinMouseToLGMouse(msg, wParam, lParam, mouseEvent))
    {
        // Dispatch event...
        CONNECTION_POINT_ITERATE()
        {
            if (pSink->OnMouse(&mouseEvent) != NOERROR)
                break;
        }
        return TRUE;
    }
    return FALSE;
    END_DEBUG;
}

///////////////////////////////////////
//
// Function to convert Windows message to Looking Glass keyboard events
//

// Simple crackers to easily interpret the window message
inline BOOL WasKeyDown(LPARAM lParam)
{
    return (lParam & 0x40000000);
}

inline BOOL IsExtendedKey(LPARAM lParam)
{
    return (lParam & 0x01000000);
}

inline uchar GetLGKeyState(LPARAM lParam)
{
    return (lParam & 0x80000000) ? KBS_UP : KBS_DOWN;
}

inline uchar GetKeyRepeatCount(LPARAM lParam)
{
    return (uchar)(lParam & 0x0000FFFF);
}

inline BOOL IsAltDown(LPARAM lParam)
{
    return (bool)(lParam & 0x20000000);
}

inline uchar GetKeyScanCode(LPARAM lParam)
{
    return ((uchar)((lParam & 0x00FF0000) >> 16));
}

// Cracking function
BOOL cWinInputDevices::WinKeyToLGKey(UINT msg, WPARAM /*wParam*/, LPARAM lParam, sInpKeyEvent & Result)
{
    BOOL bKeyIsRepeating;
    switch (msg)
    {
        // These are messages we're interested in
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
            bKeyIsRepeating = WasKeyDown(lParam);
            break;

        case WM_SYSKEYUP:
        case WM_KEYUP:
            bKeyIsRepeating = FALSE;
            break;

        // All others are not interesting (e.g., WM_CHAR)
        default:
            return FALSE;
    }

    // Translate scan code to the asctab format (i.e., winscancode_to_lgscancode())
    Result.code = GetKeyScanCode(lParam);
    // if running on a NEC98 we remap the key to what a Plain Old 101 
    // keyboard would have produced. 

    if (m_fNEC98KeyBoard) 
    {
        uchar tempCode = Result.code;
        Result.code=(uchar) nec2pc[Result.code];
    }

    if (IsAltDown(lParam))
        Result.code |= KBC_SHIFT_PREFIX;
    if (IsExtendedKey(lParam))
        Result.code |= 0x80;   // Set the extended bit as the DOS input library likes it
    Result.state = GetLGKeyState(lParam);

    return TRUE;
}

///////////////////////////////////////
//
// Function to convert Windows message to mouse events. Does no x/y adjustment
//

BOOL cWinInputDevices::WinMouseToLGMouse(UINT msg, WPARAM /*wParam*/, LPARAM lParam, sInpMouseEvent & Result)
{
    memset(&Result, 0, sizeof(sInpMouseEvent));

    static uchar uMouseFlags[] =
    {
        MOUSE_MOTION,                  // WM_MOUSEMOVE/WM_MOUSEFIRST
        MOUSE_LDOWN,                   // WM_LBUTTONDOWN
        MOUSE_LUP,                     // WM_LBUTTONUP
        0,                             // WM_LBUTTONDBLCLK (we shouldn't be
                                       // getting these because the game
                                       // wndclass.style should not have
                                       // the CS_DBLCLK flag set)
        MOUSE_RDOWN,                   // WM_RBUTTONDOWN
        MOUSE_RUP,                     // WM_RBUTTONUP
        0,
        MOUSE_CDOWN,                   // WM_MBUTTONDOWN
        MOUSE_CUP,                     // WM_MBUTTONUP
        0                              // WM_MBUTTONDBLCLK/WM_MOUSELAST
    };

    // Scale mouse coordinates and make the lgMouseEvent
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    m_pOuter->ClientToGamePoint(x, y);

    Result.x = (short) x;
    Result.y = (short) y;
    Result.type = uMouseFlags[msg - WM_MOUSEFIRST];
    Result.timestamp = m_pOuter->WindowsTimeToGameTime(GetMessageTime());

    // @TBD: Is this field actually used as-is? If so, this faking is not entirely correct
    Result.buttons = 0;

    switch(msg)
    {
        case WM_LBUTTONDOWN:
            Result.buttons = MouseMakeButtons(1, 0, 0);
            break;

        case WM_MBUTTONDOWN:
            Result.buttons = MouseMakeButtons(0, 1, 0);
            break;

        case WM_RBUTTONDOWN:
            Result.buttons = MouseMakeButtons(0, 0, 1);
            break;
    }
    return TRUE;
}

///////////////////////////////////////
