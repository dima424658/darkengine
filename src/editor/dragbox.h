/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/dragbox.h,v 1.2 2000/01/29 13:11:27 adurant Exp $
#pragma once

#ifndef __DRAGBOX_H
#define __DRAGBOX_H
#include <editbr.h>
#include <vslider.h>

EXTERN void* brushVSsetupSz(floatSlider *s, editBrush* br, int axis, float scale);
EXTERN void* brushVSsetupPos(floatSlider *s, editBrush* br, int axis, float scale);
EXTERN void* brushVSsetupAng(fixangSlider *s, editBrush* br, int axis);

#endif // __DRAGBOX_H
