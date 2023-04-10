/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propfac_.h,v 1.3 1998/09/07 01:07:18 mahk Exp $
#pragma once  
#ifndef __PROPFAC__H
#define __PROPFAC__H

#include <propface.h>
#include <propert_.h>
#include <dataops_.h>
#include <matrixs.h>
#include <propbool.h>

////////////////////////////////////////////////////////////
// 
// IMPLEMENTATION CLASSES FOR STANDARD PROPERTY INTERFACES 
//

class cGenPropStorePtr : public cAutoIPtr<IPropertyStore>
{
public:
   cGenPropStorePtr(ePropertyImpl impl) : 
      cAutoIPtr<IPropertyStore> (CreateGenericPropertyStore(impl))
   {
   }
};

#define GENSTORE(x) cGenPropStorePtr(x)

//
// BOOL
//

class cGenericBoolProperty : public cGenericProperty<IBoolProperty,&IID_IBoolProperty,BOOL>
{
   typedef cGenericProperty<IBoolProperty,&IID_IBoolProperty,BOOL> cParent; 

   static cSimpleDataOps gOps; 

public:
   cGenericBoolProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cParent(desc,store)
   {
      SetOps(&gOps); 
   }

   cGenericBoolProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cParent(desc,GENSTORE(impl))
   {
      SetOps(&gOps); 
   }

   STANDARD_DESCRIBE_TYPE(BOOL); 
};

// "Non-generic" version that uses the cBoolPropertyStore

class cBoolProperty : public cSpecificProperty<IBoolProperty,&IID_IBoolProperty,BOOL,cBoolPropertyStore>
{
   typedef cSpecificProperty<IBoolProperty,&IID_IBoolProperty,BOOL,cBoolPropertyStore> cParent; 
public:
   cBoolProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(BOOL); 

      
}; 


//
// INT
//

class cGenericIntProperty : public cGenericProperty<IIntProperty,&IID_IIntProperty,int>
{
   typedef cGenericProperty<IIntProperty,&IID_IIntProperty,int> cParent; 

   static cSimpleDataOps gOps; 
public:

   cGenericIntProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cParent(desc,store)
   {
      SetOps(&gOps); 
   }

   cGenericIntProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cParent(desc,GENSTORE(impl))
   {
      SetOps(&gOps); 
   }

   STANDARD_DESCRIBE_TYPE(int); 
};

//
// FLOAT
//

class cGenericFloatProperty : public cGenericProperty<IFloatProperty,&IID_IFloatProperty,float>
{
   typedef cGenericProperty<IFloatProperty,&IID_IFloatProperty,float> cParent; 

   static cSimpleDataOps gOps; 

public:
   cGenericFloatProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cParent(desc,store)
   {
      SetOps(&gOps); 
   }

   cGenericFloatProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cParent(desc,GENSTORE(impl))
   {
      SetOps(&gOps); 
   }

   STANDARD_DESCRIBE_TYPE(float); 
};



//
// Label
//

class cGenericLabelProperty : public cGenericProperty<ILabelProperty,&IID_ILabelProperty,Label*>
{
   typedef cGenericProperty<ILabelProperty,&IID_ILabelProperty,Label*> cParent; 

   static cClassDataOps<Label> gOps; 
public:

   cGenericLabelProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cParent(desc,store)
   {
      SetOps(&gOps); 
   }

   cGenericLabelProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cParent(desc,GENSTORE(impl))
   {
      SetOps(&gOps); 
   }

   STANDARD_DESCRIBE_TYPE(Label); 
};


//
// Vector
//

typedef mxs_vector Vector; 

class cGenericVectorProperty : public cGenericProperty<IVectorProperty,&IID_IVectorProperty,Vector*>
{
   typedef cGenericProperty<IVectorProperty,&IID_IVectorProperty,Vector*> cParent; 

   static cClassDataOps<Vector> gOps; 
public:

   cGenericVectorProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cParent(desc,store)
   {
      SetOps(&gOps); 
   }

   cGenericVectorProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cParent(desc,GENSTORE(impl))
   {
      SetOps(&gOps); 
   }

   STANDARD_DESCRIBE_TYPE(Vector); 
};

#undef GENSTORE


#endif // __PROPFAC__H
