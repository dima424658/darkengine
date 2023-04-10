/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkarry.h,v 1.8 2000/01/29 13:23:13 adurant Exp $
#pragma once

#ifndef __LINKARRY_H
#define __LINKARRY_H
#include <dynarray.h>
#include <objtype.h>
#include <linktype.h>
#include <linkqdb_.h>
#include <linksto_.h>
#include <linkknow.h>
#include <linkid.h>

typedef struct LinkEnds
{
   ObjID source;
   ObjID dest;

   LinkEnds(ObjID s, ObjID d) :source(s),dest(d) {};
} LinkEnds;


typedef LinkEnds LinkElem;

#define FREELIST_END 0xFFFFFFFF

typedef cDynArray<LinkElem> cLinkVec; 


class LinkArray : protected cLinkManagerKnower
{

   cLinkVec links[kNumLinkPartitions];
   ulong FreeHead[kNumLinkPartitions];
   RelationID Rel; 

public:
   LinkArray() 
      : Rel(RELID_NULL) 
   {
      Clear(); 
   };

   class Iterator
   {
      LinkArray& vec; 
      int part; 
      int subidx; 

      void Skip(); 

   public:
      Iterator(LinkArray& v);

      BOOL Done() const;
      void Next();
      LinkID ID() const; 
      void Link(sLink* link) const;
   }; 

   friend class Iterator; 

   LinkID Add(ObjID source, ObjID dest);
   void AddAtID(LinkID id, ObjID source, ObjID dest);
   void Remove(LinkID id);

   BOOL InUse(LinkID id); 
   LinkEnds& operator[](LinkID id) { return links[LINKID_PARTITION(id)][LINKID_IDX(id)];}
   RelationID Relation() { return Rel;};
   BOOL SetRelation(RelationID id);
   void Clear();  

   Iterator Iter() { return Iterator(*this); }; 
};


//
// LinkArray query database
//


class cLinkArrayQueryDatabase : public ILinkQueryDatabase
{
   LinkArray* Vec;
   
public:
   DECLARE_DELEGATION();

   cLinkArrayQueryDatabase(IUnknown* pOuter, LinkArray* vec) : Vec(vec) { INIT_DELEGATION(pOuter);};

   STDMETHOD(AddRelation)(RelationID id ) { return Vec->Relation() == id ?  S_OK : E_FAIL; }
   STDMETHOD(RemoveRelation)(RelationID  ) { return S_OK ; }; 

   STDMETHOD_(ILinkQuery*,QueryComplex)(ObjID source, ObjID dest, RelationID rel, tQueryDate birthday);
   STDMETHOD_(ILinkQuery*,Query)(ObjID source, ObjID dest, RelationID rel)
   { return QueryComplex(source,dest,rel,QUERY_DATE_NONE); }; 

   STDMETHOD(Add)(LinkID ,sLink*) { return S_OK;};
   STDMETHOD(Remove)(LinkID ,sLink*) { return S_OK;};
   STDMETHOD(Reset)(RelationID ) { return S_OK;};
};

//
// LinkArray Link store
// 

class cLinkArrayLinkStore : public cBaseLinkStore
{
   LinkArray& Vec;
   
public:
   // This is a little wasteful, since cBaseLinkStore is already unaggregatable
   DECLARE_DELEGATION(); 

   cLinkArrayLinkStore(IUnknown* pOuter, LinkArray* vec) 
      : Vec(*vec) { INIT_DELEGATION(pOuter); }; 

   STDMETHOD(AddRelation)(RelationID id);
   STDMETHOD(RemoveRelation)(RelationID id);
   STDMETHOD_(LinkID,Add)(sLink* link);
   STDMETHOD(AddAtID)(LinkID id, sLink* link);
   STDMETHOD(Remove)(LinkID id);
   STDMETHOD_(BOOL,Get)(LinkID id, sLink* out);
   STDMETHOD_(ILinkQuery*,GetAll)(RelationID id); 
   STDMETHOD(Reset)(RelationID id);
};

class cLinkArrayDelegateBase : public IUnknown 
{
public:
   virtual ~cLinkArrayDelegateBase() {}; 
   DECLARE_UNAGGREGATABLE();
};

class cLinkArrayDelegate : public cLinkArrayDelegateBase
{
   LinkArray Vec;
   cLinkArrayQueryDatabase* qdb;
   cLinkArrayLinkStore* store;

public:
   STDMETHOD(QueryInterface)(REFIID id, void** ppI);

   cLinkArrayDelegate() : qdb(NULL), store(NULL) { };
   ~cLinkArrayDelegate() { delete qdb; delete store;} ;
   
};

#endif // __LINKARRY_H






