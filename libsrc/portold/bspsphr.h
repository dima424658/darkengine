/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/portold/bspsphr.h,v 1.1 1997/10/17 18:14:02 MAT Exp $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   bspsphr.h

   export header for bspsphr.c

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _BSPSPHR_H_
#define _BSPSPHR_H_

#include <lg.h>

EXTERN int portal_cells_intersecting_sphere(Location *loc, float radius, 
                                            int *output_list);

#endif // ~_BSPSPHR_H_

