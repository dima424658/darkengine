/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////
//
// $Header: x:/prj/tech/libsrc/namedres/RCS/resbastm.h 1.13 1970/01/01 00:00:00 TOML Exp $
//
// Standard no-frills Resource implementation template.
//
// @HACK: at the moment, this contains a bunch of code dealing with
// "m_pAssignedData". This is a hack, present solely for the Dark texmem
// system. Dark is structured to assume that it can take an image, transform
// that image into a mipmap in portal memory, and assign that new mipmap to
// the resource. This is pretty bogus; the *right* solution is to create a
// proper mipmap resource type. However, transforming Dark to do that will
// take time that we don't have. So we have temporarily enabled that mode of
// operation.
//
// As soon as is feasible, the Dark Mipmap resource type should be created.
// At that point, go through here, and rip out anything pertaining to
// m_pAssignedData.
//

#ifndef __RESBASTM_H
#define __RESBASTM_H

#ifdef WAT110
#pragma message("resbastm.h is known not to compile in Watcom 110; skipping...")
#else

#include <lg.h>
#include <appagg.h>
#include <arqapi.h>

#include <str.h>
#include <filespec.h>
#include <fnamutil.h>

#include <resapi.h>
#include <storeapi.h>
#include <reshelp.h>
#include <reshack.h>

#include <resutil.h>

#include <resthred.h>

#include <allocapi.h>
#include <resbase.h>
#include <dbmem.h>

#include <thrdtool.h>

#undef INTERFACE

#define RES_BASE_TEMPLATE  template <class INTERFACE, const GUID * pIID_INTERFACE>
#define RES_BASE           cResourceBase<INTERFACE, pIID_INTERFACE>

#define AUTO_RES_THREAD_LOCK() cAutoResThreadLock resThreadLock{}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP RES_BASE::QueryInterface(REFIID id, void **ppI)
{
   AUTO_RES_THREAD_LOCK();
   
   *ppI = NULL;

   if (IsEqualGUID(id, IID_IRes))
   {
      AddRef();
      *ppI = (IRes *)this;
      return S_OK;
   }
   if (IsEqualGUID(id, IID_IResControl))
   {
      AddRef();
      *ppI = (IResControl *)this;
      return S_OK;
   }
   if (IsEqualGUID(id, IID_IResHack))
   {
      AddRef();
      *ppI = (IResHack *)this;
      return S_OK;
   }
   if (IsEqualGUID(id, IID_IDataSource))
   {
      AddRef();
      *ppI = (IDataSource *)this;
      return S_OK;
   }
   if (IsEqualGUID(id, *pIID_INTERFACE))
   {
      AddRef();
      *ppI = (INTERFACE *)this;
      return S_OK;
   }
   return E_NOINTERFACE;
}

//////////////////////////////////////////////////////////////////////////
//
// @NOTE: The pName parameter is expected to be the full streamname,
// including the extension. This is then split into the root name (which
// is used as the canonical name of the resource) and the extension
// separately.
//

RES_BASE_TEMPLATE
RES_BASE::cResourceBase(IStore *pStore,
                        const char *pName,
                        IResType *pType)
 : m_bAllowStorageReset(TRUE),
   m_pStore(0),
   m_pName(0),
   m_pExt(0),
   m_pCanonStore(NULL),
   m_bDoingAsynch(FALSE),
   m_pType(0),
   m_AppData(NO_RES_APP_DATA),
   m_ManData(NO_RES_APP_DATA),
   m_pStream(NULL),
   m_pAssignedData(0),
   m_nAssignedCount(0),
   m_ulRefs(1)
{
   IResMan *pResMan;
   pResMan = AppGetObj(IResMan);
   AssertMsg(pResMan, "No Resource Manager for a resource!");
   if (FAILED(pResMan->QueryInterface(IID_IResManHelper,
                                      (void **) &m_pResMan))) {
      CriticalMsg("Resource Manager does not implement IResManHelper!");
   }
   pResMan->Release();

   if (pStore) {
      SetStore(pStore);
   }

   if (pName) {
      SetName(pName);
   }

   if (pType) {
      m_pType = pType;
      pType->AddRef();
   }
}

