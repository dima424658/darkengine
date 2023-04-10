///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/wdispgdi.cpp $
// $Author: TOML $
// $Date: 1997/09/10 14:00:39 $
// $Revision: 1.25 $
//

//
// Core headers
//
#include <windows.h>
#include <lg.h>

//
// Component APIs
//
#include <comtools.h>
#include <dispapi.h>

//
// Application aggregate
//
#include <appagg.h>


//
// Other required LG libraries
//
#include <dev2d.h>

//
// Winows display implementation headers
//
#include <wdisp.h>
#include <wdspprov.h>
#include <wdspmode.h>
#include <wdisptls.h>
#include <wdspgshl.h>

#include <wdispgdi.h>

///////////////////////////////////////////////////////////////////////////////

cGDIProvider::cGDIProvider(cWinDisplayDevice * pDisplayDevice)
  : cWinDisplayProvider(pDisplayDevice),
    m_pGDIModeOps(new cGDIModeOps(pDisplayDevice, this)),
    m_fUsingStaticColors(0),
    m_h16to8RemapPalette(0)
{
    m_fStrictPalette = GetPrivateProfileInt("WinShell", "StrictGDIPalette", FALSE, "lg.ini");

    // Create palette used to help Windows translate 16-bit source
    // to 8-bit screen driver, if needed
    HDC hScreenDC = GetDC(NULL);
    int iCurrentDisplayDepth = GetDeviceCaps(hScreenDC, BITSPIXEL);
    ReleaseDC(NULL, hScreenDC);

    // If Windows is in an 8-bit mode, make a 16-to-8 translation palette (gray scale right now)
    if (iCurrentDisplayDepth == 8)
    {
        cWinDisplayDevice::sPalette256 palette;

        palette.palVersion = 0x300;
        palette.palNumEntries = 256;
        int r = 0;
        int g = 0;
        int b = 0;
        for (int i = 0; i < 256; i++)
        {
            palette.palPalEntry[i].peRed = (BYTE) i;
            palette.palPalEntry[i].peGreen = (BYTE) i;
            palette.palPalEntry[i].peBlue = (BYTE) i;
            palette.palPalEntry[i].peFlags = 0;
        }

        m_h16to8RemapPalette = CreatePalette((LOGPALETTE *) & palette);
    }
}

///////////////////////////////////////

cGDIProvider::~cGDIProvider()
{
    // Ensure palette restored
    if (m_fStrictPalette)
        SetSystemPaletteEntries(TRUE);

    if (m_h16to8RemapPalette)
        DeleteObject(m_h16to8RemapPalette);

    delete m_pGDIModeOps;
}

///////////////////////////////////////

void cGDIProvider::DoGetInfo(sGrDeviceInfo * pGrDeviceInfo, sGrModeInfo * pModeInfo)
{
    memset(pGrDeviceInfo, 0, sizeof(sGrDeviceInfo));

    // Claim support for all
    for (short i = 0; i < GRD_MODES; i++)
    {
        pGrDeviceInfo->modes[i] = i;
    }
    pGrDeviceInfo->modes[GRD_MODES] = -1;

    // Claim fullscreen for all
    short * pModes = pGrDeviceInfo->modes;
    while (*pModes != -1)
    {
        pModeInfo[*pModes].flags |= kGrModeCanFullscreen;
        pModes++;
    }

    // Claim windowed for all
    pModes = pGrDeviceInfo->modes;
    while (*pModes != -1)
    {
        pModeInfo[*pModes].flags |= kGrModeCanWindow;
        pModes++;
    }

    // Claim native for all
    pModes = pGrDeviceInfo->modes;
    while (*pModes != -1)
    {
        pModeInfo[*pModes].flags |= kGrModeIsNative;
        pModes++;
    }
}

///////////////////////////////////////

BOOL cGDIProvider::DoOpen(sGrModeCap *, int /*flags*/)
{
    if (m_fStrictPalette)
        ClearSystemPalette();

    return TRUE;
}

///////////////////////////////////////

