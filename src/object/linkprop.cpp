/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkprop.cpp,v 1.5 1998/01/27 18:51:07 mahk Exp $
#include <linkprop.h>
#include <linkbase.h>
#include <lnkquer_.h>
#include <linkqdb_.h>
#include <propface.h>
#include <osysbase.h>
#include <linkid.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// Property query
//

class cPropertyLinkQuery : public cBaseLinkQuery 
{
   IIntProperty* Prop;
   sPropertyObjIter Iter;
   sLinkTemplate Pattern;
   sLink TheLink;
   
   BOOL Eligible(); 

public:
   cPropertyLinkQuery(IIntProperty* prop, const sLinkTemplate& pattern, 
                      tQueryDate bday = QUERY_DATE_NONE)
      : Prop(prop), 
        Pattern(pattern), 
        TheLink(pattern),
        cBaseLinkQuery(bday)
   { prop->IterStart(&Iter); prop->AddRef();  Next();}

   ~cPropertyLinkQuery();

   STDMETHOD_(BOOL,Done)() const { return TheLink.source == OBJ_NULL; } ; 
   STDMETHOD(Next)();
   STDMETHOD(Link)(LinkID* id, sLink* link) const;

   STDMETHOD_(LinkID,ID)() const { return PROPLINKID_MAKE(Pattern.flavor,TheLink.source);};

};

cPropertyLinkQuery::~cPropertyLinkQuery()
{
   Prop->IterStop(&Iter);
   SafeRelease(Prop);
}


STDMETHODIMP cPropertyLinkQuery::Link(LinkID* id, sLink* link) const
{
   if (Done()) return E_FAIL;
   if (id != NULL)
      *id = ID();
   if (link != NULL)
   {
      *link = TheLink;
      //      link->data = LinkMan()->GetData(ID());
   }
   return S_OK;
}


BOOL cPropertyLinkQuery::Eligible()
{
   if (Pattern != TheLink) return FALSE;
   return VerifyLink(ID());
}

STDMETHODIMP cPropertyLinkQuery::Next() 
{
   while(Prop->IterNextValue(&Iter,&TheLink.source,&TheLink.dest))
      if (Eligible())
         return S_OK;
   TheLink.source = OBJ_NULL;   
   return S_FALSE;
}


////////////////////////////////////////////////////////////
// Property Link Query Database
//

class cPropertyLinkQDB : public cBaseLinkQueryDatabase
{
   RelationID Rel;
   IIntProperty* Prop;
   IUnknown* QI;

public:
   cPropertyLinkQDB(IUnknown* pqi, IIntProperty* prop, RelationID rel = RELID_NULL)
      : Rel(rel),Prop(prop),QI(pqi)
   {
      Prop->AddRef();
      pqi->AddRef();
   }

   ~cPropertyLinkQDB()
   {
      SafeRelease(Prop);
      SafeRelease(QI);
   }

   STDMETHOD(AddRelation)(RelationID id );
   STDMETHOD(QueryInterface)(REFIID id, void** ppI) ;
   STDMETHOD(Add)(LinkID , sLink* ) { return S_OK;};
   STDMETHOD(Remove)(LinkID , sLink* ) { return S_OK; };
   STDMETHOD(Reset)(RelationID ) { return S_OK; };

   STDMETHOD_(ILinkQuery*,QueryComplex)(ObjID source, ObjID dest, RelationID rel, tQueryDate bday);
};

STDMETHODIMP cPropertyLinkQDB::AddRelation(RelationID id)
{
   if (Rel == RELID_NULL)
   {
      Rel = id;
      return S_OK;
   }
   return (Rel == id) ? S_FALSE : E_FAIL;
}


STDMETHODIMP cPropertyLinkQDB::QueryInterface(REFIID id, void** ppI)
{
   if (id == IID_ILinkQueryDatabase)
   {
      *ppI = this;
      AddRef();
      return S_OK;
   }

   return QI->QueryInterface(id,ppI);
}

