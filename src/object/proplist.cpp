/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/proplist.cpp,v 1.9 1997/12/19 15:28:35 TOML Exp $
#include <proplist.h>
#include <dlisttem.h>

// must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
//
// cPropertyImplLlist
//
////////////////////////////////////////////////////////////

cPropertyImplLlist::cPropertyImplLlist (int elem_size) :
   cPropertyImpl (elem_size)
{
}

BOOL cPropertyImplLlist::Get (ObjID obj, PropertyID, void **pptr) const
{
   for (cListNode* node = List.GetFirst(); node != NULL; node = node->GetNext())
   {
      if (node->obj == obj)
      {
         *pptr = node->data;
         return TRUE;
      }
   }
   return FALSE;
}

BOOL cPropertyImplLlist::Set (ObjID obj, PropertyID, void *ptr)
{
   for (cListNode* node = List.GetFirst(); node != NULL; node = node->GetNext())
   {
      if (node->obj == obj)
      {
         // Change in place
         Copy(obj,node->data,ptr);

         return FALSE;
      }
   }

   // Add new node
   node = new cListNode(obj,elem_size_);
   List.Prepend(node);
   Init(obj,node->data);
   Copy(obj,node->data,ptr);
   return TRUE;
}


void* cPropertyImplLlist::Create (ObjID obj, PropertyID)
{
   cListNode* pnode = new cListNode(obj,elem_size_);
   List.Prepend(pnode);

   Init(obj,pnode->data);

   return pnode->data;
}



BOOL cPropertyImplLlist::Delete(ObjID obj, PropertyID)
{
   for (cListNode* pnode = List.GetFirst(); pnode != NULL; pnode = pnode->GetNext())
   {
      if (pnode->obj == obj)
      {
         Term(obj,pnode->data);
         List.Remove(pnode);
         delete pnode;
         return TRUE;
      }
   }

   // It never had the property anyway
   return FALSE;
}

BOOL cPropertyImplLlist::Relevant (ObjID obj, PropertyID ) const
{
   for (cListNode* pnode = List.GetFirst(); pnode != NULL; pnode = pnode->GetNext())
   {
      if (pnode->obj == obj) 
      {
         return TRUE;
      }
   }
   return FALSE;
}

void cPropertyImplLlist::Reset(PropertyID)
{
   List.DestroyAll();
   List.SetEmpty();
}



////////////////////////////////////////

void cPropertyImplLlist::IterStart(PropertyID , sPropertyObjIter* iter) const
{
   cListNode* first = List.GetFirst();
   iter->state[0] = first;
   iter->state[1] = (first != NULL) ? first->GetNext() : NULL;
}

////////////////////////////////////////

void cPropertyImplLlist::IterStop(PropertyID , sPropertyObjIter* ) const
{
}


////////////////////////////////////////

BOOL cPropertyImplLlist::IterNext(PropertyID , sPropertyObjIter* iter, ObjID* next, void** value) const
{
   cListNode* node = (cListNode*)iter->state[0];
   if (node == NULL) return FALSE;
   *next = node->obj;
   *value = node->data;
   iter->state[0] = iter->state[1];
   if (iter->state[0] != NULL)
   {
      node = (cListNode*)iter->state[0];
      iter->state[1] = node->GetNext();
   }
   return TRUE;
}

