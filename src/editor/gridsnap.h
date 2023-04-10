/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/gridsnap.h,v 1.4 2000/01/29 13:12:19 adurant Exp $
#pragma once

// grid functions

#ifndef __GRIDSNAP_H
#define __GRIDSNAP_H

typedef struct
{
   mxs_real line_spacing;
   mxs_vector phase_shift;   
   mxs_angvec orientation;
   bool grid_enabled;
} Grid;

 // @HACK: perhaps this should be less goofy than just here, eh?
EXTERN Grid cur_global_grid;

void gedit_vertex_snap(Grid *g, mxs_vector *dest, mxs_vector *src);

 //  prime_axis is axis to draw grid perpendicular to
 //  height is location along that axis at which to place grid
 //  start,end are corners of a bounding box of the area to engrid
void gedit_render_grid(Grid *g, int prime_axis, mxs_real height,
                       mxs_vector *start, mxs_vector *end);

#endif
