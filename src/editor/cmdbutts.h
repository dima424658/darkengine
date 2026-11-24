/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/cmdbutts.h,v 1.2 2000/01/29 13:11:21 adurant Exp $
#pragma once

#ifndef __CMDBUTTS_H
#define __CMDBUTTS_H

#include <gadget.h>
#include <gadblist.h>

//
// Tool for laying out rectangles
// 
// fill rvec with a matrix of rectangles whose matrix dimensions are dims
// with spacing between each rect.

EXTERN void LayoutRectangles(Rect* bounds, Rect rvec[], int num_rects, Point dims, Point spacing);

//
#endif
