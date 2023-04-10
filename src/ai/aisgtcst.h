/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////////
//$Header: r:/t2repos/thief2/src/ai/aisgtcst.h,v 1.6 2000/03/25 22:14:54 adurant Exp $
//
// line-of-sight check from pStartLoc to pEndLoc, accounting for those
// doors which are set to block visibility
//
// We actually cull our list of doors, using only those within a fixed
// radius of pStartLoc.
//
#pragma once

#ifndef __AISGTCST_H
#define __AISGTCST_H

struct Location;

void AIInitSightCast();
void AITermSightCast();
void AIDBResetSightCast();

BOOL AISightCast(Location *pStartLoc, Location *pEndLoc);

// preperatory func, on entering sim: gather ObjIDs of all objects
// doors want to avoid seeing through
void AISightCastCollectRefs();

#endif /* !__AISGTCST_H */
