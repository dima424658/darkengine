/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/traisrch.h,v 1.7 2000/02/22 20:00:47 toml Exp $
#pragma once
#ifndef __TRAISRCH_H
#define __TRAISRCH_H
#include <appagg.h>
#include <objquer_.h>
#include <traitman.h>
#include <dlist.h>
#include <trait.h>
#include <donorq_.h>
#include <relation.h>
#include <dynarray.h>

#include <dbmem.h>

typedef cSimpleDList<IObjectQuery*> LinkQueryList;



template <class TYPE>
class cSimpleStack 
{
   typedef cDynArray<TYPE> cArray; 

protected:
   cArray mArray; 
   int top; 

   enum { kInitSize = 16 }; 

public:
   cSimpleStack() :mArray(kInitSize),top(0) { }; 

   void Push(const TYPE& q) 
   {
      int sz = mArray.Size(); 
      if (top >= sz)
         mArray.Grow(sz);  // double in size

      mArray[top++] = q; 
   }

   TYPE& Top()
   {
      if (top <= 0)
      {
         mArray[0] = TYPE(); 
         return mArray[0]; 
      }
      return mArray[top-1]; 
   }

   TYPE Pop()
   {
      if (top <= 0)
      {
         top = 0; 
         return TYPE(); 
      }
      return mArray[--top]; 
   }

   int Size()
   {
      return top; 
   }

   // Returns nth from top, zero is top  
   BOOL Nth(int n, TYPE* val)
   {
      if (n >= top)
         return FALSE; 
      *val = mArray[top-1 -n]; 
      return TRUE; 
   }
   
};

//
// Dumb pointer class to interface with template
// 

class QueryPtr : public cIPtr<IObjectQuery>
{
   typedef cIPtr<IObjectQuery> cParent;

public:
   QueryPtr(IObjectQuery* q = NULL) : cParent(q) {};
   QueryPtr(const QueryPtr& q) : cParent((IObjectQuery*)q) {};

   IObjectQuery* operator =(const QueryPtr& q) { return (IObjectQuery*) (pUnknown = q.pUnknown); }; 
};

class QueryStack : public cSimpleStack<QueryPtr>
{

};


////////////////////////////////////////////////////////////
// TRAIT SEARCH QUERY 
//
// This one searches the entire inheritance graph upwards, 
// in prioritized order.
//
// Takes the object to start the search from, and doesn't
// yield that object
//

class cDepthFirstObjectQuery : public cBaseObjectQuery
{
protected:
   QueryStack Queries; 
   ObjID CurObj; 

   // "close up" an object.  Second arg is the parent
   // third is whether it was "finished"

   BOOL Expand();  // search deeper
   BOOL Contract(); // unwind

   //
   // Generate the successors of a particular object
   //
   virtual IObjectQuery* Successors(ObjID obj) = 0; 

public:
   cDepthFirstObjectQuery(ObjID obj) : CurObj(obj) {};
   virtual ~cDepthFirstObjectQuery() 
   { 
      while(!!Queries.Top()) 
      {
         Queries.Pop()->Release();
      }
   };


   STDMETHOD_(BOOL,Done)();
   STDMETHOD_(ObjID,Object)();
   STDMETHOD(Next)();

};

////////////////////////////////////////////////////////////
// Transitive Prioritized Link Query
//

class cTransitiveLinkObjQuery : public cDepthFirstObjectQuery
{
   IRelation* Rel;
protected:
   IObjectQuery* Successors(ObjID obj)
   {
      ILinkQuery* q = Rel->Query(obj,LINKOBJ_WILDCARD);
      IObjectQuery* out = new cPriLinkDonorQuery(q);
      SafeRelease(q);
      return out;
   }

public:
   cTransitiveLinkObjQuery(ObjID obj, IRelation* rel)
      : cDepthFirstObjectQuery(obj),Rel(rel)
   { Rel->AddRef();};
   
   ~cTransitiveLinkObjQuery() { SafeRelease(Rel);};

};


////////////////////////////////////////////////////////////
// OBJECT QUERY FILTER
//
// Skips past things that are not intrinsic to the trait
//

class cFilterObjectQuery : public cBaseObjectQuery
{
   IObjectQuery* Inner;   

protected:
   // overload this to filter differently
   virtual BOOL Filter(ObjID ) { return TRUE;};

   // call this in your constructor
   void Skip()
   {
      for (; !Inner->Done(); Inner->Next())
         if (Filter(Inner->Object()))
            break;
   }

public:
   cFilterObjectQuery(IObjectQuery* inner)
      : Inner(inner)
   {
      Inner->AddRef();
   }

   virtual ~cFilterObjectQuery() 
   { 
      SafeRelease(Inner);
   };

   STDMETHOD_(BOOL,Done)() { return Inner->Done();};
   STDMETHOD_(ObjID,Object)() { return Inner->Object(); };
   STDMETHOD(Next)() 
   { 
      Inner->Next();
      Skip();
      return S_OK;
   }
};

////////////////////////////////////////////////////////////
// CONCRETENESS FILTER
//

class cConcretenessFilterQuery : public cFilterObjectQuery
{
   eObjConcreteness Which;
protected:
   BOOL Filter(ObjID ); 

public:
   cConcretenessFilterQuery(IObjectQuery* q, eObjConcreteness which)
      : cFilterObjectQuery(q), Which(which) { Skip();}
   ~cConcretenessFilterQuery() {};
   
};

#include <undbmem.h>

#endif // __TRAISRCH_H






