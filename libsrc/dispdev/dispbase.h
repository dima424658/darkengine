///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/dispbase.h $
// $Author: KEVIN $
// $Date: 1998/01/12 15:44:29 $
// $Revision: 1.7 $
//
// Base classes of all dispdev classes.  Provides default implementations of
// IDisplayDevice functions and interface specifications for display provider
// and display mode operations
//

#ifndef __DISPBASE_H
#define __DISPBASE_H

#include <comtools.h>
#include <objcoll.h>
#include <aggmemb.h>
#include <dispapi.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDisplayDevice
//

class cDisplayDevice : public IDisplayDevice
{
    // IUnknown methods
    DECLARE_SIMPLE_AGGREGATION(cDisplayDevice);

public:
    //
    // Query kind
    //
    STDMETHOD_(eDisplayDeviceKind, GetKind)();

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

    // Set kind (must be outside Open/Close pair)
    STDMETHOD_(void, SetKind)(eDisplayDeviceKind, int flags);

    // Get information about the device
    STDMETHOD_(void, GetInfo)(sGrDeviceInfo *, sGrModeInfo * pModeInfo);

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

    // Pause/unpause display flushing 
    STDMETHOD_(void, PauseFlush)(BOOL bPause);

    // Query display flushing state
    STDMETHOD_(BOOL, IsFlushingPaused)();

    // Add/Remove a pointer to the monitor set.
    STDMETHOD_(BOOL, SetMonitor)(tGrDispBits **, eDisplayDeviceMonitorFlag);

    // Create a device based bitmap
    STDMETHOD (CreateBitmap)(sGrDevBitmapDesc *, int flags, IDeviceBitmap **);

    // Locate an IDeviceBitmap for the given sGrBitmap, if any exists
    STDMETHOD (FindBitmap)(const sGrBitmap *, IDeviceBitmap **);

    //
    STDMETHOD (GetRenderTargets)(sGrRenderTargets *);

    // Set kind with specified ddraw guid (must be outside Open/Close pair)
    STDMETHOD_(void, SetKind2)(eDisplayDeviceKind, int flags, GUID *pDDrawGuid);

    // Query kind, flags, and ddraw guid
    STDMETHOD_(void, GetKind2)(eDisplayDeviceKind *, int *, GUID **);

    //
    // Interface to functions in the 2d. They are here self-contained to
    // make it easy to redo if this library moves into a DLL other than
    // Dev2d
    //
    static const sGrModeInfo & EnumModeToModeInfo(eGrDispMode mode);
    static BOOL                ModeInfoToEnumMode(int width, int height, int bitDepth, int *retMode);
    static const char *        EnumModeToName(eGrDispMode mode);
    static int                 GetAvailableMode(int width, int height, int bitDepth);
    static void                SetModeFlags(eGrDispMode mode, int flags);

    ///////////////////////////////////
    //
    // Enums and constants
    //

    enum eDispBaseFlags
    {
        kFlushingPaused = 0x01                   // told externally that flushing is
                                                 // unsafe (because surface being
                                                 // flushed is under construction)
    };

protected:

    cDisplayDevice(IUnknown * pOuterUnknown, eDisplayDeviceKind kind, int flags);
    virtual ~cDisplayDevice();

    ///////////////////////////////////
    //
    // Instance Data
    //

    int     m_flags;
};

#endif /* !__DISPBASE_H */
