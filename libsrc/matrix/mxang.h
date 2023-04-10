// $Header: x:/prj/tech/libsrc/matrix/RCS/mxang.h 1.3 1999/04/22 10:41:38 JAEMZ Exp $
// angle/angvec stuff for mx
// since it is single/double precision invariant

#ifndef __MXANG_H
#define __MXANG_H

typedef ushort mxs_ang;

typedef struct mxs_angvec {
   union {
      struct {mxs_ang tx,ty,tz;};
      mxs_ang el[3];
   };
} mxs_angvec;

#define MX_ANG_PI 0x8000
#define MX_REAL_PI 3.14159265358979323846
#define MX_REAL_2PI (2.0*MX_REAL_PI)

#endif  // __MXANG_H
