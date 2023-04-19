///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/aggddraw.h $
// $Author: TOML $
// $Date: 1996/11/18 12:41:43 $
// $Revision: 1.3 $
//
// A proxy that provides and aggregateable DirectDraw.
//
// @Note (toml 08-12-96): Because there's now IDirectDraw,
// IDirectDraw2 and IDirectDraw3, I've decided to expose DD
// in a different manner.  This code is dead, but kept for a little
// while as reference.
//

#ifndef __AGGDDRAW_H
#define __AGGDDRAW_H

#include <aggmemb.h>

class cAggDirectDraw : public IDirectDraw
{
public:
    cAggDirectDraw(IUnknown * pOuterUnknown);
    virtual ~cAggDirectDraw();

    void Set(IDirectDraw *);
    void Clear();

private:

    // IUnknown methods
    DECLARE_DELEGATION();

    DECLARE_SIMPLE_AGGREGATE_CONTROL(cAggDirectDraw);

    // IDirectDraw methods
    STDMETHOD (Compact)();
    STDMETHOD (CreateClipper)(DWORD, LPDIRECTDRAWCLIPPER *, IUnknown *);
    STDMETHOD (CreatePalette)(DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE *, IUnknown *);
    STDMETHOD (CreateSurface)(LPDDSURFACEDESC, LPDIRECTDRAWSURFACE *, IUnknown *);
    STDMETHOD (DuplicateSurface)(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE *);
    STDMETHOD (EnumDisplayModes)(DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK);
    STDMETHOD (EnumSurfaces)(DWORD, LPDDSURFACEDESC, LPVOID,LPDDENUMSURFACESCALLBACK);
    STDMETHOD (FlipToGDISurface)();
    STDMETHOD (GetCaps)(LPDDCAPS, LPDDCAPS);
    STDMETHOD (GetDisplayMode)(LPDDSURFACEDESC);
    STDMETHOD (GetFourCCCodes)(LPDWORD, LPDWORD);
    STDMETHOD (GetGDISurface)(LPDIRECTDRAWSURFACE *);
    STDMETHOD (GetMonitorFrequency)(LPDWORD);
    STDMETHOD (GetScanLine)(LPDWORD);
    STDMETHOD (GetVerticalBlankStatus)(LPBOOL);
    STDMETHOD (Initialize)(GUID *);
    STDMETHOD (RestoreDisplayMode)();
    STDMETHOD (SetCooperativeLevel)(HWND, DWORD);
    STDMETHOD (SetDisplayMode)(DWORD, DWORD,DWORD);
    STDMETHOD (WaitForVerticalBlank)(DWORD, HANDLE);

    IDirectDraw * m_pDirectDraw;
};

#endif /* !__AGGDDRAW_H */
