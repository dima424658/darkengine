#ifdef _WIN32
#include <winerror.h>
#else
/**
 * Adapted from mingw-w64 headers, revision 93f3505a758fe70e56678f00e753af3bc4f640bb.
 * Modified for Dark Engine native POSIX/Windows builds on 2026-11-20.
 * See COPYING.mingw-w64 and DISCLAIMER in this directory.
 */
#pragma once

#include "windef.h"

// HRESULT subset used by the engine, from mingw-w64 winerror.h.
#define NOERROR 0
#define S_OK ((HRESULT)0x00000000L)
#define S_FALSE ((HRESULT)0x00000001L)
#define E_NOTIMPL ((HRESULT)0x80004001L)
#define E_NOINTERFACE ((HRESULT)0x80004002L)
#define E_POINTER ((HRESULT)0x80004003L)
#define E_FAIL ((HRESULT)0x80004005L)
#define E_OUTOFMEMORY ((HRESULT)0x8007000EL)
#define E_INVALIDARG ((HRESULT)0x80070057L)
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#define ResultFromScode(sc) ((HRESULT)(sc))
#define FACILITY_ITF 4
#define MAKE_HRESULT(sev, fac, code) \
   ((HRESULT)(((unsigned __LONG32)(sev) << 31) | ((unsigned __LONG32)(fac) << 16) | \
              ((unsigned __LONG32)(code))))

#endif
