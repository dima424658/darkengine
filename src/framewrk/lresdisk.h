/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////
//
// $Header: r:/t2repos/thief2/src/framewrk/lresdisk.h,v 1.3 2000/01/29 13:21:09 adurant Exp $
//
#pragma once

#ifndef __LRESDISK_H
#define __LRESDISK_H

#include <resapi.h>

typedef BOOL (*tNameCallback)(char *name);

EXTERN void *BuildResBlock(IRes **handles, int cnt);
EXTERN BOOL ParseResBlock(void *blk, 
                          /* OUT */ IRes **handles, 
                          tStoredResCallback storeCb,
                          tNameCallback nameCb);

#define ResBlockSize(blockptr)   (*(((int *)blockptr)+0))
#define ResBlockHndCnt(blockptr) (*(((int *)blockptr)+1))

#endif // __LRESDISK_H
