///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/compapis/RCS/dispapi.h $
// $Author: KEVIN $
// $Date: 1998/04/14 15:03:32 $
// $Revision: 1.35 $
//
// @TBD (toml 03-28-96): This should move if 2dapi is created
// @Note (toml 12-10-96): Some of these interfaces are really more closely
// related to dev2d & may want to move there if the 2d ever becomes more
// COM aware
//

#ifndef __DISPAPI_H
#define __DISPAPI_H

#if defined(__SC__) || defined(__RCC__)
#pragma once
#endif

///////////////////////////////////////

// Expected includes:
//  "lg.h"
//  "comtools.h"

#include "dispguid.h"

///////////////////////////////////////////////////////////////////////////////
//
// Forward Declarations
//

// Imported declarations
typedef struct grs_sys_info grs_sys_info;
struct grs_drvcap;
typedef struct grs_bitmap grs_bitmap;
typedef struct grs_mode_info grs_mode_info;

// Some mild renaming...
typedef grs_sys_info        sGrDeviceInfo;
typedef void                tGrDispBits;
typedef int                 eGrDispMode; // Corresponds to GRM_... in 2d
typedef struct grs_drvcap   sGrModeCap;
typedef grs_mode_info       sGrModeInfo;
typedef grs_bitmap          sGrBitmap;

// Structures in this header
typedef struct sGrRenderTargets sGrRenderTargets;
typedef struct sGrModeInfoEx sGrModeInfoEx;
typedef struct sGrDevBitmapDesc sGrDevBitmapDesc;

// Interfaces in this header
F_DECLARE_INTERFACE(IDisplayDevice);
F_DECLARE_INTERFACE(IDeviceBitmap);
#ifdef IN_PROGRESS
F_DECLARE_INTERFACE(IDeviceComposeBitmap);
#endif


///////////////////////////////////////////////////////////////////////////////
//
// Low-level Display Device API
//

enum eDisplayDeviceKind
{
    //
    // Default
    //
    kDispDefault,

    //
    // Windowed
    //
    kDispWindowed,

    //
    // Full screen
    //
    kDispFullScreen,

    //
    // Debug-superior
    //
    kDispDebug,

    //
    // Undefined
    //
    kDispUndefined,

};

typedef enum eDisplayDeviceKind eDisplayDeviceKind;


enum eDisplayDeviceCreateFlags
{
    //
    // Create flippable surfaces, if available
    //
    kDispAttemptFlippable = 0x01,

    //
    // Look for appropriate 3d devices
    //
    kDispAttempt3D = 0x02,

    //
    // Enable Strict Pointer Monitoring
    //
    kDispStrictMonitors = 0x04,

};


//
// Create a display device and add it to the global app-object
//
#define DisplayDeviceCreate(kind) \
{ \
    IUnknown * pAppUnknown = AppGetObj(IUnknown); \
    _DisplayDeviceCreate(IID_TO_REFIID(IID_IDisplayDevice), NULL, pAppUnknown, kind, 0); \
    COMRelease(pAppUnknown); \
}

#define DisplayDeviceCreate2(kind, flags) \
{ \
    IUnknown * pAppUnknown = AppGetObj(IUnknown); \
    _DisplayDeviceCreate(IID_TO_REFIID(IID_IDisplayDevice), NULL, pAppUnknown, kind, flags); \
    COMRelease(pAppUnknown); \
}

//
// Creates a display device, aggregating it with specfied pOuter,
// use IAggregate protocol if ppDisplayDevice is NULL, else self-init
//
EXTERN tResult LGAPI _DisplayDeviceCreate(REFIID, IDisplayDevice ** ppDisplayDevice, IUnknown * pOuter, eDisplayDeviceKind, int flags);

///////////////////////////////////////////////////////////////////////////////
//
// Structures and enumerations
//

///////////////////////////////////////
//
// Flags detailing properties of a mode
//
enum eGrModeFlags
{
    // Static properties
    kGrModeSupported     = 0x01,
    kGrModeLinear        = 0x02,
    kGrModeUseWinA       = 0x04,
    kGrModeIsModeX       = 0x08,
    kGrModeIsNative      = 0x10,
    kGrModeCanFullscreen = 0x20,
    kGrModeCanWindow     = 0x40,

    // DynamicProperties
    kGrModeIsWindowed   = 0x0100,
    kGrModeIsDirect     = 0x0200,
    
    kGrModeNotSet       = 0x80000000
};

#ifdef IN_PROGRESS
///////////////////////////////////////
//
// Flags indicating the style of rendering for the present mode
//
enum eGrRenderFlags
{
    kGrRenderComposed   = 0x01,
    kGrRenderFlip       = 0x02,
};
#endif


