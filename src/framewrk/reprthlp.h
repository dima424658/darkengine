/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/reprthlp.h,v 1.2 1998/10/21 23:40:52 dc Exp $

#pragma once

#ifndef __REPRTHLP_H
#define __REPRTHLP_H

#include <osysbase.h>
#include <matrixs.h>

// this require matrixs, obviously, but if you have pVec, you have it
#define RepVecInts(pVec)  \
  (int)pVec->x, (int)pVec->y, (int)pVec->z

// for room names
EXTERN char *ReportHelperRoomString(ObjID obj, mxs_vector *pos, char *buffer);
// short form so your code can be made to fit on a finite number of pages
#define RHRoomS(obj)        ReportHelperRoomString(obj,NULL,NULL)
#define RHRoomPosS(obj,pos) ReportHelperRoomString(obj,pos,NULL)

// are we in the room db, fill string in buffer if it is Non-Null
EXTERN BOOL ReportHelperInRoom(ObjID obj, mxs_vector *pos, char *buffer);

// for "room and pos or what im in"
EXTERN char *ReportHelperLocationString(ObjID obj, char *buffer);
#define RHLocS(obj)         ReportHelperLocationString(obj,NULL)

// for stack counts
EXTERN char *ReportHelperStackString(ObjID obj, char *buffer);
// short form so your code can be made to fit on a finite number of pages
#define RHStakS(obj)        ReportHelperStackString(obj,NULL)
// is there one (ie. is there an error?)
EXTERN BOOL ReportHelperHasStack(ObjID obj);
#define RHStakB(obj)        ReportHelperHasStack(obj)

// to start and finish using the helpers
EXTERN void ReportHelpersStartUse(void);
EXTERN void ReportHelpersDoneWith(void);


#endif // __REPRTHLP_H
