/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/lr_cache.h,v 1.2 2000/01/29 13:21:19 adurant Exp $
// lresname interface to the real cache
#pragma once

#ifndef __LR_CACHE_H
#define __LR_CACHE_H

typedef BOOL (*lr_callback)(int hnd, void *data);

// system startup/shutdown
EXTERN void lrCache_Init(lr_callback toast_it, int size);
EXTERN void lrCache_Term(void);

// these BOOLs return FALSE if no cache in debug, fault in opt

// main API to client
EXTERN BOOL lrCache_StoreFreedData(int hnd, void *pData, int size);
EXTERN BOOL lrCache_TakeBackData(int hnd);

// flush control
EXTERN BOOL lrCache_FlushAll(void);
EXTERN BOOL lrCache_FlushHandle(int hnd);

// misc data about the cache - returns size in use, pass non-NULL if you want max
EXTERN int lrCache_GetSize(int *maxBytes);
EXTERN int lrCache_GetGlobalSize(int *maxBytes);

#endif  // __LR_CACHE_H

