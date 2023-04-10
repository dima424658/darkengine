/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkarry.cpp,v 1.7 1998/10/03 20:52:27 mahk Exp $
#include <linktype.h>
#include <relation.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <linkarry.h>
#include <lnkquer_.h>
#include <osysbase.h> // is this the only place that has obj_null?

#include <allocapi.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// LinkArray functions
//

#define AUTO_BLAME()   LGALLOC_AUTO_CREDIT()


#define LINK_IN_USE(elem) ((elem).source)
#define LINK_NEXT_FREE(elem) ((elem).dest)



BOOL LinkArray::InUse(LinkID id)
{
   int part = LINKID_PARTITION(id);
   int subidx = LINKID_IDX(id);
   if (subidx >= links[part].Size()) 
      return FALSE;
   return LINK_IN_USE(links[part][subidx]); 
}



LinkID LinkArray::Add(ObjID source, ObjID dest)
{
   AUTO_BLAME(); 
   int part = SuggestedLinkPartition(source,dest); 

   if (FreeHead[part] == FREELIST_END)
   {
      return LINKID_MAKE(Rel,part,links[part].Append(LinkEnds(source,dest)));
   }
   else
   {
      uint idx = FreeHead[part];
      FreeHead[part] = LINK_NEXT_FREE(links[part][idx]);
      links[part][idx] = LinkEnds(source,dest);
      return LINKID_MAKE(Rel,part,idx); 
   }
} 

void LinkArray::AddAtID(LinkID id, ObjID source, ObjID dest)
{
   AUTO_BLAME(); 

   int part = LINKID_PARTITION(id); 
   int idx = LINKID_IDX(id); 

   // Grow the array, adding to the free list as you go.  
   if (idx >= links[part].Size())
   {
      int s = links[part].Size();
      links[part].SetSize(idx+1);
      // put lower indices at the head of the free list, in the hopes that 
      // our id space will become compact again.

      for (int i = idx - 1; i >= s; i--)
      {
         LINK_NEXT_FREE(links[part][i]) = FreeHead[part];
         LINK_IN_USE(links[part][i]) = OBJ_NULL;
         FreeHead[part] = i;
      }
   }
   else
   {
      // Fix up the free list
      if (idx == FreeHead[part])
      {
         FreeHead[part] = LINK_NEXT_FREE(links[part][FreeHead[part]]);
      }
      else  
      {
         // Grovel through the free list looking for idx
         if (LINK_IN_USE(links[part][idx]) == OBJ_NULL)
            for (int i = FreeHead[part]; i != FREELIST_END; i = LINK_NEXT_FREE(links[part][i]))
               if (LINK_NEXT_FREE(links[part][i]) == idx)
               {
                  LINK_NEXT_FREE(links[part][i]) = LINK_NEXT_FREE(links[part][idx]);
                  break;
    
               }
      }
   }

   // now set the link data
   links[part][idx] = LinkEnds(source,dest);
} 

void LinkArray::Remove(LinkID link)
{
   AUTO_BLAME(); 

   int part = LINKID_PARTITION(link); 
   uint idx = LINKID_IDX(link);
   LINK_NEXT_FREE(links[part][idx]) = FreeHead[part];
   LINK_IN_USE(links[part][idx]) = OBJ_NULL;
   FreeHead[part] = idx;
}

BOOL LinkArray::SetRelation(RelationID id)
{
   if (Rel != RELID_NULL) 
      return FALSE; 
   Rel = id;
   return TRUE;
}

void LinkArray::Clear()
{
   AUTO_BLAME(); 
   for (int i = 0; i < kNumLinkPartitions; i++)
   {
      links[i].SetSize(0);
      FreeHead[i] = FREELIST_END; 
   }
}

////////////////////////////////////////////////////////////
// LinkArray::Iterator
//


LinkArray::Iterator::Iterator(LinkArray& v)
  : vec(v), part(0), subidx(0)
{ 
   Skip (); 
}; 

void LinkArray::Iterator::Skip()
{
   do
   {
      cLinkVec& partvec = vec.links[part]; 

      while (subidx < partvec.Size()) 
      {
         if (LINK_IN_USE(partvec[subidx]))
            return ; 
         subidx ++; 
      }
      
      part++;
      subidx = 0; 
   }   
   while (part < kNumLinkPartitions);
      
}

BOOL LinkArray::Iterator::Done() const
{ 
   return part >= kNumLinkPartitions; 
}; 

void LinkArray::Iterator::Next()
{
   if (!Done())
   {
      subidx ++;  
      Skip(); 
   }
}

LinkID LinkArray::Iterator::ID() const 
{
   return LINKID_MAKE(vec.Relation(),part,subidx); 
}

void LinkArray::Iterator::Link(sLink* link) const
{
   LinkEnds& ends = vec[ID()]; 
   link->source = ends.source;
   link->dest = ends.dest;
   link->flavor = vec.Relation(); 
}

////////////////////////////////////////////////////////////
// cLinkArrayQuery
//

