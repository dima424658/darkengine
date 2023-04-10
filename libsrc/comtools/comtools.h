//////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/comtools/rcs/comtools.h $
// $Author: Johan $
// $Date: 1999/04/21 17:29:31 $
// $Revision: 1.55 $
// Description :
//
// comtools.h: Tools for using and implementing COM component objects (toml)
//
// (c) Copyright 1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//

#pragma once

#ifndef __COMTOOLS_H
#define __COMTOOLS_H

#include <types.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////////
//
// Usage macros & classes
//

// Commonly used base macros
#ifdef __cplusplus
#define IID_TO_REFIID(id)               id
#define __COMCallFunc(p, Func)          (p)->Func
#define __COMCallFuncPtr(p, FuncPtr)    ((p)->*FuncPtr)
#define __COMThis(p)
#define __COMThis_(p)
#else
#define IID_TO_REFIID(id)               &(id)
#define __COMCallFunc(p, Func)          ((p)->lpVtbl)->Func
#define __COMCallFuncPtr(p, FuncPtr)    (*FuncPtr)
#define __COMThis(p)                    p
#define __COMThis_(p)                   p,
#endif

///////////////////////////////////////
//
// Generic macros to use when implmenting C wrapper macros.
// These will work in both C and C++ source.
//

#ifndef COMTOOL_NO_WRAPPERS

#define COMCall0(p, Func)                                           __COMCallFunc(p, Func) (__COMThis (p))
#define COMCall1(p, Func, p1)                                       __COMCallFunc(p, Func) (__COMThis_(p) p1)
#define COMCall2(p, Func, p1, p2)                                   __COMCallFunc(p, Func) (__COMThis_(p) p1, p2)
#define COMCall3(p, Func, p1, p2, p3)                               __COMCallFunc(p, Func) (__COMThis_(p) p1, p2, p3)
#define COMCall4(p, Func, p1, p2, p3, p4)                           __COMCallFunc(p, Func) (__COMThis_(p) p1, p2, p3, p4)
#define COMCall5(p, Func, p1, p2, p3, p4, p5)                       __COMCallFunc(p, Func) (__COMThis_(p) p1, p2, p3, p4, p5)
#define COMCall6(p, Func, p1, p2, p3, p4, p5, p6)                   __COMCallFunc(p, Func) (__COMThis_(p) p1, p2, p3, p4, p5, p6)
#define COMCall7(p, Func, p1, p2, p3, p4, p5, p6, p7)               __COMCallFunc(p, Func) (__COMThis_(p) p1, p2, p3, p4, p5, p6, p7)
#define COMCall8(p, Func, p1, p2, p3, p4, p5, p6, p7, p8)           __COMCallFunc(p, Func) (__COMThis_(p) p1, p2, p3, p4, p5, p6, p7, p8)
#define COMCall9(p, Func, p1, p2, p3, p4, p5, p6, p7, p8, p9)       __COMCallFunc(p, Func) (__COMThis_(p) p1, p2, p3, p4, p5, p6, p7, p8, p9)
#define COMCall10(p, Func, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10) __COMCallFunc(p, Func) (__COMThis_(p) p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)

#define COMQueryInterface(p, a, b)      COMCall2(p, QueryInterface, IID_TO_REFIID(a), b)
#define COMAddRef(p)                    COMCall0(p, AddRef)
#define COMRelease(p)                   COMCall0(p, Release)

#endif /* COMTOOL_NO_WRAPPERS */

///////////////////////////////////////////////////////////////////////////////
//
// Definition of IUnknown cribbed Windows headers so we are not forced to
// include big Windows headers
//

#ifndef _WIN32
#define __based(a)
#define __segname(a)
#endif

#ifndef CDECL
#define CDECL
#endif

#ifndef FAR
#define FAR
#endif
typedef long HRESULT;
typedef ulong ULONG;
typedef struct _GUID GUID;
typedef struct _GUID IID;
typedef struct _GUID CLSID;
#ifndef __IID_DEFINED__
#if defined(__cplusplus)
#ifndef _REFGUID_DEFINED
#define _REFGUID_DEFINED
#define REFGUID             const GUID &
#endif // !_REFGUID_DEFINED
#ifndef _REFIID_DEFINED
#define _REFIID_DEFINED
#define REFIID              const IID &
#endif // !_REFIID_DEFINED
#ifndef _REFCLSID_DEFINED
#define _REFCLSID_DEFINED
#define REFCLSID            const CLSID &
#endif // !_REFCLSID_DEFINED
#else // !__cplusplus
#ifndef _REFGUID_DEFINED
#define _REFGUID_DEFINED
#define REFGUID             const GUID * const
#endif // !_REFGUID_DEFINED
#ifndef _REFIID_DEFINED
#define _REFIID_DEFINED
#define REFIID              const IID * const
#endif // !_REFIID_DEFINED
#ifndef _REFCLSID_DEFINED
#define _REFCLSID_DEFINED
#define REFCLSID            const CLSID * const
#endif // !_REFCLSID_DEFINED
#endif // !__cplusplus
#endif // !__IID_DEFINED__

#if defined( _WIN32 ) && !defined( _NO_COM )
#include <winerror.h>
#include <basetyps.h>
#else
//#error "Need objbase.h equivalent for target platform"
#include <nt\winerror.h>
#include <nt\basetyps.h>
#endif


#ifndef __IUnknown_INTERFACE_DEFINED__
#define __IUnknown_INTERFACE_DEFINED__

DEFINE_GUID(IID_IUnknown, 0x00000000L, 0, 0, 0xC0,0,0,0,0,0,0,0x46);

#ifdef CONST_VTABLE
#define CONST_VTBL const
#else
#define CONST_VTBL
#endif

typedef struct IUnknown  IUnknown;

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface IUnknown
    {
    public:
        virtual HRESULT __stdcall QueryInterface(
            /* [in] */ REFIID riid,
            /* [out] */ void  **ppvObject) = 0;

        virtual ULONG __stdcall AddRef( void) = 0;

        virtual ULONG __stdcall Release( void) = 0;

    };

