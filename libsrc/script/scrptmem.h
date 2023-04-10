/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/script/scrptmem.h,v 1.6 1998/06/14 12:55:08 TOML Exp $

// This is the custom allocator included in each script module;
// actually just a wrapper which keeps track separately of all
// allocations so that it can destroy them when the DLL is detached.

// Since this file is included exactly once per DLL we can safely have
// the whole implementation right here.


////////////////////////////////////////////////////

// Each time the script allocates a block of memory, we really grab an
// extra four bytes to make room for the integer here, which is the
// block's index into an array inside cScriptModuleAlloc.  When we
// view the memory as one of these, the memory the script knows about
// starts at &m_pAllocation.

struct sScriptAllocRecord
{
   int m_iArrayIndex;
   char m_pAllocation;  // really the first of any # of characters allocated
};


// We allocate this many slots at a time in our record-keeping array.
#define kScriptAllocBlockSize 1024


////////////////////////////////////////////////////////////////
class cScriptModuleAlloc : public IAllocator
{
public:
   cScriptModuleAlloc::cScriptModuleAlloc()
   {
   }

   STDMETHOD(QueryInterface) (REFIID riid, void ** ppvObj);
   STDMETHOD_(ULONG, AddRef)  ();
   STDMETHOD_(ULONG, Release) ();

   //
   // IMalloc methods
   //
   STDMETHOD_(void *, Alloc)   (ULONG cb);
   STDMETHOD_(void *, Realloc) (void * pv, ULONG cb);
   STDMETHOD_(void,   Free)    (void * pv);
   STDMETHOD_(ULONG,  GetSize) (void * pv);
   STDMETHOD_(int,    DidAlloc)(void * pv);
   STDMETHOD_(void,   HeapMinimize)();

   //
   // debugging allocation functions
   //
   STDMETHOD_(void *, AllocEx)  (ULONG cb, const char * pszFile, int line);
   STDMETHOD_(void *, ReallocEx)(void * pv, ULONG cb, const char * pszFile,
                                 int line);
   STDMETHOD_(void,   FreeEx)   (void * pv, const char * pszFile, int line);

   STDMETHOD (VerifyAlloc)(void * pv);
   STDMETHOD (VerifyHeap)();

   STDMETHOD_(void, DumpHeapInfo)();

   STDMETHOD_(void, DumpStats)();
   STDMETHOD_(void, DumpBlocks)();
   STDMETHOD_(void, DumpModules)();

   STDMETHOD_(void, PushCredit)(const char * pszFile, int line);
   STDMETHOD_(void, PopCredit)();

   STDMETHOD_(void, Attach)(IMalloc **ppPrev); // init state, too
   STDMETHOD_(void, Remove)(); // free all allocations, too

   BOOL HaveRecord(void *pv)
   {
      int iIndex = VoidToRecord(pv)->m_iArrayIndex;

      if (iIndex >= 0 && iIndex < m_nAllocs
       && m_pAllocations[iIndex] == VoidToRecord(pv))
         return TRUE;
      else
         return FALSE;
   }

   sScriptAllocRecord *VoidToRecord(void *pv)
   {
      return (sScriptAllocRecord *) ((char *) pv - sizeof(int));
   }

   void *RealAddress(void *pv)
   {
      if (HaveRecord(pv))
         return VoidToRecord(pv);
      else
         return pv;
   }

   void GrowBlocks()
   {
      if (m_nAllocs >= m_nTotalBlocks) {
         m_nTotalBlocks += kScriptAllocBlockSize;
         m_pAllocations
            = (sScriptAllocRecord **) m_pMalloc->Realloc(m_pAllocations,
                            m_nTotalBlocks * sizeof(sScriptAllocRecord *));
      }
   }


private:
   IMalloc *m_pMalloc;
   IDebugMalloc *m_pDebugMalloc;
   IMalloc **m_ppPrev;
   sScriptAllocRecord **m_pAllocations;
   uint m_nTotalBlocks;
   uint m_nAllocs;
};


STDMETHODIMP cScriptModuleAlloc::QueryInterface(REFIID riid, void ** ppvObj)
{
   *ppvObj = this;
   return S_OK;
}


// Sitting out by itself in a script DLL, this class wouldn't get much
// use out of reference counting.
STDMETHODIMP_(ULONG) cScriptModuleAlloc::AddRef()
{
   return 0;
}

STDMETHODIMP_(ULONG) cScriptModuleAlloc::Release()
{
   return 0;
}


STDMETHODIMP_(void *) cScriptModuleAlloc::Alloc(ULONG cb)
{
   sScriptAllocRecord *pAlloc
      = (sScriptAllocRecord *)m_pMalloc->Alloc(cb + sizeof(int));
   if (!pAlloc)
      return 0;

   GrowBlocks();

   pAlloc->m_iArrayIndex = m_nAllocs;
   m_pAllocations[m_nAllocs] = pAlloc;

   ++m_nAllocs;

   return &pAlloc->m_pAllocation;
}


