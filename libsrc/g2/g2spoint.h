// $Header: x:/prj/tech/libsrc/g2/RCS/g2spoint.h 1.2 1997/01/31 13:34:22 KEVIN Exp $
//
// 2d point structure for polygons.

#ifndef __G2SPOINT_H
#define __G2SPOINT_H

#include <fix.h>

typedef struct g2s_point {
   fix sx, sy;
   float w;
   ulong flags;
   union {
      float coord[32];
      struct {
         float i,u,v,h,d;
      };
   };
} g2s_point; // 140 bytes.  ick.

#endif