#else   /* C style interface */

    typedef struct IUnknownVtbl
    {

        HRESULT ( __stdcall  *QueryInterface )(
            IUnknown  * This,
            /* [in] */ REFIID riid,
            /* [out] */ void  **ppvObject);

        ULONG ( __stdcall  *AddRef )(
            IUnknown  * This);

        ULONG ( __stdcall  *Release )(
            IUnknown  * This);

    } IUnknownVtbl;

    interface IUnknown
    {
        CONST_VTBL struct IUnknownVtbl  *lpVtbl;
    };

#endif
#endif

#ifndef __cplusplus
#define IUnknown_QueryInterface(This,riid,ppvObject)    \
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUnknown_AddRef(This)   \
    (This)->lpVtbl -> AddRef(This)

#define IUnknown_Release(This)  \
    (This)->lpVtbl -> Release(This)

#else
#define IUnknown_QueryInterface(p, a, b)   COMQueryInterface(p, a, b)
#define IUnknown_AddRef(p)                 COMAddRef(p)
#define IUnknown_Release(p)                COMRelease(p)
#endif


#ifndef _OBJBASE_H_
#ifdef __cplusplus
inline BOOL IsEqualGUID(REFGUID rguid1, REFGUID rguid2)
{
    return !memcmp(&rguid1, &rguid2, sizeof(GUID));
}
#else   //  ! __cplusplus
#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))
#endif  //  __cplusplus

#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)

#ifdef __cplusplus

// because GUID is defined elsewhere in WIN32 land, the operator == and !=
// are moved outside the class to global scope.

inline BOOL operator==(const GUID& guidOne, const GUID& guidOther)
{
#ifdef _WIN32
    return !memcmp(&guidOne,&guidOther,sizeof(GUID));
#else
    return !_fmemcmp(&guidOne,&guidOther,sizeof(GUID));
#endif
}

inline BOOL operator!=(const GUID& guidOne, const GUID& guidOther)
{
    return !(guidOne == guidOther);
}

#endif // __cplusplus
#endif

#undef INTERFACE

#ifndef __CGUID_H__
#if defined(__WATCOMC__)
EXTERN const IID __cdecl GUID_NULL;
#else
EXTERN const IID GUID_NULL;
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
//
// "Interface Pointers," IPtrs: These macros provide automatic query &
// Release() interface pointers. These simplify interface client code by:
//      1. Eliminating the need to explicitly QueryInterface()
//      2. Eliminating the need to Release() at every return point
//
// Forms are available for finer control if default naming too is restrictive
//
// IPtrs These are only available in C and C++ source compiled using the C++
// front end.
//
// The macros are used to provide consistent naming and promote usage by
// virtue of convenience, but the templates may be used directly.
//

#if !defined(COMTOOL_NO_IPTRS)
#ifdef __cplusplus

//
// DeclIPtr(): Declare a non-self releasing pointer
//
#define DeclIPtr(INTERFACE_ROOT)                        cIPtr< I##INTERFACE_ROOT >      p##INTERFACE_ROOT
#define DeclMemberIPtr(INTERFACE_ROOT)                  cIPtr< I##INTERFACE_ROOT >      m_p##INTERFACE_ROOT
#define DeclIPtr_(INTERFACE_ROOT, PtrIdent)             cIPtr< I##INTERFACE_ROOT >      PtrIdent


//
// AutoIPtr(): Declare a generic self-releasing pointer
//
#define AutoIPtr(INTERFACE_ROOT)                        cAutoIPtr< I##INTERFACE_ROOT >  p##INTERFACE_ROOT
#define AutoMemberIPtr(INTERFACE_ROOT)                  cAutoIPtr< I##INTERFACE_ROOT >  m_p##INTERFACE_ROOT
#define AutoIPtr_(INTERFACE_ROOT, PtrIdent)             cAutoIPtr< I##INTERFACE_ROOT >  PtrIdent


//
// AutoConIPtr(): Declare and connect a generic self-releasing pointer
//
#define AutoConIPtr(INTERFACE_ROOT, pQuery)             cAutoIPtr< I##INTERFACE_ROOT >  p##INTERFACE_ROOT   ( IID_I##INTERFACE_ROOT, (pQuery) )
#define AutoConIPtr_(INTERFACE_ROOT, PtrIdent, pQuery)  cAutoIPtr< I##INTERFACE_ROOT >  PtrIdent            ( IID_I##INTERFACE_ROOT, (pQuery) )


//
// ConIPtr(): Connect an interface declared with either DeclIPtr() or AutoIPtr()
//
#define ConIPtr(INTERFACE_ROOT, pQuery)                 p##INTERFACE_ROOT.Connect( IID_##INTERFACE_ROOT, (pQuery) )
#define ConIPtr_(INTERFACE_ROOT, pQuery, PtrIdent)             (PtrIdent).Connect( IID_##INTERFACE_ROOT, (pQuery) )


//
// Application aggregate aware IPtrs
//
#define AutoAppIPtr(INTERFACE_ROOT)                     cAutoIPtr<I##INTERFACE_ROOT>    p##INTERFACE_ROOT   ( AppGetObj( I##INTERFACE_ROOT ) )
#define AutoAppIPtr_(INTERFACE_ROOT, PtrIdent)          cAutoIPtr<I##INTERFACE_ROOT>    PtrIdent            ( AppGetObj( I##INTERFACE_ROOT ) )
#define ConAppIPtr(INTERFACE_ROOT)                      p##INTERFACE_ROOT.Connect( AppGetObj( I##INTERFACE_ROOT ) )
#define ConAppIPtr_(INTERFACE_ROOT, PtrIdent)                  (PtrIdent).Connect( AppGetObj( I##INTERFACE_ROOT ) )


///////////////////////////////////////
//
// CLASS: cIPtrBase
//
// Provides root smart pointer functionality
//

class cIPtrBase
{
public:
    //
    // Pointer operators
    //
        operator void *();
        operator IUnknown *() const;
    int operator =(int);
    int operator !() const;

    int operator==(void * p)                      { return ((void *)pUnknown == p); }
    int operator!=(void * p)                      { return ((void *)pUnknown != p); }

    //
    // IPtr services
    //
    void DoSafeRelease();
    void Connect(REFIID id, IUnknown *pUnknownFrom);
    void Connect(IUnknown * pUnknownInit);

    cIPtrBase *         GetRealPtr();
    const cIPtrBase *   GetRealPtr() const;

