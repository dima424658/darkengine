/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propnull.h,v 1.9 2000/01/29 13:24:42 adurant Exp $
#pragma once

#ifndef PROPNULL_H
#define PROPNULL_H
#include <property.h>

////////////////////////////////////////////////////////////
// NULL PROPERTY
//
// This is the IProperty implementation for PROPID_NULL.
// it essentially does nothing.  The Property manager 
// returns it for all undefined properties. 
//

class cNullProperty : public IProperty
{
public: 
   DECLARE_UNAGGREGATABLE();

   cNullProperty() {} ; 
   STDMETHOD_(const sPropertyDesc*, Describe) () const;
   STDMETHOD_(PropertyID,GetID)() const;
   STDMETHOD_(const sPropertyTypeDesc*, DescribeType) () const;
   STDMETHOD(Create) (ObjID obj);
   STDMETHOD(Copy) (ObjID obj,ObjID example);
   STDMETHOD(Delete) (ObjID obj);
   STDMETHOD_(void,Notify) (ePropertyNotifyMsg msg, PropNotifyData data);
   STDMETHOD_(BOOL,IsRelevant)(ObjID obj) const;
   STDMETHOD_(BOOL,IsSimplyRelevant)(ObjID obj) const;
   STDMETHOD_(BOOL,Touch) (ObjID obj); 
   STDMETHOD_(PropListenerHandle, Listen)(PropertyListenMsgSet interests, PropertyListenFunc func, PropListenerData data);
   STDMETHOD(Unlisten)(PropListenerHandle ) { return S_OK; }; 

   STDMETHOD_(void, IterStart) (sPropertyObjIter* iter) const;
   STDMETHOD_(BOOL, IterNext)  (sPropertyObjIter* iter, ObjID* next) const;
   STDMETHOD_(void, IterStop) (sPropertyObjIter* iter) const;
};



#endif // PROPNULL_H
