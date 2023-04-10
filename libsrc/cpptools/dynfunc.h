///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/dynfunc.h $
// $Author: TOML $
// $Date: 1996/03/22 10:45:47 $
// $Revision: 1.1 $
//
// (c) Copyright 1994-1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//

#ifndef __DYNFUNC_H
#define __DYNFUNC_H

#if defined(__SC__) || defined(__RCC__)
#pragma once
#endif

//
// Description:
//
// The class cDynFunc and the associated macros simplify the process of
// dynamic function loading from DLLs.  Functions may be explicitly loaded,
// or implicitly on first use.
//
// The primary motivation for using dynamic functions is to avoid loading
// DLLs that are not normally used.  For example, to not load WinG every time
// you run, but only if it's used.
//
// It also provides a clean package for this sort of indirection.
//
//
// Use:
//
//  DeclDynFunc(Return, FuncName, Params)
//  DeclDynFunc_(Return, Linkage, FuncName, Params)
//  ImplDynFunc(FuncName, pszLib, pszSig, pfnFail)
//  GetDynFuncAddress(FuncName)()
//
//  DynFunc(foo).Find(): Function can be explicitly loaded
//  DynFunc(foo).GetProcAddress(): Address of function can be obtained
//  DynFunc(foo).Unload(): Function can be explicitly unloaded
//
//  pfnFail allows specificiation of a fall-back function if NULL return unsafe
//
// E.g.:
//  given a function int foo(int i);
//
//  DeclDynFunc(int, foo, (int i));
//  ImplDynFunc(foo, "foo.dll", "_foo" /* or MAKEINTRESOURCE(N) */, bar /* or NULL */)
//
//  Additional conventions specify defining convenience macros:
//
//  #if defined(_WIN32)
//  #define LoadDynFoo()    DynFunc(foo).Load()
//  #define UnloadDynFoo()  DynFunc(foo).Unload()
//  #define DynFoo          (DynFunc(foo).GetProcAddress())
//  #else
//  #define LoadDynFoo()    (1)
//  #define UnloadDynFoo()
//  #define DynFoo          foo
//  #endif
//
//  main()
//  {
//      // Call foo, will load if not found
//      DynFoo();
//
//      // Get address of foo, will load if not found
//      void * p = DynFoo;
//  }
//

///////////////////////////////////////

#define DeclDynFunc(Return, FuncName, ParamList) \
    typedef Return (*__t##FuncName)ParamList; \
    _DeclDynFunc(FuncName)

#define DeclDynFunc_(Return, Linkage, FuncName, ParamList) \
    typedef Return (Linkage *__t##FuncName)ParamList; \
    _DeclDynFunc(FuncName)

#define DynFunc(FuncName) \
    _cDFName(FuncName)::_DFName(FuncName)

///////////////////////////////////////

class cDynFunc
    {
public:
    BOOL Load();
    void Unload();

protected:
    cDynFunc(const char * pszLibName, const char * pszFuncSig, void * pfnFail);
    ~cDynFunc();

    void * GetFunc();
    void * FindFunc();

    void * pfnFunc;
    BOOL fTriedToLoad;
    void * pfnFail;
    HINSTANCE hInstLib;
    const char * pszLibName;
    const char * pszFuncSig;
    };

///////////////////////////////////////

inline BOOL LoadedDLL(HINSTANCE hInstLib)
    {
    #if !defined(_WIN32)
    return (hInstLib > HINSTANCE_ERROR);
    #else
    return hInstLib != 0;
    #endif
    }

///////////////////////////////////////

inline cDynFunc::cDynFunc(const char * pszLibName, const char * pszFuncSig, void * pfnFail)
    : pfnFunc(0), fTriedToLoad(FALSE), hInstLib(0),
      pszLibName(pszLibName), pszFuncSig(pszFuncSig), pfnFail(pfnFail)
    {
    }

///////////////////////////////////////

inline cDynFunc::~cDynFunc()
    {
    if (LoadedDLL(hInstLib))
        FreeLibrary(hInstLib);
    }

///////////////////////////////////////

inline void cDynFunc::Unload()
    {
    pfnFunc = NULL;
    fTriedToLoad = FALSE;
    if (LoadedDLL(hInstLib))
        FreeLibrary(hInstLib);
    hInstLib = 0;
    }

///////////////////////////////////////

inline void * cDynFunc::GetFunc()
    {
    if (pfnFunc)
        return pfnFunc;
    return FindFunc();
    }

///////////////////////////////////////

#define _DFName(FuncName)    __DynFunc__##FuncName
#define _cDFName(FuncName)   __cDynFunc__##FuncName

#define _DeclDynFunc(FuncName) \
    class _cDFName(FuncName) : public cDynFunc \
        { \
    public: \
        _cDFName(FuncName)(); \
        ~_cDFName(FuncName)()      {} \
        __t##FuncName GetProcAddress()     { return (__t##FuncName)GetFunc(); } \
        static _cDFName(FuncName) _DFName(FuncName); \
        }

#define ImplDynFunc(FuncName, pszLib, pszSig, pfnFail) \
    static const char * __pcszDynFunc##FuncName##_Lib = pszLib; \
    static const char * __pcszDynFunc##FuncName##_Sig = pszSig; \
    _cDFName(FuncName)::_cDFName(FuncName)() \
        : cDynFunc(__pcszDynFunc##FuncName##_Lib, __pcszDynFunc##FuncName##_Sig, pfnFail) \
        { \
        } \
    _cDFName(FuncName) _cDFName(FuncName)::_DFName(FuncName)

///////////////////////////////////////

#endif /* __DYNFUNC_H */
