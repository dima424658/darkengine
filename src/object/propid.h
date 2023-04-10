/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propid.h,v 1.5 2000/01/29 13:24:29 adurant Exp $
#pragma once

#ifndef PROPID_H
#define PROPID_H

////////////////////////////////////////////////////////////
// PROPERTY INTERFACE BY ID
//
// This is a bunch of macros for using properties by ID
//

#define PropID_Describe(id)               IProperty_Describe(_GetProperty(id))
#define PropID_GetID(id)                  IProperty_GetID(_GetProperty(id))  
#define PropID_DescribeType(id)           IProperty_DescribeType(_GetProperty(id))
#define PropID_Create(id,obj)             IProperty_Create(_GetProperty(id),obj)
#define PropID_Copy(id,obj,ex)            IProperty_Copy(_GetProperty(id),obj,ex)
#define PropID_Delete(id,obj)             IProperty_Delete(_GetProperty(id),obj)
#define PropID_IsRelevant(id,obj)         IProperty_IsRelevant(_GetProperty(id),obj)  
#define PropID_IsSimplyRelevant(id,obj)   IProperty_IsSimplyRelevant(_GetProperty(id),obj)  
#define PropID_Touch(id,obj)              IProperty_Touch(_GetProperty(id),obj)  
#define PropID_Notify(id, a, b)           IProperty_Notify(_GetProperty(id), a, b)        
#define PropID_Listen(id, a, b, c)        IProperty_Listen(_GetProperty(id), a, b, c)     
#define PropID_IterStart(id, a)           IProperty_IterStart(_GetProperty(id), a)        
#define PropID_IterNext(id, a, b)         IProperty_IterNext(_GetProperty(id), a, b)      
#define PropID_IterStop(id, a)            IProperty_IterStop(_GetProperty(id), a)         

#endif // PROPID_H
