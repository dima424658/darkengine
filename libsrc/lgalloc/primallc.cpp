///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lgalloc/RCS/primallc.cpp $
// $Author: TOML $
// $Date: 1998/06/10 13:58:15 $
// $Revision: 1.7 $
//

#ifdef _WIN32

#include <lg.h>
#undef Free
#undef Malloc
#undef Realloc

#include <memcore.h>
#include <primallc.h>

#pragma code_seg("lgalloc")

///////////////////////////////////////////////////////////////////////////////

volatile cFastMutex g_AllocMutex;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPrimaryMalloc, members
//

cPrimaryMalloc::cPrimaryMalloc()
{
    initAllocCap = 0x2000000;
    allocCap     = 0x2000000;
    totalAlloc   = 0;
    #ifndef SHIP
    peakAlloc    = 0;
    #endif
}

///////////////////////////////////////

cPrimaryMalloc::~cPrimaryMalloc()
{

}

///////////////////////////////////////

ulong LGAPI cPrimaryMalloc::DefPageFunc(ulong needed, sAllocLimits * pLimits)
{
    return 0;
}

///////////////////////////////////////

STDMETHODIMP_(void *) cPrimaryMalloc::Alloc(ULONG size)
{
    // @Note (toml 07-14-97): This code is block copied in AllocEx and should be kept in sync
    if (!size)
        size = 1;  // For VC compatibility, allow 0 sized allocations.

    IAllocator * const pNext = m_pNext;
    void * p;
    BOOL   fMoreToPage = FALSE;

    // Until we actually allocate a block...
    do
    {
        // Page memory if over limit
        if (totalAlloc > allocCap)
            fMoreToPage = ((*m_pfnPage)(size, this) != 0);

        // Allocate our block
        AllocThreadLock();
        p = pNext->Alloc(size);
        if (p)
        {
            totalAlloc += pNext->GetSize(p);
#ifndef SHIP
            if (totalAlloc > peakAlloc)
                peakAlloc = totalAlloc;
#endif
            AllocThreadUnlock();
            return p;
        }
        AllocThreadUnlock();

    } while (fMoreToPage);

    g_fQuietAssert = FALSE;
    CriticalMsg("Out of memory");

    return NULL;
}

///////////////////////////////////////

STDMETHODIMP_(void *) cPrimaryMalloc::Realloc(void * pOld, ULONG newClientSize)
{
    // @Note (toml 07-14-97): This code is block copied in ReallocEx and should be kept in sync

    // Let's handle ANSI behaviors here...
    if (!pOld)
        return cPrimaryMalloc::Alloc(newClientSize);

    if (!newClientSize)
    {
        cPrimaryMalloc::Free(pOld);
        return 0;
    }

    AllocThreadLock();

    void *    pNew;
    const int oldClientSize = m_pNext->GetSize(pOld);
    BOOL      fMoreToPage = FALSE;
    const int difference = newClientSize - oldClientSize;

    do
    {
        if (difference > 0 && m_pfnPage && totalAlloc > allocCap)
        {
            AllocThreadUnlock();
            fMoreToPage = ((*m_pfnPage)(difference, this) != 0);
            AllocThreadLock();
        }

        pNew = m_pNext->Realloc(pOld, newClientSize);

    } while (!pNew && fMoreToPage);

    if (pNew)
    {
        totalAlloc += m_pNext->GetSize(pNew) - oldClientSize;
        #ifndef SHIP
        if (totalAlloc > peakAlloc)
            peakAlloc = totalAlloc;
        #endif
    }
    else
    {
        g_fQuietAssert = FALSE;
        CriticalMsg("Out of memory");
    }

    AllocThreadUnlock();

    return pNew;
}

///////////////////////////////////////

STDMETHODIMP_(void) cPrimaryMalloc::Free(void * p)
{
    // @Note (toml 07-14-97): This code is block copied in FreeEx and should be kept in sync
    if (!p)
        return;

    AllocThreadLock();
    IAllocator * const pNext = m_pNext;
    totalAlloc -= pNext->GetSize(p);
    pNext->Free(p);
    AllocThreadUnlock();
}

///////////////////////////////////////

STDMETHODIMP_(ULONG) cPrimaryMalloc::GetSize(void * p)
{
    ulong result;

    if (!p)
        return 0;

    AllocThreadLock();
    result = m_pNext->GetSize(p);
    AllocThreadUnlock();

    return result;
}

