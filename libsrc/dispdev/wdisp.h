///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/wdisp.h $
// $Author: KEVIN $
// $Date: 1998/01/12 15:44:26 $
// $Revision: 1.28 $
//

#ifndef __WDISP_H
#define __WDISP_H

#include <dispbase.h>
#include <wdispapi.h>

#include <thrdtool.h>
#include <relocptr.h>

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IWinApp);

class cWinDisplayProvider;
class cWinDisplayModeOperations;

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sGdiBitmapInfo
//
// Initializes and stores information required to create a GDI bitmap
//

struct sGdiBitmapInfo : public BITMAPINFO
{
    sGdiBitmapInfo(int width, int height, int significantDepth);
    RGBQUAD bmiColors2[255];
};

// Typedef Task Switch callback function

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cWinDisplayDevice
//
// Implements IDisplayDevice.  Acts primarily as an adaptor for instances
// of cWinDisplayProvider and cWinDisplayModeOperations
//

class cWinDisplayDevice : public cDisplayDevice
{
public:
    cWinDisplayDevice(IUnknown * pOuterUnknown, eDisplayDeviceKind kind, int flags);
    virtual ~cWinDisplayDevice();

    ///////////////////////////////////
    //
    // Query functions for client Providers and Operations
    //

    //
    // Query if the device has been opened
    //
    BOOL IsOpen() const;

    //
    // Query if the device is useful (i.e., is in some mode, any mode)
    //
    BOOL IsModeSet() const;

    //
    // Query the lock count
    //
    int GetLockCount() const;

    //
    // Query kind
    //
    STDMETHOD_(eDisplayDeviceKind, GetKind)();

    //
    // Query the creation flags
    //
    int GetCreateFlags() const;

    //
    // Temporarily break any lock on surfaces, returning lock count
    //
    STDMETHOD_(int, BreakLock)();

    //
    // Restore broken lock
    //
    STDMETHOD_(void, RestoreLock)(int iLockCount);

    //
    // Try to gain exclusive use of display object, without locking video memory.
    //
    STDMETHOD_(BOOL, IsLocked)();

    //
    // Threading hooks
    //
    void WaitForMutex();
    void ReleaseMutex();
    int  BreakMutex();
    void RestoreMutex(int);

    //
    // Get the properties of the current mode
    //
    const sGrModeInfo & GetCurrentModeInfo() const;

    //
    // Palette tools
    //
    struct sPalette256 : public LOGPALETTE
    {
        sPalette256()
        {
            palVersion = 0x300;
            palNumEntries = 256;
        }
        PALETTEENTRY AddEntries[255];
    };

    LOGPALETTE &   GetLogPalette();
    PALETTEENTRY * GetPaletteEntries();

    // Set kind (must be outside Open/Close pair)
    STDMETHOD_(void, SetKind)(eDisplayDeviceKind, int flags);

    // Get information about the device
    STDMETHOD_(void, GetInfo)(sGrDeviceInfo *, sGrModeInfo *);

    // Open/Close the device
    STDMETHOD (Open)(sGrModeCap *, int fFlags);
    STDMETHOD (Close)();

    // Set/get the mode
    STDMETHOD (SetMode)(eGrDispMode, int fFlags);
    STDMETHOD_(eGrDispMode, GetMode)(sGrModeInfoEx * pOptionalModeInfo);

    // Return TRUE of retrace is in progress
    STDMETHOD_(BOOL, StatHTrace)();
    STDMETHOD_(BOOL, StatVTrace)();

    // Set/Get the palette (if 8-bit palletized)
    STDMETHOD_(BOOL, SetPalette)(unsigned uStart, unsigned n, const unsigned char * pRGBs);
    STDMETHOD_(BOOL, GetPalette)(unsigned uStart, unsigned n, unsigned char * pRGBs);

    // Flip the visible surface. No-op if no back buffer.
    STDMETHOD (PageFlip)();

    // Try to lock video memory.  Locked pointers will be placed in
    // structure provided in SetMode(). Returns lock count.
    STDMETHOD_(int, Lock)();

    // Release a lock on memory
    STDMETHOD_(int, Unlock)();

    // Verify display reflects what is expected.
    STDMETHOD_(void, Flush)();

    // Flush a specific region
    STDMETHOD_(void, FlushRect)(int x0, int y0, int x1, int y1);

    // Add/Remove a pointer to the monitor set.
    STDMETHOD_(BOOL, SetMonitor)(tGrDispBits **, eDisplayDeviceMonitorFlag);

    // Create a device based bitmap
    STDMETHOD (CreateBitmap)(sGrDevBitmapDesc *, int flags, IDeviceBitmap **);

