/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/lnkbyobj.cpp,v 1.14 1998/10/15 00:28:21 mahk Exp $
#include <lnkbyobj.h>
#include <linkman.h>
#include <lnkquery.h>
#include <hshpptem.h>
#include <lnkquer_.h>
#include <linkqdb_.h>
#include <linkid.h>

#include <allocapi.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
//
// LinksByObj
//
////////////////////////////////////////////////////////////
#ifdef __MSVC 
template LinksByObjTable;
#endif 

#define AUTO_BLAME() LGALLOC_AUTO_CREDIT()

void LinksByObj::AddLink(ObjID src, LinkID id)
{
   AUTO_BLAME(); 
   LinkSet* set;

   if (!Lookup(src,&set))
   {
      set = new LinkSet;
      Set(src,set);
   }

   // check empty list case
   if (set->GetFirst() == NULL)
   {
      set->Append(id);
   }
   else
   {
      LinkID lastid = set->GetLast()->Value();  
      int cmp = CompareLinks(id,lastid);
      if (cmp > 0 || (cmp == 0 && id > lastid)) // check "I am biggest" case
            set->Append(id);
      else
      {
         LinkSet::cIter iter;

         // grovel through looking for the link
         for (iter = set->Iter(); !iter.Done(); iter.Next())
         {
            LinkSet::cNode& node = iter.Node(); 
            LinkID nodeid = node.Value(); 
            if (id == nodeid) // already there
               break;

            int cmp = CompareLinks(id,nodeid); 
            if (cmp < 0 || (cmp == 0 && id < nodeid))
            {
               set->InsertBefore(node,id);
               break;
            }
         }

      }
   }

}


void LinksByObj::RemoveLink(ObjID obj, LinkID id)
{
   AUTO_BLAME(); 
   LinkSet* set;
   if (!Lookup(obj,&set)) // no links here!
      return;

   LinkSet::cIter iter;
   for (iter = set->Iter(); !iter.Done(); iter.Next())
   {
      LinkID lnk = iter.Value();
      if (lnk == id)
      {
         set->Delete(iter.Node());
         break;
      }
      if (CompareLinks(id,lnk) < 0)
         break;
   }
   if (set->GetFirst() == NULL)
   {
      delete set;
      Delete(obj);
   }
}

void LinksByObj::RemoveRelation(RelationID id)
{
   AUTO_BLAME(); 
   for (cIter iter = Iter(); !iter.Done(); iter.Next())
   {
      ObjID obj = iter.Key();
      LinkSet* set = iter.Value();
      for (LinkSet::cIter iter = set->Iter(); !iter.Done(); iter.Next())
      {
         if (id == LINKID_RELATION(iter.Value()))
         {
            set->Delete(iter.Node());
         }
      }
      if (set->GetFirst() == NULL)
      {
         delete set;
         Delete(obj);
      }
   }
}



////////////////////////////////////////////////////////////
// 
// INTERNAL CLASS: cLinksByObjQuery
//
////////////////////////////////////////////////////////////

class cLinksByObjQuery : public cBaseLinkQuery
{
   sLinkTemplate Pattern;
   ILinkQuery* Set;
   LinksByObj::cIter Iter;
   BOOL Forward;

public:

   cLinksByObjQuery(const LinksByObj& table, 
                    const sLinkTemplate& pattern, 
                    tQueryDate bday = QUERY_DATE_NONE,
                    BOOL forward = TRUE) 
      : Iter(table.Iter()), 
        Pattern(pattern),
        Set(NULL),
        cBaseLinkQuery(bday),
        Forward(forward)
   { Next();} ;

   virtual ~cLinksByObjQuery() { SafeRelease(Set);};

   STDMETHOD_(BOOL,Done)() const;
   STDMETHOD(Link)(sLink* link) const;
   STDMETHOD(Next)();
   STDMETHOD_(LinkID,ID)() const { return Set->ID();};
};


STDMETHODIMP_(BOOL) cLinksByObjQuery::Done() const
{
   return Set == NULL || Set->Done();
}

STDMETHODIMP cLinksByObjQuery::Link(sLink* link) const
{
   return Set->Link(link);
}

