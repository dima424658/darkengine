// $Header: x:/prj/tech/libsrc/ui/RCS/tngslidr.h 1.10 1998/06/18 13:31:32 JAEMZ Exp $

#ifndef __TNGSLIDR_H
#define __TNGSLIDR_H
#pragma once


// Includes
#include <lg.h>  // every file should have this
#include <tng.h>

// Typedefs
typedef struct {
   TNG *tng_data;
   Point size;
   int alignment;
   int min, max;
   int value, increm;
   bool dragging;
   Ref left_id, right_id, up_id, down_id;
   Ref slider_id;
} TNG_slider;

#define TNG_SL_HORIZONTAL  0
#define TNG_SL_VERTICAL    1

#define TNG_SL_INCREMENTER 0x0001
#define TNG_SL_DECREMENTER 0x0002
#define TNG_SL_SLIDER      0x0004
#define TNG_SL_ALLPARTS    TNG_ALLPARTS

#define TNG_SL_LEFT_KEY        0x4b
#define TNG_SL_RIGHT_KEY       0x4d
#define TNG_SL_UP_KEY          0x48
#define TNG_SL_DOWN_KEY        0x50

// Prototypes

// Initializes the TNG slider
EXTERN errtype tng_slider_init(void *ui_data, TNG *ptng, TNGStyle *sty, int alignment, int min, int max, int value, int increm, Point size);

// Initializes the TNG slider
EXTERN errtype tng_slider_full_init(void *ui_data, TNG *ptng, TNGStyle *sty, int alignment, int min, int max, int value, int increm, Point size,
   Ref left_id, Ref right_id, Ref up_id, Ref down_id, Ref slider_id);

// Deallocate all memory used by the TNG slider
EXTERN errtype tng_slider_destroy(TNG *ptng);

// Draw the specified parts (may be all) of the TNG slider at screen coordinates loc
// assumes all appropriate setup has already been done!
EXTERN errtype tng_slider_2d_draw(TNG *ptng, ushort partmask, Point loc);

// Fill in ppt with the size of the TNG slider
EXTERN errtype tng_slider_size(TNG *ptng, Point *ppt);

// Returns the current "value" of the TNG slider
EXTERN int tng_slider_getvalue(TNG *ptng);

// React appropriately for receiving the specified cooked key
EXTERN bool tng_slider_keycooked(TNG *ptng, ushort key);

// React appropriately for receiving the specified mouse button event
EXTERN bool tng_slider_mousebutt(TNG *ptng, uchar type, Point loc);

// React to a click at the given location
EXTERN bool tng_slider_apply_click(TNG *ptng, Point loc);

// Handle incoming signals
EXTERN bool tng_slider_signal(TNG *ptng, ushort signal);

EXTERN bool tng_slider_increm(TNG_slider *ptng);
EXTERN bool tng_slider_decrem(TNG_slider *ptng);
EXTERN errtype tng_slider_set(TNG_slider *ptng, int perc);

// Macros
#define TNG_SL(ptng) ((TNG_slider *)ptng->type_data)
#define TNG_SL_VALFRAC(psltng) ((float)(psltng->value - psltng->min)) / ((float)(psltng->max - psltng->min))

#endif // __TNGSLIDR_H




