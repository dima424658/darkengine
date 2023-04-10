///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/dispbase.cpp $
// $Author: KEVIN $
// $Date: 1998/01/12 15:44:28 $
// $Revision: 1.7 $
//

#include <lg.h>
#include <dispbase.h>
#include <dev2d.h>

#if defined(__WATCOMC__)
    #pragma off (unreferenced)
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDisplayDevice
//

///////////////////////////////////////
//
// Pre-fab COM implementations
//
IMPLEMENT_SIMPLE_AGGREGATION_SELF_DELETE(cDisplayDevice);

///////////////////////////////////////

cDisplayDevice::cDisplayDevice(IUnknown * pOuterUnknown, eDisplayDeviceKind kind, int flags)
  : m_flags(0)
{
    // Add internal components to outer aggregate...
    INIT_AGGREGATION_1( pOuterUnknown,
                        IID_IDisplayDevice, this,
                        kPriorityLibrary,
                        NULL );

}

///////////////////////////////////////

cDisplayDevice::~cDisplayDevice()
{
}

///////////////////////////////////////
//
// Query kind
//

STDMETHODIMP_(eDisplayDeviceKind) cDisplayDevice::GetKind()
{
    eDisplayDeviceKind kind;
    GetKind2(&kind, NULL, NULL);
    return kind;
}

///////////////////////////////////////
//
// Query kind, flags, and ddraw guid
//

STDMETHODIMP_(void) cDisplayDevice::GetKind2(eDisplayDeviceKind *pKind, int *pFlags, GUID **ppDDrawGuid)
{
    if (pKind != NULL)
        *pKind = kDispFullScreen;
    if (pFlags != NULL)
        *pFlags = 0;
    if (ppDDrawGuid != NULL)
        *ppDDrawGuid = NULL;
}

///////////////////////////////////////
//
// Set kind
//

STDMETHODIMP_(void) cDisplayDevice::SetKind(eDisplayDeviceKind newKind, int fNewOptions)
{
}

///////////////////////////////////////
//
// Set kind
//

STDMETHODIMP_(void) cDisplayDevice::SetKind2(eDisplayDeviceKind newKind, int fNewOptions, GUID *)
{
}

///////////////////////////////////////
//
// Get information about the device
//

STDMETHODIMP_(void) cDisplayDevice::GetInfo(sGrDeviceInfo * pInfo, sGrModeInfo * pModeInfo)
{
}

///////////////////////////////////////
//
// Open the device
//

STDMETHODIMP cDisplayDevice::Open(sGrModeCap * pModeCap, int fFlags)
{
    return S_OK;
}

///////////////////////////////////////
//
// Close the device
//

STDMETHODIMP cDisplayDevice::Close()
{
    return S_OK;
}

///////////////////////////////////////
//
// Set the mode.  Info parameter on GetMode() cDisplayDevice::may be null.
//

STDMETHODIMP cDisplayDevice::SetMode(eGrDispMode dispMode, int flags)
{
    return S_OK;
}

///////////////////////////////////////
//
// Get the mode.  Info parameter on GetMode() cDisplayDevice::may be null.
//

STDMETHODIMP_(eGrDispMode) cDisplayDevice::GetMode(sGrModeInfoEx * pOptionalModeInfo)
{
    return 0;
}

///////////////////////////////////////
//
// Return TRUE of retrace is in progress
//

STDMETHODIMP_(BOOL) cDisplayDevice::StatHTrace()
{
    return FALSE;
}

///////////////////////////////////////
//
// Return TRUE of retrace is in progress
//

STDMETHODIMP_(BOOL) cDisplayDevice::StatVTrace()
{
    return FALSE;
}

///////////////////////////////////////
//
// Set the palette (if 8-bit palletized)
//

STDMETHODIMP_(BOOL) cDisplayDevice::SetPalette(unsigned uStart, unsigned n, const unsigned char * pRGBs)
{
    return TRUE;
}

///////////////////////////////////////
//
// Get the palette (if 8-bit palletized)
//

STDMETHODIMP_(BOOL) cDisplayDevice::GetPalette(unsigned uStart, unsigned n, unsigned char * pRGBs)
{
    return TRUE;
}