BOOL cGDIProvider::DoClose()
{
    HideMainWnd();

    if (m_pGDIModeOps->m_hOffscreenDC)
    {
        // clean up GDI objects
        SelectObject(m_pGDIModeOps->m_hOffscreenDC, m_pGDIModeOps->m_hOldBitmap);
        DeleteObject(m_pGDIModeOps->m_hOffscreenBitmap);
        DeleteDC(m_pGDIModeOps->m_hOffscreenDC);
    }

    DeleteObject(m_pGDIModeOps->m_hStrict8BitPalette);
    DeleteObject(m_pGDIModeOps->m_hMappable8BitPalette);

    return TRUE;
}

///////////////////////////////////////

cWinDisplayModeOperations *
cGDIProvider::DoSetMode(const sGrModeInfo & modeInfo,
                              int flags,
                              sGrModeCap * pReturnModeInfo)
{
    // Pick best palette for mode
    PickPalette();

    // Start the mode
    if (!m_pGDIModeOps->StartMode(modeInfo, flags, pReturnModeInfo))
        return NULL;

    int iPreviousLock       = m_pDisplayDevice->BreakLock();
    int iPreviousMutexLevel = m_pDisplayDevice->BreakMutex();

    // Size window for optimal blit speed, good positioning
    HideMainWnd();
    AdjustWindow(TRUE);

    // Show the window
    InvalidateRect(GetMainWnd(), NULL, TRUE);
    ShowMainWnd();
    WinDispBringToFront();

    // Flush the queue
    WinDispPumpEvents();
    
    m_pDisplayDevice->RestoreMutex(iPreviousMutexLevel);
    m_pDisplayDevice->RestoreLock(iPreviousLock);

    return m_pGDIModeOps;
}

///////////////////////////////////////

void cGDIProvider::AdjustWindow(BOOL fToMode)
{
    m_pGDIModeOps->m_ScaleFactor = ::AdjustWindow(GetMainWnd(),
                                                  m_pGDIModeOps->m_OffscreenInfo.biWidth,
                                                  ((m_pGDIModeOps->m_OffscreenInfo.biHeight > 0) ? m_pGDIModeOps->m_OffscreenInfo.biHeight : -m_pGDIModeOps->m_OffscreenInfo.biHeight),
                                                  fToMode);

}

///////////////////////////////////////

BOOL cGDIProvider::DoProcessMessage(UINT msg, WPARAM wParam, LPARAM /*lParam*/, long * pRetVal)
{
    // If we haven't been initialized...
    if (!m_pDisplayDevice->IsModeSet())
        // .. then always do default processing
        return FALSE;

    switch (msg)
    {
        case WM_ACTIVATEAPP:
        {
            // only do this for the currently visible window
            if (m_fStrictPalette)
            {
                SetSystemPaletteEntries(!wParam); // wParam is true if we're coming forward
                PickPalette(wParam);
                InvalidateRect(GetMainWnd(), 0, TRUE);
            }
            break;
        }

        case WM_SIZE:
            AdjustWindow(TRUE);
            break;

        case WM_DESTROY:
            if (m_fStrictPalette)
                SetSystemPaletteEntries(TRUE);
            break;

        case WM_PALETTECHANGED:
            if ((HWND) wParam == GetMainWnd() || GetParent((HWND) wParam) == GetMainWnd())
                break;

            // fall through to WM_QUERYNEWPALETTE

        case WM_QUERYNEWPALETTE:
        {
            if (m_pGDIModeOps->m_hCurrentPalette)
            {
                int f;
                HDC hdc = GetDC(GetMainWnd());
                SelectPalette(hdc, m_pGDIModeOps->m_hCurrentPalette, FALSE);

                f = RealizePalette(hdc);
                ReleaseDC(GetMainWnd(), hdc);
                InvalidateRect(GetMainWnd(), 0, TRUE);

                *pRetVal = TRUE;
                return TRUE;
            }

            break;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(GetMainWnd(), &ps);
            if (m_pGDIModeOps->m_hCurrentPalette)
            {
                SelectPalette(hdc, m_pGDIModeOps->m_hCurrentPalette, FALSE);
                RealizePalette(hdc);
            }
            if (m_pDisplayDevice->IsFlushingPaused())
            {
                FillRect(hdc, &ps.rcPaint, (HBRUSH) GetStockObject(BLACK_BRUSH));
            }
            else
            {
                m_pGDIModeOps->UpdateDisplay();
            }
            EndPaint(GetMainWnd(), &ps);
            *pRetVal = 0;
            return TRUE;
        }
    }
    return FALSE;
}

