/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/viewmgr_.h,v 1.3 2000/01/29 13:13:27 adurant Exp $
#pragma once

#ifndef ___VIEWMGR_H
#define ___VIEWMGR_H

#define MAX_CAMERAS 16
#define MAX_CAMERAS_REAL 4

EXTERN uchar camera_to_region_mapping[];
EXTERN uchar region_to_camera_mapping[];

EXTERN int vm_avail_regions;
EXTERN int vm_cur_region; 
EXTERN int vm_cur_camera;

EXTERN void vm_set_region_canvas(int r);
EXTERN void vm_unset_region_canvas(void);


#define VIEW_AVAIL(x) (vm_avail_regions & (1 << (x)))
#define VIS_CAM(x)  (vm_avail_regions & (1 << camera_to_region_mapping[x]))


#endif // ___VIEWMGR_H
