/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/proplnk_.h,v 1.5 2000/01/29 13:24:36 adurant Exp $
#pragma once
#ifndef __PROPLNK__H
#define __PROPLNK__H

#include <property.h>
#include <propfac_.h>
#include <linktype.h>
#include <linkprop.h>

class cDelegatedRelation;


class cObjIDDataOps : public cSimpleDataOps
{
public:

   STDMETHOD(Read)(sDatum* pdat, IDataOpsFile* file, int ver); 

};


class cLinkProperty : public cGenericIntProperty
{
   void Init(const sRelationDesc* rdesc, const sRelationDataDesc* rddesc);
   static void LGAPI ListenFunc(sPropertyListenMsg* msg, PropListenerData data);

public:
   cLinkProperty(const sPropertyDesc* pdesc, 
                 const sRelationDesc* rdesc,
                 const sRelationDataDesc* rddesc,
                 ILinkQueryDatabase* DestKnown,
                 ePropertyImpl impl);

   cLinkProperty(const sPropertyDesc* pdesc, 
                 const sRelationDesc* rdesc,
                 const sRelationDataDesc* rddesc,
                 ILinkQueryDatabase* DestKnown,
                 IPropertyStore* store);

   virtual ~cLinkProperty();
   
   STDMETHOD(QueryInterface)(REFIID id, void** ppI);
   STDMETHOD(Set)(ObjID obj, int val);
   STDMETHOD_(void, Notify)(ePropertyNotifyMsg msg, PropNotifyData data);

protected:
   cDelegatedRelation* Relation;
   RelationID RelID; 
   ILinkQueryDatabase* DestKnown; 
   cObjIDDataOps mOps; 
   
};


#endif // __PROPLNK__H











