/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/prj/cam/src/portal/RCS/bspsphr.h 1.2 1998/05/19 18:35:17 MAT Exp $

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
