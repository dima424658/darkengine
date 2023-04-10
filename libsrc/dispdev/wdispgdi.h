///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/wdispgdi.h $
// $Author: TOML $
// $Date: 1996/11/21 10:36:59 $
// $Revision: 1.7 $
//
// The Provider and Mode Operations for GDI.  These are really two inseperable
// halves of the same object.
//

#ifndef __WDISPGDI_H
#define __WDISPGDI_H

class cGDIModeOps;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cGDIProvider
//
// Implements GDI based version of modeless display device operations
//

class cGDIProvider : public cWinDisplayProvider
{
public:
    cGDIProvider(cWinDisplayDevice * pOwner);
    virtual ~cGDIProvider();

private:

    ///////////////////////////////////
    //
    // Hooks from cWinDisplayProvider
    //
    virtual void DoGetInfo(sGrDeviceInfo *, sGrModeInfo *);
    virtual BOOL DoOpen(sGrModeCap *, int flags);
    virtual BOOL DoClose();
    virtual cWinDisplayModeOperations * DoSetMode(const sGrModeInfo &, int flags, sGrModeCap * pReturnModeInfo);

    void AdjustWindow(BOOL fToMode = FALSE);

    ///////////////

    virtual BOOL DoProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam, long * pRetVal);

    friend class cGDIModeOps;

    ///////////////

    void PickPalette(BOOL fActive = TRUE);
    void ClearSystemPalette();
    void SetSystemPaletteEntries(BOOL);

    ///////////////////////////////////

    cGDIModeOps * const m_pGDIModeOps;

    int m_fUsingStaticColors;
    int m_fStrictPalette;

    ///////////////////////////////////

    // Palette used to help Windows show reasonable 16-bit with 8-bit screen driver
    HPALETTE m_h16to8RemapPalette;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cGDIModeOps
//
// Implements GDI based version of mode-based display device operations
//

class cGDIModeOps : public cWinDisplayModeOperations
{
private:
    cGDIModeOps(cWinDisplayDevice * pDisplayDevice, cGDIProvider * pOwner)
      : cWinDisplayModeOperations(pDisplayDevice),
        m_pOwner(pOwner),
        m_hOffscreenDC(0),
        m_hOffscreenBitmap(0),
        m_hOldBitmap(0),
        m_hStrict8BitPalette(0),
        m_hMappable8BitPalette(0),
        m_hCurrentPalette(0)
    {
    }

    virtual ~cGDIModeOps();

    BOOL StartMode(const sGrModeInfo &, int flags, sGrModeCap * pReturnModeInfo);

    //
    // Implementations of operations
    //
    virtual BOOL DoSetPalette(LOGPALETTE & palette, unsigned uStart, unsigned n);
    virtual BOOL DoLock(sGrModeCap *);
    virtual BOOL DoUnlock();
    virtual void DoFlush();
    virtual void DoFlushRect(int x0, int y0, int x1, int y1);

    void UpdateDisplay();

    BITMAPINFOHEADER    m_OffscreenInfo;
    HDC                 m_hOffscreenDC;
    HBITMAP             m_hOffscreenBitmap;
    HBITMAP             m_hOldBitmap;

    HPALETTE m_hStrict8BitPalette;
    HPALETTE m_hMappable8BitPalette;
    HPALETTE m_hCurrentPalette;

    cGDIProvider * const m_pOwner;
    friend class cGDIProvider;
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__WDISPGDI_H */
