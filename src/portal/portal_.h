/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/portal/portal_.h,v 1.11 2000/01/29 13:37:23 adurant Exp $
#pragma once

#ifndef __PORTAL__H
#define __PORTAL__H

#include <r3d.h>
#include <port.h>
#include <portclip.h>
#include <pt_clut.h>

struct port_render_data
{
   ClipData *clip;     // portal clipping information for view into here
   r3s_point *points;  // transformed points
   float dist;         // average center point of this place
   ClutChain clut;     // how do we clut this region, if at all?
   float zwater;       // z depth at which we encountered water
   uchar visit;        // have we already visited (traversed out of) this cell
   uchar num_outgoing_portals;
   uchar cell_flags;   // union of flags of cells used to reach this cell
   uchar num_unexplored_entries;
   int objs;
   int outgoing_portal_offset;
   int need_clip;
   int sorted_index;
};

#define CLIP_DATA(x)   ((x)->render_data->clip)
#define POINTS(x)      ((x)->render_data->points)
#define VISIT(x)       ((x)->render_data->visit)
#define DIST(x)        ((x)->render_data->dist)
#define CLUT(x)        ((x)->render_data->clut)
#define ZWATER(x)      ((x)->render_data->zwater)
#define OBJECTS(x)     ((x)->render_data->objs)
#define NUM_OUTGOING(x) ((x)->render_data->num_outgoing_portals)
#define FIRST_OUTGOING(x)  ((x)->render_data->outgoing_portal_offset)
#define NUM_INCOMING(x) ((x)->render_data->num_unexplored_entries)
#define NEED_CLIP(x)   ((x)->render_data->need_clip)
#define CELL_FLAGS(x)  ((x)->render_data->cell_flags)

  // as we iterate over surfaces, we keep the
  // following variables loaded up so we don't
  // have to always pass them around
extern r3s_point *cur_ph;
extern Vector *cur_pool;
extern uchar *r_vertex_list, *r_vertex_lighting;
extern void *r_clip;

// Here's the screen dimensions, stored in duv.c.
extern float xsci, ysci;

typedef short ObjVisibleID;

typedef struct ObjVisible
{
   ObjID obj;
   short next_visobj;
   union {
      ulong fragment;
      void *cell;
   };
} ObjVisible;

extern ObjVisible vis_objs[];

#endif

