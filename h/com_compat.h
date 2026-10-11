#pragma once

/* Minimal COM declaration surface used by Dark's in-process component model. */
#ifndef STDMETHODCALLTYPE
#define STDMETHODCALLTYPE
#endif
#ifndef STDMETHODIMP
#define STDMETHODIMP HRESULT STDMETHODCALLTYPE
#endif
#ifndef STDMETHODIMP_
#define STDMETHODIMP_(type) type STDMETHODCALLTYPE
#endif
#ifdef __cplusplus
#ifndef PURE
#define PURE = 0
#endif
#define STDMETHOD(method) virtual HRESULT STDMETHODCALLTYPE method
#define STDMETHOD_(type, method) virtual type STDMETHODCALLTYPE method
#define DECLARE_INTERFACE(iface) interface iface
#define DECLARE_INTERFACE_(iface, baseiface) interface iface : public baseiface
#define THIS
#define THIS_
#else
#ifndef PURE
#define PURE
#endif
#define STDMETHOD(method) HRESULT (STDMETHODCALLTYPE *method)
#define STDMETHOD_(type, method) type (STDMETHODCALLTYPE *method)
#define DECLARE_INTERFACE(iface) \
   typedef interface iface { const struct iface##Vtbl *lpVtbl; } iface; \
   typedef const struct iface##Vtbl iface##Vtbl; \
   struct iface##Vtbl
#define DECLARE_INTERFACE_(iface, baseiface) DECLARE_INTERFACE(iface)
#define THIS INTERFACE *This
#define THIS_ INTERFACE *This,
#endif

#ifndef NOERROR
#define NOERROR ((HRESULT)0)
#endif
#ifndef S_OK
#define S_OK ((HRESULT)0)
#endif
#ifndef S_FALSE
#define S_FALSE ((HRESULT)1)
#endif
#ifndef E_NOTIMPL
#define E_NOTIMPL ((HRESULT)0x80004001L)
#endif
#ifndef E_NOINTERFACE
#define E_NOINTERFACE ((HRESULT)0x80004002L)
#endif
#ifndef E_POINTER
#define E_POINTER ((HRESULT)0x80004003L)
#endif
#ifndef E_FAIL
#define E_FAIL ((HRESULT)0x80004005L)
#endif
#ifndef E_INVALIDARG
#define E_INVALIDARG ((HRESULT)0x80070057L)
#endif
#ifndef SUCCEEDED
#define SUCCEEDED(result) ((HRESULT)(result) >= 0)
#endif
#ifndef FAILED
#define FAILED(result) ((HRESULT)(result) < 0)
#endif
#ifndef ResultFromScode
#define ResultFromScode(result) ((HRESULT)(result))
#endif
