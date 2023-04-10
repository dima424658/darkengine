///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/wdispdd.cpp $
// $Author: KEVIN $
// $Date: 1998/02/02 12:46:58 $
// $Revision: 1.49 $
//

// Core headers
#include <windows.h>
#include <lg.h>
#include <ddraw.h>

// Component APIs
#include <comtools.h>
#include <dispapi.h>
#include <gshelapi.h>

// Application aggregate
#include <appagg.h>

// Other required LG libraries
#include <dev2d.h>
#include <mprintf.h>

// Windows display implementation headers
#include <dddynf.h>
#include <wdisp.h>
#include <wdspprov.h>
#include <wdspmode.h>
#include <wdcreate.h>
#include <wdisptls.h>
#include <wdspgshl.h>

#include <wdispdd.h>
#include <wddmode.h>

///////////////////////////////////////////////////////////////////////////////

#ifndef DEBUG_SETMODE
#define DEBUG_SETMODE 0
#else
#pragma message ("DEBUG_SETMODE defined")
#endif

///////////////////////////////////////////////////////////////////////////////

// @TBD (toml 05-01-96): This could (should?) really be coupled with wdisp

class cDDCritMsgHandler
{
public:
    void Set(cDDProvider * pNewDisplayDevice)
    {
        if (pNewDisplayDevice && !pDisplayDevice)
            CritMsgSetHandler(CritMsgNotificationHandler);

        else if (!pNewDisplayDevice && pDisplayDevice)
            CritMsgSetHandler(NULL);

        pDisplayDevice = pNewDisplayDevice;
    }

private:
    static void LGAPI CritMsgNotificationHandler(enum eCritMsgNotification);
    static cDDProvider * pDisplayDevice;
    static int iLockCount;
    static BOOL fCantContinue;
};

///////////////////////////////////////

cDDProvider * cDDCritMsgHandler::pDisplayDevice;
int cDDCritMsgHandler::iLockCount;
BOOL cDDCritMsgHandler::fCantContinue;

///////////////////////////////////////

void LGAPI cDDCritMsgHandler::CritMsgNotificationHandler(enum eCritMsgNotification Notification)
{
    if (!pDisplayDevice)
        return;

    switch (Notification)
    {
        case kCritMsgEnter:
            iLockCount = pDisplayDevice->m_pDisplayDevice->BreakLock();
            pDisplayDevice->m_pDD->FlipToGDISurface();
            // @TBD (toml 05-16-96): when flippable, we'll have to flip to GDI here
            break;

        case kCritMsgExit:
            if (fCantContinue)
            {
// @TBD (toml 11-13-96): this used to be true, but it should be verified whether it still is...
                MessageBox(NULL, "Can't continue after debug break under DirectDraw", NULL, MB_OK);
                ExitProcess(1); // swoosh
            }
            pDisplayDevice->m_pDisplayDevice->RestoreLock(iLockCount);
            break;

        case kCritMsgIgnoring:
            break;

        case kCritMsgTerminating:
        case kCritMsgDebugging:
            fCantContinue = TRUE;
            pDisplayDevice->m_pDD->RestoreDisplayMode();
            pDisplayDevice->m_pDD->SetCooperativeLevel(pDisplayDevice->GetMainWnd(), DDSCL_NORMAL);
            break;
    }
}

///////////////////////////////////////

static cDDCritMsgHandler g_DDCritMsgNotificationHandler;

///////////////////////////////////////////////////////////////////////////////

