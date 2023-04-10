/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/listset.h,v 1.1 1997/10/06 19:05:16 mahk Exp $
#pragma once  
#ifndef __LISTSET_H
#define __LISTSET_H

#include <dlistsim.h>

////////////////////////////////////////////////////////////
// "Set" augmentation for cSimpleDList
//


template <class ELEM>
class cSimpleListSet : public cSimpleDList<ELEM>
{
   ulong nElems;

public:
   cSimpleListSet() : nElems(0) {};

   BOOL AddElem(const ELEM& elem);
   BOOL RemoveElem(const ELEM& elem);
   BOOL HasElem(const ELEM& elem);
   void RemoveAll();
   ulong Size() { return nElems; }; 
};

#endif // __LISTSET_H



