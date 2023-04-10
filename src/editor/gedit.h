/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/gedit.h,v 1.38 2000/01/29 13:12:06 adurant Exp $
#pragma once

// gedit.c -  geometry editing tools

#ifndef __GEDIT_H
#define __GEDIT_H

#include <editbrs.h>

///////////////////
// initialize geom editor, specifically brush lists
EXTERN void gedit_init(void);
EXTERN void gedit_term(void);

// mode enter and exit, for when you enter and exit edit mode
EXTERN void gedit_enter(void);
EXTERN void gedit_exit(void);

///////////////////
// update/expose controls

// reportalize level if in auto-portalize mode, else set "editted"
EXTERN void gedit_db_update(BOOL redraw);  // redraw as well always

// so you can tell things they need to redraw
EXTERN void gedit_full_redraw(void);
// you changed to a new selection
EXTERN void gedit_change_selection(void);
// redraw selected brush, assuming no material changes (ie. selection changes only)
EXTERN void gedit_redraw_selection(void);
// for making changes which affect the world look, but not the brush look
EXTERN void gedit_redraw_3d(void);
// causes brush to know it needs to redraw, ie. material changes to cur brush
EXTERN void gedit_change_current_brush(void);

// teleport the camera to the current brush
EXTERN void gedit_cam_to_brush(void);

// if you need to know if the database has been editted
#ifdef EDITOR
EXTERN BOOL gedit_editted;
#else
#define gedit_editted FALSE
#endif 
// is the user allowing "lazy" updates, or are they anal
EXTERN bool gedit_allow_lazy_updates;

// these are basic hacks to set up lazy checked drags
// in particular, they check for lazy, and redraw main view without current brush
EXTERN void gedit_setup_check_lazy_drag(void);
EXTERN void gedit_finish_check_lazy_drag(void);

//////////////////
// actual brush/environment change controls

// raw change and refresh
EXTERN void gedit_raw_change_brush(editBrush *us, BOOL is_new, BOOL change_world, BOOL change_db);

// when you change a brush you really had better call this!!!
EXTERN void i_changed_the_brush(editBrush *us, BOOL snap, BOOL is_new, BOOL change_world);

// tell brush br you changed textures or alignments, returns whether it could change the db
EXTERN BOOL gedit_reassign_texture(editBrush *br, BOOL texture_only);
   
// brush size snapping wackiness, Null for current
EXTERN BOOL gedit_snap_brush(editBrush *us);   
EXTERN void gedit_set_snaps(mxs_real snap_pos, mxs_real snap_sz, mxs_ang snap_ang);

// call to see if the value in question is snapped on the current grid
EXTERN BOOL _gedit_float_snap(float *val);

// misc_setting means whether to go fill def fields, use_type cur create brush type
EXTERN BOOL gedit_place_brush(editBrush *us, BOOL misc_setting, BOOL use_type);

// if build_type is -1, then if old is non-null, use old's type, else type is left alone, else use build_type
// if old is non-Null, then misc_settings (old format) are assumed, and old is passed down to field_update

#define GEDIT_DEFAULT_BRTYPE -1
#define GEDIT_CREATE_AT_CUR (0)
#define GEDIT_CREATE_AT_END (1 << 0) // at_end means place new brush at end, else place at current
#define GEDIT_CREATE_CLONE  (1 << 1) // clone old brush, else create new using old as hint


EXTERN BOOL gedit_full_create_brush(editBrush *new_br, editBrush *old, ulong flags, int build_type);

// set current texture on an explicitly noted brush
EXTERN void gedit_texture_brush(editBrush *br, int new_tx);

//////////////////
// misc stuff

// sizing up the world
EXTERN mxs_vector *gedit_get_extent(BOOL hot_region_p);

// woo woo - select on click maybe
EXTERN BOOL gedit_click(int flags, int x, int y, int camera);
#define GEDIT_CLICK_IS3D      (1<<0)
#define GEDIT_CLICK_VBRUSH    (1<<1)
#define GEDIT_CLICK_FANCY     (1<<2)
#define GEDIT_CLICK_NOTOGGLE  (1<<3)

// return the default brush type, used for creation of new brushes
EXTERN int gedit_default_brush_type(void);

// set the same
EXTERN void gedit_set_default_brush_type(int type);

// if current brush is an object, floor it correctly
EXTERN void gedit_floor_brush(editBrush *us, mxs_vector *down);

////////////////
// 

#endif // __GEDIT_H
