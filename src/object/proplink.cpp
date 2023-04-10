/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/proplink.cpp,v 1.7 1998/03/26 13:50:34 mahk Exp $

#include <proplnk_.h>
#include <linktype.h>
#include <linkbase.h>
#include <relation.h>
#include <reldeleg.H>

#include <proplink.h>
#include <linkprop.h>
#include <linkstor.h>
#include <linkqdb.h>
#include <linkid.h>

#include <string.h>

#include <objremap.h>
#include <osysbase.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
//
// cObjIDDataOps
//
////////////////////////////////////////////////////////////

STDMETHODIMP cObjIDDataOps::Read(sDatum* pdat, IDataOpsFile* file, int ver)
{
   cSimpleDataOps::Read(pdat,file,ver);
   ObjID* pObj = (ObjID*)pdat;
   // remap on load
   *pObj = ObjRemapOnLoad(*pObj);
   Assert_(*pObj == OBJ_NULL || ObjExists(*pObj));
   return S_OK; 
}


////////////////////////////////////////////////////////////
//
// cLinkProperty
//
////////////////////////////////////////////////////////////

//------------------------------------------------------------
// CONSTRUCTION
//

cLinkProperty::cLinkProperty(const sPropertyDesc* pdesc, 
                 const sRelationDesc* rdesc,
                 const sRelationDataDesc* rddesc,
                 ILinkQueryDatabase* destknown,
                 ePropertyImpl impl)
   : cGenericIntProperty(pdesc,impl),
     Relation(NULL),
     RelID(RELID_NULL),
     DestKnown(destknown)
{
   Init(rdesc,rddesc);
}

cLinkProperty::cLinkProperty(const sPropertyDesc* pdesc, 
                 const sRelationDesc* rdesc,
                 const sRelationDataDesc* rddesc,
                 ILinkQueryDatabase* destdb,
                 IPropertyStore* store)
   : cGenericIntProperty(pdesc,store),
     Relation(NULL),
     RelID(RELID_NULL),
     DestKnown(destdb)
{
   Init(rdesc,rddesc);
}

void cLinkProperty::Init(const sRelationDesc* rdesc, 
                         const sRelationDataDesc* rddesc)
{
   sRelationDesc copy_rdesc = *rdesc;
   copy_rdesc.flags |= kRelationTransient; 

   mpStore->SetOps(&mOps); 

   sCustomRelationDesc custdesc;
   memset(&custdesc,0,sizeof(custdesc));

   custdesc.store = new cPropertyLinkStore(this);
   custdesc.DBs[kRelationDestKnown] = DestKnown;
   IRelation* inner = CreateCustomRelation(&copy_rdesc,rddesc,&custdesc);
   Relation = new cDelegatedRelation(this,inner);
   RelID = Relation->GetID();
   if (DestKnown != NULL)
      DestKnown->AddRef();
   else
      Verify(SUCCEEDED(custdesc.store->QueryInterface(IID_ILinkQueryDatabase,(void**)&DestKnown)));

   // Listen for sets/unsets
   Listen(kListenPropSet|kListenPropUnset|kListenPropModify,ListenFunc,(PropListenerData)this);
   
   SafeRelease(custdesc.store);
   SafeRelease(inner);
}

cLinkProperty::~cLinkProperty()
{
   delete Relation;
   SafeRelease(DestKnown);
}

//------------------------------------------------------------
// IUNKNOWN METHODS
//

STDMETHODIMP cLinkProperty::QueryInterface(REFIID id, void** ppI)
{
   if (id == IID_IRelation)
   {
      Relation->AddRef();
      *ppI = Relation;
      return S_OK;
   }
   return cGenericIntProperty::QueryInterface(id,ppI);
}

//------------------------------------------------------------
// IPROPERTY METHODS
//

//
// Note that this relies upon the fact that the link with a source 
// of obj has the id LINKID_MAKE(RelID,obj)
//

STDMETHODIMP cLinkProperty::Set(ObjID obj, int val)
{
   // Bust the link
   LinkID id = PROPLINKID_MAKE(RelID,obj);
   sLink link = { OBJ_NULL, OBJ_NULL } ;
   if (Relation->Get(id,&link))
      DestKnown->Remove(id,&link);
   return cGenericIntProperty::Set(obj,val);
}

//
// Notify, remapping on load
//

STDMETHODIMP_(void) cLinkProperty::Notify(ePropertyNotifyMsg msg, PropNotifyData data)
{
   cGenericIntProperty::Notify(msg,data);
}


////////////////////////////////////////////////////////////
// LISTENER CALLBACK
//

void LGAPI cLinkProperty::ListenFunc(sPropertyListenMsg* msg, PropListenerData data)
{
   cLinkProperty* prop = (cLinkProperty*)data;
   sLink link;

   link.source = msg->obj;
   link.dest = msg->value.intval;
   link.flavor = prop->RelID;
   
   LinkID id = PROPLINKID_MAKE(prop->RelID,link.source);
   if (msg->type & kListenPropUnset)
   {
      // bust the link
      prop->DestKnown->Remove(id,&link);
   }
   if (msg->type & (kListenPropSet|kListenPropModify))
   {
      // make the link
      prop->DestKnown->Add(id,&link);
   }
}

////////////////////////////////////////////////////////////

IProperty* CreateLinkedProperty(const sPropertyDesc* pdesc, 
                                       const sRelationDesc* rdesc,
                                       const sRelationDataDesc* rddesc,
                                       ILinkQueryDatabase* DestKnown,
                                       ePropertyImpl impl)
{
   return new cLinkProperty(pdesc,rdesc,rddesc,DestKnown,impl);
}




IProperty* CreateLinkedPropertyFromStore(const sPropertyDesc* pdesc, 
                                       const sRelationDesc* rdesc,
                                       const sRelationDataDesc* rddesc,
                                       ILinkQueryDatabase* DestKnown,
                                       IPropertyStore* impl)
{
   return new cLinkProperty(pdesc,rdesc,rddesc,DestKnown,impl);
}






