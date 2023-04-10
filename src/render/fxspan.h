/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $ Header: $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   fxspan.h

   export for fxspan.c--simple span buffer for special effects

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _FXSPAN_H_
#define _FXSPAN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <lg.h>
#include <fix.h>

#include <fxspant.h>

// When a span is off the screen, we put this into its "left" field.
// No baseball puns, please.
#define SPAN_NOT_USED -1

// This clips a set of spans against the clipping info for a given
// canvas.  We return FALSE if all the spans were off the canvas, TRUE
// if there's anything left.  x and y are the upper-left corner of the
// sFXSpanSet within the canvas.
extern bool FXSpanClipToCanvas(sFXSpanSet *set, grs_canvas *canvas, 
                               fix x, fix y);

extern bool FXSpanClipToSet(sFXSpanSet *input_set, sFXSpanSet *clip_set,
                            sFXSpanSet *output_set_1,
                            sFXSpanSet *output_set_2,
                            fix x, fix y);


// The bounding width is however far over any used span reaches.
extern void FXSpanSetBoundingWidth(sFXSpanSet *set);

// The span buffer is expected to be clipped in x and y by the time
// this gets it.
extern void FXSpanDrawSetFromSnarf(sFXSpanSet *set, sFXSpanSnarf *snarf, 
                                   grs_canvas *canvas, 
                                   fix x_in_canvas, fix y_in_canvas,
                                   fix x_in_snarf, fix y_in_snarf);


extern void FXSpanDrawSetFromCanvas(sFXSpanSet *set, grs_canvas *canvas, 
                                    fix x_start, fix y_start,
                                    fix x_end, fix y_end);

// This is mostly for testing.
extern void FXSpanDrawFlatColor(sFXSpanSet *set, sFXSpanSnarf *snarf, 
                                grs_canvas *canvas, 
                                fix x_in_canvas, fix y_in_canvas,
                                fix x_in_snarf, fix y_in_snarf,
                                uchar color);

// FXSpanSnarfSetup allocates the necessary memory for snarfing, so you
// won't have to!
void FXSpanSnarfSetup(sFXSpanSnarf *snarf, int x, int y, grs_canvas *canvas);

extern void FXSpanSnarf(sFXSpanSnarf *snarf, sFXSpanSet *set, 
                        grs_canvas *canvas, fix x, fix y);

// FXUnsnarf deallocates the memory allocated by FXSpanSnarf.  snarf
// itself is not freed since we don't know whether it's from the free
// store.
extern void FXSpanUnsnarf(sFXSpanSnarf *snarf);

#ifdef __cplusplus
};
#endif

#endif // ~_FXSPAN_H_
