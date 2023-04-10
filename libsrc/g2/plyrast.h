// $Header: x:/prj/tech/libsrc/g2/RCS/plyrast.h 1.1 1996/10/31 15:38:04 KEVIN Exp $
// Polygon raster structure.

#ifndef __PLYRAST_H
#define __PLYRAST_H

#include <fix.h>

typedef struct g2s_raster {
   union {
      uchar *p;
      int y;
   };
   int x;
   int n;
   union {
      fix coord[32];
      struct {
         fix i,u,v,h,d;
      };
   };
} g2s_raster;

#endif
