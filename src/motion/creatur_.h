/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/creatur_.h,v 1.12 2000/01/31 09:50:07 adurant Exp $
#pragma once

#ifndef __CREATUR__H
#define __CREATUR__H

#include <objtype.h>
#include <crettype.h>
#include <mp.h>

//// for property module
//

#define MAX_CREATURES 40

EXTERN ObjID chandle_obj_id(int idx); 
EXTERN sCreatureHandle* CreatureHandle(int idx); 
EXTERN int obj_chandle_id(ObjID obj); 
EXTERN int max_chandle_id(void);

//// for creature description modules (one per creature type)
//

// model can be placed at a fractional offset between two joints
struct sCrPhysModOffset
{
   int j1;
   int j2;
   float frac;
   float radius;
   BOOL isBallistic;
};

struct sCrPhysModOffsetTable
{
   int nPhysModels;
   sCrPhysModOffset *pPhysModels;
};

typedef struct sCreatureDesc
{
   int actorType;
   int nTorsos;
   int nLimbs;
   int nJoints;
   char *defLengthsName;
   int nPhysModels;
   float physRadius; 
   sCrPhysModOffset *pCrPhysModOffsets;
   int *pJointParents;
   int *pJointMap;
   int nFeet; // number of feet
   int *footJoints;
   torso *pDefTorsos; // default torso values (if defLengthsName not found)
   limb  *pDefLimbs; // default limb values (if defLengthsName not found)
   int nWeapons;
   sCrPhysModOffsetTable *pWeapPhysOffsets;
   BOOL  alwaysBallistic; // whether creature never self-propells via motions
   BOOL  canHeadTrack;
   BOOL hasSpecialPhysics; // do our physics models use the "special" flag?
} sCreatureDesc;

#endif

