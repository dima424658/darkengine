/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/libsrc/script/scrobj.h,v 1.6 1998/08/04 22:30:06 mahk Exp $
//
//
//

#ifndef __SCROBJ_H
#define __SCROBJ_H

#include <allocapi.h>
#include <hashset.h>
#include <dlist.h>
#include <str.h>

#include <scrptapi.h>


///////////////////////////////////////////////////////////////////////////////
//
// class cScrScriptInfo
//

class cScrScriptInfo;

typedef cDList<cScrScriptInfo, 1> cScrScriptInfoList;
typedef cDListNode<cScrScriptInfo, 1> cScrScriptInfoBase;

enum eScrScriptInfoFlags
{
   kSIF_SentBegin = 0x01
};

class cScrScriptInfo : public cScrScriptInfoBase
{
public:
   cScrScriptInfo(const char * pszClassName)
    : className(pszClassName),
      flags(0),
      pScript(NULL)
   {
   }

   cStr      className;
   unsigned  flags;
   IScript * pScript;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cScrObj
//

class cScrObj;
typedef int (*tScrObjFunc)(cScrScriptInfo *pScrScriptInfo, cScrObj *pScrObj,
                           void *pV1, void *pV2);

class cScrObj : public cScrScriptInfoList
{
public:
   cScrObj(ObjID objId)
    : m_ObjId(objId)
   {
   }

   ~cScrObj();

   ObjID GetObjID();

   cScrScriptInfo * Search(const char *);

   HRESULT Connect();
   HRESULT Disconnect();
   void DispatchBeginScripts();
   void DispatchEndScripts();

   int CountUninitialized();
   void SetInfoFlags(int iFlags);
   void SaveScrInfo(tPersistIOFunc pfnIO, void *pContextIO);

private:
   ObjID m_ObjId;
};

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sScrObjFilterTag
// helper for cScrObjsTable--specifies one script instance
//
struct sScrObjFilterTag
{
   sScrObjFilterTag(const char *pszClass, ObjID obj)
      : m_obj(obj)
   {
      strcpy(m_Class, pszClass);
   }

   char m_Class[kScrMaxClassName + 1];
   ObjID m_obj;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cScrObjFilter
//

// list of script objects to include in an operation
class cScrObjFilter
{
public:
   BOOL Match(cScrObj *pObj)
   {
      if (Find(pObj) == -1)
         return FALSE;
      else
         return TRUE;
   }

   void Remove(cScrObj *pObj)
   {
      int i = Find(pObj);
      if (i != -1)
         m_Data.FastDeleteItem(i);
   }

   void Clear()
   {
      m_Data.SetSize(0);
   }

   void Add(sScrObjFilterTag *pTag)
   {
      m_Data.Append(*pTag);
   }

protected:
   cDynArray<sScrObjFilterTag> m_Data;
   int Find(cScrObj *pObj);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cScrObjsTable
//

class cScrObjsTable : public cHashSet<cScrObj *, ObjID, cHashFunctions>
{
public:
   void DisconnectAll();
   void ConnectAll();
   void DispatchBeginScriptsAll();
   void DispatchBeginScriptsFiltered(cScrObjFilter *pFilter);
   void DispatchEndScriptsAll();
   int CountUninitializedAll();
   void SetInfoFlagsAll(int iFlags);
   void SaveScrInfo(tPersistIOFunc pfnIO, void *pContextIO);

   void ClearInfoFlags(char *pszClassName, ObjID obj, int iFlags);
   virtual tHashSetKey GetKey(tHashSetNode p) const;
};

///////////////////////////////////////////////////////////////////////////////
//
// Script Data
//

///////////////////////////////////////
//
// STRUCT: sScrDatum
//

struct sScrDatum : public sScrDatumTag, public sPersistent
{
   sScrDatum()
   {
      objId = 0;
      pszClass = 0;
      pszName = 0;
   }

   sScrDatum(const sScrDatumTag * pTag)
   {
      LGALLOC_AUTO_CREDIT();
      objId = pTag->objId;
      pszClass = strdup(pTag->pszClass);
      pszName = strdup(pTag->pszName);
   }

   ~sScrDatum()
   {
      free((void *)pszClass);
      free((void *)pszName);
   }

   cMultiParm value;

   PERSIST_DECLARE_PERSISTENT();
};

///////////////////////////////////////
//
// CLASS: cScriptsData
//

class cScriptsDataHash
{
public:
   static unsigned Hash(const sScrDatumTag * p)
   {
      return (HashLong(p->objId) ^
              HashString(p->pszName));
   }

   static BOOL IsEqual(const sScrDatumTag * p1, const sScrDatumTag * p2)
   {
      return (p1->objId == p2->objId)
           && !strcmp(p1->pszName, p2->pszName)
           && !strcmp(p1->pszClass, p2->pszClass);
   }
};

///////////////////////////////////////

class cScriptsData : public cHashSet<sScrDatum *, const sScrDatumTag *, cScriptsDataHash>
{
public:
   cScriptsData()
   {
   }

   ~cScriptsData()
   {
      DestroyAll();
   }

   tHashSetKey GetKey(tHashSetNode p) const
   {
      return (tHashSetKey) (sScrDatumTag *) (sScrDatum *) p;
   }

   void DestroyAll()
   {
      tHashSetHandle handle;

      sScrDatum * p = GetFirst(handle);
      sScrDatum * pNext;

      while (p)
      {
         pNext = GetNext(handle);
         delete Remove(p);
         p = pNext;
      }
   }

};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cScrObj, inline functions
//

inline cScrObj::~cScrObj()
{
   cScrScriptInfo * pInfo;

   while ((pInfo = GetFirst()) != NULL)
   {
      delete Remove(pInfo);
   }
}

///////////////////////////////////////

inline cScrScriptInfo * cScrObj::Search(const char * pszClassName)
{
   cScrScriptInfo * pInfo = GetFirst();

   while (pInfo && pInfo->className != pszClassName)
   {
      pInfo = pInfo->GetNext();
   }

   return pInfo;
}

///////////////////////////////////////

inline ObjID cScrObj::GetObjID()
{
   return m_ObjId;
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__SCROBJ_H */
