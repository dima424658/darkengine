///////////////////////////////////////////////////////////////////////////////
// $Header: x:/prj/tech/libsrc/lgalloc/RCS/nullallc.cpp 1.1 1997/08/16 14:01:40 TOML Exp $
//
//
//

#include <windows.h>
#include <nullallc.h>

///////////////////////////////////////////////////////////////////////////////
//
// A place-holder "class" in the event the allocator is called during exit, after
// having been destroyed (which it *is* under Watcom)
//

struct sNoOpMalloc;

///////////////////////////////////////

static void * __stdcall NoOpAlloc(sNoOpMalloc * pThis, ulong n)
{
   return HeapAlloc(GetProcessHeap(), 0, n);
}

///////////////////////////////////////

static void __stdcall NoOpFree(sNoOpMalloc * /*pThis*/, void * /*pv*/)
{
}

///////////////////////////////////////

struct sNoOpMallocVtbl
{

    HRESULT (__stdcall * QueryInterface)(sNoOpMalloc * pThis, REFIID riid, void **ppvObject);
    ULONG (__stdcall * AddRef)(sNoOpMalloc * pThis);
    ULONG (__stdcall * Release)(sNoOpMalloc * pThis);
    void * (__stdcall * Alloc)(sNoOpMalloc * pThis, ULONG cb);
    void * (__stdcall * Realloc)(sNoOpMalloc * pThis, void * pv, ULONG cb);
    void (__stdcall * Free)(sNoOpMalloc * pThis, void * pv);
};

///////////////////////////////////////

struct sNoOpMalloc
{
    sNoOpMallocVtbl * pVtbl;
};

///////////////////////////////////////

static sNoOpMallocVtbl g_NoOpMallocVtbl = { 0, 0, 0, NoOpAlloc, 0, NoOpFree };
static sNoOpMalloc     g_NoOpMalloc = { &g_NoOpMallocVtbl };

IMalloc * g_pNullMalloc = (IMalloc *) &g_NoOpMalloc;

///////////////////////////////////////////////////////////////////////////////
