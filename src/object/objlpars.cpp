/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/object/objlpars.cpp,v 1.4 1998/10/05 17:26:44 mahk Exp $
//
//
//

#include <stdlib.h>

#include <appagg.h>
#include <delmlist.h>
#include <hashpp.h>
#include <mprintf.h>

#include <iobjsys.h>
#include <objquery.h>
#include <objlpars.h>
#include <traitbas.h>
#include <traitman.h>

#include <hshpptem.h>

// Must be last header 
#include <dbmem.h>


#define kDescendChar '@'
#define kExcludeChar '~'
#define kDelimiters  ",;"

///////////////////////////////////////////////////////////////////////////////

class cOLPTable : public cHashTable<ObjID, BOOL, cHashTableFunctions<ObjID> >
{
};

#if defined(_MSC_VER)
template cHashTable<ObjID, BOOL, cHashTableFunctions<ObjID> >;
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cObjListParser
//

cObjListParser::~cObjListParser()
{
   SafeRelease(m_pObjectSystem);
   SafeRelease(m_pTraitManager);
}

///////////////////////////////////////

inline ObjID cObjListParser::ObjFromString(const char * pszObject)
{
   ObjID object = atoi(pszObject);
   if (!object)
      object = m_pObjectSystem->GetObjectNamed(pszObject);
   return object;
}
   
///////////////////////////////////////

void cObjListParser::Parse(const char * pszObjList, cDynArray<ObjID> * pResult, unsigned flags)
{
   if (!m_pObjectSystem)
   {
      m_pObjectSystem = AppGetObj(IObjectSystem);
      m_pTraitManager = AppGetObj(ITraitManager);
   }

   m_pResult = pResult;

   FromText(pszObjList, kDelimiters);
   
   if (flags & kOLP_RemoveDups)
      RemoveDuplicates();

   if (m_pExclusions)
   {
      RemoveExcluded();
      delete m_pExclusions;
      m_pExclusions = NULL;
   }

   m_pResult = NULL;
}

///////////////////////////////////////

void cObjListParser::AddDescendants(ObjID ancestor)
{
   if (!OBJ_IS_CONCRETE(ancestor))
   {
      ObjID descendant;
      IObjectQuery * pQuery = m_pTraitManager->Query(ancestor, kTraitQueryAllDescendents);
      for (; !pQuery->Done(); pQuery->Next())
      {
         descendant = pQuery->Object();
         if (OBJ_IS_CONCRETE(descendant))
         {
            m_pResult->Append(descendant);
         }
      }
      SafeRelease(pQuery);
   }
   else
      m_pResult->Append(ancestor);
}

///////////////////////////////////////

void cObjListParser::RemoveDuplicates()
{
   cDynArray<ObjID> & result = *m_pResult;
   int                i;
   BOOL               dummy;
   cOLPTable          found;
   
   for (i = result.Size() - 1; i >= 0; i--)
   {
      if (found.Lookup(result[i], &dummy))
         result.FastDeleteItem(i);
      else
         found.Insert(result[i], TRUE);
   }
}

///////////////////////////////////////

void cObjListParser::AddExclusions(const char * pszExcluded)
{
   BOOL descend;
   if (pszExcluded[0] == kDescendChar)
   {
      pszExcluded++;
      descend = TRUE;
   }
   else
      descend = FALSE;

   if (!m_pExclusions)
      m_pExclusions = new cOLPTable;

   ObjID root = ObjFromString(pszExcluded);
   m_pExclusions->Set(root, TRUE);
   
   if (descend)
   {
      IObjectQuery * pQuery = m_pTraitManager->Query(root, kTraitQueryAllDescendents);
      for (; !pQuery->Done(); pQuery->Next())
      {
         m_pExclusions->Set(pQuery->Object(), TRUE);
      }
      SafeRelease(pQuery);
   }
}

///////////////////////////////////////

void cObjListParser::RemoveExcluded()
{
   cDynArray<ObjID> & result = *m_pResult;
   int                i;
   BOOL               dummy;
   
   for (i = result.Size() - 1; i >= 0; i--)
   {
      if (m_pExclusions->Lookup(result[i], &dummy))
         result.FastDeleteItem(i);
   }
}

///////////////////////////////////////

void cObjListParser::DestroyAll()
{
}

///////////////////////////////////////
//
// Callback from base class
//

BOOL cObjListParser::Append(const char * pszObject)
{
   BOOL descend;
   
   if (pszObject[0] == kExcludeChar)
      AddExclusions(++pszObject);
   
   if (pszObject[0] == kDescendChar)
   {
      descend = TRUE;
      pszObject++;
   }
   else
      descend = FALSE;
   
   ObjID object = ObjFromString(pszObject);

   if (object)
   {
      if (descend)
         AddDescendants(object);
      else
         m_pResult->Append(object);
   }

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
