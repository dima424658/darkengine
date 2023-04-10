// $Header: x:/prj/tech/libsrc/ui/RCS/butarray.h 1.12 1998/06/18 13:28:05 JAEMZ Exp $

#ifndef __BUTARRAY_H
#define __BUTARRAY_H
#pragma once

// Includes
#include <lg.h>  // every file should have this
#include <slider.h>
#include <tngbarry.h>

// C Library Includes

// System Library Includes

// Master Game Includes

// Game Library Includes

// Game Object Includes

// Defines

// Prototypes

// Creates a button array.  The initial button array is empty, each of the buttons must be added with
// _addbutton or _addbutton_at.  To delete a button, just add a button of type NULL_TYPE there.  The msize_
// and window_ values  indicate the size of the button matrix and the window onto it, respectively.  The units
// of those values is in "buttons".  The spacing parameter determines how much blank space is between the buttons,
// and the bsize_ parameters describe the size of each of the individual buttons.  The overall size of the
// buttonarray is automatically computed from these parameters.  The upper left of the button array is
// indicated by the coord parameter.  If NULL is passed as the style parameter, the default style is used.
EXTERN Gadget *gad_buttonarray_create(Gadget *parent, Point coord, int z, int msize_x, int msize_y, int window_x, int window_y,
   int bsize_x, int bsize_y, int num_sel, ushort options, TNGStyle *sty, char *name);

// Fills in a slot in the button array with a button of type type, with display information disp_data.
// The button will occupy the first empty slot.
EXTERN errtype gad_buttonarray_addbutton(Gadget *g, int type, void *disp_data);

// Like gad_buttonarray_addbutton but allows specification of the x and y coordinates within the 
// button matrix for the new button.
EXTERN errtype gad_buttonarray_addbutton_at(Gadget *g, int type, void *disp_data, int coord_x, int coord_y);

// Set the offset of the buttonarray to an arbitrary x & y coordinate
EXTERN errtype gad_buttonarray_setoffset(Gadget *g, int offset_x, int offset_y);

// Globals

#endif // __BUTARRAY_H

