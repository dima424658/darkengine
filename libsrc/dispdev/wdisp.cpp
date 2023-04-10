///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/wdisp.cpp $
// $Author: KEVIN $
// $Date: 1998/04/14 15:02:01 $
// $Revision: 1.46 $
//
// @TBD (toml 09-09-96): mode operations, sGrModeInfoEx, sGrModeInfo
// and m_CurrentDispMode should all be consolodated into one "mode" class!!
//

#include <windows.h>

#include <lg.h>
#include <comtools.h>
#include <appagg.h>

#include <wappapi.h>
#include <dispapi.h>

#include <wdisp.h>
#include <wdispcb.h>
#include <wdspprov.h>
#include <wdspmode.h>

#include <wdcreate.h>

#include <dev2d.h>

#ifdef DEBUG_SETMODE
#undef DEBUG_SETMODE
#define DEBUG_SETMODE 1
#pragma message("DEBUG_SETMODE")
#else
#define DEBUG_SETMODE 0
#endif

#ifdef DEBUG_PTRMONITOR
#undef DEBUG_PTRMONITOR
#define DEBUG_PTRMONITOR 1
#pragma message("DEBUG_PTRMONITOR")
#else
#define DEBUG_PTRMONITOR 0
#endif

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sGdiBitmapInfo
//