///////////////////////////////////////
//
// Flip the visible surface. No-op if no back buffer.
//

STDMETHODIMP cDisplayDevice::PageFlip()
{
    return S_OK;
}

///////////////////////////////////////
//
// Try to lock video memory.
//
//

STDMETHODIMP_(int) cDisplayDevice::Lock()
{
    return 1;
}

///////////////////////////////////////
//
// Release a lock on memory
//

STDMETHODIMP_(int) cDisplayDevice::Unlock()
{
    return 0;
}

///////////////////////////////////////
//
// Verify display reflects what is expected.  Useful if not particularly
// frame based & want to ensure screen is up-to-date within/without a
// BeginFrame()/EndFrame() cDisplayDevice::pair. Meaningful in "artificial" modes, no-op
// otherwise.
//

STDMETHODIMP_(void) cDisplayDevice::Flush()
{
}

///////////////////////////////////////
//
// Flush a specific region

STDMETHODIMP_(void) cDisplayDevice::FlushRect(int x0, int y0, int x1, int y1)
{
}

///////////////////////////////////////
//
// Pause/unpause display flushing 
//

STDMETHODIMP_(void) cDisplayDevice::PauseFlush(BOOL bPause)
{
    if (bPause)
        m_flags |= kFlushingPaused;
    else
        m_flags &= ~kFlushingPaused;
}

///////////////////////////////////////
//
// Query display flushing state
//

STDMETHODIMP_(BOOL) cDisplayDevice::IsFlushingPaused()
{
    return (BOOL) (m_flags & kFlushingPaused);
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

STDMETHODIMP_(BOOL) cDisplayDevice::SetMonitor(tGrDispBits ** ppDispBits, eDisplayDeviceMonitorFlag flags)
{
    return TRUE;
}

///////////////////////////////////////
//
// Temporarily break any lock on surfaces, returning lock count.
//

STDMETHODIMP_(int) cDisplayDevice::BreakLock()
{
    return 1;
}

///////////////////////////////////////
//
// Restore broken lock
//

STDMETHODIMP_(void) cDisplayDevice::RestoreLock(int iToRestore)
{
}

///////////////////////////////////////
//
// Query the lock state
//

STDMETHODIMP_(int) cDisplayDevice::IsLocked()
{
    return FALSE;
}

///////////////////////////////////////
//
// Create a device-relative canvas
//

STDMETHODIMP cDisplayDevice::CreateBitmap(sGrDevBitmapDesc *, int flags, IDeviceBitmap ** ppResult)
{
    *ppResult = NULL;
    return E_NOTIMPL;
}

///////////////////////////////////////
//
// Locate an IGrCanvas for the given sGrCanvas, if any exists
//

STDMETHODIMP cDisplayDevice::FindBitmap(const sGrBitmap *, IDeviceBitmap ** ppResult)
{
    *ppResult = NULL;
    return E_NOTIMPL;
}

///////////////////////////////////////
//
//
//

STDMETHODIMP cDisplayDevice::GetRenderTargets(sGrRenderTargets *)
{
    return E_NOTIMPL;
}

///////////////////////////////////////
//
// Convert mode property struct to enumerated mode
//

BOOL cDisplayDevice::ModeInfoToEnumMode(int width, int height, int bitDepth, int *retMode)
{
    return ((*retMode = gr_mode_from_info(width, height, bitDepth)) >= 0);
}

///////////////////////////////////////
//
// Convert enumerated mode to mode property struct
//

const sGrModeInfo & cDisplayDevice::EnumModeToModeInfo(eGrDispMode mode)
{
    return *gr_mode_info_from_mode(mode);
}

///////////////////////////////////////

const char * cDisplayDevice::EnumModeToName(eGrDispMode mode)
{
    return gr_mode_name(mode);
}

///////////////////////////////////////

int cDisplayDevice::GetAvailableMode(int width, int height, int bitDepth)
{
    return gr_find_mode(width, height, bitDepth);
}

///////////////////////////////////////

void cDisplayDevice::SetModeFlags(eGrDispMode mode, int flags)
{

}

///////////////////////////////////////////////////////////////////////////////

