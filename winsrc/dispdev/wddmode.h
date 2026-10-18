///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/wddmode.h $
// $Author: KEVIN $
// $Date: 1997/10/21 16:18:06 $
// $Revision: 1.8 $
//

#ifndef __WDDMODE_H
#define __WDDMODE_H

class cD3D11Presenter;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDDModeOpsBase
//
// This intermediate base class encapsulates the common
// elements of the "real" DirectDraw modes and the
// "spoofed" modes.  It contains the needed pointers
// to the DirectDraw interfaces, but no specific
// knowledge about how they are used.
//

class cDDModeOpsBase : public cWinDisplayModeOperations
{
public:
    cDDModeOpsBase(cWinDisplayDevice * pDisplayDevice, cDDProvider * pOuter, LPDIRECTDRAWPALETTE pPalette)
      : m_pOuter(pOuter),
        m_pPrimarySurface(0),
        m_pSecondarySurface(0),
        m_pPalette(pPalette),
        m_Flags(0),
        cWinDisplayModeOperations(pDisplayDevice)

    {
        if (m_pPalette)
            m_pPalette->AddRef();
    }
    virtual ~cDDModeOpsBase();

    ///////////////////////////////

    virtual BOOL StartMode(const sGrModeInfo &, int flags, sGrModeCap * pReturnModeInfo) = 0;
    virtual BOOL DoSetPalette(LOGPALETTE & palette, unsigned uStart, unsigned n);

    ///////////////////////////////

    void AttachPalette(LPDIRECTDRAWPALETTE pNewPalette)
    {
        if (m_pPalette)
            m_pPalette->Release();
        if (pNewPalette)
            pNewPalette->AddRef();
        m_pPalette = pNewPalette;
    }

    ///////////////////////////////

    LPDIRECTDRAWPALETTE GetPalette()
    {
        if (m_pPalette)
            m_pPalette->AddRef();
        return m_pPalette;
    }

    ///////////////////////////////
    
    void GetSurfaces(IDirectDrawSurface ** ppPrimarySurface,
                     IDirectDrawSurface ** ppSecondarySurface);

    ///////////////////////////////
    void RestoreSurfaces()
    {
        if (m_pPrimarySurface->IsLost())
            m_pPrimarySurface->Restore();
        if (m_pSecondarySurface && m_pSecondarySurface->IsLost())
            m_pSecondarySurface->Restore();
    }

    ///////////////////////////////

    void SetClipToWindow(BOOL fWantClipping);

protected:
    ///////////////////////////////
    // Utility functions to simplify access to outer object
    DWORD GetCoopFlags()
    {
        return m_pOuter->m_DDCoopFlags;
    }

    LPDIRECTDRAW GetDD()
    {
        return m_pOuter->m_pDD;
    }

    HRESULT SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP)
    {
        HRESULT result = m_pOuter->m_pDD->SetDisplayMode(dwWidth, dwHeight, dwBPP);
        YieldDisplay(m_pDisplayDevice);

        m_pOuter->SetCooperativeLevel(cDDProvider::kDDWindowedCoopFlags);
        SetWindowPos(m_pOuter->GetMainWnd(),
                     HWND_TOPMOST,
                     0, 0,
                     dwWidth, dwHeight,
                     SWP_SHOWWINDOW	| SWP_NOCOPYBITS);
        m_pOuter->SetCooperativeLevel(cDDProvider::kDDFullScreenCoopFlags);

        RegainDisplay(m_pDisplayDevice);
        return result;
    }

    void EnableDesktopPresentation()
    {
        HWND hwnd = m_pOuter->GetMainWnd();
        MONITORINFO monitorInfo;
        memset(&monitorInfo, 0, sizeof(monitorInfo));
        monitorInfo.cbSize = sizeof(monitorInfo);

        m_pOuter->m_pDD->RestoreDisplayMode();
        m_pOuter->SetCooperativeLevel(cDDProvider::kDDWindowedCoopFlags);
        m_pOuter->m_DDCoopFlags = cDDProvider::kDDWindowedCoopFlags;

        SetWindowLong(hwnd, GWL_STYLE, WS_POPUP);
        if (GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST),
                           &monitorInfo))
        {
            SetWindowPos(hwnd, HWND_NOTOPMOST,
                         monitorInfo.rcMonitor.left,
                         monitorInfo.rcMonitor.top,
                         monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                         monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                         SWP_FRAMECHANGED | SWP_NOCOPYBITS |
                         SWP_NOACTIVATE);
        }
    }

    PALETTEENTRY * GetPaletteEntries()
    {
        return m_pDisplayDevice->GetPaletteEntries();
    }

    void SetModeInfoFromSurfaceDesc(const DDSURFACEDESC &, int flags, sGrModeCap * pReturnModeInfo);

    BOOL IsModeX()
    {
        return (m_PrimaryDesc.dwWidth < 400);
    }

    BOOL WipeSurface(IDirectDrawSurface *pdds);

    ///////////////////////////////

    cDDProvider * m_pOuter;

    LPDIRECTDRAWSURFACE m_pPrimarySurface;  // DirectDraw primary surface
    LPDIRECTDRAWSURFACE m_pSecondarySurface;     // DirectDraw back surface
    LPDIRECTDRAWPALETTE m_pPalette;       // DirectDraw palette
    DDSURFACEDESC       m_PrimaryDesc;
    DDSURFACEDESC       m_SecondaryDesc;
    int                 m_Flags;

    enum eFlags
    {
        kClippingToWindow = 0x01
    };
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDirectDDModeOps
//
// This class implements the direct DirectDraw operations, where the locked
// pointer points to the visible screen
//

