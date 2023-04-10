///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/wddmode.cpp $
// $Author: KEVIN $
// $Date: 1998/04/14 15:02:23 $
// $Revision: 1.15 $
//

// Core headers
#include <windows.h>
#include <ddraw.h>

#include <lg.h>

// Component APIs
#include <comtools.h>
#include <dispapi.h>
#include <gshelapi.h>

// Application aggregate
#include <appagg.h>

// Other required LG libraries
#include <dev2d.h>

// Windows display implementation headers
#include <wdisp.h>
#include <wdspprov.h>
#include <wdspmode.h>
#include <wdcreate.h>

#include <wdispdd.h>
#include <wddmode.h>

#ifdef MONO_SPEW
#define put_mono(c) (*((uchar *)0xb0078)) = (c)
#else
#define put_mono(c)
#endif

///////////////////////////////////////////////////////////////////////////////

#ifndef DEBUG_SETMODE
#define DEBUG_SETMODE 0
#else
#pragma message ("DEBUG_SETMODE defined")
#endif

#ifndef DEBUG_PALETTE
#define DEBUG_PALETTE 0
#else
#pragma message ("DEBUG_PALETTE defined")
#endif

#if DEBUG_PALETTE
#define DebugDumpPalette(paletteEntries, uStart, n) \
{ \
  for (int i = uStart; i < n; i++) \
    DebugMsgEx4(PALETTE, "fLogicalPalette.palPalEntry[%d] (r,g,b) = (%d,%d,%d)", i, paletteEntries[i].peRed, paletteEntries[i].peGreen, paletteEntries[i].peBlue ); \
}
#else
#define DebugDumpPalette(p, uStart, n)
#endif

///////////////////////////////////////////////////////////////////////////////
//
// class cDDModeOpsBase
//

cDDModeOpsBase::~cDDModeOpsBase()
{
    GetDD()->FlipToGDISurface();
    SafeRelease(m_pSecondarySurface);
    SafeRelease(m_pPrimarySurface);
    SafeRelease(m_pPalette);
}

///////////////////////////////////////

BOOL cDDModeOpsBase::DoSetPalette(LOGPALETTE & logicalPalette, unsigned uStart, unsigned n)
{
    BEGIN_DEBUG_MSG_TRUE_EX(PALETTE, m_pPalette, "cDDProvider::DoSetPalette()");
    HRESULT result;
    if (m_pPalette != NULL)
    {
        DebugDumpPalette(logicalPalette.palPalEntry, uStart, n);

        int iLock = m_pDisplayDevice->BreakLock();
        RestoreSurfaces();
        result = m_pPalette->SetEntries(0,uStart,n,logicalPalette.palPalEntry);
        DebugMsgIfErr("DoSetPalette: SetEntries", result);
        m_pDisplayDevice->RestoreLock(iLock);
    }
    else
    {
        DebugMsgEx(PALETTE, "No palette yet");
    }

    return TRUE;
    END_DEBUG;
}

///////////////////////////////////////

void cDDModeOpsBase::GetSurfaces(IDirectDrawSurface ** ppPrimarySurface,
                                 IDirectDrawSurface ** ppSecondarySurface)

{
    if (ppPrimarySurface)
    {
        if (m_pPrimarySurface)
        {
            *ppPrimarySurface = m_pPrimarySurface;
            m_pPrimarySurface->AddRef();
        }
        else
            ppPrimarySurface = NULL;
    }
    
    if (ppSecondarySurface)
    {
        if (m_pSecondarySurface)
        {
            *ppSecondarySurface = m_pSecondarySurface;
            m_pSecondarySurface->AddRef();
        }
        else
            *ppSecondarySurface = NULL;
    }
}

///////////////////////////////////////

void cDDModeOpsBase::SetClipToWindow(BOOL fWantClipping)
{
    const BOOL fClippingNow = (m_Flags & kClippingToWindow);
    if (fWantClipping && !fClippingNow)
    {
        IDirectDrawClipper * pDirectDrawClipper;
        HRESULT result = GetDD()->CreateClipper(0, &pDirectDrawClipper, NULL);
        DebugMsgIfErr("DoSetPalette: SetEntries", result);
        if (result == S_OK)
        {
            pDirectDrawClipper->SetHWnd(0, m_pOuter->GetMainWnd());
            m_pPrimarySurface->SetClipper(pDirectDrawClipper);
            pDirectDrawClipper->Release();
            m_Flags |= kClippingToWindow;
        }
    }
    else if (!fWantClipping && fClippingNow)
    {
        m_pPrimarySurface->SetClipper(NULL);
        m_Flags &= ~kClippingToWindow;
    }
}

///////////////////////////////////////

