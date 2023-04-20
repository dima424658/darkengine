/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propcomp.h,v 1.6 2000/01/29 13:24:20 adurant Exp $
#pragma once

#ifndef __PROPCOMP_H
#define __PROPCOMP_H

#include <propert_.h>
#include <dataops_.h>

template <class IFACE, const GUID* pIID, class TYPE> 
class cComplexProperty: public cGenericProperty<IFACE,pIID,TYPE*>
{  
   typedef cGenericProperty<IFACE,pIID,TYPE*> cParent; 

   cClassDataOps<TYPE> mOps; 

public: 
   cComplexProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cParent(desc,store)
   {
      this->mpStore->SetOps(&mOps); 
   }

   cComplexProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cParent(desc,CreateGenericPropertyStore(impl))
   {
      this->mpStore->SetOps(&mOps); 
   }

   
   
};

#define COMPLEX_PARENT_CLASS(TYPE,NAME) \
cComplexProperty<I##NAME##Property,&IID_I##NAME##Property,TYPE> 

#define DECLARE_COMPLEX_PROPERTY_CLASS(CLASSNAME,TYPE,NAME)    \
class CLASSNAME : \
public COMPLEX_PARENT_CLASS(TYPE,NAME) \
{ \
   typedef COMPLEX_PARENT_CLASS(TYPE,NAME) cParent; \
public: \
   CLASSNAME(const sPropertyDesc* desc, IPropertyStore* store) : cParent(desc,store) {} \
   CLASSNAME(const sPropertyDesc* desc, ePropertyImpl impl) : cParent(desc,impl) {} \
   STANDARD_DESCRIBE_TYPE(TYPE);  \
}


#endif // __PROPCOMP_H

