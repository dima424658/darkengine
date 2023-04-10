/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkboth.h,v 1.2 1997/11/06 18:36:25 mahk Exp $
#pragma once  
#ifndef __LINKBOTH_H
#define __LINKBOTH_H

#include <linktype.h>
#include <hashpp.h>

#include <linkqdb.h>
#include <linkqdb_.h>

#include <linkset.h>
#include <listset.h>

////////////////////////////////////////////////////////////
// QUERY DATABASE FOR THE BOTH-KNOWN CASE
//


//------------------------------------------------------------
// Data Structure Types
//

//
// Hash table key, pack both objID's into a 32 bits
//

struct sTwoObjKey 
{
   short source;
   short dest;
   
   sTwoObjKey(ObjID s = OBJ_NULL, ObjID d = OBJ_NULL) 
      : source((short)s),
        dest((short)d)
   {
   }

   // Hashing functions 
   static BOOL IsEqual(const sTwoObjKey k1, const sTwoObjKey k2)
   { return  k1.source == k2.source && k1.dest == k2.dest; }; 

   static ulong Hash(const sTwoObjKey k)
   {
      return k.source << 16 | k.dest;
   }
};

//
// Hash table
//

class cBothObjTable : public cHashTable<sTwoObjKey,cLinkSet*,sTwoObjKey>
{
   // I'm too cool for typedef 

public:

   void RemoveAll()
   {
      for (cIter iter = Iter(); !iter.Done(); iter.Next())
         delete iter.Value();
      Clear();
   }

   ~cBothObjTable()
   {
      RemoveAll();
   }

};

//
// Relation List
//

class cRelationSet : public cSimpleListSet<RelationID> 
{
   // I'm too cool for typedef 

};

//------------------------------------------------------------
// THE QUERY DATABASE CLASS
//


class cBothKnownQueryDatabase : public cBaseLinkQueryDatabase
{
   cBothObjTable* Table;
   cRelationSet  Rels; 
   BOOL our_table;

public:
   cBothKnownQueryDatabase(cBothObjTable* table = NULL); 
   ~cBothKnownQueryDatabase(); 

   STDMETHOD(AddRelation)(RelationID );
   STDMETHOD(Add)(LinkID id, sLink* link);
   STDMETHOD(Remove)(LinkID id, sLink* link);
   STDMETHOD(Reset)(RelationID id);

   STDMETHOD_(ILinkQuery*,QueryComplex)(ObjID source, ObjID desc, RelationID rel,tQueryDate birthday);
};



#endif // __LINKBOTH_H

