/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/viewmgr.h,v 1.20 2000/01/29 13:13:25 adurant Exp $
#pragma once

#ifndef __VIEWMGR_H
#define __VIEWMGR_H

#include <wr.h>

#ifdef __cplusplus
extern "C" {
#endif 

void vm_init(void);
void vm_term(void);

EXTERN void vm_suspend(void);
EXTERN void vm_resume(void); 

void vm_init_cameras(void);
void vm_init_camera(int c);

void vm_cycle_camera(bool up);
void vm_disable_region(int r);
void vm_enable_region(int r);
void vm_enable_all_regions(void);
void vm_swap_cameras(int x, int y);
mxs_vector *vm_cur_loc(void);
void vm_cur_loc_set(mxs_vector *loc);
bool vm_cur_3d(void);
void vm_cur_slew(mxs_vector *slew);
void vm_cur_scroll(mxs_real x, mxs_real y);
void vm_cur_rotate(mxs_angvec *rot);
void vm_cur_level (int axis);
void vm_cur_cycle_mode(void);
void vm_cur_toggle_mode(uchar toggle);
void vm_cur_toggle_3d(void);
void vm_cur_toggle_persp(void);
void vm_cur_toggle_synch(void);

// various levels of redraw of the windows
void vm_redraw(void);
void vm_redraw_from_select(void);
void vm_redraw_from_camera(void);
void vm_redraw_from_new3d(void);

// 3d setup/control
void vm_start_3d(int c);
void vm_end_3d(int c);
// gets 3d camera location for the current started 3d
// if you call this at a bad time, sad day for you
void vm_get_3d_camera_loc(mxs_vector **pos, mxs_angvec **ang);

// setups
void vm_cur_scale(mxs_real scale);
void vm_scale_all(mxs_real scale);
void vm_fit_cameras_to_region(mxs_vector *, mxs_vector *);

// rendering a camera
void vm_render_camera(int c);
void vm_render_cameras(void);
// grid control for now - used by render to decide about grids
EXTERN bool vm_show_grid, vm_show_grid_3d;

// returns whether the pick worked or not
// if so, the two vm_pick variables will be set validly
BOOL vm_pick_camera(int c, int x, int y, int *brush, int *face);

bool vm_spotlight_loc(mxs_vector **loc, mxs_angvec **ang);

// so you dont need to know about the canvas<->region mapping silliness
void vm_set_camera_canvas(int c);
void vm_unset_camera_canvas(void);

// to set the currently selected region
void vm_set_cur_region(int n);
int vm_get_cur_region(void);

// camera set/get
void vm_set_cur_camera(int);
int vm_current_camera(void);

// rendering mode defines
#define RM_WIREFRAME_BRUSHES 1
#define RM_SOLID_PORTAL 2
#define RM_WIREFRAME_CURRENT 4
#define RM_WIREFRAME_PORTAL 8

// low-level accessing methods, specify a camera

bool        vm_get_3d      (int c);
mxs_vector *vm_get_location(int c);
mxs_angvec *vm_get_facing  (int c);
mxs_real    vm_get_zoom    (int c);
mxs_real    vm_get_scale   (int c);
mxs_vector *vm_get_loc_2d  (int c);
bool        vm_get_synch   (int c);
int         vm_get_mode    (int c);

void vm_set_3d(int c, bool flag);
void vm_set_render_mode(int c, uchar mode);
void vm_set_location(int c, mxs_vector *loc);
void vm_set_facing(int c, mxs_angvec *fac);
void vm_set_zoom(int c, mxs_real zoom);
void vm_set_perspective(int c, bool flag);
void vm_set_scale(int c, mxs_real scale);
void vm_set_axis(int c, int a);
void vm_set_synch(int c, bool flag);

// utility for world location stuff
int vm_map_screen_to_world(int c, mxs_vector *dest, int x, int y);
bool vm_screen_axes(int c, mxs_vector *x, mxs_vector *y);
int vm_normal_axis(int c);
// returns the bogus axis, fills in which axis generally for x and y
int vm_get_camera_axis(int c, int *x, int *y);
// get screen size of cameras region
BOOL vm_camera_scr_size(int c, int *w, int *h);

// pick colors
void vm_pick_colors(void); 

#ifdef __cplusplus
}
#endif

#endif


/*
Local Variables:
typedefs:("mxs_angvec" "mxs_real" "mxs_vector" "uchar")
End:
*/