    IUnknown * pUnknown;

public:
    //
    // Constructors
    //
    // @Note (toml 08-15-96): Normally these would be protected but for a bug in Watcom
    //
    cIPtrBase();
    cIPtrBase(IUnknown * pUnknownInit);
    cIPtrBase(REFIID id, IUnknown *pUnknownFrom);

private:                                       // @Note (toml 08-20-96): This shouldn't have to be protected, but Watcom sucks!
    cIPtrBase(const cIPtrBase &)        {};
    void operator=(const cIPtrBase &)   {};
};

///////////////////////////////////////

inline cIPtrBase::cIPtrBase()
{
    pUnknown = 0;
}

///////////////////

inline cIPtrBase::cIPtrBase(IUnknown * pUnknownInit)
  : pUnknown(pUnknownInit)
{
}

///////////////////

inline cIPtrBase::cIPtrBase(REFIID id, IUnknown * pUnknownFrom)
{
    if (!pUnknownFrom || pUnknownFrom->QueryInterface(id, (void **)(&pUnknown)) != 0)
        pUnknown = 0;
}

///////////////////

inline cIPtrBase::operator void *()
{
    return (void *)(pUnknown);
}

///////////////////

inline cIPtrBase::operator IUnknown *() const
{
    return pUnknown;
}

///////////////////

inline int cIPtrBase::operator=(int)
{
    return int(pUnknown = 0);
}

///////////////////

inline int cIPtrBase::operator !() const
{
    return !pUnknown;
}

///////////////////

inline void cIPtrBase::DoSafeRelease()
{
    if (pUnknown)
        pUnknown->Release();
    pUnknown = 0;
}

///////////////////

inline cIPtrBase * cIPtrBase::GetRealPtr()
{
    return this;
}

///////////////////

inline const cIPtrBase * cIPtrBase::GetRealPtr() const
{
    return this;
}

///////////////////

inline void cIPtrBase::Connect(REFIID id, IUnknown * pUnknownFrom)
{
    if (!pUnknownFrom || pUnknownFrom->QueryInterface(id, (void **)(&pUnknown)) != 0)
        pUnknown = 0;
}

///////////////////

inline void cIPtrBase::Connect(IUnknown * pUnknownInit)
{
    pUnknown = pUnknownInit;
}


///////////////////////////////////////
//
// TEMPLATE: cIPtr
//
// Provides simple templatization of smart pointer
//

#undef INTERFACE

template <class INTERFACE>
class cIPtr : public cIPtrBase
{
public:
    cIPtr()                                                                             {};
    cIPtr(INTERFACE * pUnknownInit)  : cIPtrBase((IUnknown *)pUnknownInit)              {}; // A redundant cast is done so this class can be used in headers that only forward-declare the target interface
    cIPtr(REFIID id, IUnknown * pUnknownFrom)   : cIPtrBase(id, pUnknownFrom)           {};

    //
    // Pointer operators
    //
                 operator INTERFACE *() const   { return (INTERFACE *)(cIPtrBase::operator IUnknown*()); }
    INTERFACE *  operator->() const             { return (INTERFACE *)(cIPtrBase::operator IUnknown*()); }
    INTERFACE &  operator *() const             { return *((INTERFACE *)(cIPtrBase::operator IUnknown*())); }
    INTERFACE ** operator &() const             { return (INTERFACE **)(&pUnknown); }
    INTERFACE *  operator =(INTERFACE * p)      { pUnknown = (IUnknown *)(p); return p; }
    int operator=(int)                          { return cIPtrBase::operator=(0); }

    int operator==(INTERFACE * p)               { return cIPtrBase::operator==((void*)p); }
    int operator!=(INTERFACE * p)               { return cIPtrBase::operator!=((void*)p); }

private:
    cIPtr(const cIPtr<INTERFACE> &)             {};
    void operator=(const cIPtr<INTERFACE> &)    {};
};


///////////////////////////////////////
//
// TEMPLATE: cAutoIPtr
//
// A self-releasing interface pointer
//

template <class INTERFACE>
class cAutoIPtr : public cIPtr<INTERFACE>
{
public:
    cAutoIPtr()                                     {};
    cAutoIPtr(INTERFACE * pUnknownInit)             : cIPtr<INTERFACE>(pUnknownInit)       {};
    cAutoIPtr(REFIID id, IUnknown * pUnknownFrom)   : cIPtr<INTERFACE>(id, pUnknownFrom)   {};

    ~cAutoIPtr()                                    { if (pUnknown) pUnknown->Release(); }

    INTERFACE * operator=(INTERFACE * p)            { return cIPtr<INTERFACE>::operator=(p); }
    int operator=(int)                              { return cIPtr<INTERFACE>::operator=(0); }

private:
    cAutoIPtr(const cAutoIPtr<INTERFACE> &)         {};
    void operator=(const cAutoIPtr<INTERFACE> &)    {};
};

#endif /* __cplusplus */
#endif /* !defined(COMTOOL_NO_IPTRS) */


///////////////////////////////////////
//
// SafeRelease()
//
// Release a pointer if its not NULL
// This should do the right thing for both real and smart pointers
//
// SafeRelease() is available from both C and C++ regardless of front end.
//

#if !defined(COMTOOL_NO_SAFE_RELEASE)

#define SafeRelease(pUnknown) \
    { \
        if ((IUnknown *)(pUnknown)) \
            COMRelease(((IUnknown *)(pUnknown))); \
        pUnknown = 0; \
    }

#endif /* !defined(COMTOOL_NO_IPTRS) */


///////////////////////////////////////
//
// Root macros for implementing tight loop optimization helpers.
//
// Allow access of COM interface function more directly
// by copying the desired function pointer out of the vtbl
// onto the stack.  Remember to use THIS macro in ParamList
//

#if !defined(COMTOOL_NO_LOOPHELPER)

