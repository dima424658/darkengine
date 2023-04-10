// $Header: x:/prj/tech/libsrc/g2/RCS/g2clip.h 1.4 1997/02/04 11:23:45 KEVIN Exp $
#ifndef __G2CLIP_H
#define __G2CLIP_H
#include <plytyp.h>
#include <g2spoint.h>

enum {
   G2C_CLIP_NONE,
   G2C_CLIP_I,
   G2C_CLIP_RGB,
   G2C_CLIP_UV,  
   G2C_CLIP_UVI, 
   G2C_CLIP_UVW, 
   G2C_CLIP_UVWI,
   G2C_CLIP_UVWIH,
};

extern void g2_clip_fixup(int n, g2s_point **vpl);
extern int g2_clip_poly(int n, int flags, g2s_point **vpl, g2s_point ***pcvpl);
extern int g2_clip_polygon(int n, int flags, grs_vertex **vpl, grs_vertex ***pcvpl);

#endif
