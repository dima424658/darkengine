/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkboth.cpp,v 1.8 1998/10/03 20:52:32 mahk Exp $
#include <linkboth.h>
#include <linktype.h>
#include <linkbase.h>

#include <lnkquer_.h>
#include <linkid.h>

#include <lststtem.h>
#include <hshpptem.h>

#include <allocapi.H>


// Must be last header
#include <dbmem.h>

#define AUTO_CREDIT() LGALLOC_AUTO_CREDIT()

////////////////////////////////////////////////////////////
//
// TEMPLATE: cBothObjTable, instantiation
//

#ifdef _MSC_VER
template cHashTable<sTwoObjKey,cLinkSet*,sTwoObjKey>;
template cHashIter<sTwoObjKey,cLinkSet*,sTwoObjKey>;
#endif

////////////////////////////////////////////////////////////
//
// CLASS: cBothKnownQueryDatabase
//

//------------------------------------------------------------
// Construction/Destruction
//

cBothKnownQueryDatabase::cBothKnownQueryDatabase(cBothObjTable* table)
   : Table(table),
     our_table(FALSE)
{
   if (Table == NULL)
   {
      Table = new cBothObjTable;
      our_table = TRUE;
   }
}

cBothKnownQueryDatabase::~cBothKnownQueryDatabase()
{
   AUTO_CREDIT(); 
   if (our_table)
   {
      delete Table;
   }
   else
   {
      cRelationSet::cIter iter;
      for (iter = Rels.Iter(); !iter.Done(); iter.Next())
      {
         Reset(iter.Value());
      }
   }
}

//------------------------------------------------------------
// ILinkQueryDatabase Methods
//

STDMETHODIMP cBothKnownQueryDatabase::AddRelation(RelationID id)
{
   AUTO_CREDIT(); 
   Rels.AddElem(id);
   return S_OK;
}

STDMETHODIMP cBothKnownQueryDatabase::Add(LinkID id, sLink* link)
{
   AUTO_CREDIT(); 
   sTwoObjKey key(link->source,link->dest);
   cLinkSet* set = Table->Search(key);

   if (set == NULL)
   {
      set = new cLinkSet;
      Table->Insert(key,set);
   }

   set->AddElem(id);

   return S_OK;
}

STDMETHODIMP cBothKnownQueryDatabase::Remove(LinkID id, sLink* link)
{
   AUTO_CREDIT(); 

   sTwoObjKey key(link->source,link->dest);
   cLinkSet* set = Table->Search(key);

   if (set == NULL)
      return S_FALSE;

   set->RemoveElem(id);

   return S_OK;
}

STDMETHODIMP cBothKnownQueryDatabase::Reset(RelationID id)
{
   AUTO_CREDIT(); 
   // simple case
   if (our_table && Rels.Size() <= 1)
   {
      Assert_(Rels.HasElem(id));
      Table->RemoveAll();
   }
   else  // Look for matching links and explicity remove them
   {
      cBothObjTable::cIter iter;
      for (iter = Table->Iter(); !iter.Done(); iter.Next())
      {
         cLinkSet* set = iter.Value();
         cLinkSet::cIter setiter = set->Iter();
         for (; !setiter.Done(); setiter.Next())
         {
            if (LINKID_RELATION(setiter.Value()) == id)
               set->Delete(setiter.Node());
         }
         if (set->Size() == 0)
         {
            delete set;
            Table->Delete(iter.Key());
         }
      }
   }

   return S_OK;
}


////////////////////////////////////////

STDMETHODIMP_(ILinkQuery*) cBothKnownQueryDatabase::QueryComplex(ObjID source, ObjID dest, RelationID rel, tQueryDate bday)
{
   AUTO_CREDIT(); 
   Assert_(source != LINKOBJ_WILDCARD && dest != LINKOBJ_WILDCARD);

   // Look up the link set
   sTwoObjKey key(source,dest);
   cLinkSet* set = Table->Search(key);

   // trivial case
   if (set == NULL)
      return CreateEmptyLinkQuery();

   // Simple case, we handle a single relation
   if (our_table  && (Rels.Size() <= 1 || rel == LINKOBJ_WILDCARD))
   {
      Assert_(rel == LINKOBJ_WILDCARD || Rels.HasElem(rel));
      return set->Query(bday);
   }
   else // Complex case, must filter
   {
      sLinkTemplate pattern(source,dest,rel);
      return set->PatternQuery(&pattern,bday);
   }
}

//
// Factory
//

ILinkQueryDatabase* CreateBothKnownLinkQueryDatabase(void)
{
   return new cBothKnownQueryDatabase;
}


