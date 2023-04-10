/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/scrobj.cpp,v 1.8 2000/02/22 19:49:45 toml Exp $
//
//
//

#include <lg.h>
#include <scrobj.h>
#include <scrptman.h>
#include <cfgdbg.h>

#include <allocapi.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cScrDatum
//

PERSIST_IMPLEMENT_PERSISTENT(sScrDatum)
{
   Persistent(objId);
   Persistent(pszClass);
   Persistent(pszName);
   Persistent(value);

   return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cScrObj
//

HRESULT cScrObj::Connect()
{
   HRESULT               result = S_OK;
   const sScrClassDesc * pDesc;
   cScrScriptInfo *      pInfo = GetFirst();

   LGALLOC_AUTO_CREDIT();

   while (pInfo)
   {
      if ((pDesc = g_pScriptMan->GetClass(pInfo->className)) != NULL)
      {
         pInfo->pScript = (*pDesc->pfnFactory)(pInfo->className, m_ObjId);
      }

      if (!pInfo->pScript)
      {
         ConfigSpew("ScriptAttach",("Failed to attach script %s to object %d\n", (const char *)pInfo->className, m_ObjId));
         result = S_FALSE;
      }

      pInfo = pInfo->GetNext();
   }

   return result;
}

///////////////////////////////////////

HRESULT cScrObj::Disconnect()
{
   cScrScriptInfo * pInfo = GetFirst();

   while (pInfo)
   {
      SafeRelease(pInfo->pScript);
      pInfo = pInfo->GetNext();
   }

   return S_OK;
}

///////////////////////////////////////

void cScrObj::DispatchBeginScripts()
{
   sScrMsg *        pMsg = new sScrMsg(m_ObjId, "BeginScript");
   cScrScriptInfo * pInfo = GetFirst();

   pMsg->time = g_pScriptMan->GetTime();
   
   while (pInfo)
   {
      if (pInfo->pScript && !(pInfo->flags & kSIF_SentBegin))
      {
         g_pScriptMan->DoSendDirect(pInfo->pScript, pMsg, NULL);
         pInfo->flags |= kSIF_SentBegin;
      }
      pInfo = pInfo->GetNext();
   }
   pMsg->Release();
}

////////////////////////////////////////

void cScrObj::DispatchEndScripts()
{
   sScrMsg *        pMsg = new sScrMsg(m_ObjId, "EndScript");
   cScrScriptInfo * pInfo = GetFirst();

   pMsg->time = g_pScriptMan->GetTime();
   
   while (pInfo)
   {
      if (pInfo->pScript && (pInfo->flags & kSIF_SentBegin))
      {
         g_pScriptMan->DoSendDirect(pInfo->pScript, pMsg, NULL);
         pInfo->flags &= ~kSIF_SentBegin;
      }
      pInfo = pInfo->GetNext();
   }
   pMsg->Release();
}


///////////////////////////////////////

int cScrObj::CountUninitialized()
{
   int rv = 0;
   cScrScriptInfo * pInfo = GetFirst();

   while (pInfo)
   {
      if ((pInfo->flags & kSIF_SentBegin) == 0)
         ++rv;
      pInfo = pInfo->GetNext();
   }
   return rv;
}

///////////////////////////////////////

void cScrObj::SetInfoFlags(int iFlags)
{
   cScrScriptInfo * pInfo = GetFirst();

   while (pInfo)
   {
      pInfo->flags |= iFlags;
      pInfo = pInfo->GetNext();
   }
}

///////////////////////////////////////

// This saves off those info structures which have not received
// BeginScript messages.
void cScrObj::SaveScrInfo(tPersistIOFunc pfnIO, void *pContextIO)
{
   cScrScriptInfo * pInfo = GetFirst();

   while (pInfo)
   {
      if ((pInfo->flags & kSIF_SentBegin) == 0)
      {
         pfnIO(pContextIO, 
               pInfo->className.GetBuffer(kScrMaxClassName + 1),
               kScrMaxClassName + 1);
         pInfo->className.ReleaseBuffer();

         ObjID obj = GetObjID();
         pfnIO(pContextIO, &obj, sizeof(obj));
      }

      pInfo = pInfo->GetNext();
   }
}


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cScrObjFilter
//

int cScrObjFilter::Find(cScrObj *pObj)
{
   int iSize = m_Data.Size();
   ObjID obj = pObj->GetObjID();

   for (int i = 0; i < iSize; ++i)
      if (pObj->Search(m_Data[i].m_Class) && obj == m_Data[i].m_obj)
         return i;

   return -1;
}


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cScrObjsTable
//

tHashSetKey cScrObjsTable::GetKey(tHashSetNode p) const
{
   return (tHashSetKey)(((cScrObj *)p)->GetObjID());
}

///////////////////////////////////////

void cScrObjsTable::DisconnectAll()
{
   cScrObj *      pScrObj;
   tHashSetHandle iter;

   pScrObj = GetFirst(iter);

   while (pScrObj)
   {
      pScrObj->Disconnect();
      pScrObj = GetNext(iter);
   }
}

///////////////////////////////////////

void cScrObjsTable::ConnectAll()
{
   cScrObj *      pScrObj;
   tHashSetHandle iter;

   pScrObj = GetFirst(iter);

   while (pScrObj)
   {
      pScrObj->Connect();
      pScrObj = GetNext(iter);
   }
}

///////////////////////////////////////

void cScrObjsTable::DispatchBeginScriptsAll()
{
   cScrObj *      pScrObj;
   tHashSetHandle iter;

   pScrObj = GetFirst(iter);

   while (pScrObj)
   {
      pScrObj->DispatchBeginScripts();
      pScrObj = GetNext(iter);
   }
}

///////////////////////////////////////

void cScrObjsTable::DispatchEndScriptsAll()
{
   cScrObj *      pScrObj;
   tHashSetHandle iter;

   pScrObj = GetFirst(iter);

   while (pScrObj)
   {
      pScrObj->DispatchEndScripts();
      pScrObj = GetNext(iter);
   }
}


///////////////////////////////////////

#include <mprintf.h>
void cScrObjsTable::DispatchBeginScriptsFiltered(cScrObjFilter *pFilter)
{
   cScrObj *      pScrObj;
   tHashSetHandle iter;

   pScrObj = GetFirst(iter);

   while (pScrObj)
   {
      if (pFilter->Match(pScrObj)) {
         pScrObj->DispatchBeginScripts();
      } else
         pScrObj->SetInfoFlags(kSIF_SentBegin);
      pScrObj = GetNext(iter);
   }
}

///////////////////////////////////////

int cScrObjsTable::CountUninitializedAll()
{
   cScrObj *      pScrObj;
   tHashSetHandle iter;
   int rv = 0;

   pScrObj = GetFirst(iter);

   while (pScrObj)
   {
      rv += pScrObj->CountUninitialized();
      pScrObj = GetNext(iter);
   }
   return rv;
}

///////////////////////////////////////

void cScrObjsTable::SetInfoFlagsAll(int iFlags)
{
   cScrObj *      pScrObj;
   tHashSetHandle iter;

   pScrObj = GetFirst(iter);

   while (pScrObj)
   {
      pScrObj->SetInfoFlags(iFlags);
      pScrObj = GetNext(iter);
   }
}

///////////////////////////////////////

void cScrObjsTable::SaveScrInfo(tPersistIOFunc pfnIO, void *pContextIO)
{
   cScrObj *      pScrObj;
   tHashSetHandle iter;

   pScrObj = GetFirst(iter);

   while (pScrObj)
   {
      pScrObj->SaveScrInfo(pfnIO, pContextIO);
      pScrObj = GetNext(iter);
   }
}

///////////////////////////////////////

void cScrObjsTable::ClearInfoFlags(char *pszClassName, ObjID obj, int iFlags)
{
   cScrObj *pScrObj = Search(obj);
#ifndef SHIP
   if (!pScrObj)
   {
      Warning(("Cannot clear BeginScript flag for class %s, object %d\n",
               pszClassName, obj));
      return;
   }
#endif // ~SHIP

   cScrScriptInfo * pInfo = pScrObj->GetFirst();

   while (pInfo)
   {
      if (pInfo->className == pszClassName)
      {
         pInfo->flags &= (!iFlags);
         return;
      }
      pInfo = pInfo->GetNext();
   }
#ifndef SHIP
   Warning(("Cannot clear BeginScript flag for class %s, object %d\n",
             pszClassName, obj));
#endif // ~SHIP
}


///////////////////////////////////////////////////////////////////////////////
