// $Header: x:/prj/tech/libsrc/g2/RCS/trirast.h 1.5 1998/04/28 14:29:18 KEVIN Exp $

#ifndef __TRIRAST_H
#define __TRIRAST_H

#include <g2spoint.h>
#include <plyparam.h>

extern int g2_triangle_raster_sort(g2s_point *v0, g2s_point *v1, g2s_point *v2, g2s_poly_params *tp);

#define PPF_I 1
#define PPF_U 2
#define PPF_V 4
#define PPF_H 8
#define PPF_D 16

#define PPF_UV    (PPF_U|PPF_V)
#define PPF_IUV   (PPF_I|PPF_UV)
#define PPF_IUVH  (PPF_IUV|PPF_H)
#define PPF_IUVHD (PPF_IUVH|PPF_D)
#define PPF_UVD   (PPF_UV|PPF_D)

#define PPF_MUNGE (1<<31)
#define PPF_MASK PPF_IUVHD // all supported deltas

#endif
