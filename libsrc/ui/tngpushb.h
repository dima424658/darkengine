// $Header: x:/prj/tech/libsrc/ui/RCS/tngpushb.h 1.6 1998/06/18 13:30:50 JAEMZ Exp $

#ifndef __TNGPUSHB_H
#define __TNGPUSHB_H
#pragma once

// Includes
#include <lg.h>  // every file should have this
#include <tng.h>

// Typedefs
typedef struct {
   TNG *tng_data;
   Point size;
   int type;
   bool pressed;
   void *disp_data;
} TNG_pushbutton;

#define TNG_NULL_TYPE       -1
#define TNG_RESOURCE_TYPE    0
#define TNG_TEXT_TYPE        1
#define TNG_TRANSPARENT_TYPE 2
#define TNG_COLORED_TYPE     3

// Prototypes

// Initializes the TNG 
EXTERN errtype tng_pushbutton_init(void *ui_data, TNG *ptng, TNGStyle *sty, int button_type, void *display_data, Point size);

// Deallocate all memory used by the TNG 
EXTERN errtype tng_pushbutton_destroy(TNG *ptng);

// Draw the specified parts (may be all) of the TNG at screen coordinates loc
// assumes all appropriate setup has already been done!
EXTERN errtype tng_pushbutton_2d_draw(TNG *ptng, ushort partmask, Point loc);

// Fill in ppt with the size of the TNG 
EXTERN errtype tng_pushbutton_size(TNG *ptng, Point *ppt);

// Returns the current "value" of the TNG
EXTERN int tng_pushbutton_getvalue(TNG *ptng);

// React appropriately for receiving the specified cooked key
EXTERN bool tng_pushbutton_keycooked(TNG *ptng, ushort key);

// React appropriately for receiving the specified mouse button event
EXTERN bool tng_pushbutton_mousebutt(TNG *ptng, uchar type, Point loc);

// Handle incoming signals
EXTERN bool tng_pushbutton_signal(TNG *ptng, ushort signal);

EXTERN errtype tng_pushbutton_pressed(TNG_pushbutton *ppbtng);
EXTERN errtype tng_pushbutton_released(TNG_pushbutton *ppbtng);

// Macros
#define TNG_PB(ptng) ((TNG_pushbutton *)(ptng->type_data))

#endif // __TNGPUSHB_H



