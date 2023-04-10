/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $ Header: $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   fxdisk.h

   export for fxdisk.c

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _FXDISK_H_
#define _FXDISK_H_

#include <lg.h>
#include <fix.h>

EXTERN void FXDiskDrawHeatDisks(fix first_x, fix first_y, fix first_radius, 
                                fix last_x, fix last_y, fix last_radius,
                                int num_disks);

EXTERN void FXDiskDrawHeatDisksInWorld(mxs_vector *center_1, 
                                       mxs_real first_radius, 
                                       mxs_vector *center_2, 
                                       mxs_real last_radius,
                                       int num_disks_at_z_1);

#endif // ~_FXDISK_H_

