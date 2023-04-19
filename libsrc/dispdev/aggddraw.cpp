///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/windisp/RCS/aggddraw.cpp $
// $Author: TOML $
// $Date: 1996/10/10 14:10:15 $
// $Revision: 1.4 $
//
// @Note (toml 08-12-96): Because there's now IDirectDraw,
// IDirectDraw2 and IDirectDraw3, I've decided to expose DD
// in a different manner.  This code is dead, but kept for a little
// while as reference.
//

#include <windows.h>
#include <lg.h>
#ifdef _MSC_VER
#pragma message ("Add DirectX directory as a global - use Tools:Options")
#endif
#include <ddraw.h>
#include <comtools.h>
#include <aggmemb.h>
#include <aggddraw.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAggDirectDraw
//

//
// Pre-fab COM implementations
//
IMPLEMENT_DELEGATION(cAggDirectDraw);
IMPLEMENT_SIMPLE_AGGREGATE_CONTROL_DELETE_CLIENT(cAggDirectDraw);

///////////////////////////////////////

cAggDirectDraw::cAggDirectDraw(IUnknown * pOuterUnknown)
  : m_pDirectDraw(NULL)
    {
    INIT_DELEGATION(pOuterUnknown);
    INIT_AGGREGATE_CONTROL(this);

    // Add internal components to outer aggregate...
    AddToAggregate1(pOuterUnknown,
                    IID_IDirectDraw, this,
                    &__m_AggregateControl,
                    kPriorityNormal,
                    NULL);
    }

///////////////////////////////////////

cAggDirectDraw::~cAggDirectDraw()
    {
    }

///////////////////////////////////////

void cAggDirectDraw::Set(IDirectDraw * pDirectDraw)
    {
    m_pDirectDraw = pDirectDraw;
    m_pDirectDraw->AddRef();
    }

///////////////////////////////////////

void cAggDirectDraw::Clear()
    {
    SafeRelease(m_pDirectDraw);
    }

///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::Compact()
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->Compact();
    return E_FAIL;
    }


///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::CreateClipper(DWORD dwFlags,LPDIRECTDRAWCLIPPER *lplpDDClipper,IUnknown *pUnkOuter)
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->CreateClipper(dwFlags,lplpDDClipper,pUnkOuter);
    return E_FAIL;
    }


///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::CreatePalette(DWORD dwFlags,LPPALETTEENTRY lpColorTable,LPDIRECTDRAWPALETTE *lplpDDPalette,IUnknown *pUnkOuter)
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->CreatePalette(dwFlags,lpColorTable,lplpDDPalette,pUnkOuter);
    return E_FAIL;
    }


///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::CreateSurface(LPDDSURFACEDESC lpDDSurfaceDesc,LPDIRECTDRAWSURFACE *lplpDDSurface,IUnknown *pUnkOuter)
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->CreateSurface(lpDDSurfaceDesc,lplpDDSurface,pUnkOuter);
    return E_FAIL;
    }


///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::DuplicateSurface(LPDIRECTDRAWSURFACE lpDDSurface,LPDIRECTDRAWSURFACE *lplpDupDDSurface)
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->DuplicateSurface(lpDDSurface,lplpDupDDSurface);
    return E_FAIL;
    }


///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::EnumDisplayModes(DWORD dwSurfaceDescCount,LPDDSURFACEDESC lplpDDSurfaceDescList,LPVOID lpContext,LPDDENUMMODESCALLBACK lpEnumCallback)
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->EnumDisplayModes(dwSurfaceDescCount,lplpDDSurfaceDescList,lpContext,lpEnumCallback);
    return E_FAIL;
    }


///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::EnumSurfaces(DWORD dwFlags,LPDDSURFACEDESC lpDDSD,LPVOID lpContext,LPDDENUMSURFACESCALLBACK lpEnumCallback)
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->EnumSurfaces(dwFlags,lpDDSD,lpContext,lpEnumCallback);
    return E_FAIL;
    }


///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::FlipToGDISurface()
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->FlipToGDISurface();
    return E_FAIL;
    }


///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::GetCaps(LPDDCAPS lpDDDriverCaps,LPDDCAPS lpDDHELCaps)
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->GetCaps(lpDDDriverCaps,lpDDHELCaps);
    return E_FAIL;
    }


///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::GetDisplayMode(LPDDSURFACEDESC lpDDSurfaceDesc)
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->GetDisplayMode(lpDDSurfaceDesc);
    return E_FAIL;
    }


///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::GetFourCCCodes(LPDWORD lpNumCodes,LPDWORD lpCodes)
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->GetFourCCCodes(lpNumCodes,lpCodes);
    return E_FAIL;
    }


///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::GetGDISurface(LPDIRECTDRAWSURFACE *lplpGDIDDSurface)
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->GetGDISurface(lplpGDIDDSurface);
    return E_FAIL;
    }


///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::GetMonitorFrequency(LPDWORD lpdwFrequency)
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->GetMonitorFrequency(lpdwFrequency);
    return E_FAIL;
    }


///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::GetScanLine(LPDWORD lpdwScanLine)
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->GetScanLine(lpdwScanLine);
    return E_FAIL;
    }


///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::GetVerticalBlankStatus(LPBOOL lpblsInVB)
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->GetVerticalBlankStatus(lpblsInVB);
    return E_FAIL;
    }


///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::Initialize(GUID *lpGUID)
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->Initialize(lpGUID);
    return E_FAIL;
    }


///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::RestoreDisplayMode()
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->RestoreDisplayMode();
    return E_FAIL;
    }


///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::SetCooperativeLevel(HWND hWnd,DWORD dwFlags)
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->SetCooperativeLevel(hWnd,dwFlags);
    return E_FAIL;
    }


///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::SetDisplayMode(DWORD dwWidth,DWORD dwHeight,DWORD dwBpp)
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->SetDisplayMode(dwWidth,dwHeight,dwBpp);
    return E_FAIL;
    }


///////////////////////////////////////

STDMETHODIMP cAggDirectDraw::WaitForVerticalBlank(DWORD dwFlags,HANDLE hEvent)
    {
    if (m_pDirectDraw)
        return m_pDirectDraw->WaitForVerticalBlank(dwFlags,hEvent);
    return E_FAIL;
    }


///////////////////////////////////////

