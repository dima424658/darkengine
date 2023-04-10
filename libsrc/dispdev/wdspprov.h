///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/wdspprov.h $
// $Author: TOML $
// $Date: 1997/05/23 14:17:36 $
// $Revision: 1.6 $
//

#ifndef __WDSPPROV_H
#define __WDSPPROV_H

F_DECLARE_INTERFACE(IDirectDraw);

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cWinDisplayProvider
//
// Implements device-level operations for a given implementation of the display
// device.
//
// cWinDisplayDevice calls instances of this class to perform device provider
// specific operations.  Base class contains core information and implementation
// tools.
//
// The derived class must implement:
//    GetInfo()
//    DoInit()
//    DoSetMode()
//    DoClose()
//

class cWinDisplayProvider
{
public:
    cWinDisplayProvider(cWinDisplayDevice * pDisplayDevice);
    virtual ~cWinDisplayProvider();

    // Handle a message from IGameShell instance
    virtual BOOL DoProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam, long * pRetVal);

    virtual void DoGetInfo(sGrDeviceInfo *, sGrModeInfo *) = 0;
    virtual BOOL DoOpen(sGrModeCap *, int fFlags) = 0;
    virtual BOOL DoClose() = 0;
    virtual cWinDisplayModeOperations * DoSetMode(const sGrModeInfo &,
                                                  int flags,
                                                  sGrModeCap * pReturnModeInfo) = 0;

    virtual BOOL DoGetDirectDraw(IDirectDraw **);
    virtual BOOL DoGetBitmapSurface(sGrBitmap *, IDirectDrawSurface **);
    
    HWND GetMainWnd();

protected:
    cWinDisplayDevice * const m_pDisplayDevice;

    void ShowMainWnd();
    void HideMainWnd();

private:
    AutoMemberIPtr(WinApp);
};

///////////////////////////////////////

inline void cWinDisplayProvider::ShowMainWnd()
{
    int iPreviousMutexLevel = m_pDisplayDevice->BreakMutex();
    ShowWindow(GetMainWnd(), SW_SHOW);
    SetForegroundWindow(GetMainWnd());
    m_pDisplayDevice->RestoreMutex(iPreviousMutexLevel);
}

///////////////////////////////////////

inline void cWinDisplayProvider::HideMainWnd()
{
    int iPreviousMutexLevel = m_pDisplayDevice->BreakMutex();
    ShowWindow(GetMainWnd(), SW_HIDE);
    m_pDisplayDevice->RestoreMutex(iPreviousMutexLevel);
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__WDSPPROV_H */
