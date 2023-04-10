// $Header: x:/prj/tech/libsrc/ui/RCS/qboxgadg.h 1.12 1998/06/18 13:29:14 JAEMZ Exp $

#ifndef __QBOXGADG_H
#define __QBOXGADG_H
#pragma once


// So the basic concept behind quickboxes is as follows:  A simple method of quickly throwing together dialog boxes
// that allow displaying and accessing program data, ala the Underworld editor.  Each quickbox is composed of slots, each
// of which accesses a particular piece of data.  The basic data type for each slot needs to be set, as well as a
// text label for the slot and the variable itself.  In addition, a set of option flags can be OR-ed together for
// modified the format and attributes of the slot, such as being read-only, having sliders, etc.
// 
// Here is a quick example of what it takes to throw together a quickbox:
// Say we want to be able to edit some information on a critter.  We want to throw up a box which tells the name
// of the critter, and then has some stats that can be edited.
// 
// Gadget *qb, *parent_gadget;
// Critter crit;
// Point origin;
// int min, max;
// 
// qb = gad_qbox_start(parent_gadget, origin, NULL, "sample_qbox");
// gad_qbox_add("Critter Name", QB_TEXT_SLOT, crit.name, QB_RD_ONLY);
// gad_qbox_add("Hit Points", QB_INT_SLOT, &crit.hp, QB_NO_OPTION);
// gad_qbox_add("Armor Class", QB_INT_SLOT, &crit.ac, QB_ARROWS);
// gad_qbox_add_parm("Movement Class", QB_INT_SLOT, &crit.moveclass, QB_SLIDER, min, max);
// gad_qbox_add_parm("Goofy?",QB_BOOL_SLOT, &crit.goofy, QB_RD_ONLY);
// gad_qbox_end();
// 
// That's it.  Simple enough?  Suggestions for more useful variable types and option types welcome...

// Includes
#include <lg.h>  // every file should have this
#include <gadgets.h>
//#include <slider.h>
#include <tng.h>
#include <tngqbox.h>

// C Library Includes

// System Library Includes

// Master Game Includes

// Game Library Includes

// Game Object Includes


// Defines

// Prototypes

// Begin a quick box.  Until the qbox is ended, all subsequent qbox calls will use the "current" qbox.
EXTERN Gadget *gad_qbox_start(Gadget *parent, Point coord, int z, TNGStyle *sty, ushort options, char *name, Point ss);

EXTERN Gadget *gad_qbox_start_full(Gadget *parent, Point coord, int z, TNGStyle *sty, ushort options, char *name, Point ss, Point spacing,
   Point border, Ref left_id, Ref right_id);

// Add a line to a quickbox.  slot_type describes the type of slot, var is a pointer to the variable to be
// displaying, and slot_options describes any additional modifiers to the qbox.  Note that some bizarre-o 
// combinations of options and types might not be implemented.
EXTERN errtype gad_qbox_add(char *label, int slot_type, void *var, ulong slot_options);

// Just like gad_qbox_add but allows two parameters to be set for the slot.  Certain slot options require
// this form of accessing.
EXTERN errtype gad_qbox_add_parm(char *label, int slot_type, void *var, ulong slot_options, void *parm1, void *parm2);

// This represents that the quickbox is done being created and is ready for display, input, etc.
// if end_full is used, then the passed pointer is used as the application's pointer to the quickbox
EXTERN errtype gad_qbox_end();
EXTERN errtype gad_qbox_end_full(Gadget **ptr);

// Rename a slot
EXTERN errtype gad_qbox_rename_slot(Gadget *g, int slot_num, char *new_name);

// Globals

#endif // __QBOXGADG_H




