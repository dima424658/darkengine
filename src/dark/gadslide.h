/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifndef __GADSLIDE_H
#pragma once
#define __GADSLIDE_H

#define LGSLIDER_HORIZONTAL   0
#define LGSLIDER_VERTICAL     1


#define LGSLIDER_NORMAL    0
#define LGSLIDER_DEPRESSED 1
#define LGSLIDER_DISABLED  2
#define LGSLIDER_HIGHLIT   3

typedef struct {
   TOGGLE_GUTS;
   short screenRange;
   char orient;
   int state;
   short handleSpan;
   // Internal stuff::
   grs_bitmap saveUnder;
   Point suPos;
   short handlePos;
   short oldHandlePos;
   short grabPos;
   bool bSkipSaveUnder;
} LGadSlider;


extern LGadSlider *LGadCreateSlider(LGadSlider *vs, LGadRoot *vr, short x, short y, short w, short h, char paltype);
extern LGadSlider *LGadCreateSliderArgs(LGadSlider *vs, LGadRoot *vr, short x, short y, short w, short h, 	
   LGadButtonCallback bfunc, int *val_ptr, short max_val, 
	short increm, short screenRange, char orient, char paltype);
extern int LGadDestroySlider(LGadSlider *vb, bool free_self);
extern void LGadInitSlider(LGadSlider *vs);
extern void LGadSliderSetNoSaveUnder(LGadSlider *vs);  
extern void SliderDraw(void *, LGadBox *vb);
#endif
