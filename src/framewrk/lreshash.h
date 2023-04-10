/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/lreshash.h,v 1.2 2000/01/29 13:21:11 adurant Exp $
#pragma once

#ifndef __LRESHASH_H
#define __LRESHASH_H

EXTERN rn_hnd _ResNametoHandleFast(char *name, char *tokens);
EXTERN void   _ResNameHashStore(rn_hnd hnd);
EXTERN void   _ResNameHashFree(rn_hnd hnd);
EXTERN void   _ResNameHashInitialize(void);
#endif  // __LRESHASH_H   
