/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipthdbg.h,v 1.12 2000/03/01 13:02:53 bfarquha Exp $
//
//
//

#ifndef __AIPTHDBG_H
#define __AIPTHDBG_H

#include <dynarray.h>

#pragma once

#ifndef SHIP

F_DECLARE_INTERFACE(IAIPath);

// and these are for debug stuff only...
extern tAIPathCellID gDrawOneCell;
extern tAIPathCellID AIDebugMagicLocatorCell;
extern BOOL AIDebugCenters;

//void AIPathFindTest();
extern void AIPathFindDrawPath(IAIPath * pPath);

// draw pathfinding database, both cells and links, depending on
// status of flags below
extern "C" void AIPathFindDrawDB();
extern void AIPathDrawCellsToggle();
extern void AIPathDrawCellIdsToggle();
extern void AIPathDrawLinksToggle(int i);

extern void AIPathFindSpewZone();
extern void AIPathFindSpewZones();

extern int g_AIPathDBDrawZone;
extern int g_AIPathDBDrawZoneType;
extern int g_AIPathDBDrawRoom;

void ValidateAllCellSpaces(float height, BOOL linkedOnly, cDynArray<tAIPathCellID> * pFailures);
void ValidateAllCellRooms(BOOL linkedOnly, cDynArray<tAIPathCellID> * pFailures);

#endif

#endif /* !__AIPTHDBG_H */