    // Locate an IDeviceBitmap for the given sGrBitmap, if any exists
    STDMETHOD (FindBitmap)(const sGrBitmap *, IDeviceBitmap **);

    //
    STDMETHOD (GetRenderTargets)(sGrRenderTargets *);

    // Set kind (must be outside Open/Close pair)
    STDMETHOD_(void, SetKind2)(eDisplayDeviceKind, int flags, GUID *pDDrawGuid);

    // Query kind, flags, and ddraw guid
    STDMETHOD_(void, GetKind2)(eDisplayDeviceKind *, int *, GUID **);

    // Set gamma correction level
    STDMETHOD_(BOOL, SetGamma)(double gamma);

    // Set page flipping
    STDMETHOD_(BOOL, SetFlip)(BOOL bFlip);

    // Create a GDI bitmap
    HBITMAP CreateGDIBitmap(sGdiBitmapInfo * pBitmapInfo);

private:

    ///////////////////////////////////

    // Fixes-up monitored pointers
    void Sync2DPointers();
    void InvalidateBasePointers();

    // Manipulation of m_LogicalPalette
    static void SetStoredPaletteEntries(unsigned uStart, unsigned n, const unsigned char * pRGBs, int paletteFlags);
    static void GetStoredPaletteEntries(unsigned uStart, unsigned n, unsigned char * pRGBs);

    ///////////////////////////////////

    void DoCursorBltFromGDIScreen(sGrBitmap *, int x, int y);
    void DoCursorBltToGDIScreen(sGrBitmap *, int x, int y);

    ///////////////////////////////////
    //
    // Enums and constants
    //

    enum
    {
        kTotallyBogusBase = 0xffeeffee,
        kPrimaryBogusBase = 0xfefefefe,
        kSecondaryBogusBase  = 0xfdfdfdfd
    };

    enum eWinDispFlags
    {
                                                 // Start higher than base flags!
        kStrictMonitors = 0x10,
        kSettingMode    = 0x20                   // because setting the mode often
                                                 // requires breaking the display
                                                 // device mutex, an additional
                                                 // bit ensures the message thread
                                                 // doesn't do anything bad
    };

    ///////////////////////////////////
    //
    // CLASS: cIWinDisplayDeviceProxy
    //
    // Simply passes requests to outer object
    //

    class cIWinDisplayDeviceProxy : public IWinDisplayDevice
    {
    public:
        cIWinDisplayDeviceProxy(cWinDisplayDevice * pOuter, IUnknown * pOuterUnknown)
          : m_pOuter(pOuter), m_pTaskSwitchCallback(NULL)
        {
            INIT_DELEGATION(pOuterUnknown);
        }

    private:
        DECLARE_DELEGATION();

        STDMETHOD_(BOOL, ProcessMessage)(UINT msg, WPARAM wParam, LPARAM lParam, long * pRetVal);
        STDMETHOD_(void, GetWidthHeight)(unsigned * pWidth, unsigned * pHeight);
        STDMETHOD_(void, OnTaskSwitch)  (BOOL);
        STDMETHOD_(BOOL, GetDirectDraw) (IDirectDraw4 **);
        STDMETHOD_(void, WaitForMutex)();
        STDMETHOD_(void, ReleaseMutex)();
        STDMETHOD_(void, CursorBltFromGDIScreen)(sGrBitmap *, int x, int y);
        STDMETHOD_(void, CursorBltToGDIScreen)(sGrBitmap *, int x, int y);
        STDMETHOD_(BOOL, GetBitmapSurface)(sGrBitmap *, IDirectDrawSurface4 **);
        STDMETHOD_(void, AddTaskSwitchCallback)(WinDispDevCallbackFunc *callback);
        STDMETHOD_(void, RemoveTaskSwitchCallback)(int id);

        cWinDisplayDevice * m_pOuter;
        WinDispDevCallbackFunc *m_pTaskSwitchCallback;
    };

    friend class cIWinDisplayDeviceProxy;

    ///////////////////////////////////
    //
    // Instance Data
    //
   
public:
    GUID *                      m_pDDrawGuid;

private:
    int                         m_iLock;
    cWinDisplayModeOperations * m_pDisplayModeOperations;

    ///////////////////////////////////

    sGrModeCap *                m_pModeInfo;
    eGrDispMode                 m_CurrentDispMode;

    ///////////////////////////////////

    eDisplayDeviceKind          m_DisplayDeviceKind;
    int                         m_fOptions;
    cWinDisplayProvider *       m_pDisplayProvider;
    cIWinDisplayDeviceProxy     m_IWinDisplayDeviceProxy;

    ///////////////////////////////////

    cThreadLock                 m_Mutex;
    int                         m_MutexCount;

    ///////////////////////////////////

