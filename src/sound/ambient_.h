/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/ambient_.h,v 1.5 2000/01/29 13:41:40 adurant Exp $
// ambient sound object initial cut
#pragma once

#ifndef __AMBIENT__H
#define __AMBIENT__H

#include <objtype.h>
#include <ambbase.h>

EXTERN int ambMax();  // max # of ambient objects
EXTERN ObjID ambObjID(int idx);  // the objid for ambient N 
EXTERN AmbientSound* ambState(int idx);  // the state for ambient N
EXTERN void ambRemove(int idx);
EXTERN void ambSetFlags(int idx, int flags);

// this data is run time used by the system to track playing
typedef struct {
   int   schemaHandle;   // the schemaHandle if we are now playing
   ulong last_played;    // this should be sim time
} AmbientRunTime;

#endif  // __AMBIENT__H