class cLinkArrayQuery : public cBaseLinkQuery
{
   sLinkTemplate Pattern;
   LinkArray& Vec;
   LinkArray::Iterator iter; 
   IRelation* Relation;

   BOOL LinkValid(LinkID id) 
   { 
      LinkEnds& ends = Vec[id]; 
      return LINKOBJ_MATCH(Vec[id].source,Pattern.source)
         && LINKOBJ_MATCH(Vec[id].dest,Pattern.dest)
         && VerifyLink(id); 
   } ;
   void Skip(void) { while(!iter.Done() && !LinkValid(iter.ID())) iter.Next(); }; 
   
public:
   cLinkArrayQuery(LinkArray* v, const sLinkTemplate& pattern, tQueryDate BirthDate = QUERY_DATE_NONE)
      : Vec(*v),
        cBaseLinkQuery(BirthDate),
        Pattern(pattern),
        Relation(LinkMan()->GetRelation(v->Relation())),
        iter(v->Iter())
   {
      Skip();
   }

   virtual ~cLinkArrayQuery()
   {
      SafeRelease(Relation);
   }

   STDMETHOD_(BOOL,Done)() const { return iter.Done(); };
 
   STDMETHOD(Link)(sLink* link) const 
   {
      if (iter.Done()) return E_FAIL; 
      iter.Link(link); 
      return S_OK; 
   }; 

   STDMETHOD(Next)() { iter.Next(); Skip(); return S_OK; }; 

   STDMETHOD_(LinkID,ID)() const { return iter.ID(); }; 
};

////////////////////////////////////////////////////////////
// cLinkArrayQueryDatabase
// 

IMPLEMENT_DELEGATION(cLinkArrayQueryDatabase);

STDMETHODIMP_(ILinkQuery*) cLinkArrayQueryDatabase::QueryComplex(ObjID source, ObjID dest, RelationID rel, tQueryDate birthday)
{
   if (RELID_MATCH(rel,Vec->Relation()))
   {
      sLinkTemplate pattern(source,dest,rel);
      return new cLinkArrayQuery(Vec,pattern,birthday);
   }
   else
      return CreateEmptyLinkQuery();
}


////////////////////////////////////////////////////////////
// cLinkArrayLinkStore
//

IMPLEMENT_DELEGATION(cLinkArrayLinkStore);

STDMETHODIMP cLinkArrayLinkStore::AddRelation(RelationID id)
{
   return Vec.SetRelation(id) ? S_OK : E_FAIL;
}

STDMETHODIMP cLinkArrayLinkStore::RemoveRelation(RelationID id)
{
   if (id == Vec.Relation())
      Vec.SetRelation(RELID_NULL); 
   return S_OK; 
}


STDMETHODIMP_(LinkID) cLinkArrayLinkStore::Add(sLink* link)
{

   Assert_(link->flavor == Vec.Relation());
   return Vec.Add(link->source,link->dest);
}

STDMETHODIMP cLinkArrayLinkStore::AddAtID(LinkID id, sLink* link)
{
   Assert_(link->flavor == Vec.Relation() && link->flavor == LINKID_RELATION(id));
   Vec.AddAtID(id,link->source,link->dest);
   return S_OK;
}

STDMETHODIMP cLinkArrayLinkStore::Remove(LinkID id)
{
   Assert_(LINKID_RELATION(id) == Vec.Relation());

   Vec.Remove(id);
   return S_OK;
}

STDMETHODIMP_(BOOL) cLinkArrayLinkStore::Get(LinkID id, sLink* out)
{
   Assert_(LINKID_RELATION(id) == Vec.Relation());

   if (!Vec.InUse(id))
      return FALSE; 

   if (out != NULL)
   {
      LinkEnds ends = Vec[id];

      out->source = ends.source;
      out->dest = ends.dest;
      out->flavor = Vec.Relation();
   }
   return TRUE;
}

ILinkQuery* cLinkArrayLinkStore::GetAll(RelationID id) 
{  
   Assert_(id == Vec.Relation());

   sLinkTemplate pattern(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD,Vec.Relation());
   return new cLinkArrayQuery(&Vec,pattern);
}

STDMETHODIMP cLinkArrayLinkStore::Reset(RelationID id)
{
   Assert_(id == Vec.Relation());
   Vec.Clear();
   return S_OK;
}

////////////////////////////////////////////////////////////
// cLinkArrayDelegate and DelegateBase
// 

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cLinkArrayDelegateBase,IUnknown);

STDMETHODIMP cLinkArrayDelegate::QueryInterface(REFIID id, void** ppI)
{
   if (id == IID_ILinkQueryDatabase)
   {
      if (qdb == NULL) qdb = new cLinkArrayQueryDatabase(this,&Vec);
      qdb->AddRef();
      *ppI = qdb;
      return S_OK;
   } 
   else if (id == IID_ILinkStore)
   {
      if (store == NULL) store = new cLinkArrayLinkStore(this,&Vec);
      store->AddRef();
      *ppI = store;
      return S_OK;
   }
   else return cLinkArrayDelegateBase::QueryInterface(id,ppI);
}