STDMETHODIMP_(ILinkQuery*) cPropertyLinkQDB::QueryComplex(ObjID source, ObjID dest, RelationID rel, tQueryDate date)
{
   if (!RELID_MATCH(rel,Rel)) return CreateEmptyLinkQuery();
   if (source == LINKOBJ_WILDCARD)
   {
      sLinkTemplate pattern(source,dest,Rel);
      return new cPropertyLinkQuery(Prop,pattern,date);
   }
   else 
   {
      ObjID lnkdest;
      if (Prop->Get(source,&lnkdest) && LINKOBJ_MATCH(lnkdest,dest))
         return new cSingleLinkQuery(PROPLINKID_MAKE(Rel,source),date);
      else
         return CreateEmptyLinkQuery();
   }
}


////////////////////////////////////////////////////////////
// Property link store.
//

cPropertyLinkStore::cPropertyLinkStore(IUnknown* prop)
   : Prop(NULL),Rel(RELID_NULL)
{
   Verify(SUCCEEDED(COMQueryInterface(prop,IID_IIntProperty,(void**)&Prop)));
}


cPropertyLinkStore::~cPropertyLinkStore()
{
   SafeRelease(Prop);
}

STDMETHODIMP cPropertyLinkStore::QueryInterface(REFIID id, void** ppI)
{  
   if (id == IID_ILinkQueryDatabase) 
   {
      *ppI =  new cPropertyLinkQDB(this,Prop,Rel);
      return S_OK;
   }
   return cBaseLinkStore::QueryInterface(id,ppI);
}

STDMETHODIMP cPropertyLinkStore::AddRelation(RelationID id)
{
   if (Rel == RELID_NULL)
   {
      Rel = id;
      return S_OK;
   }
   return E_FAIL;
}

STDMETHODIMP_(LinkID)  cPropertyLinkStore::Add(sLink* link)
{
   if (link->flavor != Rel) 
      return LINKID_NULL;
   Prop->Set(link->source,link->dest);
   return PROPLINKID_MAKE(Rel,link->source);
}

STDMETHODIMP cPropertyLinkStore::AddAtID(LinkID id, sLink* link)
{
   if (id != PROPLINKID_MAKE(Rel,link->source))
      return E_FAIL;
   Add(link);
   return S_OK;
}

STDMETHODIMP cPropertyLinkStore::Remove(LinkID id)
{
   if (LINKID_RELATION(id) != Rel)
      return E_FAIL;
   Prop->Delete(LINKID_IDX(id));
   return S_OK;
}

STDMETHODIMP_(BOOL) cPropertyLinkStore::Get(LinkID id, sLink* link)
{
   int dest;
   if (LINKID_RELATION(id) != Rel)
      return FALSE;
   if (!Prop->Get(LINKID_IDX(id),&dest))
      return FALSE;

   if (link != NULL)
   {
      link->source = LINKID_IDX(id);
      link->dest = dest;
      link->flavor = Rel;
   }
   return TRUE;
}

STDMETHODIMP cPropertyLinkStore::Save(RelationID , IUnknown* )
{
   return S_OK;
}

STDMETHODIMP cPropertyLinkStore::Load(RelationID , IUnknown* )
{
   return S_OK;
}

STDMETHODIMP cPropertyLinkStore::Reset(RelationID )
{
   return S_OK;
}

STDMETHODIMP_(ILinkQuery*) cPropertyLinkStore::GetAll(RelationID id)
{
   if (id != Rel) return CreateEmptyLinkQuery(); 
   sLinkTemplate pattern(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD,Rel);
   return new cPropertyLinkQuery(Prop,pattern);
}


////////////////////////////////////////

ILinkStore* CreatePropertyLinkStore(IUnknown* prop)
{
   return new cPropertyLinkStore(prop); 
}