STDMETHODIMP_(void *) cScriptModuleAlloc::Realloc(void * pv, ULONG cb)
{
   if (!pv)
      return Alloc(cb);

   if (!cb) {
      Free(pv);
      return 0;
   }

   sScriptAllocRecord *pAlloc = VoidToRecord(pv);
   int iIndex = pAlloc->m_iArrayIndex;

   sScriptAllocRecord *pNewAlloc
      = (sScriptAllocRecord *) m_pMalloc->Realloc(pAlloc, cb + sizeof(int));
   pNewAlloc->m_iArrayIndex = iIndex;
   m_pAllocations[iIndex] = pNewAlloc;

   return &pNewAlloc->m_pAllocation;
}


// When we free, we also move the end of our array of pointers to
// allocations--if there is one--into the newly available slot, and
// stuff the new index of the element we've moved into its
// sScriptAllocRecord.
STDMETHODIMP_(void) cScriptModuleAlloc::Free(void * pv)
{
   if (!pv)
      return;

   // Is this pointer _really_ one of ours?
   if (!HaveRecord(pv)) {
      m_pMalloc->Free(pv);
      return;
   }

   sScriptAllocRecord *pAlloc = VoidToRecord(pv);
   int iIndex = pAlloc->m_iArrayIndex;

   --m_nAllocs;

   if (m_nAllocs) {
      m_pAllocations[iIndex] = m_pAllocations[m_nAllocs];
      m_pAllocations[iIndex]->m_iArrayIndex = iIndex;
   }

   m_pMalloc->Free(pAlloc);
}


STDMETHODIMP_(ULONG) cScriptModuleAlloc::GetSize(void * pv)
{
   if (HaveRecord(pv))
      return m_pMalloc->GetSize(VoidToRecord(pv)) - sizeof(int);
   else
      return m_pMalloc->GetSize(pv);
}


STDMETHODIMP_(int) cScriptModuleAlloc::DidAlloc(void * pv)
{
   return m_pMalloc->DidAlloc(RealAddress(pv));
}


STDMETHODIMP_(void) cScriptModuleAlloc::HeapMinimize()
{
   m_pMalloc->HeapMinimize();
}


STDMETHODIMP_(void *) cScriptModuleAlloc::AllocEx (ULONG cb,
                                                   const char * pszFile,
                                                   int line)
{
   if (m_pDebugMalloc)
      return m_pDebugMalloc->AllocEx(cb, pszFile, line);
   return m_pMalloc->Alloc(cb);
}


STDMETHODIMP_(void *) cScriptModuleAlloc::ReallocEx(void * pv, ULONG cb,
                                                    const char * pszFile,
                                                    int line)
{
   if (m_pDebugMalloc)
      return m_pDebugMalloc->ReallocEx(RealAddress(pv), cb, pszFile, line);
   return m_pMalloc->Realloc(RealAddress(pv), cb);
}


STDMETHODIMP_(void) cScriptModuleAlloc::FreeEx(void * pv, const char * pszFile,
                                               int line)
{
   if (m_pDebugMalloc)
      m_pDebugMalloc->FreeEx(RealAddress(pv), pszFile, line);
   else
      m_pMalloc->Free(RealAddress(pv));
}


STDMETHODIMP cScriptModuleAlloc::VerifyAlloc(void * pv)
{
   return S_OK;
}


STDMETHODIMP cScriptModuleAlloc::VerifyHeap()
{
   return S_OK;
}


STDMETHODIMP_(void) cScriptModuleAlloc::DumpHeapInfo()
{
}


STDMETHODIMP_(void) cScriptModuleAlloc::DumpStats()
{
}

STDMETHODIMP_(void) cScriptModuleAlloc::DumpBlocks()
{
}

STDMETHODIMP_(void) cScriptModuleAlloc::DumpModules()
{
}

STDMETHODIMP_(void) cScriptModuleAlloc::PushCredit(const char * pszFile, int line)
{
}

STDMETHODIMP_(void) cScriptModuleAlloc::PopCredit()
{
}

STDMETHODIMP_(void) cScriptModuleAlloc::Attach(IMalloc **ppPrev)
{
   m_pMalloc = *ppPrev;
   m_pMalloc->AddRef();
   *ppPrev = this;
   m_ppPrev = ppPrev;
   m_pMalloc->QueryInterface(IID_IDebugMalloc, (void **) &m_pDebugMalloc);
   

   m_pAllocations
      = (sScriptAllocRecord **) m_pMalloc->Alloc(kScriptAllocBlockSize
                                               * sizeof(sScriptAllocRecord *));
   m_nAllocs = 0;
   m_nTotalBlocks = kScriptAllocBlockSize;
}


STDMETHODIMP_(void) cScriptModuleAlloc::Remove()
{
   for (int i = 0; i < m_nAllocs; ++i)
      m_pMalloc->Free(m_pAllocations[i]);

   m_pMalloc->Free(m_pAllocations);
   *m_ppPrev = (IAllocator *)m_pMalloc;
   m_pMalloc->Release();
   SafeRelease(m_pDebugMalloc);
}
