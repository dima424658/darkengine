/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/querynul.cpp,v 1.3 1997/10/14 11:20:38 TOML Exp $
#include <linktype.h>
#include <linkbase.h>
#include <querynul.h>

// Must be last header
#include <dbmem.h>

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cNullLinkQuery,ILinkQuery);

STDMETHODIMP_(BOOL) cNullLinkQuery::Done() const
{
   return TRUE;  // we awways dun
}

STDMETHODIMP cNullLinkQuery::Link(sLink*) const 
{
   Warning(("cNullLinkQuery::Link() has been called\n"));
   return E_FAIL;
}

STDMETHODIMP cNullLinkQuery::Next()
{
   Warning(("cNullLinkQuery::Next() has been called\n"));
   return E_FAIL;   
}

ILinkQuery* CreateEmptyLinkQuery(void)
{
   return new cNullLinkQuery;
}
