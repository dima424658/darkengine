// $Header: x:/prj/tech/libsrc/ui/RCS/tngtextg.h 1.5 1998/06/18 13:33:16 JAEMZ Exp $

#ifndef __TNGTEXTG_H
#define __TNGTEXTG_H
#pragma once

// Includes
#include <lg.h>  // every file should have this
#include <texttool.h>
#include <tng.h>

typedef struct {
   TNG *tng_data;
   Point size;
   short last_key;
   ulong options;
   TextTool *tt;
   TNG *hscroll_tng, *vscroll_tng;
} TNG_textgadget;

#define TNG_TG_TEXTAREA   0x0001
#define TNG_TG_HSCROLL  0x0002
#define TNG_TG_VSCROLL  0x0004

#define TNG_TG_BORDER_WIDTH   1
#define TNG_TG_SCROLL_X       14
#define TNG_TG_SCROLL_Y       14

#define TNG_TG_RETURN_KEY          0xd
#define TNG_TG_UP_KEY              0x48
#define TNG_TG_DOWN_KEY            0x50
#define TNG_TG_LEFT_KEY            0x4b
#define TNG_TG_RIGHT_KEY           0x4d
#define TNG_TG_SCROLL_LEFT_KEY     0x47
#define TNG_TG_SCROLL_RIGHT_KEY    0x4f
#define TNG_TG_SCROLL_UP_KEY       0x49
#define TNG_TG_SCROLL_DOWN_KEY     0x51

#define TNG_TG_SINGLE_LINE        0x0001
#define TNG_TG_LINE_SET           0x0002
#define TNG_TG_READ_ONLY          0x0004

#define TNG_TG_HORZ_SCROLL        0x1000
#define TNG_TG_VERT_SCROLL        0x2000

#define TNG_TG_SCROLLBARS   TNG_TG_HORZ_SCROLL | TNG_TG_VERT_SCROLL
// Prototypes

// Initializes the TNG 
// Note that both of these must be called!
// _init is called before the UI deals appropriately, _init2 is called afterwards.
EXTERN errtype tng_textgadget_init(void *ui_data, TNG *ptng, TNGStyle *sty, ulong options, Point size, Point abs_loc);
EXTERN errtype tng_textgadget_init2(TNG *ptng);

// Deallocate all memory used by the TNG 
EXTERN errtype tng_textgadget_destroy(TNG *ptng);

// Draw the specified parts (may be all) of the TNG at screen coordinates loc
// assumes all appropriate setup has already been done!
EXTERN errtype tng_textgadget_2d_draw(TNG *ptng, ushort partmask, Point loc);

// Fill in ppt with the size of the TNG 
EXTERN errtype tng_textgadget_size(TNG *ptng, Point *ppt);

// Returns the current "value" of the TNG
EXTERN int tng_textgadget_getvalue(TNG *ptng);

// React appropriately for receiving the specified cooked key
EXTERN bool tng_textgadget_keycooked(TNG *ptng, ushort key);

// React appropriately for receiving the specified mouse button event
EXTERN bool tng_textgadget_mousebutt(TNG *ptng, uchar type, Point loc);

// Handle incoming signals
EXTERN bool tng_textgadget_signal(TNG *ptng, ushort signal);

EXTERN errtype tng_textgadget_scroll(TNG *ptng);

EXTERN errtype tng_textgadget_addstring(TNG *ptng, char *s);

// Macros
#define TNG_TG(ptng) ((TNG_textgadget *)(ptng->type_data))
#define TNG_TG_SIZE(ptng) ((TNG_textgadget *)(ptng->type_data))->size
#define TNG_TG_LASTKEY(ptng) ((TNG_textgadget *)(ptng->type_data))->last_key
#define TNG_TG_TT(ptng) ((TNG_textgadget *)(ptng->type_data))->tt

#define TNG_TX_GETLINE(ptng,l) tt_get(TNG_TG_TT(ptng),(l))
#define TNG_TX_ADDSTRING(ptng,s) tng_textgadget_addstring(ptng, s); 
#define TNG_TX_CLEARLINE(ptng,l) tt_fill_line(TNG_TG_TT(ptng),TTF_REPLACE,(l),"\0"); _tt_do_event(TTEV_BOL)

#endif // __TNGTEXTG_H




