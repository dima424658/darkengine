/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/traitqdb.cpp,v 1.4 2000/02/24 23:41:10 mahk Exp $ 
#include <appagg.h>

#include <traitqdb.h>
#include <dlisttem.h>
#include <iobjsys.h>
#include <relation.h>
#include <linkman.h>
#include <lnkquer_.h>
#include <linkid.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//------------------------------------------------------------
// cMetaPropLinks implementation 
//

void cMetaPropLinks::AddLink(const sMetaPropLink& link)
{
   for (cIter iter = Iter(); !iter.Done(); iter.Next())
   {
      // We're bigger than some link 
      if (link.pri > iter.Value().pri)
      {
         InsertBefore(iter.Node(),link); 
         return; 
      }

      // we're already here 
      if (iter.Value().id == link.id)
         return ;
   }

   // We must be last 
   Append(link); 
}

////////////////////////////////////////

void cMetaPropLinks::RemoveLink(const sMetaPropLink& link)
{
   for (cIter iter = Iter(); !iter.Done(); iter.Next())
   {
      if (iter.Value().id == link.id) 
      {
         Delete(iter.Node()); 
         break; 
      }

      if (link.pri > iter.Value().pri)
         break; 
   }
}


//------------------------------------------------------------
// cMetaPropQDB  
//

cMetaPropQDB::cMetaPropQDB()
   : mpRel(NULL)
{
   
}

cMetaPropQDB::~cMetaPropQDB()
{
   for( int i = mObjArray.Bounds().min; i < mObjArray.Bounds().max; i++)
      delete mObjArray[i];
   SafeRelease(mpRel); 
}


STDMETHODIMP cMetaPropQDB::AddRelation(RelationID id)
{  
   if (mpRel)
      return E_FAIL; 

   AutoAppIPtr(LinkManager); 
   mpRel = pLinkManager->GetRelation(id); 

   return S_OK; 
}

STDMETHODIMP cMetaPropQDB::RemoveRelation(RelationID id)
{  
   if (!mpRel || mpRel->GetID() != id)
      return E_FAIL; 

   SafeRelease(mpRel); 
   return S_OK; 
}

STDMETHODIMP cMetaPropQDB::Reset(RelationID id)
{
   if (!mpRel || mpRel->GetID() != id)
      return E_FAIL; 

   // clear out the database 
   for (int i = mObjArray.Bounds().min; i < mObjArray.Bounds().max; i++)
   {
      delete mObjArray[i];
      mObjArray[i] = NULL;
   }

   return S_OK; 
}

STDMETHODIMP cMetaPropQDB::Add(LinkID id, sLink* link)
{
   Assert_(mpRel); 

   // find the link list 
   cMetaPropLinks* links = Get(link->source); 

   // create the link list if we have to 
   if (!links)
   {
      links = new cMetaPropLinks; 
      mObjArray[link->source] = links; 
   }

   sMetaPropLink mpl = { id, link->dest, 0}; 
   // we have to look up the priority. 
   mpl.pri = *(tMetaPropertyPriority*)mpRel->GetData(id); 

   links->AddLink(mpl); 

   return S_OK; 

}

STDMETHODIMP cMetaPropQDB::Remove(LinkID id, sLink* link)
{
   // find the link list 
   cMetaPropLinks* links = Get(link->source); 

   if (!links)
      return S_FALSE; 

   sMetaPropLink mpl = { id, link->dest, 0}; 
   // we have to look up the priority. 
   mpl.pri = *(tMetaPropertyPriority*)mpRel->GetData(id); 

   links->RemoveLink(mpl); 

   // if we emptied the list, destroy it.  
   if (links->GetFirst() == NULL)
   {
      delete links; 
      mObjArray[link->source] = NULL; 
   }

   return S_OK; 
   
}

//------------------------------------------------------------
// LINK QUERY IMPLEMENTATION 
//
// Could be a little faster due to the template comparisons, but I
// expect anyone who cares will through the back door anyway. 
//

class cMetaPropQuery: public cBaseLinkQuery
{
protected:
   sLinkTemplate mPattern; 
   cMetaPropLinks::cIter mIter; 
   sLinkTemplate mLink; 
   
   void SetLink()
   {
      sMetaPropLink& mpl = mIter.Value(); 
      mLink.source = mPattern.source; 
      mLink.dest = mpl.dest; 
      mLink.flavor = LINKID_RELATION(mpl.id); 
   }

   void Skip()
   {
      while (mPattern != mLink)
      {
         mIter.Next(); 
         if (mIter.Done())
            break; 
         SetLink(); 
      }
   }

public: 
   cMetaPropQuery(sLink& pattern, cMetaPropLinks* links,
                      tQueryDate bday = QUERY_DATE_NONE)
      : mIter(links->Iter()),
        mPattern(pattern),
        cBaseLinkQuery(bday)
   {
      Assert_(mPattern.source != LINKOBJ_WILDCARD); 
      if (!mIter.Done())
      {
         SetLink(); 
         Skip(); 
      }
   }
        
   STDMETHOD_(BOOL,Done)() const 
   {
      return mIter.Done(); 
   }

   STDMETHOD(Next)()
   {
      if (mIter.Done())
         return S_FALSE; 
      
      mIter.Next(); 
      
      if (!mIter.Done())
      {
         SetLink(); 
         Skip(); 
         return S_OK; 
      }
      return S_FALSE; 
   }

   STDMETHOD_(LinkID,ID)() const 
   {
      Assert_(!mIter.Done()); 
      return mIter.Value().id; 
   }

   STDMETHOD(Link)(sLink* link) const 
   {
      Assert_(!mIter.Done()); 
      *link = mLink; 
      return S_OK; 
   }

   STDMETHOD_(void*,Data)() const 
   {
      // A copy, not the real data
      Assert_(!mIter.Done()); 
      return &mIter.Value().pri;
   }
   
};


//------------------------------------------------------------
// QDB Query functions 
//

STDMETHODIMP_(ILinkQuery*) cMetaPropQDB::QueryComplex(ObjID source, ObjID dest, RelationID rel, tQueryDate bday)
{
   Assert_(source != LINKOBJ_WILDCARD); // I must have the source!

   if (rel != RELID_WILDCARD && rel != mpRel->GetID())
      return CreateEmptyLinkQuery(); 

   cMetaPropLinks* links = Get(source); 
   if (!links)
      return CreateEmptyLinkQuery(); 

   sLinkTemplate pattern(source,dest,rel); 
   return new cMetaPropQuery(pattern,links,bday); 
}

STDMETHODIMP_(ILinkQuery*) cMetaPropQDB::Query(ObjID source, ObjID dest, RelationID rel)
{
   return cMetaPropQDB::QueryComplex(source,dest,rel,QUERY_DATE_NONE); 
}


