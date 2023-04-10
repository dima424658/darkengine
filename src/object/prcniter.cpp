/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/object/prcniter.cpp,v 1.2 1998/10/05 17:27:07 mahk Exp $
//
//
//

#include <objdef.h>
#include <objquery.h>
#include <trait.h>

#include <prcniter.h>

// Must be last header 
#include <dbmem.h>


///////////////////////////////////////////////////////////////////////////////

BOOL cConcreteIter::RawNext(ObjID * pNext)
{
   if (m_pObjectQuery)
   {
      if (!m_pObjectQuery->Done())
      {
         *pNext = m_pObjectQuery->Object();
         m_pObjectQuery->Next();
         return TRUE;
      }
      SafeRelease(m_pObjectQuery);
   }
   
   if (m_pProperty->IterNext(&m_iter, pNext))
   {
      m_pObjectQuery = m_pTrait->GetAllHeirs(*pNext, kObjectConcrete);
      if (!OBJ_IS_CONCRETE(*pNext))
      {
         if (!m_pObjectQuery->Done())
         {
            *pNext = m_pObjectQuery->Object();
            m_pObjectQuery->Next();
         }
      }
      if (*pNext)
         return TRUE;
   }
   
   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
