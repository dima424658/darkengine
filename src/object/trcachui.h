/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/trcachui.h,v 1.3 2000/01/29 13:25:24 adurant Exp $
#pragma once

#ifndef __TRCACHUI_H
#define __TRCACHUI_H

////////////////////////////////////////////////////////////
// TRAIT DONOR CACHE UI
//
// Tools for viewing/setting cache statistics and parameters
//

EXTERN void EditDonorCacheParams(void);
EXTERN void DumpDonorCacheStats(void);
EXTERN void GetTraitStats(ulong);
EXTERN void GetNamedStats(const char *);
EXTERN void GetMaxID(void);
EXTERN void GetCacheEntryCount(void);


#endif // __TRCACHUI_H
