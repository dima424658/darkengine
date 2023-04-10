/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/hsrc/RCS/grspoint.h 1.7 1997/10/01 13:38:46 KEVIN Exp $

#ifndef __GRSPOINT_H
#define __GRSPOINT_H

#include <matrixs.h> // mxs_real
#include <fix.h>     // because of fix

// new 2D point structure stand-in

typedef struct {
   fix sx, sy;
   mxs_real w;
   ulong flags;
   mxs_real i,u,v;
} grs_point;

// Fake definition of a texture

typedef struct grs_bitmap grs_bitmap;
typedef grs_bitmap *r3s_texture;

#endif // __GRSPOINT_H
