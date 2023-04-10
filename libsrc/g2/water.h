#ifndef __WATER_H
#define __WATER_H

#include <dev2d.h>
#include <plytyp.h>

#define GRC_NOT_WATER 16

typedef struct {
   int horizon;
   uchar **clut_list;
   int *offset_table;
} grs_water;

extern void gr_water_floor_umap(grs_bitmap *bm, int n, grs_vertex **vpl, grs_water *w);

#endif
