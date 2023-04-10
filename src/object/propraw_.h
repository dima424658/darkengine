/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propraw_.h,v 1.4 2000/01/29 13:24:43 adurant Exp $
#pragma once
#include <propraw.h>
#include <propimp.h>
#include <propert_.h>


#ifndef __PROPRAW__H
#define __PROPRAW__H


class cPropRawDelegate : public cCTDelegating<IPropertyRaw>
{

public:
   cPropRawDelegate(IUnknown* pOuter)
      : cCTDelegating<IPropertyRaw>(pOuter)
   {
   }

   ~cPropRawDelegate() { }; 
};


template <class PROPCLASS> 
class cBasePropertyRaw : public cPropRawDelegate
{

public:

   cBasePropertyRaw(IUnknown* pOuter, PROPCLASS* prop)
      : Prop(prop),
        cPropRawDelegate(pOuter)
   {
   };
   virtual ~cBasePropertyRaw() {}; 

   STDMETHOD_(ulong,Size)() { return Prop->DescribeType()->size; }; 

protected:
   PROPCLASS* Prop;
};




#endif // __PROPRAW__H





