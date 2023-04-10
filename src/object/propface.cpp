/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propface.cpp,v 1.15 2000/02/24 23:41:06 mahk Exp $

#include <allocapi.h>

#include <propface.h>
#include <propfac_.h>
#include <matrixs.h>
#include <sdesc.h>
#include <sdesbase.h>

#include <pfacinit.h>

// Must be last header
#include <dbmem.h>


StructDescDefineSingleton(booldesc,BOOL,kFieldTypeBool,0);

// #define AUTO_CREDIT()
#define AUTO_CREDIT() LGALLOC_AUTO_CREDIT()


IBoolProperty* CreateBoolProperty(const sPropertyDesc* desc, ePropertyImpl impl)
{
   AUTO_CREDIT();
   return new cGenericBoolProperty(desc,impl); 
}

cSimpleDataOps cGenericBoolProperty::gOps; 

IBoolProperty* CreateBoolPropertyFromStore(const sPropertyDesc* desc, IPropertyStore* impl)
{
   AUTO_CREDIT();
   return new cGenericBoolProperty(desc,impl); 
}


////////////////////////////////////////

StructDescDefineSingleton(intdesc,int,kFieldTypeInt,0);

cSimpleDataOps cGenericIntProperty::gOps; 

IIntProperty* CreateIntProperty(const sPropertyDesc* desc, ePropertyImpl impl)
{
   AUTO_CREDIT();
   return new cGenericIntProperty(desc,impl); 
}

IIntProperty* CreateIntPropertyFromStore(const sPropertyDesc* desc, IPropertyStore* impl)
{
   AUTO_CREDIT();
   return new cGenericIntProperty(desc,impl);
}

class cIntegralProperty : public cGenericIntProperty
{
public:
   cIntegralProperty(const sPropertyDesc* desc, const sPropertyTypeDesc* tdesc, ePropertyImpl impl) 
      : cGenericIntProperty(desc,impl), TypeDesc(*tdesc) { };  
   cIntegralProperty(const sPropertyDesc* desc, const sPropertyTypeDesc* tdesc, IPropertyStore* impl)
      : cGenericIntProperty(desc,impl), TypeDesc(*tdesc) { };  
 

STDMETHOD_(const sPropertyTypeDesc*, DescribeType)() const 
   {
      return &TypeDesc; 
   };

protected:
   sPropertyTypeDesc TypeDesc;
 
};

IIntProperty* CreateIntegralProperty(const sPropertyDesc* desc, const sPropertyTypeDesc* tdesc, ePropertyImpl impl)
{
   AUTO_CREDIT();
   return new cIntegralProperty(desc,tdesc,impl); 
} 

IIntProperty* CreateIntegralPropertyFromStore(const sPropertyDesc* desc, const sPropertyTypeDesc* tdesc, IPropertyStore* impl)
{
   AUTO_CREDIT();
   return new cIntegralProperty(desc,tdesc,impl); 
} 



////////////////////////////////////////

StructDescDefineSingleton(floatdesc,float,kFieldTypeFloat,0);

IFloatProperty* CreateFloatProperty(const sPropertyDesc* desc, ePropertyImpl impl)
{
   AUTO_CREDIT();
   return new cGenericFloatProperty(desc,impl); 
}

cSimpleDataOps cGenericFloatProperty::gOps; 

IFloatProperty* CreateFloatPropertyFromStore(const sPropertyDesc* desc, IPropertyStore* impl)
{
   AUTO_CREDIT();
   return new cGenericFloatProperty(desc,impl); 
}


////////////////////////////////////////

StructDescDefineSingleton(labeldesc,Label,kFieldTypeString,0);

cClassDataOps<Label> cGenericLabelProperty::gOps; 

ILabelProperty* CreateLabelProperty(const sPropertyDesc* desc, ePropertyImpl impl)
{
   AUTO_CREDIT();
   return new cGenericLabelProperty(desc,impl); 
}

ILabelProperty* CreateLabelPropertyFromStore(const sPropertyDesc* desc, IPropertyStore* impl)
{
   AUTO_CREDIT();
   return new cGenericLabelProperty(desc,impl); 
}

////////////////////////////////////////

StructDescDefineSingleton(vecdesc,Vector,kFieldTypeVector,0);

cClassDataOps<Vector> cGenericVectorProperty::gOps; 

IVectorProperty* CreateVectorProperty(const sPropertyDesc* desc, ePropertyImpl impl)
{
   AUTO_CREDIT();
   return new cGenericVectorProperty(desc,impl); 
}

IVectorProperty* CreateVectorPropertyFromStore(const sPropertyDesc* desc, IPropertyStore* impl)
{
   AUTO_CREDIT();
   return new cGenericVectorProperty(desc,impl); 
}


////////////////////////////////////////


void PropertyInterfacesInit(void)
{
   StructDescRegister(&booldesc);
   StructDescRegister(&intdesc);
   StructDescRegister(&floatdesc);
   StructDescRegister(&labeldesc);
   StructDescRegister(&vecdesc);
}

void PropertyInterfacesTerm(void)
{
}



