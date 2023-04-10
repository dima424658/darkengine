/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/objmsg.h,v 1.3 2000/01/31 10:00:12 adurant Exp $
#pragma once

#ifndef OBJMSG_H
#define OBJMSG_H

#include <objtype.H>
#include <looptype.h>

////////////////////////////////////////////////////////////
//
// OBJECT DISPATCH MESSAGES
//

#define kMsgObjSys   kMsgApp3  // see the botton of dispatch.cc 

//
// Object message subtypes
//

typedef enum msgObjSysKind
{
   kObjectCreate,
   kObjectDelete,
   kObjectLoad,
   kMsgObjSysNumKinds,
} msgObjSysKind;

//
// Satellite data types
//
typedef union MsgObjSysData
{
   void* raw;     // raw data;
   ObjID Create;  // object being created
   ObjID Delete;  // object being deleted
} msgObjSysData;
   

#endif // OBJMSG_H