STDMETHODIMP cLinksByObjQuery::Next()
{
   if (Set == NULL || Set->Done())
   {
      if (Iter.Done()) return E_FAIL;

      for (Iter.Next(); !Iter.Done(); Iter.Next())
      {
         SafeRelease(Set);
         sLinkTemplate pattern = Pattern;
         pattern.source = Iter.Key(); 
         Set = Iter.Value()->PatternQuery(&pattern,Birthday);
         if (!Set->Done())
            break;
      }
   }
   else
      Set->Next(); 

   return S_OK;
}


////////////////////////////////////////////////////////////
// 
// cFromObjQueryDatabase
//
////////////////////////////////////////////////////////////

cFromObjQueryDatabase::cFromObjQueryDatabase(LinksByObj* table,BOOL sponsor)
   :Table(table ? *table : *new LinksByObj),
    our_table(sponsor || table == NULL)
{
   
}

cFromObjQueryDatabase::~cFromObjQueryDatabase() 
{
   if (our_table)
      delete &Table;
}


STDMETHODIMP cFromObjQueryDatabase::Add(LinkID id, sLink* link)
{
   AUTO_BLAME(); 
   Table.AddLink(link->source,id);
   return S_OK;
   
}


STDMETHODIMP cFromObjQueryDatabase::Remove(LinkID id, sLink* link)
{
   AUTO_BLAME(); 

   Table.RemoveLink(link->source,id);
   return S_OK;
}

STDMETHODIMP cFromObjQueryDatabase::Reset(RelationID id)
{
   AUTO_BLAME(); 
   Table.RemoveRelation(id);
   return S_OK;
}

STDMETHODIMP_(ILinkQuery*) cFromObjQueryDatabase::QueryComplex(ObjID source, ObjID dest, RelationID rel, tQueryDate bday)
{
   AUTO_BLAME(); 
   sLinkTemplate pattern(source,dest,rel);

   if (source == LINKOBJ_WILDCARD)
      return new cLinksByObjQuery(Table,pattern,bday);

   LinkSet* set;
   if (Table.Lookup(source,&set))
      return set->PatternQuery(&pattern,bday);
   else return CreateEmptyLinkQuery();
}



////////////////////////////////////////////////////////////
// 
// cToObjQueryDatabase
//
////////////////////////////////////////////////////////////

cToObjQueryDatabase::cToObjQueryDatabase(LinksByObj* table,BOOL sponsor)
   :Table(table ? *table : *new LinksByObj),
    our_table(sponsor||table == NULL)
{
   
}

cToObjQueryDatabase::~cToObjQueryDatabase() 
{
   AUTO_BLAME(); 
   if (our_table)
      delete &Table;
}

STDMETHODIMP cToObjQueryDatabase::Add(LinkID id, sLink* link)
{
   AUTO_BLAME(); 
   Table.AddLink(link->dest,LINKID_INVERT(id));
   return S_OK;
   
}


STDMETHODIMP cToObjQueryDatabase::Remove(LinkID id, sLink* link)
{
   AUTO_BLAME(); 
   Table.RemoveLink(link->dest,LINKID_INVERT(id));
   return S_OK;
}

STDMETHODIMP cToObjQueryDatabase::Reset(RelationID id)
{
   AUTO_BLAME(); 
   Table.RemoveRelation(RELID_INVERT(id));
   return S_OK;
}

STDMETHODIMP_(ILinkQuery*) cToObjQueryDatabase::QueryComplex(ObjID source, ObjID dest, RelationID rel, tQueryDate bday)
{
   AUTO_BLAME(); 
   sLinkTemplate pattern(dest,source,RELID_INVERT(rel));
   ILinkQuery* backwards = NULL;
   ILinkQuery* forwards;
   LinkSet* set;

   if (dest == LINKOBJ_WILDCARD)
      backwards = new cLinksByObjQuery(Table,pattern,bday,FALSE);
   else if (Table.Lookup(dest,&set))
      backwards =  set->PatternQuery(&pattern,bday);
   else return CreateEmptyLinkQuery();

   forwards = backwards->Inverse();
   SafeRelease(backwards);

   return forwards;
}



