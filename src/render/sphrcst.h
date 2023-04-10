/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/sphrcst.h,v 1.6 2000/01/31 09:53:24 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   sphrcst.h

   main export file for spherecaster

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

#ifndef _SPHRCST_H_
#define _SPHRCST_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <sphrcstt.h>
#include <bspsphr.h>    // in Portal directory

// the big puppy
extern int SphrSpherecast(Location *start_loc, Location *end_loc, 
                          mxs_real sphere_radius, uchar flags,
                          int *cell_output, int *cell_output_count);

// This wrapper for SphrSpherecast drops the list of cells
// intersecting the cast into SphrCellsReached and SphrCellCount,
// declared below, rather than expecting equivalent pointers to be
// passed in.
extern int SphrSpherecastStatic(Location *start_loc, Location *end_loc, 
                                mxs_real sphere_radius, uchar flags);

// This returns FALSE if the sphere intersects even one polygon.
extern bool SphrSphereInWorld(Location *center, mxs_real radius,
                              uchar portal_flags);

#ifndef SHIP

// This monodumps.
extern void SphrDumpCastResult(void);

#endif // ~SHIP

// Overengineer!  Overengineer!
#define SPHR_MAX_CONTACT 16
extern sSphrContact gaSphrContact[SPHR_MAX_CONTACT];
extern int gSphrContactCount;

#define SPHR_MAX_CONTACT_DATA (SPHR_MAX_CONTACT * 2)
extern sSphrContactData gaSphrContactData[SPHR_MAX_CONTACT_DATA];
extern int gSphrContactDataCount;

// These are where SphrSpherecastStatic leaves its list of cells
// intersecting the cast.  They're also used by SphrSphereInWorld
// internally.
#define SPHR_MAX_CELLS_REACHED BSPSPHR_OUTPUT_LIMIT
extern int gaSphrCellsReached[SPHR_MAX_CELLS_REACHED];
extern int gSphrCellCount;

#ifdef __cplusplus
};
#endif

#endif // ~_SPHRCST_H_

