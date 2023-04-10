/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propname.h,v 1.6 2000/01/29 13:24:40 adurant Exp $
#pragma once

#ifndef PROPNAME_H
#define PROPNAME_H

////////////////////////////////////////////////////////////
// PROPERTY INTERFACE BY NAME 
//
// Useful macros for interfacing properties by name
//

#define PropName_Describe(name)               IProperty_Describe(_GetPropertyNamed(name))
#define PropName_GetID(name)                  IProperty_GetID(_GetPropertyNamed(name))  
#define PropName_DescribeType(id)             IProperty_DescribeType(_GetPropertyNamed(name))
#define PropName_Create(name,obj)             IProperty_Create(_GetPropertyNamed(name),obj)
#define PropName_Copy(name,obj,ex)            IProperty_Copy(_GetPropertyNamed(name),obj,ex)
#define PropName_Delete(name,obj)             IProperty_Delete(_GetPropertyNamed(name),obj)
#define PropName_IsRelevant(name,obj)         IProperty_IsRelevant(_GetPropertyNamed(name),obj)  
#define PropName_IsSimplyRelevant(name,obj)   IProperty_IsSimplyRelevant(_GetPropertyNamed(name),obj)  
#define PropName_Touch(name,obj)              IProperty_Touch(_GetPropertyNamed(name),obj)  
#define PropName_Notify(name, a, b)           IProperty_Notify(_GetPropertyNamed(name), a, b)        
#define PropName_Listen(name, a, b, c)        IProperty_Listen(_GetPropertyNamed(name), a, b, c)     
#define PropName_IterStart(name, a)           IProperty_IterStart(_GetPropertyNamed(name), a)        
#define PropName_IterNext(name, a, b)         IProperty_IterNext(_GetPropertyNamed(name), a, b)      
#define PropName_IterStop(name, a)            IProperty_IterStop(_GetPropertyNamed(name), a)         

#endif // PROPNAME_H