//
// Structure exposing the display device provided rendering targets
//
struct sGrRenderTargets
{
    IDeviceBitmap *         pFrontBuffer;
    IDeviceBitmap *         pBackBuffer;
#ifdef IN_PROGRESS
    IDeviceComposeBitmap *  pComposeBuffer;
#endif
};

///////////////////////////////////////
//
// Detailed information about the present mode.  The information in the
// structure always describes the primary rendering target, usually
// either the back buffer or the compose bitmap.
//
struct sGrModeInfoEx
{
    eGrDispMode         mode;
    DWORD               width;                   // width of display
    DWORD               height;                  // height of display
    long                rowBytes;                // distance to start of next line
    DWORD               flags;                   // mode flags
    DWORD               depth;                   // how many bits per pixel (4,8,16,24,32)

    DWORD               redMask;                 // mask for red bit
    DWORD               greenMask;               // mask for green bits
    DWORD               blueMask;                // mask for blue bits

    DWORD               reserved1;
    DWORD               reserved2;
};


///////////////////////////////////////
//
// Device bitmap descriptor.  Used to allocate/query OS or hardware based
// bitmaps, mostly for use by DirectDraw and Direct3D intimate code.
//
// @Note (toml 12-10-96): Narrow support right now
//

//
// CreateBitmap() flags (sparse right now (toml 12-13-96))
//
enum eGrDevBitmapCreateFlags
{
    // Create a 2d bitmap with the same depth/pixel format properties as the current mode
    kGrDevBmpCreateDefault = 0x01
};

struct sGrDevBitmapDesc
{
    int width;
    int height;

    DWORD reserved[8];
};


///////////////////////////////////////
//
// Function call flags
//

//
// SetMode() flags
//
enum eDisplayDeviceSetModeFlags
{
    // Same as dev2d flags
    kGrSetClear             = 0x01,

    kGrSetWindowed          = 0x04,
    kGrSetFullScreen        = 0x08,

#ifdef IN_PROGRESS
    kGrSetFlip              = 0x10,
    kGrSetNoFlip            = 0x20,

    kGrSetCompose           = 0x40,
    kGrSetNoCompose         = 0x80,
#endif
};

//
// SetMonitor() flags
//
enum eDisplayDeviceMonitorFlag
{
    kGrDispPtrMonitorOn,
    kGrDispPtrMonitorOff
};

typedef enum eDisplayDeviceMonitorFlag eDisplayDeviceMonitorFlag;


///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IDisplayDevice
//

#undef INTERFACE
#define INTERFACE IDisplayDevice

