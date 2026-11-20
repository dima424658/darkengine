#ifdef _WIN32
#include <unknwn.h>
#else
/**
 * Adapted from mingw-w64 headers, revision 93f3505a758fe70e56678f00e753af3bc4f640bb.
 * Modified for Dark Engine native POSIX/Windows builds on 2026-11-20.
 * See COPYING.mingw-w64 and DISCLAIMER in this directory.
 */
#pragma once

#include "basetyps.h"
#include "winerror.h"

#ifndef __IUnknown_INTERFACE_DEFINED__
#define __IUnknown_INTERFACE_DEFINED__

typedef interface IUnknown IUnknown;
typedef IUnknown *LPUNKNOWN;
DEFINE_GUID(IID_IUnknown, 0x00000000, 0x0000, 0x0000,
   0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

#ifdef CONST_VTABLE
#define CONST_VTBL const
#else
#define CONST_VTBL
#endif

#if defined(__cplusplus) && !defined(CINTERFACE)
interface IUnknown
{
   virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) = 0;
   virtual ULONG STDMETHODCALLTYPE AddRef(void) = 0;
   virtual ULONG STDMETHODCALLTYPE Release(void) = 0;
};
#else
typedef struct IUnknownVtbl
{
   HRESULT (STDMETHODCALLTYPE *QueryInterface)(IUnknown *This, REFIID riid, void **ppvObject);
   ULONG (STDMETHODCALLTYPE *AddRef)(IUnknown *This);
   ULONG (STDMETHODCALLTYPE *Release)(IUnknown *This);
} IUnknownVtbl;

interface IUnknown
{
   CONST_VTBL IUnknownVtbl *lpVtbl;
};
#endif
#endif

#ifndef GUID_NULL
EXTERN_C const GUID GUID_NULL;
#endif

#ifndef IUnknown_QueryInterface
#if defined(__cplusplus) && !defined(CINTERFACE)
#define IUnknown_QueryInterface(This, riid, ppvObject) (This)->QueryInterface(riid, ppvObject)
#define IUnknown_AddRef(This) (This)->AddRef()
#define IUnknown_Release(This) (This)->Release()
#else
#define IUnknown_QueryInterface(This, riid, ppvObject) (This)->lpVtbl->QueryInterface(This, riid, ppvObject)
#define IUnknown_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IUnknown_Release(This) (This)->lpVtbl->Release(This)
#endif
#endif

#endif
