/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/raster.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/11 10:04:20 $
 *
 */

#ifndef __RASTER_H
#define __RASTER_H

#include <plytyp.h>

typedef struct {
   union {int left,top;};
   union {int right,bot;};
} grs_raster;

typedef struct grs_per_raster_struct grs_per_raster;
struct grs_per_raster_struct {
   union {int left,top;};
   union {int right,bot;};
   grs_per_raster *next;
};

extern grs_raster *gr_rasterize(grs_vertex **vpl, int n, grs_raster *r);
extern void gr_skew_raster(grs_raster *r, fix slope);
extern grs_per_raster **gr_flip_raster(grs_raster **pr);

#endif
