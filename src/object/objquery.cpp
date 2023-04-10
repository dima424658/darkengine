/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/objquery.cpp,v 1.5 1997/10/14 11:20:22 TOML Exp $
#include <objquer_.h>
#include <osysbase.h>
#include <lnkquery.h> 
#include <linkbase.h>

// Must be last header
#include <dbmem.h>

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cBaseObjectQuery, IObjectQuery);

#ifndef SHIP
int cBaseObjectQuery::num_outstanding = 0;
#endif 

////////////////////////////////////////////////////////////
// 
// NULL OBJECT QUERY
//
////////////////////////////////////////////////////////////

class cNullObjectQuery : public cBaseObjectQuery 
{
public:
   STDMETHOD_(BOOL,Done)() { return TRUE; }; 
   STDMETHOD_(ObjID,Object)() { return OBJ_NULL; } ; 
   STDMETHOD(Next)() { return E_FAIL; } ; 
};

IObjectQuery* CreateEmptyObjectQuery(void)
{
   return new cNullObjectQuery; 
}

////////////////////////////////////////////////////////////
//
// SINGLE OBJECT QUERY 
//
////////////////////////////////////////////////////////////

class cSingleObjectQuery : public cBaseObjectQuery
{
   ObjID TheObj;

public:
   cSingleObjectQuery(ObjID obj) : TheObj(obj) {};
   virtual ~cSingleObjectQuery() {}; 

   STDMETHOD_(BOOL,Done)() { return TheObj == OBJ_NULL;}; 
   STDMETHOD_(ObjID,Object)() { return TheObj;}; 
   STDMETHOD(Next)() { TheObj = OBJ_NULL; return S_OK;}; 


};

IObjectQuery* CreateSingleObjectQuery(ObjID obj)
{
   return new cSingleObjectQuery(obj);
}

////////////////////////////////////////////////////////////
//
// OBJECT QUERY CONCATENATION
//
////////////////////////////////////////////////////////////

class cConcatObjQuery : public cBaseObjectQuery
{
   IObjectQuery** Queries;
   int Size;
   int Idx;

   void Skip()
   {
      while (Idx < Size && Queries[Idx]->Done())
         Idx++;
   }

public:
   cConcatObjQuery(IObjectQuery** qs, int n)
      :Queries(new IObjectQuery*[n]),
       Idx(0),
       Size(n)
   {
      for (int i = 0; i < n; i++)
      {
         Queries[i] = qs[i];
         qs[i]->AddRef();
      }
      Skip();
   }


   ~cConcatObjQuery()
   {
      for (int i = 0; i < Size; i++)
      {
         SafeRelease(Queries[i]);
      }
      delete Queries;
   }

   STDMETHOD_(BOOL,Done)() 
   { 
      return Idx >= Size;
   } 
   
   STDMETHOD_(ObjID,Object)() 
   { 
      return (Idx < Size) ?  Queries[Idx]->Object() : OBJ_NULL;
   }; 

   STDMETHOD(Next)() 
   {
      if (Idx < Size)
      {
         Queries[Idx]->Next();
         Skip();
      }
      return S_OK;
   }
};


IObjectQuery* ConcatenateObjectQueries(IObjectQuery** Qvec, int n)
{
   return new cConcatObjQuery(Qvec,n);
}


////////////////////////////////////////////////////////////
//
// OBJECT QUERY WRAPPER AROUND LINK QUERY 
//
////////////////////////////////////////////////////////////

class cLinkObjectQuery : public cBaseObjectQuery 
{
   ILinkQuery* LinkQuery;
   
public:
   cLinkObjectQuery(ILinkQuery* q)
      :LinkQuery(q)
   {
      LinkQuery->AddRef();
   }

   ~cLinkObjectQuery()
   {
      SafeRelease(LinkQuery);
   }

   STDMETHOD_(BOOL,Done)() { return LinkQuery->Done(); }; 
   STDMETHOD_(ObjID,Object)() 
   {  
      sLink link; 
      if (SUCCEEDED(LinkQuery->Link(&link)))
         return link.dest;
      else
         return OBJ_NULL;
   } ; 
   STDMETHOD(Next)() { return LinkQuery->Next(); } ; 
};

IObjectQuery* CreateLinkDestObjectQuery(ILinkQuery* lq)
{
   return new cLinkObjectQuery(lq);
}