class cDirectDDModeOps : public cDDModeOpsBase
{
public:
    cDirectDDModeOps(cWinDisplayDevice * pDisplayDevice, cDDProvider * pOuter, LPDIRECTDRAWPALETTE pPalette)
      : cDDModeOpsBase(pDisplayDevice, pOuter, pPalette),
        m_pPauseSurface(NULL),
        m_pPauseSurface2(NULL)
    {
    }

    virtual ~cDirectDDModeOps();
    virtual BOOL StartMode(const sGrModeInfo &, int flags, sGrModeCap * pReturnModeInfo);
    virtual BOOL OnTaskSwitch(BOOL bActive);
    virtual BOOL DoStatVTrace();
    virtual BOOL DoPageFlip();
    virtual BOOL DoLock(sGrModeCap *);
    virtual BOOL DoUnlock();

private:
    // Jacobson, 3-4-96
    // These objects here are used to store off the contents of the
    // screen when the pause key is hit.
    LPDIRECTDRAWSURFACE m_pPauseSurface;  // DirectDraw temp surface
    LPDIRECTDRAWSURFACE m_pPauseSurface2; // DirectDraw temp surface
    DDSURFACEDESC       m_PauseDesc;
    void                PauseBlit(LPDIRECTDRAWSURFACE pDest,LPDIRECTDRAWSURFACE pSource);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhoney512ModeOps
//
// This class implements the faked 512x384 for Dark.
// @TBD (toml 12-09-96): This must be generalized for all faked "subregion" modes
//

class cPhoney512ModeOps : public cDirectDDModeOps
{
public:
    cPhoney512ModeOps(cWinDisplayDevice * pDisplayDevice, cDDProvider * pOuter, LPDIRECTDRAWPALETTE pPalette)
      : cDirectDDModeOps(pDisplayDevice, pOuter, pPalette)
    {
    }

    virtual BOOL StartMode(const sGrModeInfo &, int flags, sGrModeCap * pReturnModeInfo);
    virtual BOOL DoLock(sGrModeCap *);
    virtual BOOL DoUnlock();
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cOffVideoDDModeOps
//
// This class implements the offscreen DirectDraw operations, where the locked
// pointer is a DirectDraw surface in system memory
//

class cOffVideoDDModeOps : public cDDModeOpsBase
{
public:
    cOffVideoDDModeOps(cWinDisplayDevice * pDisplayDevice, cDDProvider * pOuter, LPDIRECTDRAWPALETTE pPalette)
      : cDDModeOpsBase(pDisplayDevice, pOuter, pPalette)
      , m_UseDesktopPresentation(FALSE)
      , m_UseD3D11Presentation(FALSE)
      , m_pPresenter(NULL)
      , m_Gamma(1.0)
    {
    }

    virtual ~cOffVideoDDModeOps();
    virtual BOOL StartMode(const sGrModeInfo &, int flags, sGrModeCap * pReturnModeInfo);
    virtual void DoFlush();
    virtual void DoFlushRect(int x0, int y0, int x1, int y1);
    virtual BOOL DoLock(sGrModeCap *);
    virtual BOOL DoUnlock();
    virtual BOOL DoSetGamma(double gamma);
    virtual BOOL HandlesWindowResize() const
    {
        return m_UseD3D11Presentation;
    }

private:
    BOOL m_UseDesktopPresentation;
    BOOL m_UseD3D11Presentation;
    cD3D11Presenter *m_pPresenter;
    double m_Gamma;
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__WDDMODE_H */