///////////////////////////////////////

void cGDIProvider::PickPalette(BOOL fActive)
{
    // Select proper palette for mode
    if (m_pDisplayDevice->GetCurrentModeInfo().bitDepth == 8)
        m_pGDIModeOps->m_hCurrentPalette = (m_fStrictPalette && fActive) ? m_pGDIModeOps->m_hStrict8BitPalette : m_pGDIModeOps->m_hMappable8BitPalette;
    else
    {
        m_pGDIModeOps->m_hCurrentPalette = m_h16to8RemapPalette;
    }
}

///////////////////////////////////////

void cGDIProvider::ClearSystemPalette()
{
    cWinDisplayDevice::sPalette256 Palette;

    // Watcom doesn't like an initialization expression here.

    Palette.palVersion = 0x300;
    Palette.palNumEntries = 256;

    // Reset everything in the system palette to black
    for (int Counter = 0; Counter < 256; Counter++)
    {
        Palette.palPalEntry[Counter].peRed = 0;
        Palette.palPalEntry[Counter].peGreen = 0;
        Palette.palPalEntry[Counter].peBlue = 0;
        Palette.palPalEntry[Counter].peFlags = PC_NOCOLLAPSE;
    }

    // Create, select, realize, deselect, and delete the palette
    HPALETTE hScreenPalette = CreatePalette((LOGPALETTE *) & Palette);

    HDC hDC = GetDC(NULL);
    AssertMsg(hDC, "Failed to get screen DC");
    if (hScreenPalette)
    {
        HPALETTE hPalPrev = SelectPalette(hDC, hScreenPalette, FALSE);
        RealizePalette(hDC);
        hScreenPalette = SelectPalette(hDC, hPalPrev, FALSE);
        DeleteObject(hScreenPalette);
    }
    ReleaseDC(NULL, hDC);
}

///////////////////////////////////////

#define NumSysColors (sizeof(SysPalIndex)/sizeof(SysPalIndex[1]))
#define rgbBlack RGB(0,0,0)
#define rgbWhite RGB(255,255,255)

///////////////////////////////////////

void cGDIProvider::SetSystemPaletteEntries(BOOL fEnable)
{
    if (!m_fStrictPalette)
        return;

    // These are the GetSysColor display element identifiers
    static int SysPalIndex[] = {
        COLOR_ACTIVEBORDER,
        COLOR_ACTIVECAPTION,
        COLOR_APPWORKSPACE,
        COLOR_BACKGROUND,
        COLOR_BTNFACE,
        COLOR_BTNSHADOW,
        COLOR_BTNTEXT,
        COLOR_CAPTIONTEXT,
        COLOR_GRAYTEXT,
        COLOR_HIGHLIGHT,
        COLOR_HIGHLIGHTTEXT,
        COLOR_INACTIVEBORDER,
        COLOR_INACTIVECAPTION,
        COLOR_MENU,
        COLOR_MENUTEXT,
        COLOR_SCROLLBAR,
        COLOR_WINDOW,
        COLOR_WINDOWFRAME,
        COLOR_WINDOWTEXT
    };
    //*** This array translates the display elements to black and white
    static COLORREF MonoColors[] = {
        rgbBlack,
        rgbWhite,
        rgbWhite,
        rgbWhite,
        rgbWhite,
        rgbBlack,
        rgbBlack,
        rgbBlack,
        rgbBlack,
        rgbBlack,
        rgbWhite,
        rgbWhite,
        rgbWhite,
        rgbWhite,
        rgbBlack,
        rgbWhite,
        rgbWhite,
        rgbBlack,
        rgbBlack
    };

    static COLORREF OldColors[NumSysColors];

    HDC hdc = GetDC(0);
    if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
    {
        static BOOL bSavedInitialColors = FALSE;

        if (!bSavedInitialColors)
        {
            // Store them
            for (int i = 0; i < NumSysColors; i++)
                OldColors[i] = GetSysColor(SysPalIndex[i]);
            bSavedInitialColors = TRUE;
        }

        if (fEnable)
        {
            if (GetSystemPaletteUse(hdc) != SYSPAL_STATIC)
            {
                SetSystemPaletteUse(hdc, SYSPAL_STATIC);
                SetSysColors(NumSysColors, SysPalIndex, OldColors);
            }
        }
        else
        {
            if (GetSystemPaletteUse(hdc) != SYSPAL_NOSTATIC)
            {
                SetSystemPaletteUse(hdc, SYSPAL_NOSTATIC);
                SetSysColors(NumSysColors, SysPalIndex, MonoColors);
            }
        }
    }

    ReleaseDC(0, hdc);
}

