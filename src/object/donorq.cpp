/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/donorq.cpp,v 1.4 1997/10/14 11:20:13 TOML Exp $
#include <donorq_.h>
#include <lnkquery.h>
#include <lnkquer_.h>
#include <linkbase.h>
#include <link.h>

// Must be last header
#include <dbmem.h>

#define BROKEN_UNAGG2
#ifndef BROKEN_UNAGG2

IMPLEMENT_UNAGGREGATABLE2_SELF_DELETE(cBaseDonorQuery,IDonorQuery,IObjectQuery);

#else

__IMPLEMENT_UNAGGREGATABLE_BASE(cBaseDonorQuery, (IsEqualIID(id, IID_IDonorQuery) || IsEqualOrIUnknownGUID(id, IID_IObjectQuery)));

inline void cBaseDonorQuery::OnFinalRelease()
{ 
   delete this; 
} 

#endif 

////////////////////////////////////////////////////////////
//
// cPriLinkDonorQuery 
//
////////////////////////////////////////////////////////////

cPriLinkDonorQuery::cPriLinkDonorQuery(ILinkQuery* links)
   :Links(new cDelegatedLinkQuery(this,links))
{
}

cPriLinkDonorQuery::~cPriLinkDonorQuery()
{
   delete Links;
}

STDMETHODIMP cPriLinkDonorQuery::QueryInterface(REFIID id, void** ppI)
{
   if (id == IID_ILinkQuery)
   {
      *ppI = Links;
      Links->AddRef();
      return S_OK;
   }
   return cBaseDonorQuery::QueryInterface(id,ppI);
}

STDMETHODIMP_(BOOL) cPriLinkDonorQuery::Done()
{
   return Links->Done();
}

STDMETHODIMP   cPriLinkDonorQuery::Next()
{
   return Links->Next();
}

STDMETHODIMP_(ObjID) cPriLinkDonorQuery::Object()
{
   sLink link = {OBJ_NULL, OBJ_NULL};
   Links->Link(&link);
   return link.dest;
}

STDMETHODIMP_(tDonorPriority) cPriLinkDonorQuery::Priority()
{
   return *(tDonorPriority*)Links->Data();
}