DECLARE_INTERFACE_(IDisplayDevice, IUnknown)
{
    //
    // IUnknown methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    // Query/Set kind (set must be outside Open/Close pair)
    //
    STDMETHOD_(eDisplayDeviceKind, GetKind)(THIS) PURE;
    STDMETHOD_(void,               SetKind)(THIS_ eDisplayDeviceKind, int flags) PURE;

    //
    // Get information about the device.
    //
    STDMETHOD_(void, GetInfo)(THIS_ sGrDeviceInfo *, sGrModeInfo * ) PURE;

    //
    // Open/Close the device, saving and restoring the state
    // Pointers in ModeCap will automatically monitored. They may
    // initially be set to invalid values, then fixed-up when Lock()
    // is called
    //
    STDMETHOD (Open)(THIS_ sGrModeCap *, int fFlags) PURE;
    STDMETHOD (Close)(THIS) PURE;

    //
    // Set/get the mode.  Info parameter on GetMode() may be null.
    //
    STDMETHOD (             SetMode)(THIS_ eGrDispMode, int flags) PURE;
    STDMETHOD_(eGrDispMode, GetMode)(THIS_ sGrModeInfoEx * pOptionalModeInfo) PURE;

    //
    // Return TRUE of retrace is in progress
    //
    STDMETHOD_(BOOL, StatHTrace)(THIS) PURE;
    STDMETHOD_(BOOL, StatVTrace)(THIS) PURE;

    //
    // Set/Get the palette (if 8-bit palletized)
    //
    STDMETHOD_(BOOL, SetPalette)(THIS_ unsigned uStart, unsigned n, const unsigned char * pRGBs) PURE;
    STDMETHOD_(BOOL, GetPalette)(THIS_ unsigned uStart, unsigned n, unsigned char * pRGBs) PURE;

    //
    // Flip the visible surface. No-op if no back buffer.
    //
    STDMETHOD (PageFlip)(THIS) PURE;

    //
    // Try to lock video memory.  Locked pointers will be placed in
    // structure provided in SetMode(). Returns lock count.
    //
    STDMETHOD_(int, Lock)(THIS) PURE;

    //
    // Release a lock on memory
    //
    STDMETHOD_(int, Unlock)(THIS) PURE;

    //
    // Verify display reflects what is expected.  Useful if not particularly
    // frame based & want to ensure screen is up-to-date within/without a
    // BeginFrame()/EndFrame() pair. Meaningful in "artificial" modes, no-op
    // otherwise.
    //
    STDMETHOD_(void, Flush)(THIS) PURE;

    //
    // Pause/unpause display flushing 
    //
    STDMETHOD_(void, PauseFlush)(THIS_ BOOL bPause) PURE;

    //
    // Query display flushing state
    //
    STDMETHOD_(BOOL, IsFlushingPaused)(THIS) PURE;

    //
    // Add a pointer to the monitor set.  Monitored pointers are automatically
    // invalidated/fixed-up across Lock/Unlock cycles.  Pointer must already
    // be set relative to values passed back in SetMode().
    //
    // Return FALSE if the set monitor fails for some reason (like we pass
    // in an invalid pointer, for instance)
    //
    STDMETHOD_(BOOL, SetMonitor)(THIS_ tGrDispBits **, eDisplayDeviceMonitorFlag) PURE;

    //
    // Temporarily break any lock on surfaces, returning lock count.
    //
    STDMETHOD_(int, BreakLock)(THIS) PURE;

    //
    // Restore broken lock
    //
    STDMETHOD_(void, RestoreLock)(THIS_ int iLockCount) PURE;

    //
    // Query the lock state
    //
    STDMETHOD_(BOOL, IsLocked)(THIS) PURE;

    //
    // Flush a specific region
    //
    STDMETHOD_(void, FlushRect)(THIS_ int x0, int y0, int x1, int y1) PURE;

    //
    // Create a device based bitmap
    //
    STDMETHOD (CreateBitmap)(THIS_ sGrDevBitmapDesc *, int flags, IDeviceBitmap **) PURE;

    //
    // Locate an IDeviceBitmap for the given sGrBitmap, if any exists
    //
    STDMETHOD (FindBitmap)(THIS_ const sGrBitmap *, IDeviceBitmap **) PURE;

    //
    // Interface pointers passed back through sGrRenderTargets * will be
    // AddRef()'d
    //
    STDMETHOD (GetRenderTargets)(THIS_ sGrRenderTargets *) PURE;

    //
    // Specify ddraw guid.
    // @tbd: move this next to SetKind next time we can't backtouch
    // 
    STDMETHOD_(void,               SetKind2)(THIS_ eDisplayDeviceKind, int flags, GUID *pDDrawGuid) PURE;

    //
    // Query kind, flags, and ddraw guid
    // @tbd: move this next to GetKind next time we can't backtouch
    // 
    STDMETHOD_(void,               GetKind2)(THIS_ eDisplayDeviceKind *, int *, GUID **) PURE;
};

#define IDisplayDevice_QueryInterface(p, a, b)      COMQueryInterface(p, a, b)
#define IDisplayDevice_AddRef(p)                    COMAddRef(p)
#define IDisplayDevice_Release(p)                   COMRelease(p)
#define IDisplayDevice_GetInfo(p, a, b)             COMCall2(p, GetInfo, a, b)
#define IDisplayDevice_Open(p, a, b)                COMCall2(p, Open, a, b)
#define IDisplayDevice_Close(p)                     COMCall0(p, Close)
#define IDisplayDevice_SetMode(p, a, b)             COMCall2(p, SetMode, a, b)
#define IDisplayDevice_GetMode(p, a)                COMCall1(p, GetMode, a)
#define IDisplayDevice_StatHTrace(p)                COMCall0(p, StatHTrace)
#define IDisplayDevice_StatVTrace(p)                COMCall0(p, StatVTrace)
#define IDisplayDevice_SetPalette(p, a, b, c)       COMCall3(p, SetPalette, a, b, c)
#define IDisplayDevice_GetPalette(p, a, b, c)       COMCall3(p, GetPalette, a, b, c)
#define IDisplayDevice_PageFlip(p)                  COMCall0(p, PageFlip)
#define IDisplayDevice_Lock(p)                      COMCall0(p, Lock)
#define IDisplayDevice_Unlock(p)                    COMCall0(p, Unlock)
#define IDisplayDevice_Flush(p)                     COMCall0(p, Flush)
#define IDisplayDevice_PauseFlush(p, a)             COMCall1(p, PauseFlush, a)
#define IDisplayDevice_IsFlushingPaused(p)          COMCall0(p, IsFlushingPaused)
#define IDisplayDevice_SetMonitor(p, a, b)          COMCall2(p, SetMonitor, a, b)
#define IDisplayDevice_BreakLock(p)                 COMCall0(p, BreakLock)
#define IDisplayDevice_RestoreLock(p, a)            COMCall1(p, RestoreLock, a)
#define IDisplayDevice_IsLocked(p)                  COMCall0(p, IsLocked)
#define IDisplayDevice_FlushRect(p, a, b, c, d)     COMCall4(p, FlushRect, a, b, c, d)
#define IDisplayDevice_CreateBitmap(p, a, b)        COMCall2(p, CreateBitmap, a, b)
#define IDisplayDevice_FindBitmap(p, a, b)          COMCall2(p, FindBitmap, a, b)


