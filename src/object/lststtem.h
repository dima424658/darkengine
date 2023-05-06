/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/lststtem.h,v 1.1 1997/10/06 19:05:57 mahk Exp $
#pragma once

#include <dlist.h>
#include <listset.h>
#include <dlisttem.h>

//------------------------------------------------------------
// TEMPLATE: cSimpleListSet
//

template <class ELEM> BOOL cSimpleListSet<ELEM>::AddElem(const ELEM& elem)
{
   for (auto iter = cParent::Iter(); !iter.Done(); iter.Next())
   {
      if (elem == iter.Value())
         return FALSE;
   }
   cParent::Prepend(elem);
   nElems++;
   return TRUE;
}

////////////////////////////////////////

template <class ELEM> BOOL cSimpleListSet<ELEM>::RemoveElem(const ELEM& elem)
{
   for (auto iter = cParent::Iter(); !iter.Done(); iter.Next())
   {
      if (elem == iter.Value())
      {
         cParent::Delete(iter.Node());
         nElems--;
         return TRUE;
      }
   }   
   return FALSE;
}

////////////////////////////////////////

template <class ELEM> BOOL cSimpleListSet<ELEM>::HasElem(const ELEM& elem)
{
   for (auto iter = cParent::Iter(); !iter.Done(); iter.Next())
   {
      if (elem == iter.Value())
      {
         return TRUE;
      }
   }   
   return FALSE;
}

////////////////////////////////////////

template <class ELEM> void cSimpleListSet<ELEM>::RemoveAll(void)
{
   for (auto iter = cParent::Iter(); !iter.Done(); iter.Next())
   {
      cParent::Delete(iter.Node());
   }      
   nElems = 0;
}