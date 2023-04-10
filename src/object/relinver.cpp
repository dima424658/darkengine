/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/relinver.cpp,v 1.10 1998/10/19 13:04:49 mahk Exp $
#include <linkbase.h>
#include <relinver.h>
#include <lnkquery.h>
#include <string.h>
#include <linkid.h>

#include <dlist.h>
#include <dlisttem.h>

// Must be last header
#include <dbmem.h>

cInverseRelation::cInverseRelation(IRelation* rel)
   : Relation(rel)
{
   rel->AddRef();

   // build the descriptor
   const sRelationDesc* desc = rel->Describe();
   Desc = *desc;
   strcpy(Desc.name,RELNAME_INVERSE_PREFIX);
   strncat(Desc.name,desc->name,sizeof(Desc.name)-strlen(Desc.name)-1);
}


cInverseRelation::~cInverseRelation()
{
   SafeRelease(Relation);
}

STDMETHODIMP_(const sRelationDesc*) cInverseRelation::Describe() const
{
   return &Desc;
}

////////////////////////////////////////

STDMETHODIMP_(RelationID) cInverseRelation::GetID() const 
{
   return RELID_INVERT(Relation->GetID());
}

////////////////////////////////////////

STDMETHODIMP cInverseRelation::SetID(RelationID id)  
{
   Warning(("Someone is calling cInverseRelation::SetID()!\n")); 
   return Relation->SetID(RELID_INVERT(id));
}


////////////////////////////////////////

STDMETHODIMP_(const sRelationDataDesc*) cInverseRelation::DescribeData() const
{
   return Relation->DescribeData();
}

////////////////////////////////////////

STDMETHODIMP_(IRelation*) cInverseRelation::Inverse()
{
   Relation->AddRef();
   return Relation;
}

////////////////////////////////////////

STDMETHODIMP_(LinkID) cInverseRelation::Add(ObjID source, ObjID dest)
{
   LinkID id = Relation->Add(dest,source);
   return LINKID_INVERT(id); 
}

STDMETHODIMP_(LinkID) cInverseRelation::AddFull(ObjID s, ObjID d, void* dat)
{
   LinkID id = Relation->AddFull(d,s,dat);
   return LINKID_INVERT(id); 
}

////////////////////////////////////////

STDMETHODIMP cInverseRelation::Remove(LinkID link)
{
   return Relation->Remove(LINKID_INVERT(link));
}


static void invert_slink(sLink* link)
{
   ObjID tmp = link->source;
   link->source = link->dest;
   link->dest = tmp; 
   link->flavor = RELID_INVERT(link->flavor);
}

STDMETHODIMP_(BOOL) cInverseRelation::Get(LinkID id, sLink* link) const
{
   if (Relation->Get(LINKID_INVERT(id),link))
   {
      invert_slink(link);
      return TRUE;
   }
   return FALSE;
}

STDMETHODIMP cInverseRelation::SetData(LinkID id, void* data)
{
   return Relation->SetData(LINKID_INVERT(id),data);
}

void* cInverseRelation::GetData(LinkID id)
{
   return Relation->GetData(LINKID_INVERT(id));
}

////////////////////////////////////////

// Nothing better to do here than hand off to the real relation
STDMETHODIMP cInverseRelation::Notify(eRelationNotifyMsg msg, RelationNotifyData data)
{
   return Relation->Notify(msg,data);
}

////////////////////////////////////////

struct sListenerData
{
   RelationListenFunc func; 
   RelationListenerData data; 
}; 


void LGAPI cInverseRelation::Listener(sRelationListenMsg* msg_, RelationListenerData data)
{
   sRelationListenMsg msg = *msg_; 
   sListenerData* listen = (sListenerData*)data;
   // invert the message
   msg.id = LINKID_INVERT(msg.id); 
   invert_slink(&msg.link); 
   
   listen->func(&msg,listen->data); 
}

// Just a list of listeners so that they get cleaned up at the end of time
// If we add unlisten, each elem is going to need a relation/handle id. 

static cSimpleDList<sListenerData> gListeners; 

STDMETHODIMP cInverseRelation::Listen(RelationListenMsgSet msgs, RelationListenFunc func , RelationListenerData data)
{
   sListenerData listener = {  func, data }; 
   // add the listener to our list
   gListeners.Append(listener); 

   // Get the actual copy that's in our list
   sListenerData* our_data = &gListeners.GetLast()->Value();

   // add our listener 
   return Relation->Listen(msgs,Listener,our_data); 
}   

////////////////////////////////////////

STDMETHODIMP_(LinkID) cInverseRelation::GetSingleLink(ObjID source, ObjID dest)
{
   return Relation->GetSingleLink(dest, source);
}

////////////////////////////////////////

STDMETHODIMP_(BOOL) cInverseRelation::AnyLinks(ObjID source, ObjID dest)
{
   return Relation->AnyLinks(dest, source);
}

////////////////////////////////////////////////////////////
//
// cInverseQuery 
//
// Take a query and invert it. 
////////////////////////////////////////////////////////////


class cInverseQuery : public ILinkQuery 
{
   ILinkQuery* Query;
public:
   DECLARE_UNAGGREGATABLE();

   cInverseQuery(ILinkQuery* q) :Query(q) { Query->AddRef();}; 
   virtual ~cInverseQuery() { SafeRelease(Query);};


   STDMETHOD_(BOOL,Done)() const; 
   STDMETHOD(Link)(sLink* link) const;
   STDMETHOD(Next)();   

   STDMETHOD_(LinkID,ID)() const 
   { 
      LinkID id = Query->ID(); 
      return LINKID_INVERT(id);
   };
   STDMETHOD_(void*,Data)() const { return Query->Data();};

   STDMETHOD_(ILinkQuery*,Inverse)() 
   { 
      Query->AddRef();
      return Query;
   };

};

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cInverseQuery,ILinkQuery);


STDMETHODIMP_(BOOL) cInverseQuery::Done() const
{
   return Query->Done();
}

STDMETHODIMP cInverseQuery::Link(sLink* link) const
{
   HRESULT result = Query->Link(link);
   if (FAILED(result)) return result;
   invert_slink(link);
   return result;
}

STDMETHODIMP cInverseQuery::Next()
{
   return Query->Next();
}

////////////////////////////////////////

STDMETHODIMP_(ILinkQuery*) cInverseRelation::Query(ObjID s, ObjID d) const
{
   ILinkQuery* q = Relation->Query(d,s);
   ILinkQuery* retval = q->Inverse();
   SafeRelease(q);
   return retval;
}

////////////////////////////////////////

ILinkQuery* CreateInverseLinkQuery(ILinkQuery* q)
{
   return new cInverseQuery(q);
}
