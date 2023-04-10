/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/lockprop.h,v 1.5 2000/01/29 13:19:46 adurant Exp $
// actually the lock property and lock link
#pragma once

#ifndef __LOCKPROP_H
#define __LOCKPROP_H

#include <objtype.h>

// locked property usage
// simple accessors
EXTERN BOOL ObjSelfLocked(ObjID obj);
EXTERN void ObjSetSelfLocked(ObjID obj, BOOL locked);

// actually goes and checks all links/so on
EXTERN BOOL ObjComputeLockedState(ObjID obj);

// overall init
EXTERN void EngineLockInit(void);
EXTERN void EngineLockTerm(void);

// call on postload to recompute all lock state
EXTERN void PostLoadLockBuild(void);

////////////////////////
// so you want to know how/what to unlock something

#define kObjAccessAnd    (1<<0)
#define kObjAccessOr     (1<<1)
#define kObjAccessMaster (1<<2)

// the actual struct
struct sObjAccess
{
   ObjID frobee;  // frob this
   ObjID frober;  // with this, if this is null, just frob it yourself
   int   flags;   // what sort of thing is this
};

#define MAX_OBJ_ACCESS 16
// pass in an array of MAX_OBJ_ACCESS sObjAccess'
EXTERN int CheckObjectAccess(ObjID accessor, ObjID target, sObjAccess *accessList, BOOL getAllLocks);

#endif  // __LOCKPROP_H