void cDDModeOpsBase::SetModeInfoFromSurfaceDesc(const DDSURFACEDESC & renderTargetDesc,
                                                int flags,
                                                sGrModeCap * pReturnModeInfo)
{
    // Get the linear address of video
    DoLock(pReturnModeInfo);
    DoUnlock();

    // Fill out various info structures (2d and internal)
    AssertMsg(renderTargetDesc.dwFlags & DDSD_PIXELFORMAT, "Didn't get pixel format in descriptor!");

    if (flags & kGrModeIsWindowed)
    {
        m_ModeInfoEx.width  = pReturnModeInfo->w;
        m_ModeInfoEx.height = pReturnModeInfo->h;
    }
    else
    {
        m_ModeInfoEx.width  = pReturnModeInfo->w = (short) renderTargetDesc.dwWidth;
        m_ModeInfoEx.height = pReturnModeInfo->h = (short) renderTargetDesc.dwHeight;
    }

    m_ModeInfoEx.rowBytes  = pReturnModeInfo->rowBytes = renderTargetDesc.lPitch;

    m_ModeInfoEx.depth     = renderTargetDesc.ddpfPixelFormat.dwRGBBitCount;
    m_ModeInfoEx.mode      = 0;
    m_ModeInfoEx.flags     = flags;
    m_ModeInfoEx.redMask   = renderTargetDesc.ddpfPixelFormat.dwRBitMask;
    m_ModeInfoEx.greenMask = renderTargetDesc.ddpfPixelFormat.dwGBitMask;
    m_ModeInfoEx.blueMask  = renderTargetDesc.ddpfPixelFormat.dwBBitMask;

    int colorBitsPerPixel;

    if (m_ModeInfoEx.depth == 16)
    {
        if ((m_ModeInfoEx.redMask | m_ModeInfoEx.greenMask | m_ModeInfoEx.blueMask) & 0x8000)
            colorBitsPerPixel = 16;
        else
            colorBitsPerPixel = 15;
    }
    else
        colorBitsPerPixel = m_ModeInfoEx.depth;

    m_pDisplayDevice->ModeInfoToEnumMode(m_ModeInfoEx.width,
                                         m_ModeInfoEx.height,
                                         colorBitsPerPixel,
                                         &m_ModeInfoEx.mode);
    m_ModeInfoEx.flags |= gr_mode_info_from_mode(m_ModeInfoEx.mode)->flags;
}

///////////////////////////////////////

