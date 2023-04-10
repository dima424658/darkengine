// $Header: x:/prj/tech/libsrc/r3d/RCS/primpoly.h 1.5 1998/06/18 11:26:51 JAEMZ Exp $
#ifndef _PRIMPOLY_H
#define _PRIMPOLY_H

#include <r3ds.h>

EXTERN int r3_check_clip_any(int n, r3s_phandle *vp, r3s_texture bm);
EXTERN int r3_check_any(int n, r3s_phandle *vp, r3s_texture bm);
EXTERN int r3_clip_any(int n, r3s_phandle *vp, r3s_texture bm);
EXTERN int r3_any(int n, r3s_phandle *vp, r3s_texture bm);

EXTERN int r3_check_clip_implicit_any(int n, r3s_phandle *vp);
EXTERN int r3_check_implicit_any(int n, r3s_phandle *vp);
EXTERN int r3_clip_implicit_any(int n, r3s_phandle *vp);
EXTERN int r3_implicit_any(int n, r3s_phandle *vp);

#endif
