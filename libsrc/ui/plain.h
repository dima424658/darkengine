// $Header: x:/prj/tech/libsrc/ui/RCS/plain.h 1.3 1998/06/18 13:28:47 JAEMZ Exp $

#ifndef __PLAIN_H
#define __PLAIN_H
#pragma once

// Includes
#include <lg.h>  // every file should have this
#include <tngplain.h>
#include <gadgets.h>

// C Library Includes

// System Library Includes

// Master Game Includes

// Game Library Includes

// Game Object Includes

// Defines

// Prototypes

// Creates a totally plain, boring TNG which is little more than
// a region in gadget's clothing
EXTERN Gadget *gad_plain_create(Gadget *parent, Rect *dim, int z, char *name);

EXTERN Gadget *gad_plain_create_from_tng(void *ui_data, Point loc, TNG **pptng, Point size);

// Globals

#endif //  __PLAIN_H