    cRelocationSet<tGrDispBits> m_PrimarySurfaceRelocationSet;
    cRelocationSet<tGrDispBits> m_SecondarySurfaceRelocationSet;

    ///////////////////////////////////

    struct sCursorScratch
    {
        sCursorScratch()
        {
            hBitmap = NULL;
            memset(&bitmap, 0, sizeof(BITMAP));
        }

        ~sCursorScratch()
        {
            if (hBitmap)
                DeleteObject(hBitmap);
        }

        HBITMAP hBitmap;
        BITMAP  bitmap;
    };

    sCursorScratch              m_CursorScratch;

    ///////////////////////////////////
    //
    // Static data
    //

    static sPalette256          gm_LogicalPalette;

};

///////////////////////////////////////////////////////////////////////////////
//
// Utility classes for display device mutex
//

class cAutoDisplayMutex
{
public:
    cAutoDisplayMutex(cWinDisplayDevice * pWinDisplayDevice)
    {
        m_pWinDisplayDevice = pWinDisplayDevice;
        m_pWinDisplayDevice->WaitForMutex();
    }

    ~cAutoDisplayMutex()
    {
        m_pWinDisplayDevice->ReleaseMutex();
    }

private:
    cWinDisplayDevice * m_pWinDisplayDevice;
};


//
// Yield the display. Sorry, only 1 per scope
//
#define YieldDisplay(pDisplayDevice) \
    int _YieldDisplay_iPreviousLock       = pDisplayDevice->BreakLock(); \
    int _YieldDisplay_iPreviousMutexLevel = pDisplayDevice->BreakMutex()

#define RegainDisplay(pDisplayDevice) \
    m_pDisplayDevice->RestoreMutex(_YieldDisplay_iPreviousMutexLevel);\
    m_pDisplayDevice->RestoreLock(_YieldDisplay_iPreviousLock)


///////////////////////////////////////////////////////////////////////////////
//
// Inline members of cWinDisplayDevice
//

inline BOOL cWinDisplayDevice::IsOpen() const
{
    return !!m_pModeInfo;
}

///////////////////////////////////////

inline BOOL cWinDisplayDevice::IsModeSet() const
{
    return (m_pDisplayModeOperations && !(m_flags & kSettingMode));
}

///////////////////////////////////////

inline int cWinDisplayDevice::GetLockCount() const
{
    return m_iLock;
}

///////////////////////////////////////

inline int cWinDisplayDevice::GetCreateFlags() const
{
    return m_fOptions;
}

///////////////////////////////////////

inline void cWinDisplayDevice::WaitForMutex()
{
    m_Mutex.Lock();
    m_MutexCount++;
}

///////////////////////////////////////

inline void cWinDisplayDevice::ReleaseMutex()
{
    m_MutexCount--;
    m_Mutex.Unlock();
}

///////////////////////////////////////

inline int cWinDisplayDevice::BreakMutex()
{
    m_Mutex.Lock();
    const int retVal = m_MutexCount;
    while (m_MutexCount)
    {
        m_Mutex.Unlock();
        m_MutexCount--;
    }
    m_Mutex.Unlock();
    return retVal;
}

///////////////////////////////////////

inline void cWinDisplayDevice::RestoreMutex(int iMutexCount)
{
    m_Mutex.Lock();
    m_MutexCount += iMutexCount;
    while (iMutexCount)
    {
        m_Mutex.Lock();
        iMutexCount--;
    }
    m_Mutex.Unlock();
}

///////////////////////////////////////

inline const sGrModeInfo & cWinDisplayDevice::GetCurrentModeInfo() const
{
    return EnumModeToModeInfo(m_CurrentDispMode);
}

///////////////////////////////////////

inline LOGPALETTE & cWinDisplayDevice::GetLogPalette()
{
    return gm_LogicalPalette;
}

///////////////////////////////////////

inline PALETTEENTRY * cWinDisplayDevice::GetPaletteEntries()
{
    return gm_LogicalPalette.palPalEntry;
}

///////////////////////////////////////

inline HBITMAP cWinDisplayDevice::CreateGDIBitmap(sGdiBitmapInfo * pBitmapInfo)
{
    HBITMAP hBitmap;
    UINT fUsage = (pBitmapInfo->bmiHeader.biCompression == BI_RGB) ? DIB_RGB_COLORS : 0;
    void * pDummy;                      // @Note (toml 04-12-96): In winG, the bits argument could be NULL. We have to get it here, even though we retreive it again with the GetObject() below. This section should be cleaned-up

    HDC hdcScreen = GetDC(NULL);
    hBitmap = CreateDIBSection(hdcScreen, pBitmapInfo, fUsage, &pDummy, NULL, 0);
    ReleaseDC(NULL, hdcScreen);
    return hBitmap;
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__WDISP_H */
