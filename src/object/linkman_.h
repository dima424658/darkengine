/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkman_.h,v 1.10 2000/01/29 13:23:27 adurant Exp $
#pragma once

#ifndef __LINKMAN__H
#define __LINKMAN__H
#include <linkman.h>
#include <aggmemb.h>
#include <linkqdb_.h>
#include <hashset.h>
#include <dynarray.h>
#include <relnull.h>
#include <hashpp.h>
#include <linkknow.h>
#include <linkint.h>
#include <linkbase.h>

F_DECLARE_INTERFACE(ITagFile); 

typedef cScalarHashFunctions<LinkID> LinkHashFuncs; 
typedef cHashTable<LinkID,tQueryDate,LinkHashFuncs> LinkTimeTable;

struct LinkQDBEntry
{
   ILinkQueryDatabase* db;
   int count; 

   LinkQDBEntry(ILinkQueryDatabase* db_, int cnt = 0) :db(db_),count(cnt) {};
};

typedef cDynArray<LinkQDBEntry> LinkQDBList;

////////////////////////////////////////////////////////////
// LINK MANAGER INTERNALS
//

class cLinkManager : public ILinkManagerInternal, public cLinkManagerKnower
{

   // Hash table mapping strings to Properties 
   class cRelationTable : public cStrIHashSet <IRelation *>
   {
   public:
      cRelationTable () {};
      tHashSetKey GetKey (tHashSetNode node) const
      {
         return (tHashSetKey) (((IRelation *) (node))->Describe()->name);
      };
   };

   // We're going to be an aggregate member when we grow up
   DECLARE_AGGREGATION(cLinkManager);

public:
   cLinkManager(IUnknown* outer); 
   virtual ~cLinkManager();

   //
   // ILinkManager methods
   //

   STDMETHOD_(IRelation*,GetRelation)(RelationID id);
   STDMETHOD_(IRelation*,GetRelationNamed)(const char* name);
   STDMETHOD(Notify)(eRelationNotifyMsg msg, RelationNotifyData data);
   STDMETHOD(IterStart)(sRelationIter* iter) ;
   STDMETHOD_(BOOL,IterNext)(sRelationIter* iter, RelationID* rel); 
   STDMETHOD(IterStop)(sRelationIter* iter);
   STDMETHOD_(LinkID, Add)(ObjID source, ObjID dest, RelationID id);
   STDMETHOD_(LinkID, AddFull)(ObjID source, ObjID desc, RelationID id, void* data);
   STDMETHOD(Remove)(LinkID id); 
   STDMETHOD_(BOOL,Get)(LinkID id, sLink* out) const; 
   STDMETHOD(SetData)(LinkID id, void* data);
   STDMETHOD_(void*,GetData)(LinkID id);
   STDMETHOD_(ILinkQuery*,Query)(ObjID source, ObjID dest, RelationID flavor);

   STDMETHOD_(LinkID, GetSingleLink)(RelationID id, ObjID source, ObjID dest);
   STDMETHOD_(BOOL, AnyLinks)(RelationID id, ObjID source, ObjID dest);

   //------------------------------------------------------------
   // ILinkManagerInternal methods
   // 

   STDMETHOD_(RelationID,AddRelation)(IRelation* relation);
   STDMETHOD(RemoveRelation)(RelationID relid);
   STDMETHOD(ReplaceRelation)(RelationID relid, IRelation* relation);
   STDMETHOD(AddQueryDB)(ILinkQueryDatabase* db,RelationQueryCaseSet cases);
   STDMETHOD(RemoveQueryDB)(ILinkQueryDatabase* db,RelationQueryCaseSet cases);
   STDMETHOD_(tQueryDate,Lock)();
   STDMETHOD(Unlock)(tQueryDate lockdate);
   STDMETHOD_(long,GetLockCount)();
   STDMETHOD_(BOOL,LinkValid)(LinkID link, tQueryDate age);
   STDMETHOD(LinkBirth)(LinkID id); 
   STDMETHOD_(BOOL,LinkDeath)(LinkID id);
   STDMETHOD_(LinkID,RemapOnLoad)(LinkID id); 

protected:
   //------------------------------------------------------------
   // Aggregate protocol
   //
   
   HRESULT Init();
   HRESULT End();

   //------------------------------------------------------------
   // Helpers
   // 
   void save_relation_ids(ITagFile* file, ulong filetype);
   void load_relation_ids(ITagFile* file, ulong filetype); 

   class cByID : public cDynArray<IRelation*>
   {
   }; 

   class cIDMap : public cDynArray<RelationID>
   {
   }; 

protected:

   cRelationTable  byName;
   cByID byID;
   cIDMap idMap[kNumLinkPartitions]; 
   cNullRelation* NullRel;
   LinkQDBList QueryDBs[4];
   tQueryDate QueryTime;
   tQueryDate LatestBirth;
   tQueryDate EarliestDeath;
   ulong LockCount;
   LinkTimeTable BirthDates,DeathDates;
   BOOL MidPurge;
};


#endif // __LINKMAN__H

