// $Header: x:/prj/tech/libsrc/ui/RCS/tngmenu.h 1.5 1998/06/18 13:33:14 JAEMZ Exp $

#ifndef __TNGMENU_H
#define __TNGMENU_H
#pragma once

// Includes
#include <lg.h>  // every file should have this
#include <error.h>
#include <hotkey.h>
#include <texttool.h>
#include <tng.h>
#include <llist.h>

// C Library Includes

// System Library Includes

// Master Game Includes

// Game Library Includes

// Game Object Includes

// Defines
typedef struct {
   struct _llist;
   char *label;
   hotkey_callback f;
   void *user_data;
   short keycode;
   ulong context;
   TNG *submenu;
} MenuElement;

typedef struct {
   TNG *tng_data;
   void *ui_struct;
   Point coord;
   Point size;
   llist_head element_header;
   int slot_height;
   int num_lines;
   bool popped_up;
   MenuElement *current_selection;
   void (*popup_func)(TNG *ptng);
   void (*popdown_func)(TNG *ptng);
}  TNG_menu;

#define TNG_MENU_SPACING   2

// Prototypes
// Initializes the TNG 

EXTERN errtype tng_menu_init(void *ui_data, TNG *ptng, TNGStyle *sty, Point coord, int width, 
   void (*upfunc)(TNG *ptng), void (*downfunc)(TNG *ptng), void *ui_struct);

// Deallocate all memory used by the TNG 
EXTERN errtype tng_menu_destroy(TNG *ptng);

// Draw the specified parts (may be all) of the TNG at screen coordinates loc
// assumes all appropriate setup has already been done!
EXTERN errtype tng_menu_2d_draw(TNG *ptng, ushort partmask, Point loc);

// Fill in ppt with the size of the TNG 
EXTERN errtype tng_menu_size(TNG *ptng, Point *ppt);

// Returns the current "value" of the TNG
EXTERN int tng_menu_getvalue(TNG *ptng);

// React appropriately for receiving the specified cooked key
EXTERN bool tng_menu_keycooked(TNG *ptng, ushort key);

// React appropriately for receiving the specified mouse button event
EXTERN bool tng_menu_mousebutt(TNG *ptng, uchar type, Point loc);

// Handle incoming signals
EXTERN bool tng_menu_signal(TNG *ptng, ushort signal);

EXTERN errtype tng_menu_add_line(TNG *ptng, char *label, hotkey_callback f, short keycode, ulong context,
   void *user_data, char *help_text);

EXTERN errtype tng_menu_add_submenu(TNG *ptng, char *label, TNG *submenu);

EXTERN MenuElement *tng_menu_add_basic(TNG *ptng, char *label);

EXTERN errtype tng_menu_selection(TNG *ptng);

EXTERN errtype tng_menu_popup(TNG *ptng);
EXTERN errtype tng_menu_popup_at_mouse(TNG *ptng);
EXTERN errtype tng_menu_popup_loc(TNG *ptng, Point poploc);

// Macros
#define TNG_MN(ptng) ((TNG_menu *)(ptng->type_data))

// Globals

#endif // __TNGMENU_H





