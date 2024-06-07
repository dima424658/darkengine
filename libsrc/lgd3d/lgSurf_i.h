#pragma once

#include <types.h>
#include <grs.h>
#include <ddraw.h>

struct sLGSurfaceDescription
{
    DWORD dwWidth;
    DWORD dwHeight;
    DWORD dwBitDepth;
    DWORD dwPitch;
    grs_rgb_bitmask sBitmask;
};

enum eLGSBlit
{
    kLGSBRead = 1,
    kLGSBWrite = 2,
    kLGSBReadWrite = 3
};

enum eLGSurfState
{
    kLGSSUninitialized = 1,
    kLGSSInitialized = 2,
    kLGSSUnlocked = 3,
    kLGSS3DRenderining = 4,
    kLGSS2DLocked = 5,
    kLGSS2DBlitting = 6
};

class ILGSurface : public IUnknown
{
public:
    ILGSurface();
    ILGSurface(const ILGSurface &);
    virtual ~ILGSurface();

    // virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    // virtual ULONG STDMETHODCALLTYPE AddRef();
    // virtual ULONG STDMETHODCALLTYPE Release();

    virtual int CreateInternalScreenSurface(HWND hMainWindow) = 0;
    virtual int ChangeClipper(HWND hMainWindow) = 0;
    virtual int SetAs3dHdwTarget(DWORD dwRequestedFlags, DWORD dwWidth, DWORD dwHeight, DWORD dwBitDepth, DWORD *pdwCapabilityFlags, grs_canvas **ppsCanvas) = 0;
    virtual int Resize3dHdw(DWORD dwRequestedFlags, DWORD dwWidth, DWORD dwHeight, DWORD dwBitDepth, DWORD *pdwCapabilityFlags, grs_canvas **ppsCanvas) = 0;
    virtual int BlitToScreen(DWORD dwXScreen, DWORD dwYScreen, DWORD dwWidth, DWORD dwHeight) = 0;
    virtual void CleanSurface() = 0;
    virtual void Start3D() = 0;
    virtual void End3D() = 0;
    virtual long LockFor2D(grs_canvas **ppsCanvas, eLGSBlit eBlitFlags) = 0;
    virtual long UnlockFor2D() = 0;
    virtual DWORD GetWidth() = 0;
    virtual DWORD GetHeight() = 0;
    virtual DWORD GetPitch() = 0;
    virtual DWORD GetBitDepth() = 0;
    virtual eLGSurfState GetSurfaceState() = 0;
    virtual long GetSurfaceDescription(sLGSurfaceDescription *psDsc) = 0;
};

class ILGDD4Surface : public IUnknown
{
public:
    ILGDD4Surface();
    ILGDD4Surface(const ILGDD4Surface &);
    virtual ~ILGDD4Surface();

    int GetDeviceInfoIndex();
    int GetDirectDraw(IDirectDraw4 **ppDD);
    int GetRenderSurface(IDirectDrawSurface4 **ppRS);
};

BOOL CreateLGSurface(ILGSurface **ppILGSurf);