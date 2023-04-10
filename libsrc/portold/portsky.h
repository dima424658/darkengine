/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/portold/portsky.h,v 1.1 1998/03/03 12:33:34 KEVIN Exp $

#ifndef __PORTSKY_H
#define __PORTSKY_H

extern int ptsky_type;

#define PTSKY_NORMAL    0  // treat sky exactly like other terrain
#define PTSKY_SPAN      1  // render sky polys into span buffer
#define PTSKY_ZBUFFER   2  // make sky polys render far away
#define PTSKY_NONE      3  // don't render sky polys at all

// set sky type
extern void ptsky_set_type(int type);

// render poly into span buffer.
// return number of scanlines in poly.
extern int ptsky_calc_spans(int n, r3s_phandle *vp);

// set callback for star rendering
extern void ptsky_set_render_func(void (*render_func)(int (*test_func)(int x, int y)));

// render span clipped stars using callback set above
extern void ptsky_render_stars(void);

#endif
