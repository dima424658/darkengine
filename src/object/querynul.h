/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/querynul.h,v 1.4 2000/01/29 13:24:49 adurant Exp $
#pragma once

#ifndef __QUERYNUL_H
#define __QUERYNUL_H
#include <lnkquery.h>

////////////////////////////////////////////////////////////
// THE NULL LINK QUERY  
// 
// Has no links.  Is always done.  So there. 
//

class cNullLinkQuery : public ILinkQuery 
{
   DECLARE_UNAGGREGATABLE();
public:
   cNullLinkQuery() {};
   
   STDMETHOD_(BOOL,Done)() const; 
   STDMETHOD(Link)(sLink* link) const;
   STDMETHOD(Next)();

   STDMETHOD_(LinkID,ID)() const { return LINKID_NULL;};
   STDMETHOD_(void*,Data)() const { return NULL;};
   STDMETHOD_(ILinkQuery*,Inverse)() { AddRef(); return this;};
};



#endif // __QUERYNUL_H












