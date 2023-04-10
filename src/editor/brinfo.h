/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/brinfo.h,v 1.15 2000/01/29 13:11:12 adurant Exp $
// brinfo has the nasty internal types and defines for gedit
#pragma once

#ifndef __BRINFO_H
#define __BRINFO_H

#include <brtypes.h>


// media works by making 0->N mean terrain medias, else - of the brType_ above
// ie -1 is Light, -2 Hotregion, and so on
// so you need to use brushGet and Set type to do this
EXTERN int brushSetType(struct _editBrush *br, int type);
EXTERN int brushGetType(struct _editBrush *br);

////////////// EDGE/POINT SELECT
// setup of the edges

#define brSelect_Edge(cb)   ((cb)->edge)
#define brSelect_Point(cb)  ((cb)->point)
#define brSelect_Flag(cb)   ((cb)->use_flg)
#define brSelect_Group(cb)  ((cb)->group_id)

#define brSelect_EDGE       (1<<1)   // if on, means we use an edge, else a point
#define brSelect_COMPLEX    (1<<2)   // if on, means edge or point, not face or whole
#define brSelect_VIRTUAL    (1<<3)

/////////////// LIGHTS
// mapping to brush fields, then defines

#define brLight_Bright(cb)    ((cb)->sz.el[0])
//#define brLight_Focal(cb)   ((cb)->sz.el[1])
//#define brLight_Ambi(cb)    ((cb)->sz.el[2])
#define brLight_Hue(cb)       ((cb)->sz.el[1])
#define brLight_Saturation(cb) ((cb)->sz.el[2])
#define brLight_Type(cb)      ((cb)->num_faces)
#define brLight_Handle(cb)    ((cb)->primal_id)
#define brLight_Script(cb)    ((cb)->tx_id)
#define brLight_isSpot(cb)    (brLight_Type(cb)&1)


/////////////// HOTREGIONS (areas)
// mapping to brush fields, then defines

// status of hot region setup and such
#define brHot_Status(cb)      ((cb)->tx_id)
#define brHot_MEONLY          2
#define brHot_ACTIVE          1
#define brHot_INACTIVE        0

#define brHot_BASEMASK        1
#define brHot_MEMASK          2

// accesors for hot regions
#define brHot_IsMEONLY(cb)    (brHot_Status(cb)&brHot_MEMASK)
#define brHot_GetBase(cb)     (brHot_Status(cb)&brHot_BASEMASK)

#define brHot_TOGACTIVE       (-2)  // just for setup calls, never store this
#define brHot_TOGMEONLY       (-1)

/////////////// OBJECTS
#define brObj_Type(cb)      ((cb)->num_faces)
#define brObj_ID(cb)        ((cb)->primal_id)

/////////////// FLOW
#define brFlow_Index(cb)    ((cb)->tx_id)

/////////////// ROOM
#define brRoom_ObjID(cb)           ((cb)->primal_id)
#define brRoom_InternalRoomID(cb)  ((cb)->tx_id)

////////////// MISC INTERNALS

// this is for creating or copying a brush - or "recreating" it
// new_brush means we are creating it from nothing, in which case we try
//  and use last to set defaults (unless last is NULL or of a different type)
// in the recreate case, we just update US and make sure any changes that 
// must occur do happen

#define brField_Old 0
#define brField_New (1 << 0)
#define brField_Clone (1 << 1)

EXTERN void brush_field_update(struct _editBrush *us, struct _editBrush *last, ulong flags);

// when deleting a brush, will do any special de-setup that is needed
EXTERN void brush_field_delete(struct _editBrush *us);

#endif  // __BRINFO_H





