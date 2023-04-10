/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propface.h,v 1.12 2000/02/24 23:41:07 mahk Exp $
#pragma once
#ifndef PROPFACE_H
#define PROPFACE_H

#include <property.h>
#include <propdef.h>
#include <label.h>

////////////////////////////////////////////////////////////
// VARIOUS PROPERTY INTERFACES
//

F_DECLARE_INTERFACE(IPropertyStore); 

//------------------------------------------------------------
// BOOL PROPERTY
//

#undef INTERFACE
#define INTERFACE IBoolProperty
DECLARE_PROPERTY_INTERFACE(IBoolProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(BOOL);
}; 

EXTERN IBoolProperty* CreateBoolProperty(const sPropertyDesc* desc, ePropertyImpl impl); 
EXTERN IBoolProperty* CreateBoolPropertyFromStore(const sPropertyDesc* desc, 
                                                  IPropertyStore* store); 
// Use the default boolean property store
//EXTERN IBoolProperty* CreateDefaultBoolProperty(const sPropertyDesc* desc); 



//------------------------------------------------------------
// INT PROPERTY
//

#undef INTERFACE
#define INTERFACE IIntProperty
DECLARE_PROPERTY_INTERFACE(IIntProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(int);
}; 

EXTERN IIntProperty* CreateIntProperty(const sPropertyDesc* desc, ePropertyImpl impl); 
EXTERN IIntProperty* CreateIntPropertyFromStore(const sPropertyDesc* desc, IPropertyStore* store); 

//
// Factory for scalar properties that use IIntProperty.  Roll-your-own type descriptor.
//
EXTERN IIntProperty* CreateIntegralProperty(const sPropertyDesc* desc, 
                                            const sPropertyTypeDesc* tdesc, 
                                            ePropertyImpl impl); 
EXTERN IIntProperty* CreateIntegralPropertyFromStore(const sPropertyDesc* desc, 
                                                    const sPropertyTypeDesc* tdesc, 
                                                    IPropertyStore* store); 


//------------------------------------------------------------
// INT PROPERTY
//

#undef INTERFACE
#define INTERFACE IFloatProperty
DECLARE_PROPERTY_INTERFACE(IFloatProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(float);
}; 

EXTERN IFloatProperty* CreateFloatProperty(const sPropertyDesc* desc, ePropertyImpl impl); 
EXTERN IFloatProperty* CreateFloatPropertyFromStore(const sPropertyDesc* desc, IPropertyStore* store); 


//------------------------------------------------------------
// LABEL PROPERTY 
//

#undef INTERFACE
#define INTERFACE ILabelProperty
DECLARE_PROPERTY_INTERFACE(ILabelProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(Label*);
}; 

EXTERN ILabelProperty* CreateLabelProperty(const sPropertyDesc* desc, ePropertyImpl impl); 
EXTERN ILabelProperty* CreateLabelPropertyFromStore(const sPropertyDesc* desc, IPropertyStore* store); 

//------------------------------------------------------------
// VECTOR PROPERTY 
//

#undef INTERFACE
#define INTERFACE IVectorProperty
DECLARE_PROPERTY_INTERFACE(IVectorProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(struct mxs_vector*);
}; 

EXTERN IVectorProperty* CreateVectorProperty(const sPropertyDesc* desc, ePropertyImpl impl); 
EXTERN IVectorProperty* CreateVectorPropertyFromStore(const sPropertyDesc* desc, IPropertyStore* store); 


//------------------------------------------------------------
// STRING PROPERTY
//

#undef INTERFACE
#define INTERFACE IStringProperty
DECLARE_PROPERTY_INTERFACE(IStringProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(const char*);
}; 

EXTERN IStringProperty* CreateStringProperty(const sPropertyDesc* desc, ePropertyImpl impl); 
EXTERN IStringProperty* CreateStringPropertyFromStore(const sPropertyDesc* desc, IPropertyStore* store); 


#endif // PROPFACE_H


