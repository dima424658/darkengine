// $Header: x:/prj/tech/libsrc/ui/RCS/slider.h 1.13 1998/06/18 13:29:19 JAEMZ Exp $

#ifndef __SLIDER_H
#define __SLIDER_H
#pragma once


// Includes
#include <lg.h>  // every file should have this
#include <tngslidr.h>
#include <gadgets.h>

// C Library Includes

// System Library Includes

// Master Game Includes

// Game Library Includes

// Game Object Includes

// Defines
// #define SL_RES_LEFT   REF_IMG_bmCursorLeft
// #define SL_RES_RIGHT  REF_IMG_bmCursorRight
// #define SL_RES_UP     REF_IMG_bmCursorUp
// #define SL_RES_DOWN   REF_IMG_bmCursorDown
// #define SL_RES_SLIDER REF_IMG_bmHelmTinyHead

// #define SL_RES_LEFT   REF_IMG_iconArrowLt
// #define SL_RES_RIGHT  REF_IMG_iconArrowRt
// #define SL_RES_UP     REF_IMG_iconArrowUp
// #define SL_RES_DOWN   REF_IMG_iconArrowDn
// #define SL_RES_SLIDER REF_IMG_bmHelmTinyHead

#define SL_VERTICAL  TNG_SL_VERTICAL
#define SL_HORIZONTAL TNG_SL_HORIZONTAL

#define SL_VALUE(x) ((TNG_slider *)((x)->tng_data->type_data))->value
#define SL_ALIGNMENT(x) ((TNG_slider *)((x)->tng_data->type_data))->alignment
#define SL_MAX(x) ((TNG_slider *)((x)->tng_data->type_data))->max
#define SL_MIN(x) ((TNG_slider *)((x)->tng_data->type_data))->min
#define SL_INCREM(x) ((TNG_slider *)((x)->tng_data->type_data))->increm
#define SL_STYLE(x) ((TNG_slider *)((x)->tng_data->type_data))->style
#define SL_CHANGE(x) ((TNG_slider *)((x)->tng_data->type_data))->change_flag

// Prototypes
EXTERN Gadget *gad_slider_create(Gadget *parent, Rect *dim, int z, int alignment, int increment,
   int min, int max, TNGStyle *sty, char *name);

EXTERN Gadget *gad_slider_create_full(Gadget *parent, Rect *dim, int z, int alignment, int increment,
   int min, int max, TNGStyle *sty, char *name, Ref res_left, Ref res_right, Ref res_up,
   Ref res_down, Ref res_slider);

EXTERN Gadget *gad_slider_create_from_tng(void *ui_data, Point loc, TNG **pptng, TNGStyle *sty, int alignemnt, int min, int max,
   int value, int increment, Point size);

// Globals

#endif // __SLIDER_H

