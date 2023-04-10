/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aimanbhv.h,v 1.2 2000/01/29 12:45:30 adurant Exp $
//
//
//
#pragma once

#ifndef __AIMANBHV_H
#define __AIMANBHV_H

#include <hashpp.h>
#include <hshpptem.h>
#include <aiapibhv.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIBehaviorSets
//

template <class KEY> 
class cHashTableCaselessFunctions
{
public:
   static unsigned Hash(const KEY & k)
   {
      return cCaselessStringHashFuncs::Hash(k);
   }

   static BOOL IsEqual(const KEY& k1, const KEY& k2)
   {
      return cCaselessStringHashFuncs::IsEqual(k1,k2);
   }
};

class cAIBehaviorSets : private cHashTable<const char *, IAIBehaviorSet *, cHashTableCaselessFunctions<const char *> >
{
public:
   void             Add(IAIBehaviorSet * pBehaviorSet);
   IAIBehaviorSet * Get(const char * pszName);
   void             ReleaseAll();
};

///////////////////////////////////////

inline void cAIBehaviorSets::Add(IAIBehaviorSet * pBehaviorSet)
{
   Insert(pBehaviorSet->GetName(), pBehaviorSet);
}

///////////////////////////////////////

inline IAIBehaviorSet * cAIBehaviorSets::Get(const char * pszName)
{
   IAIBehaviorSet * pResult = NULL;
   Lookup(pszName, &pResult);
   return pResult;
}

///////////////////////////////////////

inline void cAIBehaviorSets::ReleaseAll()
{
   cIter iter = Iter();

   while (!iter.Done())
   {
      iter.Value()->Release();
      iter.Next();
   }
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__AIMANBHV_H */
