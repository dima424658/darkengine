/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
/*
 * $Source: r:/t2repos/thief2/src/editor/lgslider.h,v $
 * $Revision: 1.2 $
 * $Author: adurant $
 * $Date: 2000/01/29 13:12:21 $
 *
 *  A lisder button gadget (works with gadget.lib)
 *
 */

#ifndef __LGSLIDER_H
#define __LGSLIDER_H

#define LGSLIDER_HORIZONTAL   0
#define LGSLIDER_VERTICAL     1

#define LGSLIDER_DEFAULT  0
#define LGSLIDER_HIGHLIT  1
#define LGSLIDER_SELECTED 2

typedef struct {
   TOGGLE_GUTS;
   short       screenRange;
   char        orient;
   int         state;
   short       handleSpan;
   // Internal stuff::
   grs_bitmap  saveUnder;
   Point       suPos;
   short       handlePos;
   short       oldHandlePos;
   short       grabPos;
} LGadSlider;


extern LGadSlider *LGadCreateSlider(LGadSlider *vs, LGadRoot *vr, short x, short y, short w, short h, char paltype);
extern LGadSlider *LGadCreateSliderArgs(LGadSlider *vs, LGadRoot *vr, short x, short y, short w, short h, 	
   DrawElement *draw, LGadButtonCallback bfunc, int *val_ptr, short max_val, 
	short increm, short screenRange, char orient, char paltype);
int LGadDestroySlider(LGadSlider *vb, bool free_self);

#endif
