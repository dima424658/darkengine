/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/relinver.h,v 1.5 2000/01/29 13:24:54 adurant Exp $
#pragma once

#ifndef __RELINVER_H
#define __RELINVER_H
#include <relation.h>

/////////////////////////////////////////////////////////////
// THE STANDARD INVERSE RELATION
//
// Takes a relation, and inverts it
//



class cInverseRelation : public cCTUnaggregated<IRelation,&IID_IRelation,kCTU_Default>
{
protected:
   IRelation* Relation;
   sRelationDesc Desc;


   static void LGAPI Listener(sRelationListenMsg* msg, RelationListenerData data); 

public:
   cInverseRelation(IRelation* relation);
   virtual ~cInverseRelation(); 
   
   STDMETHOD_(const sRelationDesc*, Describe)() const;
   STDMETHOD_(RelationID,GetID)() const; 
   STDMETHOD(SetID)(RelationID id); 
   STDMETHOD_(const sRelationDataDesc*, DescribeData)() const;
   STDMETHOD_(IRelation*, Inverse)();
   STDMETHOD_(LinkID, Add)(ObjID source, ObjID dest);
   STDMETHOD_(LinkID, AddFull)(ObjID source, ObjID desc, void* data);
   STDMETHOD(Remove)(LinkID id); 
   STDMETHOD_(BOOL,Get)(LinkID id, sLink* out) const; 
   STDMETHOD(SetData)(LinkID id, void* data);
   STDMETHOD_(void*,GetData)(LinkID id);
   STDMETHOD_(ILinkQuery*,Query)(ObjID source, ObjID dest) const;
   STDMETHOD(Notify)(eRelationNotifyMsg msg, RelationNotifyData data);
   STDMETHOD(Listen)(RelationListenMsgSet interests, RelationListenFunc func, RelationListenerData data);   
   STDMETHOD_(LinkID, GetSingleLink)(ObjID source, ObjID dest);
   STDMETHOD_(BOOL,AnyLinks)(ObjID source, ObjID dest);
   
};

#endif // __RELINVER_H
