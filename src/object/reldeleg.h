/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/reldeleg.h,v 1.5 2000/01/29 13:24:53 adurant Exp $
#pragma once

#ifndef __RELDELEG_H
#define __RELDELEG_H

#include <linkint.h>
#include <relation.h>
#include <appagg.h>

class cDelegatedRelation : public IRelation
{
   
public:
   DECLARE_DELEGATION();
   
   cDelegatedRelation(IUnknown* outer, IRelation* inner)
      : pInner(inner)
   {
      inner->AddRef();
      INIT_DELEGATION(outer);

      AutoAppIPtr_(LinkManagerInternal,LinkMan);
      LinkMan->ReplaceRelation(pInner->GetID(),this);
   }

   virtual ~cDelegatedRelation()
   {
      AutoAppIPtr_(LinkManagerInternal,LinkMan);
      LinkMan->RemoveRelation(pInner->GetID());
      pInner->Release();
   }

   STDMETHOD_(const sRelationDesc*, Describe)() const 
   { return pInner->Describe();}

   STDMETHOD_(RelationID,GetID)() const    
   { return pInner->GetID();}

   STDMETHOD(SetID)(RelationID id) 
   { return pInner->SetID(id); }; 

   STDMETHOD_(const sRelationDataDesc*, DescribeData)() const 
   { return pInner->DescribeData();} ;

   STDMETHOD_(IRelation*, Inverse)() 
   { return pInner->Inverse();}

   STDMETHOD(Listen)(RelationListenMsgSet interests, RelationListenFunc func, RelationListenerData data) 
   { return pInner->Listen(interests,func,data);};

   STDMETHOD_(LinkID, Add)(ObjID source, ObjID dest) 
   { return pInner->Add(source,dest);}; 

   STDMETHOD_(LinkID, AddFull)(ObjID source, ObjID dest, void* data)
   { return pInner->AddFull(source,dest,data);};

   STDMETHOD(Remove)(LinkID id) 
   { return pInner->Remove(id);};

   STDMETHOD_(BOOL,Get)(LinkID id, sLink* out) const
   { return pInner->Get(id,out);};

   STDMETHOD(SetData)(LinkID id, void* data)
   { return pInner->SetData(id,data); }

   STDMETHOD_(void*,GetData)(LinkID id)
   { return pInner->GetData(id);};
   
   STDMETHOD_(ILinkQuery*,Query)(ObjID source, ObjID dest) const
   { return pInner->Query(source,dest); };
   STDMETHOD(Notify)(eRelationNotifyMsg msg, RelationNotifyData data)
   { return pInner->Notify(msg,data);};

   STDMETHOD_(LinkID, GetSingleLink)(ObjID source, ObjID dest)
   { return pInner->GetSingleLink(source, dest); };
   STDMETHOD_(BOOL, AnyLinks)(ObjID source, ObjID dest)
   { return pInner->AnyLinks(source,dest); };
   
private:
   IRelation* pInner;
};


#endif // __RELDELEG_H
