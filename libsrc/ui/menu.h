// $Header: x:/prj/tech/libsrc/ui/RCS/menu.h 1.5 1998/06/18 13:28:46 JAEMZ Exp $

#ifndef __MENU_H
#define __MENU_H
#pragma once

// Includes
#include <lg.h>  // every file should have this
#include <error.h>
#include <hotkey.h>
#include <tng.h>

// C Library Includes

// System Library Includes

// Master Game Includes

// Game Library Includes

// Game Object Includes

// Defines
typedef struct {
   char *name;
   int z;
} menu_struct;

// Prototypes
EXTERN Gadget *gad_menu_create(Gadget *parent, Point *coord, int z, TNGStyle *sty, int width, char *name);

EXTERN errtype gad_menu_add_line(Gadget *menu, char *label, hotkey_callback f, short keycode, ulong context, void *user_data, char *help_text);
EXTERN errtype gad_menu_add_submenu(Gadget *menu, char *label, Gadget *sub_menu);
EXTERN errtype gad_menu_popup(Gadget *menu);
EXTERN errtype gad_menu_popup_at_mouse(Gadget *menu);
EXTERN errtype gad_menu_popdown(Gadget *menu);

// Globals

#endif // __MENU_H





