/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/proplist.h,v 1.6 1998/03/26 13:52:47 mahk Exp $
#pragma once  
#ifndef __PROPLIST_H
#define __PROPLIST_H

#include <propsto_.h>
#include <dataops_.h>
#include <dlistsim.h>



////////////////////////////////////////////////////////////
// LINKED LIST PROPERTY STORE
//

//
// Helper types
//

struct sObjDatumPair
{
   ObjID obj;
   sDatum val; 

   sObjDatumPair(ObjID o, sDatum v) : obj(o),val(v) {}; 
}; 

class cObjDatumPairList : public cSimpleDList<sObjDatumPair>
{
public:
   cNode* Search(ObjID obj) const 
   {
      for (cIter iter = Iter(); !iter.Done(); iter.Next())
      {
         if (iter.Value().obj == obj)
            return &iter.Node(); 
      }
      return NULL; 
   }

}; 

//
// cListPropertyStore
//


template <class OPS>
class cListPropertyStore : public cPropertyStore<OPS> 
{

public:
   
   typedef struct sObjDatumPair sPair; 
   typedef cObjDatumPairList cList; 

   //
   // FAST ACCESSORS
   //

   cList& GetList() { return mList; }; 

   //
   // METHODS
   //

   STDMETHOD_(const sPropertyStoreDesc*,Describe)() const 
   {
      static sPropertyStoreDesc desc = { "Linked List" }; 
      return &desc; 
   }


   STDMETHOD_(sDatum,Create)(ObjID obj)
   {
      sDatum dat = mOps.New(); 
      sPair pair (obj, dat);  

      mList.Prepend(pair); 
      return dat; 
   }


   STDMETHOD(Delete)(ObjID obj)
   {
      cList::cNode* pair = mList.Search(obj);
      if (pair)
      {
         mOps.Delete(pair->Value().val); 
         mList.Delete(*pair); 
         return S_OK; 
      }
      return S_FALSE; 
   }

   STDMETHOD_(BOOL,Relevant)(ObjID obj) const 
   {
      return mList.Search(obj) != NULL; 
   }

   STDMETHOD_(BOOL,Get)(ObjID obj, sDatum* pdat) const 
   {
      cList::cNode* pair = mList.Search(obj); 
      if (pair)
      {
         *pdat = pair->Value().val; 
         return TRUE;
      }
      return FALSE; 
   }; 

   STDMETHOD(Set)(ObjID obj, sDatum val)
   {
      cList::cNode* node = mList.Search(obj);

      if (node)
      {
         mOps.Copy(&node->Value().val,val); 
         return S_FALSE;
      }
      else
      {
         sPair newpair (obj, mOps.CopyNew(val) ); 
         mList.Prepend(newpair); 
         return S_OK; 
      }
   }

   STDMETHOD_(sDatum, Copy)(ObjID targ, ObjID src)
   {
      cList::cNode* srcnode = mList.Search(src); 
      if (!srcnode)
         return NULL; 
      sPair* srcpair = &srcnode->Value();
      cList::cNode* targnode = mList.Search(targ);
      
      if (targnode)
      {
         mOps.Copy(&targnode->Value().val,srcpair->val); 
         return targnode->Value().val; 
      }
      else
      {
         sPair newpair (targ, mOps.CopyNew(srcpair->val) ); 
         mList.Prepend(newpair); 
         return newpair.val; 
      }
   }


   STDMETHOD(Reset)()
   {
      for (cList::cIter iter = mList.Iter(); !iter.Done(); iter.Next())
      {
         mOps.Delete(iter.Value().val);
         mList.Delete(iter.Node()); 
      }
      return S_OK; 
   }

   STDMETHOD(IterStart)(sPropertyObjIter* piter) const 
   {
      Assert_(sizeof(*piter) >= sizeof(cList::cIter)); 
      cList::cIter& iter = *(cList::cIter*)piter; 
      iter = mList.Iter(); 
      return S_OK; 
   }

   STDMETHOD_(BOOL,IterNext)(sPropertyObjIter* piter, 
                             ObjID* obj, sDatum* value) const 
   {
      cList::cIter& iter = *(cList::cIter*)piter; 
      if (iter.Done()) return FALSE;
      if (obj) 
         *obj = iter.Value().obj; 
      if (value)
         *value = iter.Value().val; 
      iter.Next(); 
      return TRUE; 
   }

   STDMETHOD(IterStop)(sPropertyObjIter* ) const 
   {
      return S_OK; 
   }

protected:
   cList mList; 

}; 

//------------------------------------------------------------
// Generic version
//


class cGenericListPropertyStore : public cListPropertyStore<cDelegatingDataOps>
{
public:
   cGenericListPropertyStore()
   {
      mOps.InitDelegation(this); 
   }

   STDMETHOD(SetOps)(IDataOps* ops)
   {  
      mOps.SetOps(ops); 
      return S_OK; 
   }

}; 



#endif // __PROPLIST_H






