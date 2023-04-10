// $Header: x:/prj/tech/libsrc/ui/RCS/textgadg.h 1.9 1998/06/18 13:29:52 JAEMZ Exp $

#ifndef __TEXTGADG_H
#define __TEXTGADG_H
#pragma once


// Includes
#include <lg.h>  // every file should have this
#include <slider.h>
#include <texttool.h>
#include <tngtextg.h>

// C Library Includes

// System Library Includes

// Master Game Includes

// Game Library Includes

// Game Object Includes

// Defines

// Prototypes

// Creates a text gadget of size dim.
EXTERN Gadget *gad_text_create(Gadget *parent, Rect *dim, int z, ulong options, TNGStyle *sty, char *name);

EXTERN Gadget *gad_textgadget_create_from_tng(void *ui_data, Point loc, TNG **pptng, TNGStyle *sty, ulong options, Point size);
// Globals

#endif // __TEXTGADG_H






