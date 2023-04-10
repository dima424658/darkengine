/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/objsys.h,v 1.6 2000/01/29 13:24:06 adurant Exp $
#pragma once

#ifndef OBJSYS_H
#define OBJSYS_H

#include <appagg.h>
#include <iobjsys.h>
#include <objtype.h>
#include <osystype.h>

////////////////////////////////////////////////////////////
// OBJECT SYSTEM INTERFACE
// 

//
// This is now a wrapper interface around IObjectSystem.
// Newer code should use IObjectSystem
//


#define ObjSysCommitDeletions()  ObjSysEndOfFrame()

EXTERN ObjID BeginObjectCreate(ObjID archetype, eObjConcreteness concrete);
EXTERN void EndObjectCreate(ObjID obj);
EXTERN BOOL ObjectExists(ObjID obj);

#define DestroyObject(obj) \
do { IObjectSystem* sys = AppGetObj(IObjectSystem); IObjectSystem_Destroy(sys,obj); SafeRelease(sys); } while(0)

#endif // OBJSYS_H









