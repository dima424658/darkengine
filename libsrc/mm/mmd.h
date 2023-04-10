// $Header: x:/prj/tech/libsrc/mm/RCS/mmd.h 1.3 1998/05/05 15:48:55 kate Exp $

#ifndef __MMD_H
#define __MMD_H

#include <mms.h>
#include <fix.h>

void mm_dbg_draw_joints(mms_model *m);

// transforms model vertices and draws them as disks,
// color-coded by segment.  if index is non-negative, only draws that
// segments vertices, otherwise shows all. r is disk radius
void mm_dbg_draw_seg_colored_verts(mms_model *m,int index,fix r);

// transforms model vertices and draws them as disks
// c1 and c2 are colors to use, depending on whether vert is a dup
// r is desired radius of drawn vertices
void mm_dbg_draw_verts(mms_model *m,uchar c1, uchar c2, fix r);

#endif
