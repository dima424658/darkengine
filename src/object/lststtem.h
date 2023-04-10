/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/lststtem.h,v 1.1 1997/10/06 19:05:57 mahk Exp $
#pragma once  
#ifndef __LSTSTTEM_H
#define __LSTSTTEM_H

#include <listset.h>
#include <dlisttem.h>

//------------------------------------------------------------
// TEMPLATE: cSimpleListSet
//
#define LIST_TEMPLATE template <class ELEM>
#define LISTSET cSimpleListSet<ELEM>


LIST_TEMPLATE BOOL LISTSET::AddElem(const ELEM& elem)
{
   for (cIter iter = Iter(); !iter.Done(); iter.Next())
   {
      if (elem == iter.Value())
         return FALSE;
   }
   Prepend(elem);
   nElems++;
   return TRUE;
}

////////////////////////////////////////

LIST_TEMPLATE BOOL LISTSET::RemoveElem(const ELEM& elem)
{
   for (cIter iter = Iter(); !iter.Done(); iter.Next())
   {
      if (elem == iter.Value())
      {
         Delete(iter.Node());
         nElems--;
         return TRUE;
      }
   }   
   return FALSE;
}

////////////////////////////////////////

LIST_TEMPLATE BOOL LISTSET::HasElem(const ELEM& elem)
{
   for (cIter iter = Iter(); !iter.Done(); iter.Next())
   {
      if (elem == iter.Value())
      {
         return TRUE;
      }
   }   
   return FALSE;
}

////////////////////////////////////////

LIST_TEMPLATE void LISTSET::RemoveAll(void)
{
   for (cIter iter = Iter(); !iter.Done(); iter.Next())
   {
      Delete(iter.Node());
   }      
   nElems = 0;
}

#undef LIST_TEMPLATE
#undef LISTSET


#endif // __LSTSTTEM_H
