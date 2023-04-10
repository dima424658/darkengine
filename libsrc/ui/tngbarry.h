// $Header: x:/prj/tech/libsrc/ui/RCS/tngbarry.h 1.13 1998/06/18 13:29:57 JAEMZ Exp $

#ifndef __TNGBARRY_H
#define __TNGBARRY_H
#pragma once


// Includes
#include <lg.h>  // every file should have this
#include <tng.h>

// Typedefs
typedef struct {
   int type;
   void *disp_data;
} TNGButtonArrayElement;

typedef struct _butdrawcallback
{
   void (*func)(Rect* r, Point butcoord, void* data);
   void* data;
} ButtonDrawCallback;


typedef struct {
   TNG *tng_data;
   Point bsize, msize, wsize, scroll_size;
   Point size, offset, lsel;
   ubyte spacing;
   int num_selectable;
   short lastkey;
   ushort options;
   TNGButtonArrayElement *matrix;
   bool *selected;
   TNG *hscroll_tng, *vscroll_tng;
} TNG_buttonarray;

#define TNG_BA_MATRIX   0x0001
#define TNG_BA_HSCROLL  0x0002
#define TNG_BA_VSCROLL  0x0004

#define TNG_BA_SELECT_SIZE    1
#define TNG_BA_BORDER_WIDTH   1
#define TNG_BA_SCROLL_X       7
#define TNG_BA_SCROLL_Y       7


#define TNG_BA_NO_OPTIONS          0x00
#define TNG_BA_OUTLINE_MODE        0x01
#define TNG_BA_CHECKBOX_MODE       0x02
#define TNG_BA_TIGHTPACK           0x04
#define TNG_BA_LOOSEPACK           0x08

#define TNG_BA_CHECKBOX_SIZE       4
// Prototypes

// Initializes the TNG 
// Note that both of these must be called!
// _init is called before the UI deals appropriately, _init2 is called afterwards.
EXTERN errtype tng_buttonarray_init(void *ui_data, TNG *ptng, TNGStyle *sty, ushort options, Point msize, Point wsize, Point bsize, int num_sel);
EXTERN errtype tng_buttonarray_init2(TNG *ptng);

// Deallocate all memory used by the TNG 
EXTERN errtype tng_buttonarray_destroy(TNG *ptng);

// Draw the specified parts (may be all) of trdinates loc
// assumes all appropriate setup has already been done!
EXTERN errtype tng_buttonarray_2d_draw(TNG *ptng, ushort partmask, Point loc);

// Fill in ppt with the size of the TNG 
EXTERN errtype tng_buttonarray_size(TNG *ptng, Point *ppt);

// Returns the current "value" of the TNG
EXTERN int tng_buttonarray_getvalue(TNG *ptng);

// React appropriately for receiving the specified cooked key
EXTERN bool tng_buttonarray_keycooked(TNG *ptng, ushort key);

// React appropriately for receiving the specified mouse button event
EXTERN bool tng_buttonarray_mousebutt(TNG *ptng, uchar type, Point loc);

// Handle incoming signals
EXTERN bool tng_buttonarray_signal(TNG *ptng, ushort signal);

EXTERN errtype tng_buttonarray_select(TNG *ptng);
EXTERN errtype tng_buttonarray_scroll(TNG *ptng);
EXTERN errtype tng_buttonarray_addbutton_at(TNG *ptng, int type, void *disp_data, int coord_x, int coord_y);
EXTERN errtype tng_buttonarray_addbutton(TNG *ptng, int type, void *disp_data);
EXTERN errtype tng_buttonarray_setoffset(TNG *ptng, int offset_x, int offset_y);

// Draws button i,j of the buttonarray pointed to by ptng.
EXTERN errtype tng_buttonarray_draw_button(TNG *ptng, int i, int j);

// Macros
#define TNG_BA(ptng) ((TNG_buttonarray *)(ptng->type_data))
#define TNG_BA_MSIZE(ptng) ((TNG_buttonarray *)(ptng->type_data))->msize
#define TNG_BA_BSIZE(ptng) ((TNG_buttonarray *)(ptng->type_data))->bsize
#define TNG_BA_WSIZE(ptng) ((TNG_buttonarray *)(ptng->type_data))->wsize
#define TNG_BA_LSEL(ptng) ((TNG_buttonarray *)(ptng->type_data))->lsel
#define TNG_BA_SPACING(ptng) ((TNG_buttonarray *)(ptng->type_data))->spacing
#define TNG_BA_OFFSET(ptng) ((TNG_buttonarray *)(ptng->type_data))->offset
#define TNG_BA_LASTKEY(ptng) ((TNG_buttonarray *)(ptng->type_data))->lastkey
#define TNG_BA_NUMSEL(ptng) ((TNG_buttonarray *)(ptng->type_data))->num_selectable
#define TNG_BA_SELECTED(ptng, x1, y) ((TNG_buttonarray *)((ptng)->type_data))->selected[(x1) + ((y) * TNG_BA_MSIZE(ptng).x)]
#define TNG_BA_INDEX(ptng,x1,y) ((TNG_buttonarray *)((ptng)->type_data))->matrix[(x1) + ((y) * TNG_BA_MSIZE(ptng).x)]

#endif // __TNGBARRY_H





