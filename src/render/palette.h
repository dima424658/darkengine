/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/palette.h,v 1.5 2000/01/31 09:52:59 adurant Exp $
// dumb code for simple in editor palette switching
// very pathetic at the moment
#pragma once

EXTERN void pal_update(void);
EXTERN int pal_bind_color(int r, int g, int b);
EXTERN void pal_build_clut(ubyte (*pal)[3], ubyte* clut);
