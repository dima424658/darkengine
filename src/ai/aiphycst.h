/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiphycst.h,v 1.2 2000/03/01 16:22:47 toml Exp $
//

#ifndef INCLUDED_AIPHYCST_H
#define INCLUDED_AIPHYCST_H

#include <physcast.h>

#pragma once

///////////////////////////////////////////////////////////////////////////////

EXTERN void AIPhysCastCollectObjects();

///////////////////////////////////////

enum eAIPRObjFilter
{
   kAIPR_Pathables,
   kAIPR_Pathblocking,
   kAIPR_PathblockingOrPathable,
   // someday, should add kAIPR_PhysicalToAIs and change other physcasts (toml 03-01-00) 
   
   kAIPR_Any,
};

///////////////////////////////////////

EXTERN void AIPhysRaycastSetObjFilter(eAIPRObjFilter filter);
EXTERN void AIPhysRaycastClearObjFilter();

EXTERN int AIPhysRaycast(Location &start, Location &end, Location *hit, ObjID *hit_obj, 
                         mxs_real radius, int filter = kCollideAll);


///////////////////////////////////////////////////////////////////////////////

#endif /* !INCLUDED_AIPHYCST_H */
