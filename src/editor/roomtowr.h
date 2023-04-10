/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/roomtowr.h,v 1.1 1999/12/01 09:26:03 MAT Exp $

/* --=<= --/-/-/-/-/-/-/ <(< (( ((( /^^\ ))) )) >)> \-\-\-\-\-\-\-- =>=-- *\
   roomtowr.h

   There are a few cases where we want to use room brushes to flag, at
   least in a rough way, cells in the world rep.  This works something
   like flow brushes, finding which geometry intersects which.
   Clients should install their callbacks once on app init.

   RoomToWRProcess, the main function, should be called right after
   the world rep has been generated.

   This stuff should only exist in editor builds.

\* --=<= --\-\-\-\-\-\-\ <(< (( ((( \vv/ ))) )) >)> /-/-/-/-/-/-/-- =>=-- */

#ifndef _ROOMTOWR_H_
#pragma once
#define _ROOMTOWR_H_

#include <wrdb.h>

class cRoom;

// We call these on every cell/room combination which intersect.
typedef void (* tRoomToWRCallback)(int iCell, cRoom *pRoom, void *pData);

// We call these as the first step in RoomToWRProcess.
typedef void (* tRoomToWRStartCallback)();

// We call these as the final step in RoomToWRProcess.
typedef void (* tRoomToWREndCallback)();

extern void RoomToWRAddCallback(tRoomToWRCallback pfnCallback, void *pData);
extern void RoomToWRAddStartCallback(tRoomToWRStartCallback pfnCallback);
extern void RoomToWRAddEndCallback(tRoomToWREndCallback pfnCallback);
extern void RoomToWRProcess();

#endif // ~_ROOMTOWR_H_