///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IDeviceBitmap
//
// A husk for exposing display device surfaces as 2d bitmaps
//

#undef INTERFACE
#define INTERFACE IDeviceBitmap

DECLARE_INTERFACE_(IDeviceBitmap, IUnknown)
{
    //
    // IUnknown methods
    //
    DECLARE_UNKNOWN_PURE();

    //
    // Perform a bit block transfer, clipping to destination
    //
    STDMETHOD (SimpleBlt)(THIS_ int xDest, int yDest, IDeviceBitmap * pSource) PURE;

    //
    // Query whether bitmap is even useful
    //
    STDMETHOD_(BOOL, IsAvailable)(THIS) PURE;

    //
    // Gain access to the bitmap as a 2d bitmap.  The bitmap bits pointer
    // is "monitored." Monitored pointers are automatically
    // invalidated/fixed-up across Lock/Unlock cycles.
    //
    // If the client makes a persistent copy of the sGrBitmap, or a
    // sub-canvas/bitmap of it, the implementor object must be notified.
    //
    // Client must notify IDeviceBitmap implementor object when finished with
    // a bitmap either directly obtained or copied by calling EndAccess()
    //
    STDMETHOD (BeginAccess)(THIS_ sGrBitmap * pGrsBitmap) PURE;
    STDMETHOD (NotifyCopy)(THIS_ sGrBitmap * pNewBitmap) PURE;
    STDMETHOD (EndAccess)(THIS_ sGrBitmap * pGrsBitmap) PURE;

    //
    // Gain access to object doing most basic work. This is usually an
    // IDirectDrawSurface, and can return NULL.  It is dangerous to
    // hold this accross a mode change if the bitmap is one of the
    // render targets.
    //
    STDMETHOD (GetImplementor)(THIS_ IUnknown ** ppUnknown) PURE;

    //
    // Try to lock video memory.  Locked pointers will be placed in
    // bitmap structures obtained through BeginAccess() or NotifyCopy().
    //
    STDMETHOD_(int, Lock)(THIS) PURE;

    //
    // Release a lock on memory
    //
    STDMETHOD_(int, Unlock)(THIS) PURE;

};

#define IDeviceBitmap_QueryInterface(p, a, b)           COMQueryInterface(p, a, b)
#define IDeviceBitmap_SimpleBlt(p, a, b, c)             COMCall3(p, SimpleBlt, a, b, c)
#define IDeviceBitmap_IsAvailable(p)                    COMCall0(p, IsAvailable)
#define IDeviceBitmap_AddRef(p)                         COMAddRef(p)
#define IDeviceBitmap_Release(p)                        COMRelease(p)
#define IDeviceBitmap_BeginAccess(p, a)                 COMCall1(p, BeginAccess, a)
#define IDeviceBitmap_NotifyCopy(p, a)                  COMCall1(p, NotifyCopy, a)
#define IDeviceBitmap_EndAccess(p, a)                   COMCall1(p, EndAccess, a)
#define IDeviceBitmap_GetImplementor(p, a)              COMCall1(p, GetImplementor, a)
#define IDeviceBitmap_Lock(p)                           COMCall0(p, Lock)
#define IDeviceBitmap_Unlock(p)                         COMCall0(p, Unlock)


#ifdef IN_PROGRESS

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IDeviceComposeBitmap
//
// Adds special functionality for a bitmap intended as the primary composition
// buffer

#undef INTERFACE
#define INTERFACE IDeviceComposeBitmap

