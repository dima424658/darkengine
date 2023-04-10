// $Header: x:/prj/tech/libsrc/g2/RCS/permap.h 1.1 1997/01/31 13:39:04 KEVIN Exp $
// subdividing perspective mapper.

#ifndef __PERMAP_H
#define __PERMAP_H
#include <dev2d.h>
#include <g2spoint.h>
typedef void lmap_func(grs_bitmap *bm, int n, g2s_point **vpl);
extern void pmap_shell(grs_bitmap *bm, int n, g2s_point **vpl, int clip_type, lmap_func *f);
#endif
