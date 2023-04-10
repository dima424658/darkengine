// $Header: x:/prj/tech/libsrc/dev2d/RCS/alpha.h 1.2 1996/12/06 15:29:30 KEVIN Exp $

#ifndef __ALPHA_H
#define __ALPHA_H

#include <matrixs.h>
typedef mxs_real alpha_real;

typedef struct alpha_vector {
   union {
      mxs_vector v;
      struct {
         alpha_real blue, green, red;
      };
   };
} alpha_vector;

#endif


