/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/relatio_.h,v 1.10 2000/01/29 13:24:51 adurant Exp $
#pragma once

#ifndef __RELATIO__H
#define __RELATIO__H
#include <relation.h>
#include <linkbase.h>
#include <linkknow.h>
#include <dynarray.h>
#include <linkqdb.h>
#include <linkstor.h>
#include <linkint.h>
#include <dataops.h>
#include <iobjnet.h>
#include <netman.h>

////////////////////////////////////////////////////////////
// 
// cBaseRelation 
//
// Common methods that almost every relation implementation would want to use. 
//
////////////////////////////////////////////////////////////

class cUnknownRelation : public IRelation
{
public:
   DECLARE_UNAGGREGATABLE(); 

   virtual ~cUnknownRelation() {};

};

class cBaseRelation : public cUnknownRelation, protected cLinkManagerKnower
{
public: 


   cBaseRelation(const sRelationDesc& desc, const sRelationDataDesc& datadesc) 
      : Desc(desc), DataDesc(datadesc)
   { ID = LinkMan()->AddRelation(this);};
   ~cBaseRelation() { delete DataDesc.data_ops; };

   //
   // IRelation Methods
   // 

   STDMETHOD_(const sRelationDesc*, Describe)() const;
   STDMETHOD_(RelationID,GetID)() const;
   STDMETHOD_(const sRelationDataDesc*, DescribeData)() const;
   STDMETHOD_(IRelation*, Inverse)();
   STDMETHOD(Listen)(RelationListenMsgSet interests, RelationListenFunc func, RelationListenerData data);
   
   STDMETHOD_(LinkID, GetSingleLink)(ObjID source, ObjID dest);
   STDMETHOD_(BOOL, AnyLinks)(ObjID source, ObjID dest);

   //
   // Helper functions
   //

   void CallListeners(eRelationListenMsg msg, LinkID id, sLink* linkdata = NULL);
protected:
   sRelationDesc Desc;
   sRelationDataDesc DataDesc;
   RelationID ID;

   struct Listener
   {
      RelationListenMsgSet interests; 
      RelationListenFunc func;
      RelationListenerData data; 
      Listener(RelationListenMsgSet i, RelationListenFunc f, 
                   RelationListenerData d) 
         :interests(i),func(f),data(d) {};
   }; 
   cDynArray<Listener> Listeners;

   // Interfaces that are needed by & shared by all properties.
   static INetManager *gmNetMan;
   static IObjectNetworking *gmObjNet;

   // Temporary flag variables, to prevent loops in networking. These
   // are global solely because they are needed in static methods.
   static ObjID gmRespondSourceObj;
   static ObjID gmRespondDestObj;

   // Private networking function which must be implemented by any descendent.
   virtual void SendLinkMsg(eRelationListenMsg ops, LinkID id, sLink* linkdata) = 0;
};

////////////////////////////////////////////////////////////
//
// cStandardRelation
//
// The standard relation implementation.
// Basically, since queries over a single relation fall in 
// to four cases (enumerated in linkbase.h), there is a separate
// query database for each case.
//
////////////////////////////////////////////////////////////


class cLinkQueryDatabaseSet 
{
   ILinkQueryDatabase* DB[kRelationNumQueryCases]; 
   void Init(void);

public:
   cLinkQueryDatabaseSet(ILinkQueryDatabase* none, 
                         ILinkQueryDatabase* source = NULL, 
                         ILinkQueryDatabase* dest = NULL,
                         ILinkQueryDatabase* both = NULL);
   cLinkQueryDatabaseSet(const cLinkQueryDatabaseSet& db);
   cLinkQueryDatabaseSet(IUnknown* db);
   ~cLinkQueryDatabaseSet();
      
   ILinkQueryDatabase* operator[](int n) const {return DB[n];};
   int Size() const { return sizeof(DB)/sizeof(DB[0]);};
};

class cStandardRelation : public cBaseRelation
{
   void Init(); // Common constructor stuff
   
public:
   // Constructor, takes a store and a database. 
   cStandardRelation(const sRelationDesc& desc, const sRelationDataDesc& ddesc, IUnknown* store, IUnknown* Database, IUnknown* datastor = NULL);
   // Takes the whole set of four
   cStandardRelation(const sRelationDesc& desc, const sRelationDataDesc& ddesc, IUnknown* store, const cLinkQueryDatabaseSet& dbs, IUnknown* datastor = NULL);
   virtual ~cStandardRelation();

   //
   // IRelation methods
   //
   STDMETHOD(SetID)(RelationID id); 
   STDMETHOD_(LinkID, Add)(ObjID source, ObjID dest);
   STDMETHOD_(LinkID, AddFull)(ObjID source, ObjID desc, void* data);   
   STDMETHOD(Remove)(LinkID id); 
   STDMETHOD_(BOOL,Get)(LinkID id, sLink* out) const; 
   STDMETHOD(SetData)(LinkID id, void* data);
   STDMETHOD_(void*,GetData)(LinkID id);
   STDMETHOD_(ILinkQuery*,Query)(ObjID source, ObjID dest) const;
   STDMETHOD(Notify)(eRelationNotifyMsg msg, RelationNotifyData data);

private:
   cLinkQueryDatabaseSet DB;
   ILinkStore* Store;
   ILinkDataStore* DataStore;

   static tNetMsgHandlerID gmNetMsgHandlerID; // handler ID included at top of network messages

   // private networking functions
   void SendLinkMsg(eRelationListenMsg ops, LinkID id, sLink* linkdata);
   static void ReceiveLinkMsg(const sNetMsg_Generic *pMsg, ulong size, ObjID from, void*);
};

#endif // __RELATIO__H