BOOL cDDModeOpsBase::WipeSurface(IDirectDrawSurface *pdds)
{
    if (pdds)
    {
        BEGIN_DEBUG_MSG_EX1(VERBOSE, "WipeSurface(0x%lx)", pdds);

        DDSURFACEDESC surfaceDesc;
        surfaceDesc.dwSize = sizeof(surfaceDesc);
        HRESULT result = pdds->Lock(NULL, &surfaceDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);
        DebugMsgIfErr("WipeSurface Lock failed.", result);
        if (result != DD_OK)
            return FALSE;

        uchar * pRow = (uchar*)surfaceDesc.lpSurface;
        for (int y=0; y < surfaceDesc.dwHeight; y++)
        {
            memset(pRow, 0, surfaceDesc.dwWidth * (surfaceDesc.ddpfPixelFormat.dwRGBBitCount / 8));
            pRow += surfaceDesc.lPitch;
        }

        pdds->Unlock(NULL);

        END_DEBUG;
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// class cDirectDDModeOps
//

cDirectDDModeOps::~cDirectDDModeOps()
{
    // Jacobson, 3-4-96
    // Release the pause surface if it exists (it shouldn't usually)
    SafeRelease(m_pPauseSurface);
}

BOOL cDirectDDModeOps::DoStatVTrace()
{
    HRESULT result;
    BOOL bResult;
    result = GetDD()->GetVerticalBlankStatus(&bResult); // force synchoniousity
    DebugMsgIfErr("DoSetMode: GetVerticalBlankStatus", result);
    return bResult;
}

///////////////////////////////////////

BOOL cDirectDDModeOps::StartMode(const sGrModeInfo & modeInfo,
                                 int flags,
                                 sGrModeCap * pReturnModeInfo)
{
    HRESULT result;
    BEGIN_DEBUG_MSG_EX(SETMODE, "cDirectDDModeOps::StartMode()");

    // Release the previous mode, if any
    DebugMsgEx(SETMODE, "Release previous surfaces, if any,,,");
    SafeRelease(m_pPrimarySurface);          // Sets m_pPrimarySurface to null
    SafeRelease(m_pSecondarySurface);

    // If we're not running in a window, set the screen display mode
    if (!(GetCoopFlags() & DDSCL_NORMAL))
    {
        DebugMsgEx(SETMODE, "Setting display mode...");
        int realBitsPerPixel = (modeInfo.bitDepth != 15) ? modeInfo.bitDepth : 16;
        result = SetDisplayMode(modeInfo.w, modeInfo.h, realBitsPerPixel);

        if (result != DD_OK)
        {
            DebugMsgIfErr("DoSetMode: SetDisplayMode", result);
            return FALSE;
        }
    }
    else
        DebugMsgEx(SETMODE, "Didn't set display mode...");

    pReturnModeInfo->w = modeInfo.w;
    pReturnModeInfo->h = modeInfo.h;

    // Create the new surface(s)
    const BOOL fAttemptFlippable = (m_pDisplayDevice->GetCreateFlags() & kDispAttemptFlippable) && !(GetCoopFlags() & DDSCL_NORMAL);
    const BOOL fAttempt3D = (m_pDisplayDevice->GetCreateFlags() & kDispAttempt3D);

    DDSURFACEDESC surfaceDesc;
    surfaceDesc.dwSize = sizeof(surfaceDesc);

    DebugMsgEx(SETMODE, "Creating the surface(s)...");

    // Try flippable, if requested...
    DWORD caps = DDSCAPS_PRIMARYSURFACE;
    if (fAttempt3D)
        caps |= DDSCAPS_3DDEVICE;
    if (fAttemptFlippable)
    {
        DebugMsgEx(SETMODE, "Attempting to create flippable surface...");
        surfaceDesc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
        surfaceDesc.ddsCaps.dwCaps = caps | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_VIDEOMEMORY;
        surfaceDesc.dwBackBufferCount = 1;

        result = GetDD()->CreateSurface(&surfaceDesc, &m_pPrimarySurface, NULL);
        DebugMsgIfErr("DoSetMode: CreateSurface", result);
        // Fall through in event of failure
    }

    // If we either failed or didn't want to get flippable surface, create a non-flippable one
    if (!m_pPrimarySurface)
    {
        surfaceDesc.dwFlags = DDSD_CAPS;
        surfaceDesc.ddsCaps.dwCaps = caps;

        result = GetDD()->CreateSurface(&surfaceDesc, &m_pPrimarySurface, NULL);
        if (result != DD_OK)
        {
            DebugMsgIfErr("DoSetMode: CreateSurface", result);
            return FALSE;
        }
    }

    // Make sure surfaces were not discarded, or are restored
    RestoreSurfaces();

    if (modeInfo.bitDepth == 8 && m_pPalette)
    {
        // Set the palette for the new surface(s)
        DebugMsgEx(SETMODE, "Setting the palettes...");
        result = m_pPrimarySurface->SetPalette(m_pPalette);
        DebugMsgIfErr("StartMode: SetPalette", result);

        // Set the entries of the palette
        DebugMsgEx(SETMODE, "Setting the palette entries...");
        result = m_pPalette->SetEntries(0, 0, 256, GetPaletteEntries());
        DebugMsgIfErr("StartMode: SetEntries", result);

        if (result != DD_OK)
            return FALSE;
    }

    // Jacobson, 3-5-96
    // Now confirm that we're actually able to perform direct writes
    // and that this is a valid mode by trying to lock the surface for
    // writing.

    result = m_pPrimarySurface->Lock(NULL, &surfaceDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);
    DebugMsgIfErr("StartMode: Direct write test failed. Can't do direct mode.",result);

    if (result == DDERR_SURFACELOST)
    {
        RestoreSurfaces();
        result = m_pPrimarySurface->Lock(NULL, &surfaceDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);
    }

    if (result != DD_OK)
    {
        CriticalMsg1("Failed to lock primary DirectDraw surface (%s)", WhatDDError(result));
        return FALSE;
    }
    else
        m_pPrimarySurface->Unlock(NULL);

    // Get the back buffer, if any.  Store pointer in secondary surface
    DDSCAPS ddsCaps;
    ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
    result = m_pPrimarySurface->GetAttachedSurface(&ddsCaps, &m_pSecondarySurface);

    // DebugMsgIfErr("DoSetMode: Couldn't Find a back buffer", result);

    // Get the descriptors
    DebugMsgEx(SETMODE, "Getting the descriptors...");
    m_PrimaryDesc.dwSize = sizeof(m_PrimaryDesc);
    result = m_pPrimarySurface->GetSurfaceDesc(&m_PrimaryDesc);

    if (result != DD_OK)
    {
        DebugMsgIfErr("DoSetMode: GetSurfaceDesc", result);
        return FALSE;
    }

    DebugMsgDesc(m_PrimaryDesc);

    if (m_pSecondarySurface)
    {
        // Get the secondary descriptor
        m_SecondaryDesc.dwSize = sizeof(m_SecondaryDesc);
        result = m_pSecondarySurface->GetSurfaceDesc(&m_SecondaryDesc);

        if (result != DD_OK)
        {
            DebugMsgIfErr("DoSetMode::GetSurfaceDesc", result);
            return FALSE;
        }

        DebugMsgDesc(m_SecondaryDesc);
    }

    SetModeInfoFromSurfaceDesc((!m_pSecondarySurface) ? m_PrimaryDesc : m_SecondaryDesc,
                               (flags & kGrSetWindowed) ? (kGrModeIsWindowed | kGrModeIsDirect) : kGrModeIsDirect,
                               pReturnModeInfo);

    DebugMsgEx(SETMODE, "Clearing the new surface...");
    if (!(m_ModeInfoEx.flags & kGrModeIsWindowed))
    {
        WipeSurface(m_pPrimarySurface);
        WipeSurface(m_pSecondarySurface);
    }

    //
    // @Note (toml 06-03-96): The flipping model used in this implementation
    // essentially hides the flipping from the client.  If a back buffer is
    // available, the game is pointed at that.  Otherwise, it points at
    // the front, visible buffer.
    //
    pReturnModeInfo->vbase2 = NULL;

    return TRUE;
    END_DEBUG;
}

static void lock_bitmap_from_surface(grs_bitmap *bm, LPDIRECTDRAWSURFACE surf)
{
   DDSURFACEDESC surfaceDesc;
   HRESULT result;

   surfaceDesc.dwSize = sizeof(surfaceDesc);

   result = surf->GetSurfaceDesc(&surfaceDesc);

   DebugMsgIfErr("lock_bitmap_from_surface(): GetSurfaceDesc failed.", result);

   if (surfaceDesc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
       // Hack Hack Hack Hack!!!!
       memcpy(bm, grd_screen_canvas, sizeof(*bm));
   else
       gr_init_bitmap(bm, NULL, 
           BMT_DEVICE_VIDMEM_TO_FLAT, 0,
           bm->w, bm->h);

   result = surf->Lock(NULL, &surfaceDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);

   DebugMsgIfErr("lock_bitmap_from_surface(): Lock failed.", result);

   bm->bits = (uchar *)surfaceDesc.lpSurface;
}
///////////////////////////////////////


// when our apps can repaint themselves, we should get rid of this
#define PAUSE_BLIT

void cDirectDDModeOps::PauseBlit(LPDIRECTDRAWSURFACE pDest,LPDIRECTDRAWSURFACE pSource)
{
#ifdef PAUSE_BLIT
    // Jacobson, 3-4-96
    // This function blits back and forth from the pause blit buffer.

    BEGIN_DEBUG_MSG2("cDirectDDModeOps::PauseBlit(0x%x, 0x%x)", pDest, pSource);

    if (pDest && pSource)
    {

        // Blit from secondary surface to primary surface
        RECT blitRect;

        blitRect.left = 0;
        blitRect.top = 0;
        blitRect.right = m_PauseDesc.dwWidth;
        blitRect.bottom = m_PauseDesc.dwHeight;

        int iLock = m_pDisplayDevice->BreakLock();
        HRESULT result = pDest->Blt(&blitRect, pSource, &blitRect, DDBLT_WAIT, NULL);

        if (result == DDERR_NOBLTHW) { // use 2d to blit
            grs_bitmap src_bm, dst_bm;
            grs_canvas dst_cnv;
            src_bm.w = dst_bm.w = (short )m_PauseDesc.dwWidth;
            src_bm.h = dst_bm.h = (short )m_PauseDesc.dwHeight;

            lock_bitmap_from_surface(&dst_bm, pDest);
            lock_bitmap_from_surface(&src_bm, pSource);

            gr_make_canvas(&dst_bm, &dst_cnv);
            gr_push_canvas(&dst_cnv);
            gr_bitmap(&src_bm, 0, 0);
            gr_pop_canvas();
            pSource->Unlock((void *)src_bm.bits);
            pDest->Unlock((void *)dst_bm.bits);
        }

        m_pDisplayDevice->RestoreLock(iLock);

        DebugMsgIfErr("Pause Blit -> 1", result);
    }

    END_DEBUG;
#endif
}

///////////////////////////////////////

BOOL cDirectDDModeOps::OnTaskSwitch(BOOL bActive)
{
    static BOOL bAppActive = TRUE;
    static BOOL bSurfaceLost = FALSE;

    // Jacobson, 3/4/96.
    // Ok, here is where we copy the current contents of the screen into
    // a temporarily allocated back buffer DirectDraw surface.

    BEGIN_DEBUG_MSG1("cDirectDDModeOps::OnTaskSwitch(%d)", !!bActive);

    // Check for deactivation request (and that we're in a deactivated state)
    if (bActive == FALSE)
    {
        if (!bAppActive)
            return FALSE;

        bAppActive = FALSE;

#ifdef PAUSE_BLIT
        RestoreSurfaces();

        // If the current visible or back buffer surface is lost, we're totally hosed.
        // The contents are therefore lost and we can't do anything.
        // In this case, we'll barf a warning and tell the app to repaint when we regain focus.
        if (m_pPrimarySurface->IsLost())
        {
            bSurfaceLost = TRUE;
            Warning(("Activation error: primary surface is lost!\n"));
        }
        if (m_pSecondarySurface&&m_pSecondarySurface->IsLost()) {
            bSurfaceLost = TRUE;
            Warning(("Activation error: secondary (back buffer) surface is lost!\n"));
        }

        if (bSurfaceLost)
            return FALSE;

        // Create a new offscreen DirectDraw surface to copy the curent
        // contents of the visible surface into.
        memset(&m_PauseDesc, 0, sizeof(m_PauseDesc));
        m_PauseDesc.dwSize   = sizeof(m_PauseDesc);
        m_PauseDesc.dwFlags  = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
        m_PauseDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
        m_PauseDesc.dwWidth  = m_PrimaryDesc.dwWidth;
        m_PauseDesc.dwHeight = m_PrimaryDesc.dwHeight;

        HRESULT result = GetDD()->CreateSurface(&m_PauseDesc, &m_pPauseSurface, NULL);
        DebugMsgIfErr("CreateSurface (pause surface)", result);

        // Now that we've got a buffer, copy the contents of the visible
        // buffer into it.
        if (result == DD_OK)
        PauseBlit(m_pPauseSurface, m_pPrimarySurface);
        if (m_pSecondarySurface) 
        {
            result = GetDD()->CreateSurface(&m_PauseDesc, &m_pPauseSurface2, NULL);
            DebugMsgIfErr("CreateSurface (pause surface 2)", result);

            // Now that we've got a buffer, copy the contents of the visible
            // buffer into it.
            if (result == DD_OK)
            PauseBlit(m_pPauseSurface2, m_pSecondarySurface);
        }
#endif
    }
    else
    {
        if (bAppActive)
            return FALSE;

        bAppActive = TRUE;

        RestoreSurfaces();

#ifdef PAUSE_BLIT
        if (bSurfaceLost) {
            bSurfaceLost = FALSE;
            return TRUE;
        }

        if (m_pPauseSurface==NULL)
            return TRUE;

        PauseBlit(m_pPrimarySurface, m_pPauseSurface);
        SafeRelease(m_pPauseSurface);
        if (m_pSecondarySurface) {
            if (m_pPauseSurface2 == NULL)
                return TRUE;

            PauseBlit(m_pSecondarySurface, m_pPauseSurface2);
            SafeRelease(m_pPauseSurface2);
        }
#else
        return TRUE;
#endif
    }

    return FALSE;
    END_DEBUG;
}

///////////////////////////////////////

BOOL cDirectDDModeOps::DoPageFlip()
{
    DebugMsgEx(VERBOSE, "PageFlip");

    if (m_pSecondarySurface)
    {
        // Make sure surfaces were not discarded, or are restored
        RestoreSurfaces();

        int iLock = m_pDisplayDevice->BreakLock();
        while (1)
        {
            HRESULT result;
            result = m_pPrimarySurface->Flip(NULL, DDFLIP_WAIT); // force synchoniousity (Alas, I miss noah's splleing...)
            if (result == DD_OK)
                break;

            if (result != DDERR_WASSTILLDRAWING)
            {
                DebugMsgIfErr("DoPageFlip: Flip", result);
                CriticalMsg("Flip failed!");
                break;
            }
        }
        m_pDisplayDevice->RestoreLock(iLock);
    }
    return NO_ERROR;
}

///////////////////////////////////////

BOOL cDirectDDModeOps::DoLock(sGrModeCap * pReturnModeInfo)
{
#ifndef SHIP
    static BOOL bShowLockFailWarning = TRUE;
#endif

    AssertMsg(m_pPrimarySurface != NULL, "Attempt to lock uninitialized display device");
    DebugMsgEx(VERBOSE, "cDirectDDModeOps::DoLock()");

    // Make sure surfaces were not discarded, or are restored
    RestoreSurfaces();

    IDirectDrawSurface * pClientTargetSurface = (m_pSecondarySurface) ? m_pSecondarySurface : m_pPrimarySurface;
    HRESULT result;
    DDSURFACEDESC surfaceDesc;
    surfaceDesc.dwSize = sizeof(surfaceDesc);

    put_mono('L');
    result = pClientTargetSurface->Lock(NULL, &surfaceDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);

    if (result == DDERR_SURFACELOST)
    {
        put_mono('?');
        RestoreSurfaces();
        result = pClientTargetSurface->Lock(NULL, &surfaceDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);
    }
    put_mono('.');

    if (result != DD_OK)
    {
#ifndef SHIP
        if (bShowLockFailWarning)
        {
            Warning(("Failed to lock primary DirectDraw surface (%s)\n", WhatDDError(result)));
            bShowLockFailWarning = FALSE;
        }
#endif
        // No way to recover...
        return FALSE;
    }
#ifndef SHIP
    else
        bShowLockFailWarning = TRUE;
#endif


    if (!(m_ModeInfoEx.flags & kGrModeIsWindowed))
    {
        pReturnModeInfo->vbase = (uchar *)surfaceDesc.lpSurface;
    }
    else
    {
        POINT originClient = { 0, 0 };
        ClientToScreen(m_pOuter->GetMainWnd(), &originClient);

        pReturnModeInfo->vbase = (((uchar *)surfaceDesc.lpSurface) + (originClient.y * m_ModeInfoEx.rowBytes)) + (originClient.x * (m_ModeInfoEx.depth >> 3));
    }

    pReturnModeInfo->vbase2   = NULL;

    return !!pReturnModeInfo->vbase;
}

///////////////////////////////////////

BOOL cDirectDDModeOps::DoUnlock()
{
    DebugMsgEx(VERBOSE, "cDirectDDModeOps::DoUnlock()");

    // Make sure surfaces were not discarded, or are restored
    RestoreSurfaces();

    HRESULT result;

    IDirectDrawSurface * pClientTargetSurface = (m_pSecondarySurface) ? m_pSecondarySurface : m_pPrimarySurface;
    if (pClientTargetSurface != NULL)
    {
        result = pClientTargetSurface->Unlock(NULL);
        DebugMsgIfErr("DoLock::Unlock client surface", result);

        if (result != DD_OK)
        {
            CriticalMsg1("Lock count of DirectDraw surfaces appears corrupted (%s)", WhatDDError(result));
            // No way to recover...
            SafeRelease(m_pSecondarySurface);
            SafeRelease(m_pPrimarySurface);
            SafeRelease(m_pPalette);
            exit(0);
        }
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//
// class cPhoney512ModeOps
//

BOOL cPhoney512ModeOps::StartMode(const sGrModeInfo & modeInfo, int flags, sGrModeCap * pReturnModeInfo)
{
    sGrModeInfo realModeInfo = modeInfo;

    realModeInfo.w = 640;
    realModeInfo.h = 480;

    if (cDirectDDModeOps::StartMode(realModeInfo, flags, pReturnModeInfo))
    {
        pReturnModeInfo->w = modeInfo.w;
        pReturnModeInfo->h = modeInfo.h;
        return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////

BOOL cPhoney512ModeOps::DoLock(sGrModeCap * pReturnModeInfo)
{
    if (cDirectDDModeOps::DoLock(pReturnModeInfo))
    {
        pReturnModeInfo->vbase = (((uchar *)pReturnModeInfo->vbase) + (48 * m_ModeInfoEx.rowBytes)) + (64 * (m_ModeInfoEx.depth >> 3));
        return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////

BOOL cPhoney512ModeOps::DoUnlock()
{
    return cDirectDDModeOps::DoUnlock();
}

///////////////////////////////////////////////////////////////////////////////
//
// class cOffVideoDDModeOps
//

BOOL cOffVideoDDModeOps::StartMode(const sGrModeInfo & modeInfo, int flags, sGrModeCap * pReturnModeInfo)
{
    HRESULT result;
    BEGIN_DEBUG_MSG("cOffVideoDDModeOps::StartMode()");

    const int realBitsPerPixel = (modeInfo.bitDepth != 15) ? modeInfo.bitDepth : 16;

    // Release the previous mode, if any
    DebugMsgEx(SETMODE, "Releasing previous surfaces, if any...");
    SafeRelease(m_pPrimarySurface);          // Sets m_pPrimarySurface to null
    SafeRelease(m_pSecondarySurface);

    // If we're not running in a window, set the screen display mode to the best available
    if (!(GetCoopFlags() & DDSCL_NORMAL))
    {
        int targetMode = -1;

        // First look for an exact match, if we're not trying a ModeX mode
        if (modeInfo.w > 400)
        {
            targetMode = cDisplayDevice::GetAvailableMode(modeInfo.w, modeInfo.h, modeInfo.bitDepth);
            DebugMsgTrueEx(SETMODE, targetMode != -1, "Picked exact match");
        }

        // Next try a perfect doubling
        if (targetMode == -1)
        {
            targetMode = cDisplayDevice::GetAvailableMode(modeInfo.w * 2, modeInfo.h * 2, modeInfo.bitDepth);
            DebugMsgTrueEx(SETMODE, targetMode != -1, "Picked double w and double h");
        }

        // Next try a double of the horizontal axis
        if (targetMode == -1)
        {
            targetMode = cDisplayDevice::GetAvailableMode(modeInfo.w * 2, modeInfo.h, modeInfo.bitDepth);
            DebugMsgTrueEx(SETMODE, targetMode != -1, "Picked double w and double h");
        }

        // Finally, just choose 640x480
        if (targetMode == -1)
        {
            targetMode = cDisplayDevice::GetAvailableMode(640, 480, modeInfo.bitDepth);
            DebugMsgTrueEx(SETMODE, targetMode != -1, "Picked highest res");
        }

        if (targetMode == -1)
            return FALSE;

        // Now set the mode
        const sGrModeInfo & targetModeInfo = cDisplayDevice::EnumModeToModeInfo(targetMode);
        result = SetDisplayMode(targetModeInfo.w, targetModeInfo.h, targetModeInfo.bitDepth);

        if (result != DD_OK)
        {
            DebugMsgIfErr("DoSetMode::SetDisplayMode", result);
            return FALSE;
        }
    }

    // Create the primary ("real") surface
    DebugMsgEx(SETMODE, "Creating primary surface...");

    DDSURFACEDESC surfaceDesc;
    memset(&surfaceDesc, 0, sizeof(surfaceDesc));
    surfaceDesc.dwSize = sizeof(surfaceDesc);

    surfaceDesc.dwFlags = DDSD_CAPS;
    surfaceDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    result = GetDD()->CreateSurface(&surfaceDesc, &m_pPrimarySurface, NULL);
    if (result != DD_OK)
    {
        DebugMsgIfErr("DoSetMode::CreateSurface (primary)", result);
        return FALSE;
    }

    // Get the primary descriptor
    m_PrimaryDesc.dwSize = sizeof(m_PrimaryDesc);
    result = m_pPrimarySurface->GetSurfaceDesc(&m_PrimaryDesc);

    if (result != DD_OK)
    {
        DebugMsgIfErr("DoSetMode::GetSurfaceDesc", result);
        return FALSE;
    }

    DebugMsgDesc(m_PrimaryDesc);

    // Create the secondary ("spoofed") surface
    //
    // What about a 2 surface flip-on-endframe approach?
    //
    DebugMsgEx(SETMODE, "Creating secondary surface...");

    memset(&surfaceDesc, 0, sizeof(surfaceDesc));
    surfaceDesc.dwSize = sizeof(surfaceDesc);
    surfaceDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    surfaceDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

    surfaceDesc.dwWidth = modeInfo.w;
    surfaceDesc.dwHeight = modeInfo.h;

    surfaceDesc.dwFlags |= DDSD_PIXELFORMAT;
    surfaceDesc.ddpfPixelFormat.dwSize = sizeof(surfaceDesc.ddpfPixelFormat);
    result = m_pPrimarySurface->GetPixelFormat(&surfaceDesc.ddpfPixelFormat);
    DebugMsgIfErr("m_pPrimarySurface->GetPixelFormat()", result);

    result = GetDD()->CreateSurface(&surfaceDesc, &m_pSecondarySurface, NULL);
    DebugMsgIfErr("DoSetMode::CreateSurface (secondary 1)", result);

    if (result != DD_OK)
    {
        Warning(("Failed to create offscreen DirectDraw surface (%d: %s)\n", result, WhatDDError(result)));
        return FALSE;
    }

    // Get the secondary descriptor
    m_SecondaryDesc.dwSize = sizeof(m_SecondaryDesc);
    result = m_pSecondarySurface->GetSurfaceDesc(&m_SecondaryDesc);

    if (result != DD_OK)
    {
        DebugMsgIfErr("DoSetMode::GetSurfaceDesc", result);
        return FALSE;
    }

    DebugMsgDesc(m_SecondaryDesc);

    // Store info about the render target
    SetModeInfoFromSurfaceDesc(m_SecondaryDesc,
                               (flags & kGrSetWindowed) ? kGrModeIsWindowed : 0,
                               pReturnModeInfo);

    // Make sure surfaces were not discarded, or are restored
    RestoreSurfaces();

    // Set the palette for the new surface(s)
    if (m_pPalette)
    {
        DebugMsgEx(SETMODE, "Setting palettes...");
        result = m_pPrimarySurface->SetPalette(m_pPalette);
        DebugMsgIfErr("DoSetMode::SetPalette (primary)", result);

        // Set the entries of the palette
        result = m_pPalette->SetEntries(0, 0, 256, GetPaletteEntries());
    }


    if (result != DD_OK)
    {
        DebugMsgIfErr("DoSetMode::SetEntries", result);
        return FALSE;
    }

    // Note: The paint request to the primary surface may fail for
    // certain hardware boards that are unable to lock the primary
    // surface. But that's ok here.
    if (!(m_ModeInfoEx.flags & kGrModeIsWindowed))
    {
        if (!IsModeX())
            WipeSurface(m_pPrimarySurface);
        WipeSurface(m_pSecondarySurface);
    }
    else
        SetClipToWindow(TRUE);

    // Get the linear address of the video
    pReturnModeInfo->vbase2   = NULL;

    return TRUE;
    END_DEBUG;
}

///////////////////////////////////////

void cOffVideoDDModeOps::DoFlushRect(int x0Source, int y0Source, int x1Source, int y1Source)
{
    DebugMsgEx(VERBOSE, "DoFlushRect()");

    // Make sure surfaces were not discarded, or are restored
    RestoreSurfaces();

    // Blit from secondary surface to primary surface
    RECT sourceRect;
    RECT destRect;

    // Adjust rectangle to look good when window is halved or doubled
    sourceRect.left   = x0Source - (x0Source & 1);
    sourceRect.top    = y0Source - (y0Source & 1);
    sourceRect.right  = x1Source + (x1Source & 1);
    sourceRect.bottom = y1Source + (y1Source & 1);

    // Clip the rectangles
    if (sourceRect.left < 0)
        sourceRect.left = 0;
    if (sourceRect.top < 0)
        sourceRect.top = 0;
    if (sourceRect.right > m_ModeInfoEx.width)
        sourceRect.right = m_ModeInfoEx.width;
    if (sourceRect.bottom > m_ModeInfoEx.height)
        sourceRect.bottom = m_ModeInfoEx.height;

    if (sourceRect.right - sourceRect.left > 0 && sourceRect.bottom - sourceRect.top > 0)
    {
        destRect.left   = Scale(sourceRect.left);
        destRect.top    = Scale(sourceRect.top);
        destRect.right  = Scale(sourceRect.right);
        destRect.bottom = Scale(sourceRect.bottom);

        // Move destination to right place on screen
        if (m_ModeInfoEx.flags & kGrModeIsWindowed)
        {
            POINT originClient = { 0, 0 };
            ClientToScreen(m_pOuter->GetMainWnd(), &originClient);

            destRect.left   += originClient.x;
            destRect.top    += originClient.y;
            destRect.right  += originClient.x;
            destRect.bottom += originClient.y;
        }
        else
        {
            const int xScale = (m_PrimaryDesc.dwWidth / m_SecondaryDesc.dwWidth) - 1;
            const int yScale = (m_PrimaryDesc.dwHeight / m_SecondaryDesc.dwHeight) - 1;

            if (xScale < 0)
            {
                destRect.left >>= -xScale;
                destRect.right >>= -xScale;
            }
            else
            {
                destRect.left <<= xScale;
                destRect.right <<= xScale;
            }

            if (yScale < 0)
            {
                destRect.top >>= -yScale;
                destRect.bottom >>= -yScale;
            }
            else
            {
                destRect.top <<= yScale;
                destRect.bottom <<= yScale;
            }
        }

        // Now blit!
        int iLock = m_pDisplayDevice->BreakLock();

        // Hack to address jerky Windows mouse cursor (see http://www.microsoft.com/mediadev/overviews/winfaq.htm (toml 12-30-96))
        {
            RECT rectClient;
            POINT pt;

            GetClientRect(m_pOuter->GetMainWnd(), &rectClient);
            MapWindowPoints(m_pOuter->GetMainWnd(), NULL, (POINT *) &rectClient, 2);
            GetCursorPos(&pt);

            if (!PtInRect(&rectClient, pt))
            {
                SetCursorPos(pt.x, pt.y);
            }
        }

        HRESULT result = m_pPrimarySurface->Blt(&destRect, m_pSecondarySurface, &sourceRect, DDBLT_WAIT, NULL);
        if (result == DDERR_SURFACELOST)
        {
            RestoreSurfaces();
            result = m_pPrimarySurface->Blt(&destRect, m_pSecondarySurface, &sourceRect, DDBLT_WAIT, NULL);
        }
        m_pDisplayDevice->RestoreLock(iLock);

        DebugMsgIfErr("Blit SecondaryBuffer to Primary", result);
    }
}

///////////////////////////////////////

void cOffVideoDDModeOps::DoFlush()
{
    DoFlushRect(0, 0, m_SecondaryDesc.dwWidth, m_SecondaryDesc.dwHeight);
}

///////////////////////////////////////

BOOL cOffVideoDDModeOps::DoLock(sGrModeCap * pReturnModeInfo)
{
    AssertMsg(m_pPrimarySurface != NULL, "Attempt to lock uninitialized display device");
    AssertMsg(m_pSecondarySurface != NULL, "Attempt to lock uninitialized display device");

    DebugMsgEx(VERBOSE, "cOffVideoDDModeOps::DoLock()");

    // Make sure surfaces were not discarded, or are restored
    RestoreSurfaces();

    HRESULT result;
    DDSURFACEDESC surfaceDesc;
    memset(&surfaceDesc, 0, sizeof(surfaceDesc));
    surfaceDesc.dwSize = sizeof(surfaceDesc);

    result = m_pSecondarySurface->Lock(NULL, &surfaceDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);
#if 1
    m_pSecondarySurface->Unlock(NULL);
#endif
    DebugMsgIfErr("DoLock::Lock SecondaryBuffer", result);

    if (result == DDERR_SURFACELOST)
    {
        RestoreSurfaces();
        result = m_pSecondarySurface->Lock(NULL, &surfaceDesc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL);
#if 1
        m_pSecondarySurface->Unlock(NULL);
#endif
    }

    if (result != DD_OK)
    {
        CriticalMsg("Lock count of DirectDraw surfaces appears corrupted");
        // No way to recover...
        SafeRelease(m_pSecondarySurface);
        SafeRelease(m_pPrimarySurface);
        SafeRelease(m_pPalette);
        exit(0);
    }
    pReturnModeInfo->vbase    = (uchar *)surfaceDesc.lpSurface;
    pReturnModeInfo->vbase2   = NULL;

    return !!pReturnModeInfo->vbase;
}

///////////////////////////////////////

BOOL cOffVideoDDModeOps::DoUnlock()
{
    DebugMsgEx(VERBOSE, "cOffVideoDDModeOps::DoUnlock()");

    // Make sure surfaces were not discarded, or are restored
    RestoreSurfaces();

#if 0
    HRESULT result;
    if (m_pSecondarySurface != NULL)
    {
        result = m_pSecondarySurface->Unlock(NULL); //&m_PrimaryDesc);
        DebugMsgIfErr("DoLock::Unlock BackBuffer", result);
        if (result != DD_OK)
        {
            CriticalMsg("Lock count of DirectDraw surfaces appears corrupted");
            // No way to recover...
            SafeRelease(m_pSecondarySurface);
            SafeRelease(m_pPrimarySurface);
            SafeRelease(m_pPalette);
            exit(0);
        }
    }
#endif
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
