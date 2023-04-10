/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/traisrch.cpp,v 1.9 1998/10/05 17:28:33 mahk Exp $
#include <traisrch.h>
#include <dlisttem.h>
#include <osysbase.h>
#include <traitbas.h>

// Must be last header 
#include <dbmem.h>


#ifdef __MSVC
template LinkQueryList;
template LinkQueryList::cParent;
//template cSimpleStack<QueryPtr>;
template cDList<class cSimpleDListNode<class QueryPtr>,1>;
#endif 

////////////////////////////////////////////////////////////
//
// cDepthFirstDonorQuery
//
////////////////////////////////////////////////////////////

STDMETHODIMP_(BOOL) cDepthFirstObjectQuery::Done()
{
   return CurObj == OBJ_NULL;
}

STDMETHODIMP_(ObjID) cDepthFirstObjectQuery::Object()
{
   return CurObj;
}

STDMETHODIMP cDepthFirstObjectQuery::Next()
{
   if (!Expand())
   {
      if (!Contract())
      {
         CurObj = OBJ_NULL;
         return S_OK;
      }
   }
   CurObj = Queries.Top()->Object();
   return S_OK;
}


BOOL cDepthFirstObjectQuery::Expand()
{
   if (CurObj == OBJ_NULL)
      return FALSE;

   IObjectQuery* query = Successors(CurObj);
   Queries.Push(query);
   return !query->Done();
}

BOOL cDepthFirstObjectQuery::Contract()
{
   if (Queries.Top() == NULL) 
      return FALSE;

   while(Queries.Top()->Done())
   {
      Queries.Pop()->Release();
      if (Queries.Top() == NULL)
         return FALSE;
      Queries.Top()->Next();
   }
   return TRUE;
}

////////////////////////////////////////

BOOL cConcretenessFilterQuery::Filter(ObjID obj)
{
   switch(Which)
   {
      case kObjectConcrete:
         return OBJ_IS_CONCRETE(obj);

      case kObjectAbstract:
         return OBJ_IS_ABSTRACT(obj);
         
      case kObjectAll:
         return TRUE;

      default:
         Warning(("Unknown concreteness for query: %d\n",Which));
   }
   return FALSE;
}


