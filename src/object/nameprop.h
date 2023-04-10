/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/nameprop.h,v 1.4 2000/01/29 13:23:46 adurant Exp $
#pragma once
#ifndef __NAMEPROP_H
#define __NAMEPROP_H
#include <propface.h>
#include <propinv.h>

////////////////////////////////////////////////////////////
// THE INVERTABLE LABEL INTERFACE IInvLabelProperty
// 
// This property interface is just like ILabelProperty, 
// except it has an additional method, GetObj, that can 
// look an object given a label.
//

F_DECLARE_INTERFACE(IInvStringProperty);

#undef INTERFACE 
#define INTERFACE IInvStringProperty 
DECLARE_INTERFACE_(IInvStringProperty,IStringProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(const char*); 
   DECLARE_INVERSE_PROPERTY_ACCESSORS(const char*); 
};
#undef INTERFACE
 
EXTERN IInvStringProperty* CreateInvStringProperty(const sPropertyDesc* desc, ePropertyImpl impl);  
EXTERN IInvStringProperty* CreateInvStringPropertyFromStore(const sPropertyDesc* desc, IPropertyStore* impl); 

#endif // __NAMEPROP_H






