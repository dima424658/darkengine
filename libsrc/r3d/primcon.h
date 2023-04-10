// $Header: x:/prj/tech/libsrc/r3d/RCS/primcon.h 1.5 1996/06/06 19:08:43 JAEMZ Exp $

#ifndef __PRIMCON_H
#define __PRIMCON_H

#include <r3ds.h>

typedef struct
{
   ulong color;

   r3s_texture bm;    // default texture if poly

      // flags which indicate what function we use
   uchar clip_2d;

   uchar point_flags;
   uchar line_flags;
   uchar poly_flags;

   uchar bm_type;  // plus transparency flag
} r3s_prim_context;

#endif
