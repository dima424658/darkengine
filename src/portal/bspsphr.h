/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/portal/bspsphr.h,v 1.3 2000/01/29 13:37:12 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   bspsphr.h

   export header for bspsphr.c

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _BSPSPHR_H_
#define _BSPSPHR_H_

#include <lg.h>

// Clients will want to use this limit in sizing their output arrays.
#define BSPSPHR_OUTPUT_LIMIT 512

EXTERN int portal_cells_intersecting_sphere(Location *loc, float radius, 
                                            int *output_list);

#endif // ~_BSPSPHR_H_

