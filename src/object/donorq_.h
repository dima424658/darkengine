/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/donorq_.h,v 1.3 2000/01/29 13:23:03 adurant Exp $
#pragma once
#ifndef __DONORQ__H
#define __DONORQ__H
#include <donorq.h>
#include <objquer_.h>

//
// Base donor query
//


class cBaseDonorQuery : public IDonorQuery
{
public:
   DECLARE_UNAGGREGATABLE();

   virtual ~cBaseDonorQuery() {};

};

////////////////////////////////////////

class cSingleDonorQuery : public cBaseDonorQuery 
{
   ObjID TheObj;
   tDonorPriority Pri; 

public:
   cSingleDonorQuery(ObjID obj, tDonorPriority pri = 0) : TheObj(obj), Pri(pri) {};
   virtual ~cSingleDonorQuery() {}; 

   STDMETHOD_(BOOL,Done)() { return TheObj == OBJ_NULL;}; 
   STDMETHOD_(ObjID,Object)() { return TheObj;}; 
   STDMETHOD(Next)() { TheObj = OBJ_NULL; return S_OK;}; 
   STDMETHOD_(tDonorPriority,Priority)(THIS) { return Pri;};
};   

////////////////////////////////////////////////////////////

//
// Make a donor query out of a set of links whose data field is a tDonorPriority.
// The destinations are the yielded objects
//

class cDelegatedLinkQuery;


F_DECLARE_INTERFACE(ILinkQuery);

class cPriLinkDonorQuery : public cBaseDonorQuery
{
   cDelegatedLinkQuery* Links;
   
public: 
   cPriLinkDonorQuery(ILinkQuery* links);
   virtual ~cPriLinkDonorQuery();

   STDMETHOD(QueryInterface)(REFIID id, void** ppI);
   STDMETHOD_(BOOL,Done)();
   STDMETHOD_(ObjID,Object)();
   STDMETHOD(Next)();
   STDMETHOD_(tDonorPriority,Priority)(THIS);
};


#endif // __DONORQ__H

