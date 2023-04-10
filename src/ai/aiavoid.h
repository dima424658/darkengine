/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiavoid.h,v 1.3 2000/03/25 22:13:01 adurant Exp $
//
//

#ifndef __AIAVOID_H
#define __AIAVOID_H

#include <comtools.h>
#include <dynarray.h>

#include <objtype.h>

#pragma once
#pragma pack(4)

F_DECLARE_INTERFACE(IAIManager);

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitObjectAvoid(IAIManager *);
BOOL AITermObjectAvoid();
BOOL AIDBResetObjectAvoid();

///////////////////////////////////////////////////////////////////////////////
//
//
//

enum eAIObjectAvoidFlags
{
   kAIOA_MarkDB = 0x01,
   kAIOA_Repel  = 0x02
};

///////////////////////////////////////

struct sAIObjectAvoid
{
   ObjID    object;
   unsigned flags;
};

///////////////////////////////////////
//
// Get avoidance for one object
//

const sAIObjectAvoid * AIGetObjAvoid(ObjID obj);

///////////////////////////////////////
//
// Get all repel objects (null terminated)
//

void AIGetAllObjRepel(const ObjID **, int *);

///////////////////////////////////////
//
// Get all marked objects.
//

struct sAIObjectAvoidMark
{
   ObjID      object;
   mxs_vector vertices[4];
};

void AIGetAllObjMarkDB(cDynArray<sAIObjectAvoidMark> * pResult);

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIAVOID_H */
