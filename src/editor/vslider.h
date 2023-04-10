/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/vslider.h,v 1.8 2000/01/29 13:13:29 adurant Exp $
// virtual slider header file
#pragma once

#ifndef __VSLIDER_H
#define __VSLIDER_H
#include <vsliderg.h>

///////////////////////////////////
// the real call

int virtual_slider(int n_axis, void **axis, void (*update_world)(void));
// values virtual_slider can return
#define VSLIDER_MOUSEUP (-1)   // means the mouse was already up when we were called
#define VSLIDER_ABORTED ( 0)   // user hit esc to cancel
#define VSLIDER_CHANGED ( 1)   // we changed the brush
#define VSLIDER_SAMEVAL ( 2)   // user used us, but never did anything

///////////////////////////////////
// actual typed stuff

// these generate groups will prototype
//   void *VSlider##type##Build(##type##Slider *s)
// as well as the ##type##Slider struct
// which will look like
//      bool (*parse)(float inp, void *data, int act);
//      type *val;
//      type lo,hi;
//      float scale;
// parse will be defaulted to the internal parser
// val lo and hi MUST be set
// if lo==hi then it is an infinite slider
// scale is multiplied in, defaults to 1.0, i guess

// if you want to write your own parse function
// inp is -1 to 1, data is the ##type##Slider struct, act is a define
#define SLIDER_START  (1)
#define SLIDER_SLIDE  (2)
#define SLIDER_END    (3)
#define SLIDER_ABORT  (4)
#define SLIDER_UPDATE (5)

GenerateVSliderType(float);
GenerateVSliderParse(float);
GenerateVSliderBuild(float);

GenerateVSliderType(fixang);
GenerateVSliderParse(fixang);
GenerateVSliderBuild(fixang);

GenerateVSliderType(int);
GenerateVSliderParse(int);
GenerateVSliderBuild(int);

GenerateVSliderType(short);
GenerateVSliderParse(short);
GenerateVSliderBuild(short);


#endif
