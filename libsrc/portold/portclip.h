/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/portold/portclip.h,v 1.7 1997/09/11 22:56:54 buzzard Exp $
#include <r3ds.h>

#ifndef __PORTCLIP_H
#define __PORTCLIP_H

  // Clipping abstract data type--only portclip sees inside it
typedef struct st_ClipData ClipData;

  // get a clipping region the size of the screen
extern ClipData *PortalClipRectangle(int l, int t, int r, int b);

  // get a clipping region from a polygon and bounded by another clip region
extern ClipData *PortalClipFromPolygon(int n, r3s_phandle *p, ClipData *clipsrc);

  // check if a clipping region overlaps a bounding octagon
extern bool PortalClipOverlap(ClipData *c, fix *min2d, fix *max2d);

  // get a clipping region which is the union of two other clipping regions
extern bool PortalClipUnion(ClipData *src, ClipData *s2);

  // free a clipping region gotten from one of the above three functions
extern void PortalClipFree(ClipData *c);

  // test if a given point is unclipped
extern bool PortClipTestPoint(ClipData *c, fix x, fix y);

  // free all clipping regions, initialize for a new pass
extern void PortalClipInit(void);

  // clip a polygon to a portal
extern int portclip_clip_polygon(int n, r3s_phandle *p, r3s_phandle **q, ClipData *c);

extern bool clip_lighting;

#define C_CLIP_UV_OFFSET  8
extern int clip_uv;            // set to C_CLIP_UV_OFFSET if needed

// probably should just provide a function
//     set_clip_uv(bool)
// which hides the details internally...

#endif
