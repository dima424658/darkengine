/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/resloop.h,v 1.3 2000/01/31 09:48:37 adurant Exp $
#pragma once

#ifndef __RESLOOP_H
#define __RESLOOP_H

DEFINE_LG_GUID(LOOPID_Res, 0x23);
EXTERN struct sLoopClientDesc ResLoopClientDesc;

//
// Context for loopmode go 
// 

#define NUM_RESLOOP_FILES 16

typedef struct _ResContext
{
   char* resfiles[NUM_RESLOOP_FILES]; // list of res files to open
} ResLoopContext;

#endif // __RESLOOP_H
