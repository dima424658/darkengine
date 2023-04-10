/*
 * $Source: x:/prj/tech/libsrc/matrix/RCS/matrixs.h $
 * $Revision: 1.10 $
 * $Author: JAEMZ $
 * $Date: 1998/06/18 13:07:09 $
 *
 * Matrix library structures and constructors
 */


#ifndef __MATRIXS_H
#define __MATRIXS_H
#pragma once

#include <types.h>

#include <mxang.h>

// it is unclear what the "right thing" to do is
// #include <float.h>
// #define MXS_REAL_MAX FLT_MAX
// i think, however, we want everyone to just use floats, sadly, in the new age
// in particular, almost no one uses this, so it is out for now

typedef float mxs_real;

typedef struct mxs_vector {
   union {
      struct {mxs_real x,y,z;};
      mxs_real el[3];
   };
} mxs_vector;

typedef struct mxs_plane {
   union {
       mxs_real el[4];
       struct {
          union {
             struct {mxs_real x,y,z; };
             struct {mxs_vector v; };
          };
          mxs_real d;
      };
   };
} mxs_plane;

typedef struct mxs_matrix {
   union {
      struct {mxs_real m0,m1,m2,m3,m4,m5,m6,m7,m8;};
      struct {mxs_vector vec[3];};
      mxs_real el[9];
   };
} mxs_matrix;

typedef struct mxs_trans {
   mxs_matrix mat;
   mxs_vector vec;
} mxs_trans;

#define mx_mk_mat(matptr,m1blorf,m2blorf,m3blorf,m4blorf,m5blorf,m6blorf,m7blorf,m8blorf,m9blorf) \
   ((matptr)->el[0]=(m1blorf),(matptr)->el[1]=(m2blorf),(matptr)->el[2]=(m3blorf), \
   (matptr)->el[3]=(m4blorf),(matptr)->el[4]=(m5blorf),(matptr)->el[5]=(m6blorf), \
   (matptr)->el[6]=(m7blorf),(matptr)->el[7]=(m8blorf),(matptr)->el[8]=(m9blorf))

#define mx_mk_vec(vecptr,xblorf,yblorf,zblorf) \
   ((vecptr)->x=(xblorf),(vecptr)->y=(yblorf),(vecptr)->z=(zblorf))

#define mx_mk_angvec(vecptr,xblorf,yblorf,zblorf) \
   ((vecptr)->tx=(xblorf),(vecptr)->ty=(yblorf),(vecptr)->tz=(zblorf))

#define mx_mk_trans(tptr,m1blorf,m2blorf,m3blorf,m4blorf,m5blorf,m6blorf,m7blorf,m8blorf,m9blorf,xblorf,yblorf,zblorf) \
   ((tptr)->mat.el[0]=(m1blorf),(tptr)->mat.el[1]=(m2blorf),(tptr)->mat.el[2]=(m3blorf), \
   (tptr)->mat.el[3]=(m4blorf),(tptr)->mat.el[4]=(m5blorf),(tptr)->mat.el[5]=(m6blorf), \
   (tptr)->mat.el[6]=(m7blorf),(tptr)->mat.el[7]=(m8blorf),(tptr)->mat.el[8]=(m9blorf), \
   (tptr)->vec.x=(xblorf),(tptr)->vec.y=(yblorf),(tptr)->vec.z=(zblorf))

#endif /* __MATRIXS_H */