DECLARE_INTERFACE_(IDeviceComposeBitmap, IUnknown)
{
    //
    // Functions from IDeviceBitmap
    //
    DECLARE_UNKNOWN_PURE();
    STDMETHOD_(BOOL, IsAvailable)(THIS) PURE;
    STDMETHOD (BeginAccess)(THIS_ sGrBitmap * pGrsBitmap) PURE;
    STDMETHOD (NotifyCopy)(THIS_ sGrBitmap * pNewCopy) PURE;
    STDMETHOD (EndAccess)(THIS_ sGrBitmap * pGrsBitmap) PURE;
    STDMETHOD (GetImplementor)(THIS_ IUnknown ** ppUnknown) PURE;
    STDMETHOD_(int, Lock)(THIS) PURE;
    STDMETHOD_(int, Unlock)(THIS) PURE;

    //
    // Add a rectangle to the dirty list
    //
    STDMETHOD (DirtyRect)(THIS_ int x0, int y0, int x1, int y1) PURE;

    //
    // Flush all dirty rectangles to the screen target
    //
    STDMETHOD (FlushDirty)(THIS) PURE;

    //
    // Flush a specific rectangle, ignoring dirty list (or should it flush the dirty if there's overlap? (toml 12-10-96))
    //
    STDMETHOD (FlushRect)(THIS_ int x0, int y0, int x1, int y1) PURE;

    //
    // Flush the whole buffer, mark the entire buffer as clean
    //
    STDMETHOD (FlushAll)(THIS) PURE;

};

#define IDeviceComposeBitmap_QueryInterface(p, a, b)    COMQueryInterface(p, a, b)
#define IDeviceComposeBitmap_AddRef(p)                  COMAddRef(p)
#define IDeviceComposeBitmap_Release(p)                 COMRelease(p)
#define IDeviceComposeBitmap_IsAvailable(p)             COMCall0(p, IsAvailable)
#define IDeviceComposeBitmap_BeginAccess(p, a)          COMCall1(p, BeginAccess, a)
#define IDeviceComposeBitmap_NotifyCopy(p, a)           COMCall1(p, NotifyCopy, a)
#define IDeviceComposeBitmap_EndAccess(p, a)            COMCall1(p, EndAccess, a)
#define IDeviceComposeBitmap_GetImplementor(p, a)       COMCall1(p, GetImplementor, a)
#define IDeviceComposeBitmap_Lock(p)                    COMCall0(p, Lock)
#define IDeviceComposeBitmap_Unlock(p)                  COMCall0(p, Unlock)
#define IDeviceComposeBitmap_DirtyAll(p)                COMCall0(p, DirtyAll)
#define IDeviceComposeBitmap_DirtyRect(p, a, b, c, d)   COMCall4(p, DirtyRect, a, b, c, d)
#define IDeviceComposeBitmap_FlushDirty(p)              COMCall0(p, FlushDirty)
#define IDeviceComposeBitmap_FlushRect(p, a, b, c, d)   COMCall4(p, FlushRect, a, b, c, d)

#endif

///////////////////////////////////////////////////////////////////////////////
//
// Utility classes to ease scope lock/unlock demands
//

#if defined(__cplusplus) && !defined(NO_DISPLAY_SCOPE_HELPERS)

#define DisplayLockForScope(pDisplayDevice)         cDisplayScopedLock       __dispLock      (pDisplayDevice)
#define DisplayBreakLockForScope(pDisplayDevice)    cDisplayScopedLockBreak  __dispLockBreak (pDisplayDevice)

///////////////////////////////////////

class cDisplayScopedLock
{
public:
    cDisplayScopedLock(IDisplayDevice * pDisplayDevice)
      : m_pDisplayDevice(pDisplayDevice)
    {
        if (m_pDisplayDevice)
        {
            m_pDisplayDevice->AddRef();
            m_pDisplayDevice->Lock();
        }
    }

    ~cDisplayScopedLock()
    {
        if (m_pDisplayDevice)
        {
            m_pDisplayDevice->Unlock();
            m_pDisplayDevice->Release();
        }
    }

private:
    IDisplayDevice * m_pDisplayDevice;
};

///////////////////////////////////////

class cDisplayScopedLockBreak
{
public:
    cDisplayScopedLockBreak(IDisplayDevice * pDisplayDevice)
      : m_pDisplayDevice(pDisplayDevice)
    {
        if (m_pDisplayDevice)
        {
            m_pDisplayDevice->AddRef();
            m_LockCount = m_pDisplayDevice->BreakLock();
        }
    }

    ~cDisplayScopedLockBreak()
    {
        if (m_pDisplayDevice)
        {
            m_pDisplayDevice->RestoreLock(m_LockCount);
            m_pDisplayDevice->Release();
        }
    }

private:
    IDisplayDevice * m_pDisplayDevice;
    ulong            m_LockCount;
};

#endif

///////////////////////////////////////////////////////////////////////////////

#endif /* __DISPAPI_H */
