// $Header: x:/prj/tech/libsrc/r3d/RCS/clip.h 1.13 1998/06/18 11:25:48 JAEMZ Exp $

#include <r3ds.h>

#ifndef __CLIP_H
#define __CLIP_H

typedef enum {
   R3_INSIDE=0,
   R3_OUTSIDE=1,
   R3_CROSSES=2
} r3e_clipinfo;

typedef enum {
   R3_CLIP,        // do all clipping
   R3_NO_CLIP,     // do no clipping
   R3_USER_CLIP    // don't view clip, but do user clipping
} r3e_clipmode;

EXTERN void r3_set_clipmode(r3e_clipmode e);
EXTERN r3e_clipmode r3_get_clipmode();
EXTERN void r3_set_clip_flags(int flags);
EXTERN int r3_get_clip_flags(void);
EXTERN void r3_set_near_plane(mxs_real dist);
EXTERN mxs_real r3_get_near_plane();

// force recalculation of internal clip state
EXTERN void r3_set_clip(void);

EXTERN void r3_std_code_points(int n, r3s_point *src);

#endif // __CLIP_H
