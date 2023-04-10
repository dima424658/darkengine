// $Header: x:/prj/tech/libsrc/ui/RCS/pushbutt.h 1.9 1998/06/18 13:29:11 JAEMZ Exp $

#ifndef __PUSHBUTT_H
#define __PUSHBUTT_H
#pragma once

// Includes
#include <lg.h>  // every file should have this
#include <tngpushb.h>
#include <gadgets.h>

// C Library Includes

// System Library Includes

// Master Game Includes

// Game Library Includes

// Game Object Includes

// Defines

#define PB_TYPE(x) ((TNG_pushbutton *)((x)->tng_data->type_data))->type
#define PB_DISPDATA(x) ((TNG_pushbutton *)((x)->tng_data->type_data))->disp_data
#define PB_FONTBUF(x) ((TNG_pushbutton *)((x)->tng_data->type_data))->font_buf
#define PB_PRESSED(x) ((TNG_pushbutton *)((x)->tng_data->type_data))->pressed

// Prototypes

// Creates a pushbutton, which can display stuff and respond to basic events.
EXTERN Gadget *gad_pushbutton_create(Gadget *parent, Rect *dim, int z, int type, void *disp_data, TNGStyle *sty, char *name);

EXTERN Gadget *gad_pushbutton_create_from_tng(void *ui_data, Point loc, TNG **pptng, TNGStyle *sty, int button_type, void *display_data, Point size);

// Globals

#endif //  __PUSHBUTT_H