///////////////////////////////////////

RES_BASE_TEMPLATE
RES_BASE::~cResourceBase()
{
   m_pResMan->UnregisterResource(this, m_ManData);

   m_pResMan->Release();
   m_pResMan = 0;

   if (m_pStream)
   {
      m_pStream->Close();
      m_pStream->Release();
      m_pStream = NULL;
   }

   if (m_pStore)
   {
      m_pStore->Release();
      m_pStore = 0;
   }

   if (m_pType)
   {
      m_pType->Release();
      m_pType = 0;
   }

   if (m_pName)
   {
      free (m_pName);
      m_pName = 0;
   }

   if (m_pExt)
   {
      free (m_pExt);
      m_pExt = 0;
   }

   SafeRelease(m_pCanonStore);
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(IResType *) RES_BASE::GetType()
{
   AUTO_RES_THREAD_LOCK();
   m_pType->AddRef();
   return m_pType;
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(char **) RES_BASE::GetTranslatableTypes(int *pnTypes)
{
   *pnTypes = 0;
   return NULL;
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(void *) RES_BASE::Lock()
{
   AUTO_RES_THREAD_LOCK();
   if (m_pAssignedData) {
      m_nAssignedCount++;
      return m_pAssignedData;
   } else {
      return m_pResMan->LockResource(this);
   }
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(void) RES_BASE::Unlock()
{
   AUTO_RES_THREAD_LOCK();
   if (m_pAssignedData) {
      m_nAssignedCount--;
   } else {
      m_pResMan->UnlockResource(this);
   }
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(unsigned) RES_BASE::GetLockCount()
{
   AUTO_RES_THREAD_LOCK();
   if (m_pAssignedData) {
      return m_nAssignedCount;
   }
   return m_pResMan->GetResourceLockCount(this);
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(BOOL) RES_BASE::Drop()
{
   AUTO_RES_THREAD_LOCK();
   if (m_pAssignedData) {
      return (m_nAssignedCount == 0);
   } else {
      return m_pResMan->DropResource(this);
   }
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(long) RES_BASE::GetSize()
{
   AUTO_RES_THREAD_LOCK();
   AssertMsg(!m_pAssignedData, "Can't GetSize for an assigned resource!");
   return m_pResMan->GetResourceSize(this);
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(void *) RES_BASE::Extract(void *pBuf)
{
   AUTO_RES_THREAD_LOCK();
   AssertMsg(!m_pAssignedData, "Can't Extract an assigned resource!");
   return m_pResMan->ExtractResource(this, pBuf);
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(void *) RES_BASE::LoadData(ulong * pSize,
                                         ulong * pTimestamp,
                                         IResMemOverride * pResMem)
{
   if (!pResMem)
      return 0;

   AUTO_RES_THREAD_LOCK();

   AssertMsg(!m_pAssignedData, "Can't load into an assigned resource!");

   IStoreStream *pStream;
   long nSize;
   void *pData;
   pStream = OpenStream();
   AssertMsg1(pStream, "Unable to open stream: %s", GetName());
   if (!pStream)
      return 0;

   nSize = pStream->GetSize();
   if (pSize)
      *pSize = nSize;

   if (pTimestamp)
      *pTimestamp = pStream->LastModified();

   LGALLOC_PUSH_CREDIT();
   pData = pResMem->ResMalloc(nSize);
   LGALLOC_POP_CREDIT();

   pStream->Read(nSize, (char *) (pData));
   pStream->Close();
   pStream->Release();

   return pData;
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(BOOL) RES_BASE::FreeData(void *pData,
                                       ulong /* nSize */,
                                       IResMemOverride *pResMem)
{
   AUTO_RES_THREAD_LOCK();
   // In the basic resource, we simply free the memory...
   pResMem->ResFree(pData);
   return TRUE;
}

///////////////////////////////////////
//
// Should not get called for standard resource, since we don't support
// translations.
//
RES_BASE_TEMPLATE
STDMETHODIMP_(void *) RES_BASE::LoadTranslation
  (void *,
   long,
   const char *,
   /* OUT */ BOOL *,
   /* OUT */ ulong *,
   IResMemOverride *)
{
   CriticalMsg("StandardResource::LoadTranslation called!");
   return NULL;
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(void *) RES_BASE::PreLoad()
{
   AUTO_RES_THREAD_LOCK();
   void *pData = m_pResMan->LockResource(this);
   m_pResMan->UnlockResource(this);
   return pData;
}

///////////////////////////////////////
//
// Fetch part of the resource into the given buffer.
//
// This grabs the specified expanse of this resource, putting it into the
// given buffer. No memory management is performed on this data; it is
// up to the caller to free the buffer when it needs to.
// --------------
// @TBD : We monkey around here to avoid potentially expensive seeks,
// which may force a physical seek and/or invalidate the file's memory
// cache.  Stepping through the stdio assembly shows that it at least
// grabs and releases 2 critical sections and calls into the OS even
// when seeking to the exact position that it's already pointing to.
//
// So.. we need to break this up into a pair of public methods that better
// mirror the standard way of doing i/o: seek and read (or possibly expose
// the storage stream via get or queryinterface).  Besides efficiency,
// this would allow future use of the resource system to cleanly access
// non-randomly accessible streams (such as TCP/IP and other network
// protocols, MPEG and other sequential streaming media protocols,
// and of course.. tape drives!).
//
RES_BASE_TEMPLATE
STDMETHODIMP_(int)
RES_BASE::ExtractPartial(const long nStart,
                         const long nEnd,
                         void *pBuf)
{
   void *pData;
   long nSize;
   int nNumToRead;
   if (!pBuf)
      return 0;

   AUTO_RES_THREAD_LOCK();

   // First try to find data if already loaded, else, load file.
   pData = m_pResMan->FindResource(this, &nSize);
   if (pData && nSize)
   {
      // found it. Just apply offsets here.
      if (nEnd >= nSize)
         nNumToRead = nSize - nStart;
      else
         nNumToRead = (nEnd - nStart) + 1;

      memmove(pBuf, ((char *) pData) + nStart, nNumToRead);
      return nNumToRead;
   }
   else
   {
#ifdef OPEN_AND_CLOSE_EACH_TIME
      // extract data from stream
      IStoreStream *pStream;

      pStream = OpenStream();
      AssertMsg1(pStream,
                 "Unable to open stream: %s",
                 GetName());
      if (!pStream)
         return 0;

      nNumToRead = pStream->ReadAbs(nStart, nEnd, (char *) pBuf);

      pStream->Close();
      pStream->Release();

      return nNumToRead;
#else
      // We keep a stream open for efficiency during multiple calls
      // to ExtractPartial. (MRW 3/16/99)
      if (NULL == m_pStream)
      {
         m_pStream = OpenStream();
         AssertMsg1(m_pStream, "Unable to open stream: %s",
                    GetName());
         if (NULL == m_pStream)
            return 0;
      }
      // Only seek if we need to.
      if (nStart != m_pStream->GetPos())
      {
         m_pStream->SetPos(nStart);
      }
      nNumToRead = nEnd - nStart + 1;
      return (m_pStream->Read(nNumToRead, (char*) pBuf));
#endif
   }
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(void) RES_BASE::ExtractBlocks
  (void *pBuf,
   const long nSize,
   tResBlockCallback Callback,
   void *pCallbackData)
{
   IStoreStream *pStream;

   if (!Callback)
      return;

   AUTO_RES_THREAD_LOCK();

   pStream = OpenStream();
   AssertMsg1(pStream, "Unable to open stream: %s", GetName());
   if (!pStream)
      return;

   sExtractData ExtractData;
   ExtractData.pResource = this;
   ExtractData.Callback = Callback;
   ExtractData.pCallbackData = pCallbackData;

   // ResBaseExtractCallback is defined in resutil
   pStream->ReadBlocks(pBuf, nSize, ResBaseExtractCallback, &ExtractData);

   pStream->Close();
   pStream->Release();
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(IStoreStream *) RES_BASE::OpenStream()
{
   AUTO_RES_THREAD_LOCK();
   if (!m_pStore) {
      return NULL;
   }

   char pFullName[MAX_STORENAME];
   strcpy(pFullName, m_pName);
   strcat(pFullName, m_pExt);

   // We don't need to AddRef, because IStore::OpenStream has already
   // done so.
   return m_pStore->OpenStream(pFullName,0);
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(BOOL) RES_BASE::SetStore(IStore * pNewStorage)
{
   AUTO_RES_THREAD_LOCK();

   if (!m_pStore || m_bAllowStorageReset)
   {
      if (m_pStore) {
         m_pStore->Release();
      }

      m_pStore = pNewStorage;

      if (m_pStore) {
         m_pStore->AddRef();
      }

      return TRUE;
   }

   return FALSE;
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(IStore *) RES_BASE::GetStore()
{
   AUTO_RES_THREAD_LOCK();

   if (m_pStore)
   {
      m_pStore->AddRef();
      return m_pStore;
   }

   return NULL;
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(void) RES_BASE::AllowStorageReset(BOOL bAllow)
{
   AUTO_RES_THREAD_LOCK();
   m_bAllowStorageReset = bAllow;
}

///////////////////////////////////////
//
// Sets the name of this resource within its storage
//
RES_BASE_TEMPLATE
STDMETHODIMP_(void) RES_BASE::SetName(const char *pNewName)
{
   AUTO_RES_THREAD_LOCK();
   if (!pNewName || m_pName)
      // No name, or already have one. Can't set it again.
      return;

   cStr Root;
   cStr Ext;
   cFileSpec fileSpec (pNewName);
   fileSpec.GetFileExtension(Ext);
   m_pExt = (char *) malloc(strlen(Ext) + 1);
   strcpy(m_pExt, Ext);
   fileSpec.GetFileRoot(Root);
   m_pName = (char *) malloc(strlen(Root) + 1);
   strcpy(m_pName, Root);
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(const char *) RES_BASE::GetName()
{
   AUTO_RES_THREAD_LOCK();
   return m_pName ? m_pName : "";
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(void) RES_BASE::GetStreamName(BOOL bFullpath, char **ppPathname)
{
   int nLen;

   AUTO_RES_THREAD_LOCK();

   if (!m_pStore || !m_pName) {
      *ppPathname = NULL;
      return;
   }

   const char *pStorePath;
   if (bFullpath) {
      pStorePath = m_pStore->GetFullPathName();
      // We assume that pStorePath ends correctly, with a backslash, because
      // we have faith in the storage system.
   } else {
      pStorePath = "";
   }
   nLen = strlen(pStorePath) + strlen(m_pName) + strlen(m_pExt) + 1;

   // Create and fill the string
   *ppPathname = (char *) malloc(nLen);
   strcpy(*ppPathname, pStorePath);
   strcat(*ppPathname, m_pName);
   strcat(*ppPathname, m_pExt);
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(void) RES_BASE::SetCanonStore (IStore *pCanonStore)
{
   AUTO_RES_THREAD_LOCK();
   SafeRelease(m_pCanonStore);
   m_pCanonStore = pCanonStore;
   if (pCanonStore) {
      pCanonStore->AddRef();
   }
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(IStore *) RES_BASE::GetCanonStore()
{
   AUTO_RES_THREAD_LOCK();
   m_pCanonStore->AddRef();
   return m_pCanonStore;
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(void) RES_BASE::GetCanonPath(char **ppPath)
{
   AUTO_RES_THREAD_LOCK();
   if (m_pCanonStore) {
      m_pCanonStore->GetCanonPath(ppPath);
   } else {
      m_pStore->GetCanonPath(ppPath);
   }
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(void) RES_BASE::GetVariant(char **ppVariantPath)
{
   AUTO_RES_THREAD_LOCK();
   if (!m_pStore || !m_pCanonStore) {
      *ppVariantPath = (char *) malloc(1);
      **ppVariantPath = '\0';
   } else {
      ComputeAnchoredPath(m_pCanonStore->GetFullPathName(),
                          m_pStore->GetFullPathName(),
                          ppVariantPath);
   }
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(void) RES_BASE::GetCanonPathName(char **ppPathname)
{
   AUTO_RES_THREAD_LOCK();
   // Default in case something goes wrong:
   *ppPathname = NULL;

   if (!m_pName || !m_pStore) {
      return;
   }

   int nLen;
   char *pCanonPath;
   if (m_pCanonStore) {
      m_pCanonStore->GetCanonPath(&pCanonPath);
   } else {
      m_pStore->GetCanonPath(&pCanonPath);
   }

   if (!pCanonPath) {
      return;
   }

   // We assume that pCanonPath ends correctly...
   nLen = strlen(pCanonPath) + strlen(m_pName) + 1;

   // Create and fill the string
   *ppPathname = (char *) malloc(nLen);
   strcpy(*ppPathname, pCanonPath);
   strcat(*ppPathname, m_pName);

   free(pCanonPath);
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(BOOL) RES_BASE::AsyncLock(const int nPriority)
{
   return m_pResMan->AsyncLock(this, nPriority);
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(BOOL) RES_BASE::AsyncExtract
  (const int nPriority,
   void *pBuf,
   const long bufSize)
{
   return m_pResMan->AsyncExtract(this, nPriority, pBuf, bufSize);
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(BOOL) RES_BASE::AsyncPreload()
{
   return m_pResMan->AsyncPreload(this);
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(BOOL) RES_BASE::IsAsyncFulfilled()
{
   return m_pResMan->IsAsyncFulfilled(this);
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP RES_BASE::AsyncKill()
{
   return m_pResMan->AsyncKill(this);
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP RES_BASE::GetAsyncResult(void **ppResult)
{
   return m_pResMan->GetAsyncResult(this, ppResult);
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(void) RES_BASE::SetAppData(DWORD AppData)
{
   AUTO_RES_THREAD_LOCK();
   m_AppData = AppData;
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(DWORD) RES_BASE::GetAppData()
{
   AUTO_RES_THREAD_LOCK();
   return m_AppData;
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(void) RES_BASE::SetManData(DWORD ManData)
{
   AUTO_RES_THREAD_LOCK();
   m_ManData = ManData;
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(DWORD) RES_BASE::GetManData()
{
   AUTO_RES_THREAD_LOCK();
   return m_ManData;
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(void *) RES_BASE::DataPeek()
{
   AUTO_RES_THREAD_LOCK();
   long dummy;

   if (m_pAssignedData) {
      return m_pAssignedData;
   } else {
      return m_pResMan->FindResource(this, &dummy);
   }
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(void) RES_BASE::SetData(void *pNewData)
{
   AUTO_RES_THREAD_LOCK();
   if (!Drop()) {
      // It's locked, so we can't drop it
      CriticalMsg("Trying to SetData on a Locked Resource!");
      return;
   }

   m_pAssignedData = pNewData;
}

///////////////////////////////////////

RES_BASE_TEMPLATE
STDMETHODIMP_(BOOL) RES_BASE::HasSetData()
{
   AUTO_RES_THREAD_LOCK();
   if (m_pAssignedData) {
      return TRUE;
   } else {
      return FALSE;
   }
}

//////////////////////////////////////////////////////////////////////////
#include <undbmem.h>
#endif // !WAT110
#endif // !__RESBASTM_H
