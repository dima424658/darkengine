/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/objcast.h,v 1.7 2000/01/31 09:52:54 adurant Exp $
#pragma once

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\
   objcast.h

   This is a raycaster which checks for intersections with terrain,
   md objects, and mesh objects.  Note that this is not C-callable
   since it has a default parameter.

   @TODO: Some of these intersections are pretty darked slow.
   We may want a property to flag objects to pay attention to,
   maybe with bitfields to let us be more selective.

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */


#ifndef _OBJCAST_H_
#define _OBJCAST_H_

#include <objtype.h>


typedef enum eObjCastResult
{
   kObjCastNone,
   kObjCastTerrain,
   kObjCastMD,
   kObjCastMesh
} eObjCastResult;


// to test all objects along our ray, rather than a given list
#define kObjCastAllObjs -1

struct IBoolProperty;

#ifdef __cplusplus

// We cast against terrain, then objects, but can choose to ignore all
// but certain objects.
extern eObjCastResult ObjRaycast(Location *pStartLoc, Location *pEndLoc,
                                 Location *pHitLoc, BOOL ShortCircuit,
                                 ObjID *pObjList = 0,
                                 uint nNumObjs = kObjCastAllObjs,
                                 IBoolProperty *pObjProp = 0,
                                 BOOL UseZeroEpsilon = FALSE);

// other info you might enjoy--state not guaranteed for those which
// don't apply to the result of the most recent cast
struct mms_ray_hit_detect_info;         // in mm.h
extern mms_ray_hit_detect_info g_ObjCastMeshHit;

struct mds_segment_hit_detect_info;     // in mdutil.h
extern mds_segment_hit_detect_info g_ObjCastMDHit;

// the object we struct
extern ObjID g_ObjCastObjID;

#endif

EXTERN eObjCastResult ObjRaycastC(Location *pStartLoc, Location *pEndLoc, Location *pHitLoc,
                                  BOOL ShortCircuit, ObjID *pObjList, uint nNumObjs, struct IBoolProperty *pObjProp,
                                  BOOL UseZeroEpsilon);

#endif // ~_OBJCAST_H_