sGdiBitmapInfo::sGdiBitmapInfo(int width, int height, int significantDepth)
{
    memset(&bmiHeader, 0, sizeof(bmiHeader));

    bmiHeader.biSize            = sizeof(bmiHeader);
    bmiHeader.biPlanes          = 1;
    bmiHeader.biWidth           = width;
    bmiHeader.biHeight          = -height;
    bmiHeader.biSizeImage       = width * height;

    bmiHeader.biBitCount  = (unsigned short) ((significantDepth == 15) ? 16 : significantDepth);

    if (significantDepth == 8)
    {
        bmiHeader.biBitCount    = 8;
        bmiHeader.biCompression = BI_RGB;
        memset(bmiColors, 0, sizeof(RGBQUAD) * 256);
    }
    else if (significantDepth == 16 || significantDepth == 15)
    {
        DWORD * pMasks = (DWORD *) bmiColors;

        bmiHeader.biBitCount    = 16;
        bmiHeader.biCompression = BI_BITFIELDS;

        if (significantDepth == 16)
        {
            pMasks[0] = 0xF800;
            pMasks[1] = 0x07E0;
            pMasks[2] = 0x001F;
        }
        else
        {
            pMasks[0] = 0x7C00;
            pMasks[1] = 0x03E0;
            pMasks[2] = 0x001F;
        }
    }
    else
        bmiHeader.biBitCount    = (ushort)significantDepth;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cWinDisplayDevice
//
// Implements IDisplayDevice.  Acts primarily as an adaptor for instances
// of cWinDisplayProvider and cWinModeOperations
//
// Provides shared functionality of the DirectDraw and GDI implementations
// of IDisplayDevice.
//
// This class implements most functionality, providing hooks to allow
// actual final implementation to set/extend the default behavior
//
// The client must implement a derivation of cWinDisplayProvider,
// which provides device-level operations.
//
// The client also provides derivations of cWinModeOperations
// that implements mode-specific operations.
//
// This class will guarentee that the client information
// passed in through SetMode() will be consistent and
// accurate, based on lock/unlock status
//
// While cWinDisplayDevice stores a pointer to a derived version
// cWinModeOperations, the cWinDisplayProvider instance is the owner
// of any mode object and is responsible for destroying it on Close()
//

///////////////////////////////////////
//
// Palette storage
//

cWinDisplayDevice::sPalette256  cWinDisplayDevice::gm_LogicalPalette;

///////////////////////////////////////

cWinDisplayDevice::cWinDisplayDevice(IUnknown * pOuterUnknown,
                                     eDisplayDeviceKind kind,
                                     int fCreateFlags)
  : cDisplayDevice(pOuterUnknown, kind, fCreateFlags),
    m_pDisplayProvider(NULL),
    m_pDDrawGuid(NULL),
    m_pDisplayModeOperations(NULL),
    m_IWinDisplayDeviceProxy(this, pOuterUnknown),
    m_MutexCount(0),
    m_iLock(0),
    m_pModeInfo(NULL),
    m_CurrentDispMode(GRD_MODES),        // GRD_MODES acts as "unspecified"
    m_DisplayDeviceKind(kind),
    m_fOptions(fCreateFlags),
    m_PrimarySurfaceRelocationSet((int8 *)kPrimaryBogusBase),
    m_SecondarySurfaceRelocationSet((int8 *)kSecondaryBogusBase)
{
    // Add internal components to outer aggregate...
    AddToAggregate1(pOuterUnknown,
                    IID_IWinDisplayDevice, &m_IWinDisplayDeviceProxy,
                    NULL,
                    kPriorityDontCare,
                    NULL);

    // Debug options...

    if (m_DisplayDeviceKind == kDispDefault)
    {
        unsigned desiredShellKind = GetPrivateProfileInt("WinShell", "ShellKind", 100, "lg.ini");

        enum eLegacyKinds
        {
            kDirectDrawShell,
            kGDIShell
        };

        if (desiredShellKind != 100)
        {
            switch (desiredShellKind)
            {
                case kDirectDrawShell:
                    m_DisplayDeviceKind = kDispFullScreen;
                    break;

                case kGDIShell:
                    m_DisplayDeviceKind = kDispDebug;
                    break;
            }
        }
    }

}

///////////////////////////////////////

cWinDisplayDevice::~cWinDisplayDevice()
{
    DebugMsg("cWinDisplayDevice::~cWinDisplayDevice()");
    if (m_pDisplayProvider)
    {
        Close();
    }
}

///////////////////////////////////////
//
// Query kind
//

STDMETHODIMP_(eDisplayDeviceKind) cWinDisplayDevice::GetKind()
{
    eDisplayDeviceKind kind;
    GetKind2(&kind, NULL, NULL);
    return kind;
}

///////////////////////////////////////
//
// Query kind, flags, and ddraw guid
//

STDMETHODIMP_(void) cWinDisplayDevice::GetKind2(eDisplayDeviceKind *pKind, int *pFlags, GUID **ppDDrawGuid)
{
    if (pKind != NULL)
        *pKind = m_DisplayDeviceKind;
    if (pFlags != NULL)
        *pFlags = m_fOptions;
    if (ppDDrawGuid != NULL)
        *ppDDrawGuid = m_pDDrawGuid;
}

///////////////////////////////////////
//
// Set kind, flags, and ddraw guid
//

STDMETHODIMP_(void) cWinDisplayDevice::SetKind2(eDisplayDeviceKind newKind, int fNewOptions, GUID *pDDrawGuid)
{
    cAutoDisplayMutex mutex(this);

    AssertMsg(!m_pDisplayProvider, "Must Close() (i.e., gr_close()) before SetKind()");

    m_DisplayDeviceKind = newKind;
    m_pDDrawGuid = pDDrawGuid;
    m_fOptions = fNewOptions;

//    m_pDisplayProvider = CreateProvider(m_DisplayDeviceKind, this);
}

STDMETHODIMP_(void) cWinDisplayDevice::SetKind(eDisplayDeviceKind newKind, int fNewOptions)
{
    SetKind2(newKind, fNewOptions, NULL);
}

///////////////////////////////////////
//
// Get information about the device
//

STDMETHODIMP_(void) cWinDisplayDevice::GetInfo(sGrDeviceInfo * pInfo, sGrModeInfo * pModeInfo)
{
    cAutoDisplayMutex mutex(this);
    AssertMsg(m_pDisplayProvider, "Bad call to cWinDisplayDevice: no display provider");

    m_pDisplayProvider->DoGetInfo(pInfo, pModeInfo);
}

///////////////////////////////////////
//
// Open the device
//

STDMETHODIMP cWinDisplayDevice::Open(sGrModeCap * pModeCap, int fFlags)
{
    cAutoDisplayMutex mutex(this);
    m_pModeInfo = pModeCap;
    InvalidateBasePointers();

    if (!m_pDisplayProvider)
    {
#ifndef SHIP
        if ((m_DisplayDeviceKind == kDispFullScreen)||
            (m_fOptions & kDispStrictMonitors))
            m_flags |= kStrictMonitors;
        else
            m_flags &= ~kStrictMonitors;
#endif
        m_pDisplayProvider = CreateProvider(m_DisplayDeviceKind, this);
    }

    AssertMsg(m_pDisplayProvider, "Bad call to cWinDisplayDevice: no display provider");
    return (m_pDisplayProvider->DoOpen(pModeCap, fFlags)) ? NOERROR : E_FAIL;
}

///////////////////////////////////////
//
// Close the device
//

STDMETHODIMP cWinDisplayDevice::Close()
{
    AssertMsg(m_pDisplayProvider, "Bad call to cWinDisplayDevice: no display provider");
    AssertMsg(!IsLocked(), "Cannot close locked display device!\n(Also verify mouse cursor has been turned off before closing device)");

    cWinDisplayProvider * pDisplayProvider = m_pDisplayProvider;

    m_pDisplayProvider = NULL;
    m_pDisplayModeOperations = NULL; // @Note (toml 05-16-96): m_pDisplayProvider is responsible for cleaning up mode operations

    HRESULT result = (pDisplayProvider->DoClose()) ? S_OK : E_FAIL;
    
    delete pDisplayProvider;

    return result;
}

///////////////////////////////////////
//
// Set the mode.  Info parameter on GetMode() cWinDisplayDevice::may be null.
//

STDMETHODIMP cWinDisplayDevice::SetMode(eGrDispMode dispMode, int flags)
{
    cAutoDisplayMutex mutex(this);

    BEGIN_DEBUG_MSG("cWinDisplayDevice::SetMode()");
    AssertMsg(m_pDisplayProvider, "Bad call to cWinDisplayDevice: no display provider");

    HRESULT result = S_OK;

    m_flags |= kSettingMode;

    // Clear out any mouse cursor scratch buffer
    if (m_CursorScratch.hBitmap)
    {
        DeleteObject(m_CursorScratch.hBitmap);
        memset(&m_CursorScratch, 0, sizeof(m_CursorScratch));
    }

    // Check to make sure we're not locked
    int iPreviousLock = BreakLock();
    DebugMsgTrueEx(SETMODE, iPreviousLock, "SetMode while locked");

    m_CurrentDispMode = dispMode;

    // Now try to set the mode
    if ((m_pDisplayModeOperations = m_pDisplayProvider->DoSetMode(EnumModeToModeInfo(dispMode), flags, m_pModeInfo)) != 0)
    {
        DebugMsgEx(SETMODE, "Created mode-specific operations");
        DebugMsg("SetMode() returns success");

        // @TBD (toml 09-10-96): Too much state retention m_CurrentDispMode and GetCurrentModeInfo() MUST be
        // eliminated in favor of sGrModeInfoEx stored in mode ops
        m_CurrentDispMode = GetMode(NULL);

#if ENABLE_LATER
        result = (m_CurrentDispMode == dispMode) ? S_OK : S_FALSE; // @Note (toml 09-10-96): Kevin should enable this code when 2d can handle "soft failure" of set mode
#endif
    }
    else
    {
        DebugMsg("SetMode() returns failure");
        CriticalMsg("Failed to set video mode!");
        m_flags &= ~kSettingMode;
        result = E_FAIL;
    }

    m_flags &= ~kSettingMode;
    // Reset any monitored pointers
    Lock();
    Unlock();

    RestoreLock(iPreviousLock);

    return result;

    END_DEBUG;
}

///////////////////////////////////////
//
// Get the mode.  Info parameter on GetMode() cWinDisplayDevice::may be null.
//

STDMETHODIMP_(eGrDispMode) cWinDisplayDevice::GetMode(sGrModeInfoEx * pOptionalModeInfo)
{
    cAutoDisplayMutex mutex(this);
    
    if (m_pDisplayModeOperations)
        return m_pDisplayModeOperations->DoGetMode(pOptionalModeInfo);

    if (pOptionalModeInfo)
    {
        memset(pOptionalModeInfo, 0, sizeof(sGrModeInfoEx));
        pOptionalModeInfo->flags = kGrModeNotSet;
    }
        
    return GRD_MODES; // GRD_MODES acts as "unspecified";
}

///////////////////////////////////////
//
// Return TRUE of retrace is in progress
//

STDMETHODIMP_(BOOL) cWinDisplayDevice::StatHTrace()
{
    cAutoDisplayMutex mutex(this);
    AssertMsg(m_pDisplayModeOperations, "Bad call to cWinDisplayDevice: no mode set");

    return m_pDisplayModeOperations->DoStatHTrace();
}

///////////////////////////////////////
//
// Return TRUE of retrace is in progress
//

STDMETHODIMP_(BOOL) cWinDisplayDevice::StatVTrace()
{
    cAutoDisplayMutex mutex(this);
    AssertMsg(m_pDisplayModeOperations, "Bad call to cWinDisplayDevice: no mode set");

    return m_pDisplayModeOperations->DoStatVTrace();
}

///////////////////////////////////////
//
// Set the palette (if 8-bit palletized)
//

STDMETHODIMP_(BOOL) cWinDisplayDevice::SetPalette(unsigned uStart, unsigned n, const unsigned char * pRGBs)
{
    cAutoDisplayMutex mutex(this);
    AssertMsg(m_pDisplayModeOperations, "Bad call to cWinDisplayDevice: no mode set");

    SetStoredPaletteEntries(uStart, n, pRGBs, PC_RESERVED);
    if (GetCurrentModeInfo().bitDepth == 8)
        return m_pDisplayModeOperations->DoSetPalette(GetLogPalette(), uStart, n);
    return TRUE; // if mode does go to an 8-bit one, the palette will be used, hence TRUE (toml 05-20-96)
}

///////////////////////////////////////
//
// Get the palette (if 8-bit palletized)
//

STDMETHODIMP_(BOOL) cWinDisplayDevice::GetPalette(unsigned uStart, unsigned n, unsigned char * pRGBs)
{
    cAutoDisplayMutex mutex(this);
    AssertMsg(m_pDisplayModeOperations, "Bad call to cWinDisplayDevice: no mode set");

    GetStoredPaletteEntries(uStart, n, pRGBs);
    return TRUE;
}

///////////////////////////////////////
//
// Flip the visible surface. No-op if no back buffer.
//

STDMETHODIMP cWinDisplayDevice::PageFlip()
{
    cAutoDisplayMutex mutex(this);
    AssertMsg(m_pDisplayModeOperations, "Bad call to cWinDisplayDevice: no mode set");

    return (m_pDisplayModeOperations->DoPageFlip()) ? NO_ERROR : ResultFromScode(E_FAIL);
}

///////////////////////////////////////
//
// Try to lock video memory.
//
//

STDMETHODIMP_(int) cWinDisplayDevice::Lock()
{
    cAutoDisplayMutex mutex(this);

    // Locking the display implies ownership of the display mutex, so we
    // add a wait here on behalf of the client...
    WaitForMutex();

    AssertMsg(m_pDisplayModeOperations, "Bad call to Lock(): no mode set");
    DebugMsgEx(VERBOSE, "cWinDisplayDevice::Lock()");

    if (m_iLock == 0)
    {
        if (!(m_flags & kSettingMode) && m_pDisplayModeOperations->DoLock(m_pModeInfo))
            Sync2DPointers();
        else
        {
            ReleaseMutex();
            return E_FAIL; // @TBD (toml 04-09-97): New behavior to document
        }
    }

    AssertMsg((void *)m_pModeInfo->vbase != (void *)kPrimaryBogusBase, "Display device lock failed! (primary surface)");
    AssertMsg(!m_pModeInfo->vbase2 || (void *)m_pModeInfo->vbase2 != (void *)kSecondaryBogusBase, "Display device lock failed! (secondary surface)");

    return ++m_iLock;
}

///////////////////////////////////////
//
// Release a lock on memory
//

STDMETHODIMP_(int) cWinDisplayDevice::Unlock()
{
#ifndef SHIP
    static BOOL bWarnOnBadUnlock = TRUE;
#endif
    cAutoDisplayMutex mutex(this);
    AssertMsg(m_pDisplayModeOperations, "Bad call to cWinDisplayDevice: no mode set");
    
    if (m_iLock < 1)
    {
#ifndef SHIP
        if (bWarnOnBadUnlock)
        {
            Warning(("Invalid unlock of an IDisplayDevice\n"));
            bWarnOnBadUnlock = FALSE;
        }
#endif
        return E_FAIL;
    }
#ifndef SHIP
    else
        bWarnOnBadUnlock = TRUE;
#endif

    if (m_iLock == 1 && m_pDisplayModeOperations->DoUnlock())
    {
        if (m_flags & kStrictMonitors)
            InvalidateBasePointers();
        Sync2DPointers();
    }

    DebugMsgEx(VERBOSE, "cWinDisplayDevice::Unlock()");

    m_iLock--;

    ReleaseMutex();

    return m_iLock;
}

///////////////////////////////////////
//
// Verify display reflects what is expected.  Useful if not particularly
// frame based & want to ensure screen is up-to-date within/without a
// BeginFrame()/EndFrame() cWinDisplayDevice::pair. Meaningful in "artificial" modes, no-op
// otherwise.
//

STDMETHODIMP_(void) cWinDisplayDevice::Flush()
{
    cAutoDisplayMutex mutex(this);
    AssertMsg(m_pDisplayModeOperations, "Bad call to cWinDisplayDevice: no mode set");
    if (!(m_flags & (kSettingMode | kFlushingPaused)))
        m_pDisplayModeOperations->DoFlush();
}

///////////////////////////////////////
//
// Flush a specific region
//

STDMETHODIMP_(void) cWinDisplayDevice::FlushRect(int x0, int y0, int x1, int y1)
{
    cAutoDisplayMutex mutex(this);
    AssertMsg(m_pDisplayModeOperations, "Bad call to cWinDisplayDevice: no mode set");

    if (!(m_flags & (kSettingMode | kFlushingPaused)))
        m_pDisplayModeOperations->DoFlushRect(x0, y0, x1, y1);
}

///////////////////////////////////////
//
// Add a pointer to the monitor set.  Monitored pointers are automatically
// invalidated/fixed-up across Lock/Unlock cycles.  Pointer must already
// be set relative to values passed back in SetMode().
//
// Return FALSE if the set monitor fails for some reason (like we pass
// in an invalid pointer, for instance)
//

STDMETHODIMP_(BOOL) cWinDisplayDevice::SetMonitor(tGrDispBits ** ppDispBits, eDisplayDeviceMonitorFlag action)
{
    cAutoDisplayMutex mutex(this);

    // If it's an enabling call and we're not going to the frame buffer, skip it
    // Disabling calls may be from a previous mode and should be passed to the more
    // complete logic.  At the beginning of a new indirect mode, set mode
    // ensures the existing monitored pointers are set to the indirection buffer.
    if (!m_pDisplayModeOperations) 
        return FALSE;

    // Only need to monitor if we're doing strict monitors or direct to screen.
    if (!(m_flags & kStrictMonitors))
        if ((action == kGrDispPtrMonitorOn) &&
            !(m_pDisplayModeOperations->GetModeInfoEx().flags & kGrModeIsDirect))
            return FALSE;

    DebugMsgEx3(PTRMONITOR, "cWinDisplayDevice::SetMonitor(0x%lx (0x%lx), %d)", ppDispBits, *ppDispBits, action);

    AssertMsg(m_pDisplayProvider, "SetMonitor outside initialized state");

    cRelocationSet<tGrDispBits> * pPtrMonitor;
    const tGrDispBits * const pDispBits = *ppDispBits;
    const unsigned uVideoBufferSize = (m_pModeInfo->rowBytes * m_pModeInfo->h);

    if (pDispBits >= m_pModeInfo->vbase && pDispBits < (uint8*) m_pModeInfo->vbase + uVideoBufferSize)
        pPtrMonitor = &m_PrimarySurfaceRelocationSet;

    else if (m_pModeInfo->vbase2 && pDispBits >= m_pModeInfo->vbase2 && pDispBits < (uint8*) m_pModeInfo->vbase2 + uVideoBufferSize)
        pPtrMonitor = &m_SecondarySurfaceRelocationSet;

    else
        pPtrMonitor = NULL;

    if (pPtrMonitor)
    {
        switch (action)
        {
            case kGrDispPtrMonitorOn:
                pPtrMonitor->Insert(ppDispBits);
                break;

            case kGrDispPtrMonitorOff:
                pPtrMonitor->Remove(ppDispBits);
                break;
        }
    }

    return !!pPtrMonitor;
}

///////////////////////////////////////
//
// Create a device-relative canvas
//

STDMETHODIMP cWinDisplayDevice::CreateBitmap(sGrDevBitmapDesc *, int /*flags*/, IDeviceBitmap ** ppResult)
{
    *ppResult = NULL;
    return E_NOTIMPL;
}

///////////////////////////////////////
//
// Locate an IGrCanvas for the given sGrCanvas, if any exists
//

STDMETHODIMP cWinDisplayDevice::FindBitmap(const sGrBitmap *, IDeviceBitmap ** ppResult)
{
    *ppResult = NULL;
    return E_NOTIMPL;
}

///////////////////////////////////////
//
//
//

STDMETHODIMP cWinDisplayDevice::GetRenderTargets(sGrRenderTargets *)
{
    return E_NOTIMPL;
}

///////////////////////////////////////

void cWinDisplayDevice::Sync2DPointers()
{
    BEGIN_DEBUG_MSG_EX(PTRMONITOR, "cWinDispDevice::Sync2DPointers()");
    m_PrimarySurfaceRelocationSet.SetBaseAddress(m_pModeInfo->vbase);
    m_SecondarySurfaceRelocationSet.SetBaseAddress(m_pModeInfo->vbase2);
    END_DEBUG;
}

///////////////////////////////////////

void cWinDisplayDevice::InvalidateBasePointers()
{
    m_pModeInfo->vbase = (uchar *)kPrimaryBogusBase;
    if (m_pModeInfo->vbase2)
        m_pModeInfo->vbase2 = (uchar *)kSecondaryBogusBase;
}

///////////////////////////////////////
//
// Temporarily break any lock on surfaces, returning lock count.
//

STDMETHODIMP_(int) cWinDisplayDevice::BreakLock()
{
    cAutoDisplayMutex mutex(this);

    int iReturn = GetLockCount();

    AssertMsg(m_MutexCount - 1 >= iReturn, "Lock count can never be less than mutex count");

    if (iReturn)
    {
        // We release the mutex for each call previously made to Lock()
        for (int i = 0; i < iReturn - 1; i++)
            ReleaseMutex();

        m_iLock = 1;
        Unlock();
    }

    return iReturn;
}

///////////////////////////////////////
//
// Restore broken lock
//

STDMETHODIMP_(void) cWinDisplayDevice::RestoreLock(int iToRestore)
{
    cAutoDisplayMutex mutex(this);

    for (int i = 0; i < iToRestore - 1; i++)
        WaitForMutex();

    DebugMsgTrue(m_iLock != 0, "Restoring lock, but lock/unlock not matched during break");
    if (iToRestore)
    {
        Lock();
        m_iLock += (iToRestore - 1);
    }
}

///////////////////////////////////////
//
// Query the lock state
//

STDMETHODIMP_(int) cWinDisplayDevice::IsLocked()
{
    return !!m_iLock;
}

///////////////////////////////////////
//
// Set the stored palette
//

void cWinDisplayDevice::SetStoredPaletteEntries(unsigned uStart, unsigned n, const unsigned char * pRGBs, int paletteFlags)
{
    if (!pRGBs)
        return;

    char unsigned (*pRGBArray)[3] = (char unsigned (*)[3])pRGBs;

    for (int i = uStart; i < uStart + n; i++)
    {
        gm_LogicalPalette.palPalEntry[i].peRed = pRGBArray[i][0];
        gm_LogicalPalette.palPalEntry[i].peGreen = pRGBArray[i][1];
        gm_LogicalPalette.palPalEntry[i].peBlue = pRGBArray[i][2];
        gm_LogicalPalette.palPalEntry[i].peFlags = (BYTE)paletteFlags;
    }
}

///////////////////////////////////////
//
// Get the stored palette
//

void cWinDisplayDevice::GetStoredPaletteEntries(unsigned uStart, unsigned n, unsigned char * pRGBs)
{
    if (!pRGBs)
        return;

    char unsigned (*pRGBArray)[3] = (char unsigned (*)[3]) pRGBs;

    for (int i = uStart; i < uStart + n; i++)
    {
        pRGBArray[i][0] = gm_LogicalPalette.palPalEntry[i].peRed;
        pRGBArray[i][1] = gm_LogicalPalette.palPalEntry[i].peGreen;
        pRGBArray[i][2] = gm_LogicalPalette.palPalEntry[i].peBlue;
    }
}

///////////////////////////////////////
//
// Using a temporary buffer, blit from the window's client area
// This is only used by Golf's mouse cursor, and only in windowed, indirect modes
//

void cWinDisplayDevice::DoCursorBltFromGDIScreen(sGrBitmap * pDestBitmap, int x, int y)
{
    // If our current scratch buffer isn't the right size (or we don't have one), get a new one
    if (m_CursorScratch.bitmap.bmWidth  != pDestBitmap->w ||
        m_CursorScratch.bitmap.bmHeight != pDestBitmap->h)
    {
        sGdiBitmapInfo gdiBitmapInfo(pDestBitmap->w, pDestBitmap->h, m_pDisplayModeOperations->GetSignificantDepth());

        if (m_CursorScratch.hBitmap)
            DeleteObject(m_CursorScratch.hBitmap);
        m_CursorScratch.hBitmap = CreateGDIBitmap(&gdiBitmapInfo);
        GetObject(m_CursorScratch.hBitmap, sizeof(BITMAP), &m_CursorScratch.bitmap);
    }

    // Blit from the screen context into the new bitmap
    GdiFlush();

    HWND    hwndClient       = m_pDisplayProvider->GetMainWnd();
    HDC     hdcClientArea    = GetDC(hwndClient);
    HDC     hdcCursorScratch = CreateCompatibleDC(NULL);
    HBITMAP hPreviousBitmap  = (HBITMAP)SelectObject(hdcCursorScratch, m_CursorScratch.hBitmap);

    BitBlt(hdcCursorScratch, 0, 0, pDestBitmap->w, pDestBitmap->h,
           hdcClientArea,    x, y,
           SRCCOPY);

    SelectObject(hdcCursorScratch, hPreviousBitmap);
    DeleteDC(hdcCursorScratch);
    ReleaseDC(hwndClient, hdcClientArea);

    // Now bit from the GDI bitmap to the LG bitmap
    grs_bitmap grBitmap;
    grs_canvas grCanvas;

    gr_init_bitmap(&grBitmap, (uchar *)m_CursorScratch.bitmap.bmBits, (m_pDisplayModeOperations->GetModeInfoEx().depth == 8) ? BMT_FLAT8 : BMT_FLAT16, 0, pDestBitmap->w, pDestBitmap->h);
    gr_make_canvas(pDestBitmap, &grCanvas);
    gr_push_canvas(&grCanvas);
    gr_bitmap(&grBitmap, 0, 0);
    gr_pop_canvas();
    gr_close_canvas(&grCanvas);
}

///////////////////////////////////////
//
// Using a temporary buffer, blit to the window's client area
// This is only used by Golf's mouse cursor, and only in windowed, indirect modes
//

void cWinDisplayDevice::DoCursorBltToGDIScreen(sGrBitmap * pSourceBitmap, int x, int y)
{
    // If our current scratch buffer isn't the right size (or we don't have one), get a new one
    if (m_CursorScratch.bitmap.bmWidth  != pSourceBitmap->w ||
        m_CursorScratch.bitmap.bmHeight != pSourceBitmap->h)
    {
        sGdiBitmapInfo gdiBitmapInfo(pSourceBitmap->w, pSourceBitmap->h, m_pDisplayModeOperations->GetSignificantDepth());

        if (m_CursorScratch.hBitmap)
            DeleteObject(m_CursorScratch.hBitmap);
        m_CursorScratch.hBitmap = CreateGDIBitmap(&gdiBitmapInfo);
        GetObject(m_CursorScratch.hBitmap, sizeof(BITMAP), &m_CursorScratch.bitmap);
    }

    // Start by filling the GDI bitmap with the on-screen stuff
    // @Note (toml 01-28-97): this is probably not needed for normal (non-transparent?) blits
    GdiFlush();

    HWND    hwndClient       = m_pDisplayProvider->GetMainWnd();
    HDC     hdcClientArea    = GetDC(hwndClient);
    HDC     hdcCursorScratch = CreateCompatibleDC(NULL);
    HBITMAP hPreviousBitmap  = (HBITMAP)SelectObject(hdcCursorScratch, m_CursorScratch.hBitmap);

    BitBlt(hdcCursorScratch, 0, 0, pSourceBitmap->w, pSourceBitmap->h,
           hdcClientArea,    x, y,
           SRCCOPY);

    // Now bit from the LG bitmap to the GDI bitmap
    grs_bitmap grBitmap;
    grs_canvas grCanvas;

    GetObject(m_CursorScratch.hBitmap, sizeof(BITMAP), &m_CursorScratch.bitmap);
    gr_init_bitmap(&grBitmap, (uchar *)m_CursorScratch.bitmap.bmBits, (m_pDisplayModeOperations->GetModeInfoEx().depth == 8) ? BMT_FLAT8 : BMT_FLAT16, 0, pSourceBitmap->w, pSourceBitmap->h);

    gr_make_canvas(&grBitmap, &grCanvas);
    gr_push_canvas(&grCanvas);
    gr_bitmap(pSourceBitmap, 0, 0);
    gr_pop_canvas();
    gr_close_canvas(&grCanvas);

    // Blit to the screen context from the new bitmap
    BitBlt(hdcClientArea,    x, y, pSourceBitmap->w, pSourceBitmap->h,
           hdcCursorScratch, 0, 0,
           SRCCOPY);

    SelectObject(hdcCursorScratch, hPreviousBitmap);
    DeleteDC(hdcCursorScratch);
    ReleaseDC(hwndClient, hdcClientArea);

    GdiFlush();
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cWinDisplayDevice::cIWinDisplayDeviceProxy
//
// Passes all calls to outer object
//

IMPLEMENT_DELEGATION(cWinDisplayDevice::cIWinDisplayDeviceProxy);

///////////////////////////////////////

STDMETHODIMP_(BOOL) cWinDisplayDevice::cIWinDisplayDeviceProxy::ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam, long * pRetVal)
{
    if (m_pOuter->m_pDisplayProvider && m_pOuter->m_pDisplayModeOperations)
        return m_pOuter->m_pDisplayProvider->DoProcessMessage(msg, wParam, lParam, pRetVal);
    return 0;
}

///////////////////////////////////////

STDMETHODIMP_(void) cWinDisplayDevice::cIWinDisplayDeviceProxy::GetWidthHeight(unsigned * pWidth, unsigned * pHeight)
{
    if (m_pOuter->GetMode(NULL) != GRD_MODES)
    {
        *pWidth = m_pOuter->GetCurrentModeInfo().w;
        *pHeight = m_pOuter->GetCurrentModeInfo().h;
    }
    else
    {
        *pWidth = GetSystemMetrics(SM_CXSCREEN);
        *pHeight = GetSystemMetrics(SM_CYSCREEN);
    }
}

///////////////////////////////////////

STDMETHODIMP_(void) cWinDisplayDevice::cIWinDisplayDeviceProxy::OnTaskSwitch(BOOL b)
{
    sWinDispDevCallbackInfo info;
    info.message = b ? kWinDispDevCallbackActive:kWinDispDevCallbackInactive;

    if (m_pOuter->m_pDisplayModeOperations)
        b = m_pOuter->m_pDisplayModeOperations->OnTaskSwitch(b);

    if (m_pTaskSwitchCallback != NULL) {
        m_pTaskSwitchCallback(&info);
        if (b) {
            info.message = kWinDispDevCallbackRepaint;
            info.r = NULL;
            m_pTaskSwitchCallback(&info);
        }
    }
}

///////////////////////////////////////

STDMETHODIMP_(void) cWinDisplayDevice::cIWinDisplayDeviceProxy::
        AddTaskSwitchCallback(WinDispDevCallbackFunc *callback)
{                         
    static int id=0;
    sWinDispDevCallbackInfo info;

    info.chain_info.id = ++id;
    info.chain_info.message = kCallbackChainAddFunc;
    info.message = kWinDispDevCallbackUpdateChain;


    if (m_pTaskSwitchCallback == NULL) {
        m_pTaskSwitchCallback = callback;
        info.chain_info.func = NULL;
    } else
        info.chain_info.func = (callback_chain_func *)callback;

    m_pTaskSwitchCallback(&info);
}

///////////////////////////////////////

STDMETHODIMP_(void) cWinDisplayDevice::cIWinDisplayDeviceProxy::
        RemoveTaskSwitchCallback(int id)
{                         
    sWinDispDevCallbackInfo info;
    info.chain_info.id = id;
    info.chain_info.message = kCallbackChainRemoveFunc;
    info.message = kWinDispDevCallbackUpdateChain;

    if (m_pTaskSwitchCallback == NULL) {
        Warning(("RemoveTaskSwitchCallback(): No callback to remove!\n"));
        return;
    }
    m_pTaskSwitchCallback(&info);
    if (info.chain_info.id == 0)
        m_pTaskSwitchCallback = (WinDispDevCallbackFunc *)info.chain_info.func;
}


///////////////////////////////////////

STDMETHODIMP_(BOOL) cWinDisplayDevice::cIWinDisplayDeviceProxy::GetDirectDraw(IDirectDraw ** ppDD)
{
    if (m_pOuter->m_pDisplayProvider)
        return m_pOuter->m_pDisplayProvider->DoGetDirectDraw(ppDD);
    return FALSE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cWinDisplayDevice::cIWinDisplayDeviceProxy::WaitForMutex()
{
    m_pOuter->WaitForMutex();
}

///////////////////////////////////////

STDMETHODIMP_(void) cWinDisplayDevice::cIWinDisplayDeviceProxy::ReleaseMutex()
{
    m_pOuter->ReleaseMutex();
}

///////////////////////////////////////

STDMETHODIMP_(void) cWinDisplayDevice::cIWinDisplayDeviceProxy::CursorBltFromGDIScreen(sGrBitmap * pBitmap, int x, int y)
{
    m_pOuter->DoCursorBltFromGDIScreen(pBitmap, x, y);
}

///////////////////////////////////////

STDMETHODIMP_(void) cWinDisplayDevice::cIWinDisplayDeviceProxy::CursorBltToGDIScreen(sGrBitmap * pBitmap, int x, int y)
{
    m_pOuter->DoCursorBltToGDIScreen(pBitmap, x, y);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cWinDisplayDevice::cIWinDisplayDeviceProxy::GetBitmapSurface(sGrBitmap * pBm, IDirectDrawSurface ** ppDDS)
{
    if (m_pOuter->m_pDisplayProvider)
        return m_pOuter->m_pDisplayProvider->DoGetBitmapSurface(pBm, ppDDS);
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
