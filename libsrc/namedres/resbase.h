/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////
//
// $Header: x:/prj/tech/libsrc/namedres/RCS/resbase.h 1.11 1970/01/01 00:00:00 TOML Exp $
//
// Template for creating resource types from.
//

#ifndef __RESBASE_H
#define __RESBASE_H

#pragma once

#include <resapi.h>
#include <reshelp.h>
#include <reshack.h>
#include <resthred.h>

#pragma pack(4)

//////////////////////////////////////////////////////////////////////////
//
// @NOTE: It is *essential* that INTERFACE be IRes or a descendent thereof.
// Strange and mysterious things may happen otherwise...
//

#undef INTERFACE

template <class INTERFACE, const GUID * pIID_INTERFACE>
class cResourceBase : public IResHack,
                      public IResControl,
                      public INTERFACE
{
public:
   cResourceBase(IStore *pStore,
                 const char *pName,
                 IResType *pType);
   virtual ~cResourceBase();

   virtual void OnFinalRelease()
   {
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

      delete this;

      return 0;
   }

   STDMETHOD (QueryInterface)(REFIID id, void ** ppI);

   //////////
   //
   // IRes methods
   //
   STDMETHOD_(void *, Lock) ();
   STDMETHOD_(void, Unlock) ();
   STDMETHOD_(unsigned, GetLockCount)();
   
   STDMETHOD_(IResType *, GetType) ();
   STDMETHOD_(const char *, GetName) ();
   STDMETHOD_(void, GetCanonPath) (char **ppPath);
   STDMETHOD_(void, GetCanonPathName) (char **ppPathname);
   STDMETHOD_(IStore *, GetCanonStore) ();
   STDMETHOD_(void, GetVariant) (char **ppVariantPath);
   STDMETHOD_(void, GetStreamName) (BOOL bFullpath, char **ppPathname);

   STDMETHOD_(void *, DataPeek) ();
   STDMETHOD_(BOOL, Drop) ();
   STDMETHOD_(long, GetSize) ();
   STDMETHOD_(void *, Extract) (void *pBuf);
   STDMETHOD_(void *, PreLoad) ();
   // Grab data from file; no memory management. Return number of bytes read.
   STDMETHOD_(int, ExtractPartial) (const long nStart,
                                    const long nEnd,
                                    void *pBuf);
   STDMETHOD_(void, ExtractBlocks) (void *pBuf, 
                                    const long nSize, 
                                    tResBlockCallback, 
                                    void *pCallbackData);

   STDMETHOD_(BOOL, AsyncLock) (const int nPriority);
   STDMETHOD_(BOOL, AsyncExtract) (const int nPriority, 
                                   void *pBuf, 
                                   const long bufSize);
   STDMETHOD_(BOOL, AsyncPreload) ();
   STDMETHOD_(BOOL, IsAsyncFulfilled) ();
   STDMETHOD(AsyncKill) ();
   STDMETHOD(GetAsyncResult) (void **ppResult);

   STDMETHOD_(void, SetAppData) (DWORD AppData);
   STDMETHOD_(DWORD, GetAppData) ();

   //////////
   //
   // IResHack methods
   //
   STDMETHOD_(void, SetData) (void *pNewData);
   STDMETHOD_(BOOL, HasSetData) ();

   //////////
   //
   // IResControl methods
   //
   STDMETHOD_(BOOL, SetStore) (IStore *pStore);
   STDMETHOD_(IStore *, GetStore) ();
   STDMETHOD_(void, SetName) (const char *pNewName);
   STDMETHOD_(void, AllowStorageReset) (BOOL);
   STDMETHOD_(void, SetCanonStore) (IStore *pCanonStore);

   STDMETHOD_(void *, LoadData) (ulong * pSize, 
                                 ulong * pTimestamp,
                                 IResMemOverride * pResMem);
   STDMETHOD_(BOOL, FreeData) (void *pData, 
                               ulong nSize, 
                               IResMemOverride *pResMem);
   STDMETHOD_(IStoreStream *, OpenStream) ();

   STDMETHOD_(char **, GetTranslatableTypes)(/* OUT */ int *pnTypes);
   STDMETHOD_(void *, LoadTranslation)(void *pOldData,
                                       long nOldSize,
                                       const char *pOldTypeName,
                                       /* OUT */ BOOL *pAllocated,
                                       /* OUT */ ulong *pSize,
                                       IResMemOverride *pResMem);

   STDMETHOD_(void, SetManData) (DWORD ManData);
   STDMETHOD_(DWORD, GetManData) ();

 protected:
   //////////
   //
   // Data members
   //
   unsigned long m_ulRefs;

   // @TBD (toml 04-21-98): should these be ordered so most commonly used is first for efficiency?
   // @TBD (toml 04-21-98): again, probably better served by consolodating all flags into an unsigned "flags" field
   BOOL           m_bAllowStorageReset;
   
   IResManHelper *m_pResMan;// @TBD (toml 04-21-98): this could be a static?
   IStore *       m_pStore;
   IStore *       m_pCanonStore;

   BOOL           m_bDoingAsynch;

   char *         m_pName;
   char *         m_pExt;

   IResType *     m_pType;

   // Set by the application, if desired.
   DWORD          m_AppData;

   // Set by the Resource Manager, if desired.
   DWORD          m_ManData;

   // Used for multiple calls to ExtractPartial so that streams don't have to
   // be reopened each time.
   IStoreStream   *m_pStream;

   // @HACK:
   // This is set only when the app told the resource to replace its real
   // data with something app-controlled:
   void *         m_pAssignedData;
   int            m_nAssignedCount;

private:
   cCTRefCount __m_ulRefs;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__RESBASE_H */