///////////////////////////////////////////////////////////////////////////////

cGDIModeOps::~cGDIModeOps()
{
}

///////////////////////////////////////

BOOL cGDIModeOps::StartMode(const sGrModeInfo & modeInfo,
                                                 int /*flags*/,
                                                 sGrModeCap * pReturnModeInfo)
{
    if (m_hOffscreenDC)
    {
        // clean up GDI objects
        SelectObject(m_hOffscreenDC, m_hOldBitmap);
        DeleteObject(m_hOffscreenBitmap);
        DeleteDC(m_hOffscreenDC);
        m_hOffscreenDC = 0;
        m_hOffscreenBitmap = 0;
    }

    sGdiBitmapInfo bitmapInfo(modeInfo.w, modeInfo.h, modeInfo.bitDepth);

    m_hOffscreenBitmap = m_pDisplayDevice->CreateGDIBitmap(&bitmapInfo);
    m_hOffscreenDC     = CreateCompatibleDC(NULL);

    memcpy(&m_OffscreenInfo, &bitmapInfo.bmiHeader, sizeof(BITMAPINFOHEADER));

    memset(&m_ModeInfoEx, 0, sizeof(m_ModeInfoEx));

    if (modeInfo.bitDepth == 16 || modeInfo.bitDepth == 15)
    {
        DWORD * pMasks = (DWORD *) bitmapInfo.bmiColors;

        m_ModeInfoEx.depth     = 16;
        m_ModeInfoEx.redMask   = pMasks[0];
        m_ModeInfoEx.greenMask = pMasks[1];
        m_ModeInfoEx.blueMask  = pMasks[2];
    }
    else
    {
        m_ModeInfoEx.depth     = modeInfo.bitDepth;
        m_ModeInfoEx.redMask   = m_ModeInfoEx.greenMask = m_ModeInfoEx.blueMask = 0;
    }

    // Make sure that the bitmap and context was created properly.
    AssertMsg(m_hOffscreenDC && m_hOffscreenBitmap, "Failed to set mode");

    if (!m_hOffscreenDC || !m_hOffscreenBitmap)
    {
        DebugMsg("Failed to create offscreen bitmap");
        return FALSE;
    }

    // Jacobson, 1/25/96.
    // Get a descriptor describing the bitmap. We'll use it to fill in
    // the fields of the mode descriptor.

    BITMAP bmInfo;
    GetObject(m_hOffscreenBitmap, sizeof(BITMAP), &bmInfo);
    m_ModeInfoEx.width     = pReturnModeInfo->w        = (short)   bmInfo.bmWidth;
    m_ModeInfoEx.height    = pReturnModeInfo->h        = (short)   bmInfo.bmHeight;
    m_ModeInfoEx.rowBytes  = pReturnModeInfo->rowBytes =           bmInfo.bmWidthBytes;
                             pReturnModeInfo->vbase    = (uchar *) bmInfo.bmBits;
                             pReturnModeInfo->vbase2   =           NULL;

    m_ModeInfoEx.mode      = modeInfo.mode_2d;

    m_ModeInfoEx.flags     = kGrModeIsWindowed;

    // If we're down-filtering when we go to the screen, we lie about whether we're direct since blits from the real screen will be grey scale!
    HDC hScreenDC = GetDC(NULL);
    if (GetDeviceCaps(hScreenDC, BITSPIXEL) < modeInfo.bitDepth)
        m_ModeInfoEx.flags |= kGrModeIsDirect;
    ReleaseDC(NULL, hScreenDC);

    m_hOldBitmap = (HBITMAP) SelectObject(m_hOffscreenDC, m_hOffscreenBitmap);

    return TRUE;
}

