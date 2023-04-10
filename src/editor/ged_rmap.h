/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/ged_rmap.h,v 1.2 2000/01/29 13:12:14 adurant Exp $
// remap brush horror
#pragma once

#ifndef __GED_RMAP_H
#define __GED_RMAP_H

typedef void (*ged_remap_callback)(int *mapping);

// go compress br_id space
EXTERN int ged_remap_brushes(void);

// add and remove remap callbacks
EXTERN BOOL ged_remap_add_callback(ged_remap_callback cb);
EXTERN BOOL ged_remap_rem_callback(ged_remap_callback cb);

#endif  // __GED_RMAP_H