#if defined(FORCE_DEBUG_LOOPHELPER) || !defined(DEBUG)
#define COMObjCriticalUse_Begin(p, RetVal, INTERFACE, FuncName, FuncIndex, ParamList) \
    do \
    { \
        __COMFuncTypeDecl(RetVal, INTERFACE, ParamList, __COMFPName(p, INTERFACE, FuncName)) = \
            __COMGetFuncPtr(p, __COMFuncTypeDecl(RetVal, INTERFACE, ParamList), FuncIndex)

//
// Make call
//
#define COMObjCriticalUse_Call0(p, INTERFACE, Func)                                             __COMCallFuncPtr(p, __COMFPName(p, INTERFACE, Func)) (__COMThis(p))
#define COMObjCriticalUse_Call1(p, INTERFACE, Func, p1)                                         __COMCallFuncPtr(p, __COMFPName(p, INTERFACE, Func)) (__COMThis_(p) p1)
#define COMObjCriticalUse_Call2(p, INTERFACE, Func, p1, p2)                                     __COMCallFuncPtr(p, __COMFPName(p, INTERFACE, Func)) (__COMThis_(p) p1, p2)
#define COMObjCriticalUse_Call3(p, INTERFACE, Func, p1, p2, p3)                                 __COMCallFuncPtr(p, __COMFPName(p, INTERFACE, Func)) (__COMThis_(p) p1, p2, p3)
#define COMObjCriticalUse_Call4(p, INTERFACE, Func, p1, p2, p3, p4)                             __COMCallFuncPtr(p, __COMFPName(p, INTERFACE, Func)) (__COMThis_(p) p1, p2, p3, p4)
#define COMObjCriticalUse_Call5(p, INTERFACE, Func, p1, p2, p3, p4, p5)                         __COMCallFuncPtr(p, __COMFPName(p, INTERFACE, Func)) (__COMThis_(p) p1, p2, p3, p4, p5)
#define COMObjCriticalUse_Call6(p, INTERFACE, Func, p1, p2, p3, p4, p5, p6)                     __COMCallFuncPtr(p, __COMFPName(p, INTERFACE, Func)) (__COMThis_(p) p1, p2, p3, p4, p5, p6)
#define COMObjCriticalUse_Call7(p, INTERFACE, Func, p1, p2, p3, p4, p5, p6, p7)                 __COMCallFuncPtr(p, __COMFPName(p, INTERFACE, Func)) (__COMThis_(p) p1, p2, p3, p4, p5, p6, p7)
#define COMObjCriticalUse_Call8(p, INTERFACE, Func, p1, p2, p3, p4, p5, p6, p7, p8)             __COMCallFuncPtr(p, __COMFPName(p, INTERFACE, Func)) (__COMThis_(p) p1, p2, p3, p4, p5, p6, p7, p8)
#define COMObjCriticalUse_Call9(p, INTERFACE, Func, p1, p2, p3, p4, p5, p6, p7, p8, p9)         __COMCallFuncPtr(p, __COMFPName(p, INTERFACE, Func)) (__COMThis_(p) p1, p2, p3, p4, p5, p6, p7, p8, p9)
#define COMObjCriticalUse_Call10(p, INTERFACE, Func, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)   __COMCallFuncPtr(p, __COMFPName(p, INTERFACE, Func)) (__COMThis_(p) p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)

//
// End scope
//
#define COMObjCriticalUse_End(p) } while (0)

#else
//
// We want to have type checking in debug build.
//
#define COMObjCriticalUse_Begin(p, RetVal, INTERFACE, Func, FuncIndex, ParamList)               do {

#define COMObjCriticalUse_Call0(p, INTERFACE, Func)                                             COMCall0(p, Func)
#define COMObjCriticalUse_Call1(p, INTERFACE, Func, p1)                                         COMCall1(p, Func, p1)
#define COMObjCriticalUse_Call2(p, INTERFACE, Func, p1, p2)                                     COMCall2(p, Func, p1, p2)
#define COMObjCriticalUse_Call3(p, INTERFACE, Func, p1, p2, p3)                                 COMCall3(p, Func, p1, p2, p3)
#define COMObjCriticalUse_Call4(p, INTERFACE, Func, p1, p2, p3, p4)                             COMCall4(p, Func, p1, p2, p3, p4)
#define COMObjCriticalUse_Call5(p, INTERFACE, Func, p1, p2, p3, p4, p5)                         COMCall5(p, Func, p1, p2, p3, p4, p5)
#define COMObjCriticalUse_Call6(p, INTERFACE, Func, p1, p2, p3, p4, p5, p6)                     COMCall6(p, Func, p1, p2, p3, p4, p5, p6)
#define COMObjCriticalUse_Call7(p, INTERFACE, Func, p1, p2, p3, p4, p5, p6, p7)                 COMCall7(p, Func, p1, p2, p3, p4, p5, p6, p7)
#define COMObjCriticalUse_Call8(p, INTERFACE, Func, p1, p2, p3, p4, p5, p6, p7, p8)             COMCall8(p, Func, p1, p2, p3, p4, p5, p6, p7, p8)
#define COMObjCriticalUse_Call9(p, INTERFACE, Func, p1, p2, p3, p4, p5, p6, p7, p8, p9)         COMCall9(p, Func, p1, p2, p3, p4, p5, p6, p7, p8, p9)
#define COMObjCriticalUse_Call10(p, INTERFACE, Func, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)   COMCall0(p, Func, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)

#define COMObjCriticalUse_End(p)                                                                } while (0)

#endif

///////////////////////////////////////

#ifdef __cplusplus
#define __COMFuncType(RetVal, INTERFACE, Params) \
    (RetVal (STDMETHODCALLTYPE INTERFACE::*)Params)

#define __COMFuncTypeDecl(RetVal, INTERFACE, Params, Ident) \
    RetVal (STDMETHODCALLTYPE INTERFACE::*Ident)Params

#else
// @TBD (toml 01-14-96): Need to verify C version
#define __COMFuncType(RetVal, INTERFACE, Params) \
    (RetVal (STDMETHODCALLTYPE *)Params)

#define __COMFuncTypeDecl(RetVal, INTERFACE, Params, Ident) \
    RetVal (STDMETHODCALLTYPE *Ident)Params

#endif

#define __COMGetFuncPtr(p, FuncType, FuncIndex) \
    (FuncType)(((*(unsigned long**)p)[FuncIndex]))

#define __COMFPName(p, INTERFACE, Func) \
    p ## _ ## INTERFACE ## _ ## Func


#endif /* COMTOOL_NO_LOOPHELPER */


///////////////////////////////////////////////////////////////////////////////
//
// Implementation macros. Use these when implementing a COM object to simplify
// the effort and centralize QI/AddRef/Release and aggregation management.
// All are available to C And C++ code compiled using the C++ front end.
//

//
// Subsystem identifiers @TBD: Move to core header
//
// Each library has a unique 8-bit identifier, used by the unified error
// handling system, and soon by other systems.
//
// The ranges are:
//
//    0 - 63:   Application defined
//   64 - 127:  Reserved
//  128 - 255:  Technology libraries
//
//
// It is recommended subsystems identifiers be defined as (Base + Offset)
// rather than direct values so the whole sets can be shifted if needed.
//
// Preprocessor defines are used so IDs are available in Windows resource
// scripts.
//

typedef unsigned char   tSubsystemID;

#define kFirstAppSubsystem      0
#define kLastAppSubsystem       63
#define kFirstTechSubsystem     128
#define kLastTechSubsystem      255


//
// Central error handling
//
// These model 1-1 Windows HRESULTS, but dispense with some
// Microsoft-isms, and add support for "subsystem ids"
//
typedef HRESULT         tResult;
typedef unsigned char   tResultCode;

#define kNoError        0L

// Severity values
#define kSuccess    0
#define kError      1


// Generic test for success on any status value (non-negative numbers indicate success).
#define Succeeded(Status) ((HRESULT)(Status) >= 0)

// and the inverse
#define Failed(Status) ((HRESULT)(Status)<0)

// Generic test for error on any status value.
#define IsError(Status) ((unsigned long)(Status) & 0x80000000L)


// Make a result
#define MakeResult(severity, subsystem, code) \
    MAKE_HRESULT(severity, FACILITY_ITF, ((unsigned long)(subsystem) << 8) | ((unsigned long)(code)))

#define FACILITY_ITF    4
#define MAKE_HRESULT(sev,fac,code) \
    ((HRESULT) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )



//
// The standard headers define IsEqualIID() which returns 0 or 1, but
// we want also something that is more informative for complex
// data structures like AVL trees and hash tables.  We also want
// things which take advantage of our in-process address-of shortcut
//
//
// Compare two ids, returning <0, 0, and >0, like strcmp
//
// int CompareGUIDs(REFIID id1, REFIID id2):
// int CompareIIDs(REFIID id1, REFIID id2): returns <0, 0, and >0, like memcmp
// int CompareCLSIDs(REFIID id1, REFIID id2): returns <0, 0, and >0, like memcmp
//
//
#ifdef __cplusplus
#define IsSameGUIDInstance(id1, id2)    ((void *)&(id1) == (void *)&(id2))
#define CompareGUIDContent(id1, id2)    memcmp(&(id1), &(id2), sizeof(GUID))
#else
#define IsSameGUIDInstance(id1, id2)    ((void *)(id1) == (void *)(id2))
#define CompareGUIDContent(id1, id2)    memcmp((id1), (id2), sizeof(GUID))
#endif

#define CompareGUIDs(id1, id2)  ((IsSameGUIDInstance(id1, id2)) ? 0 : CompareGUIDContent(id1, id2))
#define CompareIIDs(id1, id2)   CompareGUIDs(id1, id2)
#define CompareCLSIDs(id1, id2) CompareGUIDs(id1, id2)

//
// For unaggregated QI, test for supported interfaces
//
#define IsEqualOrIUnknownGUID(test, id2) (IsSameGUIDInstance(test, id2) || IsSameGUIDInstance(test, IID_TO_REFIID(IID_IUnknown)) || CompareGUIDContent(test, id2) == 0 || CompareGUIDContent(test, IID_TO_REFIID(IID_IUnknown)) == 0)

// Forward declare an interface
#define F_DECLARE_INTERFACE(iface) \
    interface iface; \
    typedef interface iface iface; \
    EXTERN_C const GUID CDECL FAR IID_##iface

//
// Add default parameter concept to interface macro generalization (this sucks)
//
#ifdef __cplusplus
    #define DEFAULT_TO(expr) = expr
#else
    #define DEFAULT_TO(expr)
#endif

//////////////////////////////////////////////////////////////////////////////
//
// C++ Tools for building IUnknown derivations.  Useful in allowing
// common & changeable implementation of oft-used COM techniques.
//
// Implemented as macros to avoid equally gnarly template/inheritance
// approaches.
//

///////////////////////////////////////
//
// "New" tools, better for MI
//

#ifdef __cplusplus

class cCTRefCount
{
public:
   cCTRefCount()     : ul(1) {}
   ULONG AddRef()    {return ++ul;}
   ULONG Release()   {return --ul;}
   operator ULONG()  {return ul;}
private:
   ULONG ul;
};

//
//
// TEMPLATE: cCTUnaggregated
//
// Implementation is a non-aggregatable COM object
//

enum eCTUnaggregatedFlags
{
   kCTU_Default      = 0x00,
   kCTU_NoSelfDelete = 0x01
};

#ifdef __WATCOMC__
extern int g_ComtoolsQuietWatcom_TRUE; // Calms annoying and inescapable warning about expression always being true (toml 02-19-98)
extern int g_ComtoolsQuietWatcom_FALSE;
#endif

template <class INTERFACE, const GUID * pIID_INTERFACE, const int FLAGS>
class cCTUnaggregated : public INTERFACE
{
public:
   virtual ~cCTUnaggregated()
   {
   }

   virtual void OnFinalRelease()
   {
   }

   STDMETHODIMP QueryInterface(REFIID id, void ** ppI)
   {
      if (!IsEqualOrIUnknownGUID(id, *pIID_INTERFACE))
      {
         *ppI = 0;
         return E_NOINTERFACE;
      }
      *ppI = this;
      AddRef();
      return S_OK;
   }

   STDMETHODIMP_(ULONG) AddRef()
   {
      return __m_ulRefs.AddRef();
   }

   STDMETHODIMP_(ULONG) Release()
   {
      if (__m_ulRefs.Release())
         return __m_ulRefs;

      OnFinalRelease();

#ifdef __WATCOMC__
      if ((!(FLAGS & kCTU_NoSelfDelete) && g_ComtoolsQuietWatcom_TRUE) || g_ComtoolsQuietWatcom_FALSE)
#else
      if (!(FLAGS & kCTU_NoSelfDelete))
#endif
         delete this;

      return 0;
   }

private:
   cCTRefCount __m_ulRefs;
};


template <class INTERFACE1, const GUID * pIID_INTERFACE1, class INTERFACE2, const GUID * pIID_INTERFACE2, const int FLAGS>
class cCTUnaggregated2 : public INTERFACE1, public INTERFACE2
{
public:
   virtual ~cCTUnaggregated2()
   {
   }

   virtual void OnFinalRelease()
   {
   }

   STDMETHODIMP QueryInterface(REFIID id, void ** ppI)
   {
      if (IsEqualGUID(id, *pIID_INTERFACE2))
      {
         AddRef();
         *ppI = (INTERFACE2 *)this;
         return S_OK;
      }

      if (!IsEqualOrIUnknownGUID(id, *pIID_INTERFACE1))
      {
         *ppI = 0;
         return E_NOINTERFACE;
      }
      *ppI = this;
      AddRef();
      return S_OK;
   }

   STDMETHODIMP_(ULONG) AddRef()
   {
      return __m_ulRefs.AddRef();
   }

   STDMETHODIMP_(ULONG) Release()
   {
      if (__m_ulRefs.Release())
         return __m_ulRefs;

      OnFinalRelease();

#ifdef __WATCOMC__
      if ((!(FLAGS & kCTU_NoSelfDelete) && g_ComtoolsQuietWatcom_TRUE) || g_ComtoolsQuietWatcom_FALSE)
#else
      if (!(FLAGS & kCTU_NoSelfDelete))
#endif
         delete this;

      return 0;
   }

private:
   cCTRefCount __m_ulRefs;
};

//
//
// TEMPLATE: cCTDelegating
//
// Implementation is a delegating COM object
//

template <class INTERFACE>
class cCTDelegating : public INTERFACE
{
public:
   cCTDelegating()
    : __m_pOuterUnknown(NULL)
   {

   }

   cCTDelegating(IUnknown * pOuterUnknown)
    : __m_pOuterUnknown(pOuterUnknown)
   {

   }

   virtual ~cCTDelegating()
   {
   }

   void InitDelegation(IUnknown * pOuterUnknown)
   {
      __m_pOuterUnknown = pOuterUnknown;
   }

   STDMETHODIMP QueryInterface(REFIID id, void ** ppI)
   {
      return __m_pOuterUnknown->QueryInterface(id, ppI);
   }
   STDMETHODIMP_(ULONG) AddRef()
   {
      return __m_pOuterUnknown->AddRef();
   }
   STDMETHODIMP_(ULONG) Release()
   {
      return __m_pOuterUnknown->Release();
   }
   
protected:
   // Note this accessor does not AddRef()
   IUnknown * AccessOuter()
   {
      return __m_pOuterUnknown;
   }

private:
   IUnknown * __m_pOuterUnknown;
};

#endif

///////////////////////////////////////
//
// "Old" tools, used prior to quality template support from compilers
//

//
// DECLARE_UNKNOWN_PURE(): Declare Interface QI/AddRef/Release
//
#define DECLARE_UNKNOWN_PURE() \
    \
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void ** ppvObj) PURE; \
    STDMETHOD_(ULONG,AddRef)  (THIS)  PURE; \
    STDMETHOD_(ULONG,Release) (THIS) PURE

