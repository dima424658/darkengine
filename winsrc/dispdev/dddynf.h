///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/dispdev/RCS/dddynf.h $
// $Author: KEVIN $
// $Date: 1997/10/10 15:09:36 $
// $Revision: 1.2 $
//
// Dynamic function loading for windisp

#ifndef __DDDYNF_H
#define __DDDYNF_H

#include <dynfunc.h>

#if defined(__DDRAW_INCLUDED__)
DeclDynFunc_(HRESULT, WINAPI, DirectDrawCreate, (GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter));
#define DynDirectDrawCreate     (DynFunc(DirectDrawCreate).GetProcAddress())
DeclDynFunc_(HRESULT, WINAPI, DirectDrawEnumerateA, (LPDDENUMCALLBACK cb, void *data));
#define DynDirectDrawEnumerate  (DynFunc(DirectDrawEnumerateA).GetProcAddress())
#endif

BOOL LoadDirectDraw();

#endif /* !__WDSPDYNF_H */