const char*
WhatDDError(HRESULT error)
{
    switch(error)
    {
        case  DD_OK                             : return "DD_OK";
        case  DDERR_INVALIDPARAMS               : return "DDERR_INVALIDPARAMS";
        case  DDERR_OUTOFMEMORY                 : return "DDERR_OUTOFMEMORY";
        case  DDERR_ALREADYINITIALIZED          : return "DDERR_ALREADYINITIALIZED"; // 5
        case  DDERR_CANNOTATTACHSURFACE         : return "DDERR_CANNOTATTACHSURFACE"; // 10
        case  DDERR_CANNOTDETACHSURFACE         : return "DDERR_CANNOTDETACHSURFACE"; // 20
        case  DDERR_CURRENTLYNOTAVAIL           : return "DDERR_CURRENTLYNOTAVAIL"; // 40
        case  DDERR_EXCEPTION                   : return "DDERR_EXCEPTION"; // 55
        case  DDERR_HEIGHTALIGN                 : return "DDERR_HEIGHTALIGN"; // 90
        case  DDERR_INCOMPATIBLEPRIMARY         : return "DDERR_INCOMPATIBLEPRIMARY"; // 95
        case  DDERR_INVALIDCAPS                 : return "DDERR_INVALIDCAPS"; // 100
        case  DDERR_INVALIDCLIPLIST             : return "DDERR_INVALIDCLIPLIST"; // 110
        case  DDERR_INVALIDMODE                 : return "DDERR_INVALIDMODE"; // 120
        case  DDERR_INVALIDOBJECT               : return "DDERR_INVALIDOBJECT"; // 130
        case  DDERR_INVALIDPIXELFORMAT          : return "DDERR_INVALIDPIXELFORMAT"; // 145
        case  DDERR_INVALIDRECT                 : return "DDERR_INVALIDRECT"; // 150
        case  DDERR_LOCKEDSURFACES              : return "DDERR_LOCKEDSURFACES"; // 160
        case  DDERR_NO3D                        : return "DDERR_NO3D"; // 170
        case  DDERR_NOALPHAHW                   : return "DDERR_NOALPHAHW"; // 180
        case  DDERR_NOCLIPLIST                  : return "DDERR_NOCLIPLIST"; // 205
        case  DDERR_NOCOLORCONVHW               : return "DDERR_NOCOLORCONVHW"; // 210
        case  DDERR_NOCOOPERATIVELEVELSET       : return "DDERR_NOCOOPERATIVELEVELSET"; // 212
        case  DDERR_NOCOLORKEY                  : return "DDERR_NOCOLORKEY"; // 215
        case  DDERR_NOCOLORKEYHW                : return "DDERR_NOCOLORKEYHW"; // 220
        case  DDERR_NODIRECTDRAWSUPPORT         : return "DDERR_NODIRECTDRAWSUPPORT"; // 222
        case  DDERR_NOEXCLUSIVEMODE             : return "DDERR_NOEXCLUSIVEMODE"; // 225
        case  DDERR_NOFLIPHW                    : return "DDERR_NOFLIPHW"; // 230
        case  DDERR_NOGDI                       : return "DDERR_NOGDI"; // 240
        case  DDERR_NOMIRRORHW                  : return "DDERR_NOMIRRORHW"; // 250
        case  DDERR_NOTFOUND                    : return "DDERR_NOTFOUND"; // 255
        case  DDERR_NOOVERLAYHW                 : return "DDERR_NOOVERLAYHW"; // 260
        case  DDERR_NORASTEROPHW                : return "DDERR_NORASTEROPHW"; // 280
        case  DDERR_NOROTATIONHW                : return "DDERR_NOROTATIONHW"; // 290
        case  DDERR_NOSTRETCHHW                 : return "DDERR_NOSTRETCHHW"; // 310
        case  DDERR_NOT4BITCOLOR                : return "DDERR_NOT4BITCOLOR"; // 316
        case  DDERR_NOT4BITCOLORINDEX           : return "DDERR_NOT4BITCOLORINDEX"; // 317
        case  DDERR_NOT8BITCOLOR                : return "DDERR_NOT8BITCOLOR"; // 320
        case  DDERR_NOTEXTUREHW                 : return "DDERR_NOTEXTUREHW"; // 330
        case  DDERR_NOVSYNCHW                   : return "DDERR_NOVSYNCHW"; // 335
        case  DDERR_NOZBUFFERHW                 : return "DDERR_NOZBUFFERHW"; // 340
        case  DDERR_NOZOVERLAYHW                : return "DDERR_NOZOVERLAYHW"; // 350
        case  DDERR_OUTOFCAPS                   : return "DDERR_OUTOFCAPS"; // 360
        case  DDERR_OUTOFVIDEOMEMORY            : return "DDERR_OUTOFVIDEOMEMORY"; // 380
        case  DDERR_OVERLAYCANTCLIP             : return "DDERR_OVERLAYCANTCLIP"; // 382
        case  DDERR_OVERLAYCOLORKEYONLYONEACTIVE: return "DDERR_OVERLAYCOLORKEYONLYONEACTIVE";
        case  DDERR_PALETTEBUSY                 : return "DDERR_PALETTEBUSY"; // 387
        case  DDERR_COLORKEYNOTSET              : return "DDERR_COLORKEYNOTSET"; // 400
        case  DDERR_SURFACEALREADYATTACHED      : return "DDERR_SURFACEALREADYATTACHED"; // 410
        case  DDERR_SURFACEALREADYDEPENDENT     : return "DDERR_SURFACEALREADYDEPENDENT"; // 420
        case  DDERR_SURFACEBUSY                 : return "DDERR_SURFACEBUSY"; // 430
        case  DDERR_SURFACEISOBSCURED           : return "DDERR_SURFACEISOBSCURED"; // 440
        case  DDERR_SURFACELOST                 : return "DDERR_SURFACELOST"; // 450
        case  DDERR_SURFACENOTATTACHED          : return "DDERR_SURFACENOTATTACHED"; // 460
        case  DDERR_TOOBIGHEIGHT                : return "DDERR_TOOBIGHEIGHT"; // 470
        case  DDERR_TOOBIGSIZE                  : return "DDERR_TOOBIGSIZE"; // 480
        case  DDERR_TOOBIGWIDTH                 : return "DDERR_TOOBIGWIDTH"; // 490
        case  DDERR_UNSUPPORTEDFORMAT           : return "DDERR_UNSUPPORTEDFORMAT"; // 510
        case  DDERR_UNSUPPORTEDMASK             : return "DDERR_UNSUPPORTEDMASK"; // 520
        case  DDERR_VERTICALBLANKINPROGRESS     : return "DDERR_VERTICALBLANKINPROGRESS"; // 537
        case  DDERR_WASSTILLDRAWING             : return "DDERR_WASSTILLDRAWING"; // 540
        case  DDERR_XALIGN                      : return "DDERR_XALIGN"; // 560
        case  DDERR_INVALIDDIRECTDRAWGUID       : return "DDERR_INVALIDDIRECTDRAWGUID"; // 561
        case  DDERR_DIRECTDRAWALREADYCREATED    : return "DDERR_DIRECTDRAWALREADYCREATED"; // 562
        case  DDERR_NODIRECTDRAWHW              : return "DDERR_NODIRECTDRAWHW"; // 563
        case  DDERR_PRIMARYSURFACEALREADYEXISTS : return "DDERR_PRIMARYSURFACEALREADYEXISTS"; //564
        case  DDERR_NOEMULATION                 : return "DDERR_NOEMULATION"; // 565
        case  DDERR_REGIONTOOSMALL              : return "DDERR_REGIONTOOSMALL"; // 566
        case  DDERR_CLIPPERISUSINGHWND          : return "DDERR_CLIPPERISUSINGHWND"; // 567
        case  DDERR_NOCLIPPERATTACHED           : return "DDERR_NOCLIPPERATTACHED"; // 568
        case  DDERR_NOHWND                      : return "DDERR_NOHWND"; // 569
        case  DDERR_HWNDSUBCLASSED              : return "DDERR_HWNDSUBCLASSED"; // 570
        case  DDERR_HWNDALREADYSET              : return "DDERR_HWNDALREADYSET"; // 571
        case  DDERR_NOPALETTEATTACHED           : return "DDERR_NOPALETTEATTACHED"; // 572
        case  DDERR_NOPALETTEHW                 : return "DDERR_NOPALETTEHW"; // 573
        case  DDERR_BLTFASTCANTCLIP             : return "DDERR_BLTFASTCANTCLIP"; // 574
        case  DDERR_NOBLTHW                     : return "DDERR_NOBLTHW"; // 575
        case  DDERR_NODDROPSHW                  : return "DDERR_NODDROPSHW"; // 576
        case  DDERR_OVERLAYNOTVISIBLE           : return "DDERR_OVERLAYNOTVISIBLE"; // 577
        case  DDERR_NOOVERLAYDEST               : return "DDERR_NOOVERLAYDEST"; // 578
        case  DDERR_INVALIDPOSITION             : return "DDERR_INVALIDPOSITION"; // 579
        case  DDERR_NOTAOVERLAYSURFACE          : return "DDERR_NOTAOVERLAYSURFACE"; // 580
        case  DDERR_EXCLUSIVEMODEALREADYSET     : return "DDERR_EXCLUSIVEMODEALREADYSET"; // 581
        case  DDERR_NOTFLIPPABLE                : return "DDERR_NOTFLIPPABLE"; // 582
        case  DDERR_CANTDUPLICATE               : return "DDERR_CANTDUPLICATE"; // 583
        case  DDERR_NOTLOCKED                   : return "DDERR_NOTLOCKED"; // 584
        case  DDERR_CANTCREATEDC                : return "DDERR_CANTCREATEDC"; // 585
        case  DDERR_NODC                        : return "DDERR_NODC"; // 586
        case  DDERR_WRONGMODE                   : return "DDERR_WRONGMODE"; // 587
        case  DDERR_IMPLICITLYCREATED           : return "DDERR_IMPLICITLYCREATED"; // 588
        case  DDERR_NOTPALETTIZED               : return "DDERR_NOTPALETTIZED"; // 589
        case  DDERR_UNSUPPORTEDMODE             : return "DDERR_UNSUPPORTEDMODE"; // 590
        default                                 : return "DD Unknown Error Code"; // 590
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// class cDDProvider
//

cDDProvider::cDDProvider(cWinDisplayDevice * pOwner)
  : cWinDisplayProvider(pOwner),
    m_pDD(0),               // DirectDraw object
    m_pModeOps(0),
    m_Flags(0),
    m_PalFlags(DDPCAPS_8BIT | DDPCAPS_ALLOW256),  // | DDPCAPS_INITIALIZE;
    m_DDCoopFlags(0),
    m_NormalWindowStyle(0)
{
    DebugMsg("cDDProvider::cDDProvider()");
#if 0
    if (GetPrivateProfileInt("WinShell", "NoPrimaryAccess", FALSE, "lg.ini"))
    {
        m_Flags |= kAlwaysEmulate;
    }
#endif
}

///////////////////////////////////////

cDDProvider::~cDDProvider()
{
    DebugMsg("cDDProvider::~cDDProvider()");
    // This is redundent, but necessary if ->Close was not called befor we are terminated
    AssertMsg(!m_pDD, "cDDProvider destroyed without Close()");
}

///////////////////////////////////////

HRESULT CALLBACK
  cDDProvider::EnumDisplayModesCallback(LPDDSURFACEDESC pSurfaceDesc, LPVOID context)
{
    int bitDepth = pSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;

#if ENABLE_LATER
// @TBD (toml 11-13-96): This should be enabled when we have time to make the 2d and the games deal with it... For now WE LIE!
    if (bitDepth == 16 && pSurfaceDesc->ddpfPixelFormat.dwGBitMask == 0x03E0)
        bitDepth = 15;
#endif

    int mode;
    if (cDisplayDevice::ModeInfoToEnumMode(pSurfaceDesc->dwWidth, pSurfaceDesc->dwHeight, bitDepth, &mode))
    {
        DebugMsg1("%s", cDisplayDevice::EnumModeToName(mode));
        short ** ppModes = (short **)context;
        *(*ppModes) = (short)mode;
        (*ppModes)++;
    }

    return DDENUMRET_OK;
}

///////////////////////////////////////

BOOL cDDProvider::DoProcessMessage(UINT msg, WPARAM /*wParam*/, LPARAM /*lParam*/, long * pRetVal)
{
    // If we haven't been initialized...
    if (!m_pDisplayDevice->IsModeSet())
        // .. then always do default processing
        return FALSE;

    switch (msg)
    {
        case WM_SIZE:
            if (m_pModeOps && (m_pModeOps->GetModeInfoEx().flags & kGrModeIsWindowed))
            {
                m_pModeOps->SetScaleFactor(
                    AdjustWindow(GetMainWnd(),
                                 m_pModeOps->GetModeInfoEx().width,
                                 m_pModeOps->GetModeInfoEx().height,
                                 TRUE)
                    );
            }
            break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(GetMainWnd(), &ps);
            if (m_pDisplayDevice->IsFlushingPaused())
            {
                FillRect(hdc, &ps.rcPaint, (HBRUSH) GetStockObject(BLACK_BRUSH));
            }
            else
            {
                m_pDisplayDevice->FlushRect(ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);
            }
            EndPaint(GetMainWnd(), &ps);
            *pRetVal = 0;
            return TRUE;
        }
    }
    return FALSE;
}

///////////////////////////////////////
//
// Interrogate for supported modes
//

static BOOL FindMode(short * pModes, short mode)
{
    while (*pModes != -1)
    {
        if (*pModes == mode)
            return TRUE;
        pModes++;
    }
    return FALSE;
}

void cDDProvider::DoGetInfo(sGrDeviceInfo * pGrDeviceInfo, sGrModeInfo * pModeInfo)
{
    BEGIN_DEBUG_MSG("cDDProvider::GetInfo()");

    short * pModes;

    //
    // Determine which modes are native
    //
    DDCAPS ddcaps;
    HRESULT result;

    memset(&ddcaps, 0, sizeof(ddcaps));
    ddcaps.dwSize = sizeof(ddcaps);
    result = m_pDD->GetCaps(&ddcaps, NULL);

    DebugMsgIfErr("DoGetInfo: GetCaps", result);

    m_lTotalVideoMemory = (long)(ddcaps.dwVidMemTotal);

    //
    // Get Modes from direct draw
    //
    BEGIN_DEBUG_MSG("Enumerating native modes from DirectDraw");

    YieldDisplay(m_pDisplayDevice);

    // We set the cooperative level to ensure DirectDraw exposes the
    // Mode X modes. Since setting the cooperative level forces a show of
    // the window, we save the attributes of the window, make it so it
    // won't draw, then restore that state
    const BOOL fWasVisible     = IsWindowVisible(GetMainWnd());
    const long ulWindowStyle   = GetWindowLong(GetMainWnd(), GWL_STYLE);
    const long ulWindowExStyle = GetWindowLong(GetMainWnd(), GWL_EXSTYLE);

    if (!fWasVisible)
    {
        SetWindowLong(GetMainWnd(), GWL_EXSTYLE, WS_EX_TOPMOST);
        SetWindowLong(GetMainWnd(), GWL_STYLE, WS_POPUP);
        ShowWindow(GetMainWnd(), SW_SHOW);
    }
    SetCooperativeLevel(kDDFullScreenCoopFlags);

    pModes = pGrDeviceInfo->modes;
    result = m_pDD->EnumDisplayModes(0, NULL, &pModes, EnumDisplayModesCallback);
    *pModes = -1;

    SetCooperativeLevel(DDSCL_NORMAL);
    if (!fWasVisible)
    {
        ShowWindow(GetMainWnd(), SW_HIDE);
        SetWindowLong(GetMainWnd(), GWL_STYLE, ulWindowStyle);
        SetWindowLong(GetMainWnd(), GWL_EXSTYLE, ulWindowExStyle);
    }

    DebugMsgIfErr("DoGetInfo: EnumDisplayModes", result);

    RegainDisplay(m_pDisplayDevice);

    // Claim native for all modes enumerated above 
    pModes = pGrDeviceInfo->modes;
    while (*pModes != -1)
    {
        pModeInfo[*pModes].flags |= kGrModeIsNative;
        pModes++;
    }

    END_DEBUG;

    //
    // Determine modes we can fake well
    //
    BEGIN_DEBUG_MSG("Enumerating emulatable modes");

    // Start by putting in 320xN if we have doubled mode
    if (!FindMode(pGrDeviceInfo->modes, GRM_320x200x8) && FindMode(pGrDeviceInfo->modes, GRM_640x400x8))
    {
        *pModes++ = GRM_320x200x8;
        *pModes = -1;
    }

    if (!FindMode(pGrDeviceInfo->modes, GRM_320x240x8) && FindMode(pGrDeviceInfo->modes, GRM_640x480x8))
    {
        *pModes++ = GRM_320x240x8;
        *pModes = -1;
    }

    // Now advertise 512x384
    if (!FindMode(pGrDeviceInfo->modes, GRM_512x384x8) && FindMode(pGrDeviceInfo->modes, GRM_640x480x8))
    {
        *pModes++ = GRM_512x384x8;
        *pModes = -1;
    }
    END_DEBUG;

    //
    // Fill in mode support detail
    //

    // Get info about default user desktop mode
    DDSURFACEDESC desktopSurfaceDesc;

    memset(&desktopSurfaceDesc, 0, sizeof(desktopSurfaceDesc));
    desktopSurfaceDesc.dwSize = sizeof(desktopSurfaceDesc);

    m_pDD->GetDisplayMode(&desktopSurfaceDesc);

    // Claim fullscreen for all
    pModes = pGrDeviceInfo->modes;
    while (*pModes != -1)
    {
        pModeInfo[*pModes].flags |= kGrModeCanFullscreen;
        pModes++;
    }

    // Claim windowed for all with size less than screen and depth equal to screen
    pModes = pGrDeviceInfo->modes;
    while (*pModes != -1)
    {
        int modePixelSize = (pModeInfo[*pModes].bitDepth == 15) ? 16 : pModeInfo[*pModes].bitDepth;
        if (pModeInfo[*pModes].w < desktopSurfaceDesc.dwWidth &&
            pModeInfo[*pModes].h < desktopSurfaceDesc.dwHeight &&
            modePixelSize == desktopSurfaceDesc.ddpfPixelFormat.dwRGBBitCount)
            {
            pModeInfo[*pModes].flags |= kGrModeCanWindow;
            }
        pModes++;
    }

    END_DEBUG;
}

///////////////////////////////////////

BOOL cDDProvider::DoOpen(sGrModeCap *, int)
{
    BEGIN_DEBUG_MSG("cDDProvider::DoOpen()");

    HRESULT result;

    // Create an instance of a Direct Draw object.
    result = DynDirectDrawCreate(m_pDisplayDevice->m_pDDrawGuid, &m_pDD, NULL);
    if (result != DD_OK)
    {
        DebugMsgIfErr("DoOpen: DirectDrawCreate", result);
        return FALSE;
    }

    m_NormalWindowStyle = GetWindowLong(GetMainWnd(), GWL_STYLE);
    m_NormalWindowExStyle = GetWindowLong(GetMainWnd(), GWL_EXSTYLE);

    // Move window to foreground
    YieldDisplay(m_pDisplayDevice);
    SetWindowLong(GetMainWnd(), GWL_EXSTYLE, WS_EX_TOPMOST);
    RegainDisplay(m_pDisplayDevice);

    // Hook into critical message handler
    g_DDCritMsgNotificationHandler.Set(this);

    return TRUE;

    END_DEBUG;
}

///////////////////////////////////////

BOOL cDDProvider::DoClose()
{
    BEGIN_DEBUG_MSG("cDDProvider::DoClose()");
    g_DDCritMsgNotificationHandler.Set(NULL);

    if (m_pModeOps)
    {
        delete m_pModeOps;
        m_pModeOps = 0;
    }

    m_pDD->FlipToGDISurface();
    m_pDD->RestoreDisplayMode();
    m_pDD->SetCooperativeLevel(GetMainWnd(), DDSCL_NORMAL);

    SafeRelease(m_pDD);

    SetWindowLong(GetMainWnd(), GWL_STYLE, m_NormalWindowStyle);
    SetWindowLong(GetMainWnd(), GWL_EXSTYLE, m_NormalWindowExStyle);

    InvalidateRect(NULL, NULL, TRUE);

    // call show here?
    ShowWindow(GetMainWnd(), SW_SHOW);

    return TRUE;
    END_DEBUG;
}

///////////////////////////////////////

cWinDisplayModeOperations *
cDDProvider::DoSetMode(const sGrModeInfo & modeInfo,
                       int flags,
                       sGrModeCap * pReturnModeInfo)
{
    BEGIN_DEBUG_MSG3("cDDProvider::DoSetMode(width %ld, height %ld, bitDepth %ld)", modeInfo.w, modeInfo.h, modeInfo.bitDepth);

    HRESULT result = S_OK;
    LPDIRECTDRAWPALETTE pPalette = NULL;

    ///////////////////////////////////

#if 0
    // Hook for experiemental windowed DD mode (toml 05-20-96)
    if (GetPrivateProfileInt("WinShell", "ExperimentalWindowedDirectDraw", FALSE, "lg.ini"))
    {
        flags |= kGrSetWindowed;
    }
#endif

    if (!(flags & (kGrSetWindowed | kGrSetFullScreen)))
    {
        eDisplayDeviceKind kind = m_pDisplayDevice->GetKind();

        switch (kind)
        {
            case kDispDefault:
            case kDispFullScreen:
            case kDispUndefined:
            {
                flags |= kGrSetFullScreen;
                break;
            }

            case kDispDebug:
            case kDispWindowed:
            {
                flags |= kGrSetWindowed;
                break;
            }

            default:
                CriticalMsg("Unknown display device kind");
        }
    }

    // Make sure we can do the mode in the way desired
    if ((flags & kGrSetWindowed) && !(modeInfo.flags & kGrModeCanWindow))
        flags &= ~kGrSetWindowed;

    // Set proper DirectDraw cooperative level
    
    YieldDisplay(m_pDisplayDevice);
    
    if (flags & kGrSetWindowed)
    {
        if (m_DDCoopFlags != kDDWindowedCoopFlags)
        {
            SetWindowLong(GetMainWnd(), GWL_STYLE, m_NormalWindowStyle);
            m_DDCoopFlags = kDDWindowedCoopFlags;
            m_pDD->RestoreDisplayMode();
            result = SetCooperativeLevel(m_DDCoopFlags);
        }
    }
    else
    {
        if (m_DDCoopFlags != kDDFullScreenCoopFlags)
        {
            SetWindowLong(GetMainWnd(), GWL_STYLE, WS_POPUP);
            SetForegroundWindow(GetMainWnd());

            m_DDCoopFlags = kDDFullScreenCoopFlags;
            result = SetCooperativeLevel(m_DDCoopFlags);
        }
    }
    
    RegainDisplay(m_pDisplayDevice);
    
    DebugMsgIfErr("SetCooperativeLevel", result);

    if (result != S_OK)
        return NULL;

    ///////////////////////////////////

    // detach the existing palette and destroy the present mode operations
    if (m_pModeOps)
    {
        DebugMsgEx(SETMODE, "Detaching existing palette and destroying old mode operations...");
        pPalette = m_pModeOps->GetPalette();
        m_pModeOps->AttachPalette(NULL);
        delete m_pModeOps;
        m_pModeOps = NULL;
    }

    // If we're 8-bit palettized and haven't got a palette
    if ((modeInfo.bitDepth == 8)&&(pPalette == NULL))
    {
        // ... then create a palette
        DebugMsgEx(SETMODE, "Creating palette...");
        m_PalFlags = DDPCAPS_8BIT | DDPCAPS_ALLOW256 | DDPCAPS_INITIALIZE;

        result = m_pDD->CreatePalette(m_PalFlags, m_pDisplayDevice->GetPaletteEntries(), &pPalette, NULL);
        DebugMsgIfErr("CreatePalette", result);

        AssertMsg(pPalette, "Failed to create palette");
    }

    ///////////////////////////////////

    // Create the appropriate set of operations for the requested mode
//    const BOOL fModeIsNative = (cDisplayDevice::GetAvailableMode(modeInfo.w, modeInfo.h, modeInfo.bitDepth) != -1);
    const BOOL fModeIsNative = (modeInfo.flags & kGrModeIsNative);

    DebugMsgEx2(SETMODE, "using flags = 0x%x, m_Flags = 0x%x", flags, m_Flags);

    // For now, we always emulate modeX modes, and (only for now), windowed operation
    BOOL bUseEmulation = (flags & kGrSetWindowed) || (m_Flags & kAlwaysEmulate) || !fModeIsNative || (!(flags & kGrSetWindowed) && modeInfo.w < 400);

    if (modeInfo.w == 512 && !fModeIsNative) // @Note (toml 12-09-96): hack for Dark.  Need real support for direct-faked subregion modes
    {
        DebugMsg("New mode is an emulated mode");
        m_pModeOps = new cPhoney512ModeOps(m_pDisplayDevice, this, pPalette);
    }
    else if (bUseEmulation)
    {
        DebugMsgTrue1(fModeIsNative, "New mode is a native mode, but choosing emulation (%d)", !!(m_Flags & kAlwaysEmulate));
        DebugMsgTrue(!fModeIsNative, "New mode is an emulated mode");
        m_pModeOps = new cOffVideoDDModeOps(m_pDisplayDevice, this, pPalette);
    }
    else
    {
        DebugMsg("New mode is a native mode");
        m_pModeOps = new cDirectDDModeOps(m_pDisplayDevice, this, pPalette);
    }

    DebugMsgEx(SETMODE, "Created new mode operations");

    // Release our lock on palette now that mode ops has it
    SafeRelease(pPalette);

    // Make sure the main window is visible
    if (!IsWindowVisible(GetMainWnd()))
    {
        YieldDisplay(m_pDisplayDevice);

        m_pModeOps->SetScaleFactor(
            AdjustWindow(GetMainWnd(),
                         modeInfo.w,
                         modeInfo.h,
                         TRUE)
            );

        ShowMainWnd();

        RegainDisplay(m_pDisplayDevice);
    }

    // Attempt to start the mode
    DebugMsgEx(SETMODE, "Attempting to start mode...");
    if (!m_pModeOps->StartMode(modeInfo, flags, pReturnModeInfo))
    {
        static BOOL fInRetry;

        // We failed, so clean up...

        if (!fInRetry)
        {
            fInRetry = TRUE;
            cWinDisplayModeOperations * pReturn = NULL;
            if ((flags & kGrSetWindowed))
            {
                // Try full screen
                pReturn = DoSetMode(modeInfo, (flags & ~kGrSetWindowed), pReturnModeInfo);
            }
            else if (!bUseEmulation)
            {
                // In this case, DD has told us that it supports direct modes
                // but it lied, since the direct mode didn't work. So we're
                // always going to emulate and not believe DD again.
                DebugMsg("StartMode() on native mode failed, switching to emulation");
                m_Flags |= kAlwaysEmulate;
                pReturn = DoSetMode(modeInfo, flags, pReturnModeInfo);
            }
            fInRetry = FALSE;
            return pReturn;
        }
        else
        {
            DebugMsgEx(SETMODE, "Start mode failed.  No recourse.");
            delete m_pModeOps;
            m_pModeOps = NULL;
        }
    }

    DebugDumpModeInfo(m_pModeOps, pReturnModeInfo);
    DebugMsg1("DoSetMode %s", (m_pModeOps) ? "success" : "failure");

    // If we succeeded...
    if (m_pModeOps)
    {
        YieldDisplay(m_pDisplayDevice);

        // ... and windowed, adjust the outer frame
        if (m_pModeOps->GetModeInfoEx().flags & kGrModeIsWindowed)
        {
            m_pModeOps->SetScaleFactor(
                AdjustWindow(GetMainWnd(),
                             m_pModeOps->GetModeInfoEx().width,
                             m_pModeOps->GetModeInfoEx().height,
                             TRUE)
                );
        }
        // ... otherwise, make sure we obscure all other windows
        else
        {
            m_pModeOps->SetScaleFactor(0);
        }

        WinDispBringToFront();
        RegainDisplay(m_pDisplayDevice);
    }

    return m_pModeOps;

    END_DEBUG;
}

///////////////////////////////////////

BOOL cDDProvider::DoGetDirectDraw(IDirectDraw ** ppDD)
{
    *ppDD = m_pDD;

    if (m_pDD==NULL)
        return FALSE;

    m_pDD->AddRef();
    return TRUE;
}

///////////////////////////////////////

#pragma off(unreferenced)
BOOL cDDProvider::DoGetBitmapSurface(sGrBitmap * pBm, IDirectDrawSurface ** ppDDS)
{
// @Note (toml 05-23-97): hey, this implementation is wrong but will do the job for Kevin: MUST FIX, or decide a better api for doing this
    IDirectDrawSurface *pPrimary = NULL, *pSecondary = NULL;

    m_pModeOps->GetSurfaces(&pPrimary, &pSecondary);

    if (pSecondary != NULL) 
    {
        DebugMsg("DoGetBitmapSurf(): returning back buffer.\n");
        *ppDDS = pSecondary;
        SafeRelease(pPrimary);
    } 
    else
    {
        DebugMsg("DoGetBitmapSurf(): no back buffer. Returning primary surface.\n");
        *ppDDS = pPrimary;
    }

    return TRUE;
}
#pragma on(unreferenced)

///////////////////////////////////////

HRESULT cDDProvider::SetCooperativeLevel(DWORD flags)
{
    HRESULT result;

    // Set the cooperative level
    YieldDisplay(m_pDisplayDevice);

    result = m_pDD->SetCooperativeLevel(GetMainWnd(), flags);

    DebugMsgIfErr("SetCooperativeLevel", result);
    RegainDisplay(m_pDisplayDevice);

    return result;
}

///////////////////////////////////////////////////////////////////////////////
