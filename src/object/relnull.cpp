/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/relnull.cpp,v 1.4 1998/05/22 01:23:27 TOML Exp $
#include <linkbase.h>
#include <relnull.h>
#include <querynul.h>

// Must be last header
#include <dbmem.h>

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cNullRelation,IRelation);

STDMETHODIMP_(const sRelationDesc*) cNullRelation::Describe() const
{
   static sRelationDesc desc = { "NULL", }; 
   return &desc;
}

////////////////////////////////////////

STDMETHODIMP_(RelationID) cNullRelation::GetID() const 
{
   return RELID_NULL;
}

STDMETHODIMP cNullRelation::SetID(RelationID ) 
{
   return E_FAIL; 
}


////////////////////////////////////////

STDMETHODIMP_(const sRelationDataDesc*) cNullRelation::DescribeData() const
{
   static sRelationDataDesc desc = { "None", 0};
   return &desc;
}

////////////////////////////////////////

STDMETHODIMP_(IRelation*) cNullRelation::Inverse()
{
   AddRef();
   return this;
}

////////////////////////////////////////

#define Scream()  Warning(("cNullRelation method called (file: %s line %d)\n",__FILE__,__LINE__))


////////////////////////////////////////

STDMETHODIMP_(LinkID) cNullRelation::Add(ObjID, ObjID)
{
   Scream();
   return LINKID_NULL;
}

STDMETHODIMP_(LinkID) cNullRelation::AddFull(ObjID, ObjID, void*)
{
   Scream();
   return LINKID_NULL;
}

////////////////////////////////////////

STDMETHODIMP cNullRelation::Remove(LinkID)
{
   //   Scream();   
   return E_FAIL;
}

STDMETHODIMP_(BOOL) cNullRelation::Get(LinkID, sLink*) const
{
   Scream();   
   return FALSE;
}

STDMETHODIMP cNullRelation::SetData(LinkID, void*)
{
   Scream();   
   return E_FAIL;
}

void* cNullRelation::GetData(LinkID)
{
   Scream();   
   return NULL;
}

////////////////////////////////////////

STDMETHODIMP_(ILinkQuery*) cNullRelation::Query(ObjID, ObjID) const
{
   return new cNullLinkQuery(); 
}

////////////////////////////////////////

STDMETHODIMP cNullRelation::Notify(eRelationNotifyMsg, RelationNotifyData)
{
   Scream();
   return E_FAIL;
}

STDMETHODIMP cNullRelation::Listen(RelationListenMsgSet, RelationListenFunc, RelationListenerData)
{
   Scream();
   return E_FAIL;
}   

STDMETHODIMP_(LinkID) cNullRelation::GetSingleLink(ObjID source, ObjID dest)
{
   Scream();
   return E_FAIL;
}

STDMETHODIMP_(BOOL) cNullRelation::AnyLinks(ObjID source, ObjID dest)
{
   Scream();
   return E_FAIL;
}