///////////////////////////////////////

STDMETHODIMP_(int) cPrimaryMalloc::DidAlloc(void * pv)
{
    int result;

    AllocThreadLock();
    result = m_pNext->DidAlloc(pv);
    AllocThreadUnlock();

    return result;
}

///////////////////////////////////////

STDMETHODIMP_(void) cPrimaryMalloc::HeapMinimize()
{
    AllocThreadLock();
    m_pNext->HeapMinimize();
    AllocThreadUnlock();
}

///////////////////////////////////////
#ifndef SHIP

STDMETHODIMP_(void *) cPrimaryMalloc::AllocEx(ULONG size, const char * pszFile, int line)
{
    // @Note (toml 07-14-97): This code is block copied FROM Alloc and should be kept in sync
    if (!size)
        size = 1;  // For VC compatibility, allow 0 sized allocations.

    IAllocator * const pNext = m_pNext;
    void * p;
    BOOL   fMoreToPage = FALSE;

    // Until we actually allocate a block...
    do
    {
        // Page memory if over limit
        if (totalAlloc > allocCap)
            fMoreToPage = ((*m_pfnPage)(size, this) != 0);

        // Allocate our block
        AllocThreadLock();
        p = pNext->AllocEx(size, pszFile, line);
        if (p)
        {
            totalAlloc += pNext->GetSize(p);
#ifndef SHIP
            if (totalAlloc > peakAlloc)
                peakAlloc = totalAlloc;
#endif
            AllocThreadUnlock();
            return p;
        }
        AllocThreadUnlock();

    } while (fMoreToPage);

    g_fQuietAssert = FALSE;
    CriticalMsg("Out of memory");

    return NULL;
}

///////////////////////////////////////

STDMETHODIMP_(void *) cPrimaryMalloc::ReallocEx(void * pOld, ULONG newClientSize, const char * pszFile, int line)
{
    // @Note (toml 07-14-97): This code is block copied FROM Realloc and should be kept in sync

    // Let's handle ANSI behaviors here...
    if (!pOld)
        return cPrimaryMalloc::Alloc(newClientSize);

    if (!newClientSize)
    {
        cPrimaryMalloc::Free(pOld);
        return 0;
    }

    AllocThreadLock();

    void *    pNew;
    const int oldClientSize = m_pNext->GetSize(pOld);
    BOOL      fMoreToPage = FALSE;
    const int difference = newClientSize - oldClientSize;

    do
    {
        if (difference > 0 && m_pfnPage && totalAlloc > allocCap)
        {
            AllocThreadUnlock();
            fMoreToPage = ((*m_pfnPage)(difference, this) != 0);
            AllocThreadLock();
        }

        pNew = m_pNext->ReallocEx(pOld, newClientSize, pszFile, line);

    } while (!pNew && fMoreToPage);

    if (pNew)
    {
        totalAlloc += m_pNext->GetSize(pNew) - oldClientSize;
        #ifndef SHIP
        if (totalAlloc > peakAlloc)
            peakAlloc = totalAlloc;
        #endif
    }
    else
    {
        g_fQuietAssert = FALSE;
        CriticalMsg("Out of memory");
    }

    AllocThreadUnlock();

    return pNew;
}

///////////////////////////////////////

STDMETHODIMP_(void) cPrimaryMalloc::FreeEx(void * p, const char * pszFile, int line)
{
    // @Note (toml 07-14-97): This code is block copied FROM Free and should be kept in sync
    if (!p)
        return;

    AllocThreadLock();
    IAllocator * const pNext = m_pNext;
    totalAlloc -= pNext->GetSize(p);
    pNext->FreeEx(p, pszFile, line);
    AllocThreadUnlock();
}

///////////////////////////////////////

STDMETHODIMP cPrimaryMalloc::VerifyAlloc(void * p)
{
    if (!p)
        return S_OK;

    HRESULT result;

    AllocThreadLock();
    result = m_pNext->VerifyAlloc(p);
    AllocThreadUnlock();

    return result;
}

///////////////////////////////////////

STDMETHODIMP cPrimaryMalloc::VerifyHeap()
{
    HRESULT result;

    AllocThreadLock();
    result = m_pNext->VerifyHeap();
    AllocThreadUnlock();

    return result;
}

#endif
///////////////////////////////////////////////////////////////////////////////
#endif