///////////////////////////////////////

BOOL cGDIModeOps::DoSetPalette(LOGPALETTE & palette, unsigned /*uStart*/, unsigned /*n*/)
{
    int i;

    // If we previously created a set of palettes...
    if (m_hStrict8BitPalette)
    {
        // ... then destroy them
        DeleteObject(m_hStrict8BitPalette);
        DeleteObject(m_hMappable8BitPalette);
        m_hCurrentPalette = m_hStrict8BitPalette = m_hMappable8BitPalette = NULL;
    }

    // Get a version that SetDIBColorTable() likes
    static RGBQUAD colors[256];

    for (i = 0; i < 256; i++)
    {
        colors[i].rgbRed = palette.palPalEntry[i].peRed;
        colors[i].rgbGreen = palette.palPalEntry[i].peGreen;
        colors[i].rgbBlue = palette.palPalEntry[i].peBlue;
        colors[i].rgbReserved = 0;
    }

    // Palette entry zero is always black.  Change flag so Windows will allow
    // mapping of background tasks into this color
    palette.palPalEntry[0].peFlags = 0;

    // Now find a closest match to white...
    PALETTEENTRY * pClosestToWhite = &palette.palPalEntry[0];
    int iClosestIntensity = 0;
    int intensity;

    for (i = 0; i < 256; i++)
    {
        intensity = palette.palPalEntry[i].peRed +
                    palette.palPalEntry[i].peGreen +
                    palette.palPalEntry[i].peBlue;
        if (intensity > iClosestIntensity)
        {
            pClosestToWhite =  &palette.palPalEntry[i];
            iClosestIntensity = intensity;
        }
    }
    pClosestToWhite->peFlags = 0;

    // Create the foreground palette
    m_hStrict8BitPalette = CreatePalette(&palette);

    // Create the background palette.
    // Clear out the "PC_RESERVED" flag so Windows will do a best-match palette
    // when the game is in the background
    for (i = 0; i < 256; i++)
        palette.palPalEntry[i].peFlags = 0;

    // Set initial palette to the background one
    int iPreviousLock       = m_pDisplayDevice->BreakLock();
    int iPreviousMutexLevel = m_pDisplayDevice->BreakMutex();

    m_hCurrentPalette = m_hMappable8BitPalette = CreatePalette(&palette);

    AssertMsg(m_hStrict8BitPalette, "Failed to create palette m_hStrict8BitPalette");
    AssertMsg(m_hMappable8BitPalette, "Failed to create palette m_hMappable8BitPalette");

    SetDIBColorTable(m_hOffscreenDC, 0, 256, colors);

    if (m_hCurrentPalette)
    {
        HDC hdc = GetDC(m_pOwner->GetMainWnd());
        SelectPalette(hdc, m_hCurrentPalette, FALSE);
        RealizePalette(hdc);
        ReleaseDC(m_pOwner->GetMainWnd(), hdc);
    }
    InvalidateRect(m_pOwner->GetMainWnd(), 0, TRUE);

    // Flush the queue
    WinDispPumpEvents();
    
    m_pDisplayDevice->RestoreMutex(iPreviousMutexLevel);
    m_pDisplayDevice->RestoreLock(iPreviousLock);

    return TRUE;
}

