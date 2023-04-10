// $Header: x:/prj/tech/libsrc/r3d/RCS/clipplan.h 1.3 1998/06/18 11:25:49 JAEMZ Exp $

#ifndef __CLIPPLAN_H
#define __CLIPPLAN_H

#include <matrix.h>

EXTERN void r3_clear_clip_planes(void);
EXTERN void r3_pop_clip_plane(void);
EXTERN void r3_push_clip_plane(mxs_plane *p);

#endif