#if defined(__cplusplus) && !defined(COMTOOL_NO_IMPLMACS)

//
// DECLARE_UNKNOWN(): Declare Implementation QI/AddRef/Release
//
#define DECLARE_UNKNOWN() \
    \
    STDMETHOD(QueryInterface)(REFIID, void **); \
    STDMETHOD_(ULONG, AddRef)(); \
    STDMETHOD_(ULONG, Release)()

//
// These macros are for implementing interfaces that won't be aggregated.
// Reference counting is handled automatically.
// Client defines function "void OnFinalRelease()"
// The self-delete kind rely on the object being allocated using operator new.
//

//
//  DECLARE_UNAGGREGATABLE()
//
#define DECLARE_UNAGGREGATABLE() \
    \
    DECLARE_UNKNOWN(); \
    void OnFinalRelease(); \
    \
    class cRefCount \
    { \
    public: \
        cRefCount() : ul(1) {} \
        ULONG AddRef() {return ++ul;} \
        ULONG Release() {return --ul;} \
        operator ULONG() {return ul;} \
    private: \
        ULONG ul; \
    } __m_ulRefs

//
//
//  IMPLEMENT_UNAGGREGATABLE()
//
#define IMPLEMENT_UNAGGREGATABLE(CLASS, INTERFACE) \
    __IMPLEMENT_UNAGGREGATABLE_BASE(CLASS, (IsEqualOrIUnknownGUID(id, IID_ ## INTERFACE))) \

#define IMPLEMENT_UNAGGREGATABLE2(CLASS, INTERFACE1, INTERFACE2) \
    __IMPLEMENT_UNAGGREGATABLE_BASE(CLASS, (IsEqualIID(id, IID_ ## INTERFACE1) || IsEqualOrIUnknownGUID(id, IID_ ## INTERFACE2))) \

//
//  IMPLEMENT_UNAGGREGATABLE_SELF_DELETE()
//
#define IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(CLASS, INTERFACE) \
    \
    inline void CLASS::OnFinalRelease() \
    { \
        delete this; \
    } \
    IMPLEMENT_UNAGGREGATABLE(CLASS, INTERFACE)

#define IMPLEMENT_UNAGGREGATABLE2_SELF_DELETE(CLASS, INTERFACE1, INTERFACE2) \
    \
    inline void CLASS::OnFinalRelease() \
    { \
        delete this; \
    } \
    IMPLEMENT_UNAGGREGATABLE2(CLASS, INTERFACE1, INTERFACE2)


//
//  IMPLEMENT_UNAGGREGATABLE_NO_FINAL_RELEASE()
//
#define IMPLEMENT_UNAGGREGATABLE_NO_FINAL_RELEASE(CLASS, INTERFACE) \
    \
    inline void CLASS::OnFinalRelease() \
    { \
    } \
    IMPLEMENT_UNAGGREGATABLE(CLASS, INTERFACE)

#define IMPLEMENT_UNAGGREGATABLE2_NO_FINAL_RELEASE(CLASS, INTERFACE1, INTERFACE2) \
    \
    inline void CLASS::OnFinalRelease() \
    { \
    } \
    IMPLEMENT_UNAGGREGATABLE2(CLASS, INTERFACE1, INTERFACE2)


//
// Base unaggregated implementation of QI/AddRef/Release
//
#define __IMPLEMENT_UNAGGREGATABLE_BASE(CLASS, TEST_EXPR) \
    \
    STDMETHODIMP CLASS::QueryInterface(REFIID id, void ** ppI) \
    { \
        if (!(TEST_EXPR)) \
        { \
            *ppI = 0; \
            return ResultFromScode(E_NOINTERFACE); \
        } \
        *ppI = this; \
        AddRef(); \
        return 0; \
    } \
    \
    STDMETHODIMP_(ULONG) CLASS::AddRef() \
    { \
        return __m_ulRefs.AddRef(); \
    } \
    \
    STDMETHODIMP_(ULONG) CLASS::Release() \
    { \
        if (__m_ulRefs.Release()) \
        { \
            return __m_ulRefs; \
        } \
        OnFinalRelease(); \
        return 0; \
    } \
    class __SEMICOLON_REQUIRED

//
// These macros are for defining interfaces that must be aggregated
// They implement all delegation tasks.
//


//
//  DECLARE_DELEGATION()
//
#define DECLARE_DELEGATION() \
    \
        DECLARE_UNKNOWN(); \
        \
        class cOuterPointer \
        { \
        public: \
            cOuterPointer() : m_pOuterUnknown(NULL) {}; \
            void Init(IUnknown * p) { m_pOuterUnknown = p; }\
            IUnknown *operator ->() {return m_pOuterUnknown;} \
        private: \
            IUnknown * m_pOuterUnknown; \
        } __m_pOuterUnknown


//
//  IMPLEMENT_DELEGATION()
//
#define IMPLEMENT_DELEGATION(CLASS) \
    \
    STDMETHODIMP CLASS::QueryInterface(REFIID id, void ** ppI) \
    { \
        return __m_pOuterUnknown->QueryInterface(id, ppI); \
    } \
    STDMETHODIMP_(ULONG) CLASS::AddRef() \
    { \
        return __m_pOuterUnknown->AddRef(); \
    } \
    STDMETHODIMP_(ULONG) CLASS::Release() \
    { \
        return __m_pOuterUnknown->Release(); \
    } \
    class __SEMICOLON_REQUIRED


//
//  INIT_DELEGATION()
//
#define INIT_DELEGATION(pOuterUnknown)  __m_pOuterUnknown.Init(pOuterUnknown)


//
// These macros are for defining a shell which aggregates a single implementation
// of an interface.  The interface should use delegation.
//

//
//  DECLARE_SINGLE_AGGREGATE()
//
#define DECLARE_SINGLE_AGGREGATE(CLASS) \
    \
    class CLASS ## Shell : public IUnknown \
    { \
        DECLARE_UNKNOWN(); \
        void OnFinalRelease(); \
    public: \
        CLASS ## Shell(); \
        CLASS *GetSafeImpl() {return this ? &Impl : 0;} \
        CLASS &GetImpl() {return Impl;} \
    private: \
        ULONG ulRefs; \
        CLASS Impl; \
    }

//
//  IMPLEMENT_SINGLE_AGGREGATE()
//
#define IMPLEMENT_SINGLE_AGGREGATE(CLASS, INTERFACE) \
    \
    CLASS ## Shell::CLASS ## Shell() \
        : ulRefs(1), \
          Impl(*this) \
    { \
    } \
    \
    STDMETHODIMP CLASS ## Shell::QueryInterface(REFIID id, void **ppI) \
    { \
        *ppI = 0; \
        if (CompareIIDs(id, IID_ ## INTERFACE ) == 0) \
            *ppI = &Impl; \
        else if (CompareIIDs(id, IID_IUnknown) == 0) \
            *ppI = this; \
        else \
            return ResultFromScode(E_NOINTERFACE); \
        static_cast<IUnknown *>(*ppI)->AddRef(); \
        return 0; \
    } \
    \
    STDMETHODIMP_(ULONG) CLASS ## Shell::AddRef() \
    { \
        return ++ulRefs; \
    } \
    \
    STDMETHODIMP_(ULONG) CLASS ## Shell::Release() \
    { \
        if (--ulRefs) \
        { \
            return ulRefs; \
        } \
        OnFinalRelease(); \
        return 0; \
    } \
    class __SEMICOLON_REQUIRED

//
//  IMPLEMENT_SINGLE_AGGREGATE_SELF_DELETE()
//
#define IMPLEMENT_SINGLE_AGGREGATE_SELF_DELETE(CLASS, INTERFACE) \
    inline void CLASS::OnFinalRelease() \
    { \
        delete this; \
    } \
    IMPLEMENT_SINGLE_AGGREGATE(CLASS, INTERFACE)


#endif /* defined(__cplusplus) && !defined(COMTOOL_NO_IMPLMACS) */

//////////////////////////////////////////////////////////////////////////////
//
// C Tools for building IUnknown derivations.  Useful in allowing
// common & changeable implementation of oft-used COM techniques.
//

typedef void (STDMETHODCALLTYPE * tGenericOnFinalReleaseFunc)(IUnknown *);
EXTERN void STDMETHODCALLTYPE NullOnFinalRelease(IUnknown *);

#if !defined(COMTOOL_NO_IMPLMACS)
#if !defined(__cplusplus) || defined(CINTERFACE)

//
// DECLARE_C_UNKNOWN(): Declare C Implementation QI/AddRef/Release
//

#define DECLARE_C_UNKNOWN(interface_implementation_type, iface) \
    \
    EXTERN_C STDMETHODIMP           interface_implementation_type##_QueryInterface  (iface * pInterface, REFIID pIID, void ** ppI); \
    EXTERN_C STDMETHODIMP_(ULONG)   interface_implementation_type##_AddRef          (iface * pInterface); \
    EXTERN_C STDMETHODIMP_(ULONG)   interface_implementation_type##_Release         (iface * pInterface)


//
// DECLARE_C_INTERFACE()
//

#define DECLARE_C_INTERFACE_IMPLEMENTATION(iface) \
    iface           _interface


//
// (BEGIN/END)_C_VTABLE(): Used to create a vtable when implementing in C
//

#define BEGIN_C_VTABLE(interface_implementation_type, iface) \
    \
    DECLARE_C_UNKNOWN(interface_implementation_type, iface); \
    iface##Vtbl interface_implementation_type##_Vtbl =   { \

#define IUNKNOWN_C_VTABLE_ENTRIES(interface_implementation_type) \
    \
    interface_implementation_type##_QueryInterface, \
    interface_implementation_type##_AddRef, \
    interface_implementation_type##_Release

#define END_C_VTABLE() \
}


//
// INIT_C_INTERFACE_IMPLEMENTATION()
//

#define INIT_C_INTERFACE_IMPLEMENTATION(interface_implementation_type, interface_implementation_instance) \
    \
{ \
    AssertMsg((void*)(&interface_implementation_instance._interface) == (void *)(&interface_implementation_instance), "DECLARE_C_INTERFACE_IMPLEMENTATION() must be first member of structure " #interface_implementation_type " to intialize " #interface_implementation_instance); \
    interface_implementation_instance._interface.lpVtbl = &(interface_implementation_type##_Vtbl); \
}


//
// GetInterfaceImplementation(): returns "this"
//

#define GetInterfaceImplementation(interface_implementation_type, piface) \
    \
    ((interface_implementation_type *) ((void *)piface))

//
//  DECLARE_C_UNAGGREGATABLE()
//

#define DECLARE_C_UNAGGREGATABLE(iface) \
    DECLARE_C_INTERFACE_IMPLEMENTATION(iface); \
    ULONG _ulRefs


//
//  IMPLEMENT_C_UNAGGREGATABLE()
//

#define IMPLEMENT_C_UNAGGREGATABLE(interface_implementation_type, iface, fnOnFinalRelease) \
    \
    STDMETHODIMP interface_implementation_type##_QueryInterface(iface * pInterface, REFIID id, void ** ppI) \
    { \
        if (!IsEqualOrIUnknownGUID(id, IID_TO_REFIID(IID_##iface))) \
        { \
            *ppI = 0; \
            return E_NOINTERFACE; \
        } \
        *ppI = (void *)pInterface; \
        COMAddRef(pInterface); \
        return 0; \
    } \
    \
    STDMETHODIMP_(ULONG) interface_implementation_type##_AddRef(iface * pInterface) \
    { \
        return ++(GetInterfaceImplementation(interface_implementation_type, pInterface)->_ulRefs); \
    } \
    \
    STDMETHODIMP_(ULONG) interface_implementation_type##_Release(iface * pInterface) \
    { \
        if (!(--(GetInterfaceImplementation(interface_implementation_type, pInterface)->_ulRefs))) \
        { \
            ((tGenericOnFinalReleaseFunc)(fnOnFinalRelease))((IUnknown *)(pInterface)); \
            return 0; \
        } \
        return GetInterfaceImplementation(interface_implementation_type, pInterface)->_ulRefs; \
    } \
    struct __SEMICOLON_REQUIRED


//
// BEGIN_C_UNAGGREGATABLE_VTABLE(): Used to create a vtable when implementing in C
//

#define BEGIN_C_UNAGGREGATABLE_VTABLE(interface_implementation_type, iface) \
    BEGIN_C_VTABLE(interface_implementation_type, iface) \
        IUNKNOWN_C_VTABLE_ENTRIES(interface_implementation_type),


//
// INIT_C_UNAGGREGATABLE()
//

#define INIT_C_UNAGGREGATABLE(interface_implementation_type, interface_implementation_instance) \
{ \
    INIT_C_INTERFACE_IMPLEMENTATION(interface_implementation_type, interface_implementation_instance); \
    interface_implementation_instance._ulRefs = 1; \
}


//
// These macros are for defining interfaces that must be aggregated
// They implement all delegation tasks.
//

//
//  DECLARE_C_DELEGATION()
//
#define DECLARE_C_DELEGATION(iface) \
    DECLARE_C_INTERFACE_IMPLEMENTATION(iface); \
    IUnknown * _pOuterUnknown


//
//  IMPLEMENT_C_DELEGATION()
//
#define IMPLEMENT_C_DELEGATION(interface_implementation_type, iface) \
    \
    STDMETHODIMP interface_implementation_type##_QueryInterface(iface * pInterface, REFIID id, void ** ppI) \
   { \
        return COMQueryInterface(GetInterfaceImplementation(interface_implementation_type, pInterface)->_pOuterUnknown, (*id), ppI); \
    } \
    \
    STDMETHODIMP_(ULONG) interface_implementation_type##_AddRef(iface * pInterface) \
    { \
        return COMAddRef(GetInterfaceImplementation(interface_implementation_type, pInterface)->_pOuterUnknown); \
    } \
    \
    STDMETHODIMP_(ULONG) interface_implementation_type##_Release(iface * pInterface) \
    { \
        return COMRelease(GetInterfaceImplementation(interface_implementation_type, pInterface)->_pOuterUnknown); \
    } \
    struct __SEMICOLON_REQUIRED


//
// BEGIN_C_DELEGATION_VTABLE(): Used to create a vtable when implementing in C
//

#define BEGIN_C_DELEGATION_VTABLE(interface_implementation_type, iface) \
    BEGIN_C_VTABLE(interface_implementation_type, iface) \
        IUNKNOWN_C_VTABLE_ENTRIES(interface_implementation_type),


//
// INIT_C_DELEGATION()
//

#define INIT_C_DELEGATION(interface_implementation_type, interface_implementation_instance, pUnkOuter) \
{ \
    INIT_C_INTERFACE_IMPLEMENTATION(interface_implementation_type, interface_implementation_instance); \
    AssertMsg(pUnkOuter, "Outer IUnknown of delegate cannot be NULL"); \
    interface_implementation_instance._pOuterUnknown = (IUnknown *)pUnkOuter; \
}


#else /* defined(__cplusplus) && !defined(CINTERFACE) */

//
// C++ fillers for correct compiling/sizing
//

typedef struct _sFillerInterface { void * lpVtbl; } _sFillerInterface;

#define DECLARE_C_UNKNOWN(interface_implementation_type, iface)
#define DECLARE_C_INTERFACE_IMPLEMENTATION(iface)   _sFillerInterface _interface
#define DECLARE_C_UNAGGREGATABLE(iface)             DECLARE_C_INTERFACE_IMPLEMENTATION(iface); ULONG ulRefs
#define DECLARE_C_DELEGATION(iface)                 DECLARE_C_INTERFACE_IMPLEMENTATION(iface); IUnknown * _pOuterUnknown

#endif /* !defined(__cplusplus) || defined(CINTERFACE) */
#endif /* !defined(COMTOOL_NO_IMPLMACS) */

//////////////////////////////////////////////////////////////////////////////

#endif /* __COMTOOLS_H */