///////////////////////////////////////

BOOL cGDIModeOps::DoLock(sGrModeCap * pReturnModeInfo)
{
    BITMAP bmInfo;
    GetObject(m_hOffscreenBitmap, sizeof(BITMAP), &bmInfo);
    pReturnModeInfo->vbase    = (uchar *)bmInfo.bmBits;
    return !!pReturnModeInfo->vbase;
}

///////////////////////////////////////

BOOL cGDIModeOps::DoUnlock()
{
    return TRUE;
}

///////////////////////////////////////

void cGDIModeOps::DoFlush()
{
    int iPreviousLock       = m_pDisplayDevice->BreakLock();
    int iPreviousMutexLevel = m_pDisplayDevice->BreakMutex();

    UpdateDisplay();

    m_pDisplayDevice->RestoreMutex(iPreviousMutexLevel);
    m_pDisplayDevice->RestoreLock(iPreviousLock);
}

///////////////////////////////////////

void cGDIModeOps::DoFlushRect(int x0Source, int y0Source, int x1Source, int y1Source)
{
    // Adjust rectangle to look good when window is halved or doubled
    x0Source -= (x0Source & 1);
    y0Source -= (y0Source & 1);
    x1Source += (x1Source & 1);
    y1Source += (y1Source & 1);

    BITMAP bmInfo;
    GetObject(m_hOffscreenBitmap, sizeof(BITMAP), &bmInfo);

    // check if we're totally clipped
    if ((y0Source >= bmInfo.bmHeight) || (y1Source <= 0) ||
        (x0Source >= bmInfo.bmWidth) || (x1Source <= 0))
        return;

    int iPreviousLock       = m_pDisplayDevice->BreakLock();
    int iPreviousMutexLevel = m_pDisplayDevice->BreakMutex();

    HDC hdc = GetDC(m_pOwner->GetMainWnd());
    if (m_hCurrentPalette)
    {
        SelectPalette(hdc, m_hCurrentPalette, FALSE);
        RealizePalette(hdc);
    }

    if (x0Source < 0)
            x0Source = 0;
    if (y0Source < 0)
            y0Source = 0;

    if (y1Source > bmInfo.bmHeight)
            y1Source = bmInfo.bmHeight;
    if (x1Source > bmInfo.bmWidth)
            x1Source = bmInfo.bmWidth;

    int x0Dest = Scale(x0Source);
    int y0Dest = Scale(y0Source);
    int x1Dest = Scale(x1Source);
    int y1Dest = Scale(y1Source);

    StretchBlt(hdc,            x0Dest,   y0Dest,   x1Dest - x0Dest,     y1Dest - y0Dest,
               m_hOffscreenDC, x0Source, y0Source, x1Source - x0Source, y1Source - y0Source,
               SRCCOPY);

    ReleaseDC(m_pOwner->GetMainWnd(), hdc);
    m_pDisplayDevice->RestoreMutex(iPreviousMutexLevel);
    m_pDisplayDevice->RestoreLock(iPreviousLock);
}

///////////////////////////////////////

void cGDIModeOps::UpdateDisplay()
{
    HDC hdc = GetDC(m_pOwner->GetMainWnd());
    if (m_hCurrentPalette)
    {
        SelectPalette(hdc, m_hCurrentPalette, FALSE);
        RealizePalette(hdc);
    }

    int x1Source = m_OffscreenInfo.biWidth;
    int y1Source = ((m_OffscreenInfo.biHeight > 0) ? m_OffscreenInfo.biHeight : -m_OffscreenInfo.biHeight);

    int x1Dest = Scale(x1Source);
    int y1Dest = Scale(y1Source);

    StretchBlt(hdc,            0, 0, x1Dest,   y1Dest,
               m_hOffscreenDC, 0, 0, x1Source, y1Source,
               SRCCOPY);

    ReleaseDC(m_pOwner->GetMainWnd(), hdc);
}

///////////////////////////////////////////////////////////////////////////////
