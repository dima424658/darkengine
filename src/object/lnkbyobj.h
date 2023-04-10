/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/lnkbyobj.h,v 1.6 2000/01/29 13:23:40 adurant Exp $
#pragma once

#ifndef __LNKBYOBJ_H
#define __LNKBYOBJ_H
#include <objtype.h>
#include <linktype.h>
#include <dlistsim.h>
#include <hashpp.h>
#include <linkqdb_.h>
#include <linkknow.h>
#include <linkset.h>


////////////////////////////////////////////////////////////
// ONE-OBJECT-KNOWN QUERY OPTIMIZATION
//

//
// By-object link table
//

typedef cLinkSet LinkSet;
typedef cScalarHashFunctions<ObjID> ByObjHashFuncs;
typedef cHashTable<ObjID,LinkSet*,ByObjHashFuncs> LinksByObjTable;

class LinksByObj : public LinksByObjTable, protected cLinkManagerKnower
{
protected:
   virtual int CompareLinks(LinkID l1, LinkID l2) { return l1 - l2;}; 

public: 
   virtual ~LinksByObj() { } ;

   void AddLink(ObjID obj, LinkID id);
   void RemoveLink(ObjID obj, LinkID id);
   void RemoveRelation(RelationID id);
};

////////////////////////////////////////////////////////////
// QUERY DATABASE FOR THE SOURCE-KNOWN CASE
//

class cFromObjQueryDatabase : public cBaseLinkQueryDatabase
{
   LinksByObj& Table;
   BOOL our_table; 

public:
   cFromObjQueryDatabase(LinksByObj* table = NULL,BOOL sponsor = FALSE);
   ~cFromObjQueryDatabase();

   STDMETHOD(Add)(LinkID id, sLink* link);
   STDMETHOD(Remove)(LinkID id, sLink* link);
   STDMETHOD(Reset)(RelationID id);

   STDMETHOD_(ILinkQuery*,QueryComplex)(ObjID source, ObjID desc, RelationID rel,tQueryDate birthday);
};

////////////////////////////////////////////////////////////
// QUERY DATABASE FOR THE DEST-KNOWN CASE
//

class cToObjQueryDatabase : public cBaseLinkQueryDatabase
{
   LinksByObj& Table;
   BOOL our_table;

public:
   cToObjQueryDatabase(LinksByObj* table = NULL, BOOL sponsor = FALSE);
   ~cToObjQueryDatabase();

   STDMETHOD(Add)(LinkID id, sLink* link);
   STDMETHOD(Remove)(LinkID id, sLink* link);
   STDMETHOD(Reset)(RelationID id);

   STDMETHOD_(ILinkQuery*,QueryComplex)(ObjID source, ObjID desc, RelationID rel,tQueryDate birthday);
}; 

#endif // __LNKBYOBJ_H


