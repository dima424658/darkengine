// $Header: x:/prj/tech/libsrc/ui/RCS/tngapp.h 1.14 1998/06/18 13:30:49 JAEMZ Exp $

#ifndef __TNGAPP_H
#define __TNGAPP_H
#pragma once


// Includes
#include <lg.h>  // every file should have this
#include <region.h>
#include <gadgets.h>
//#include <pushbutt.h>
//#include <slider.h>

#define TNG_ALLPARTS 0xffff

// Macros!!

#define TNG_GADGET(ptng) ((Gadget *)((ptng)->ui_data))

#define TNG_DRAW(ptng) TNG_DRAWPART(ptng, TNG_ALLPARTS)
#define TNG_DRAWPART(ptng, pmask) gadget_display_part((Gadget *)(ptng->ui_data),((Gadget *)(ptng->ui_data))->rep->r, pmask)
#define TNG_DRAWRECT(ptng, r) gadget_display((Gadget *)((ptng)->ui_data), (r))

extern Point tng_absloc(TNG* ptng);
#define TNG_ABSLOC(ptng) tng_absloc(ptng)

#define GUI_MALLOC(uid, size)   Malloc(size)
#define GUI_DEALLOC(uid, victim) Free(victim)

#define TNG_GRAB_FOCUS(ptng, evmask) uiGrabFocus(((Gadget *)((ptng)->ui_data))->rep, evmask)
#define TNG_RELEASE_FOCUS(ptng, evmask) uiReleaseFocus(((Gadget *)((ptng)->ui_data))->rep, evmask)

#define TNG_GET_MOUSE(px, py) mouse_get_xy((px),(py))
#define TNG_PUT_MOUSE(x, y) mouse_put_xy((x), (y))

#define TNG_SCREEN_SIZE_X(ptng) 320;
#define TNG_SCREEN_SIZE_Y(ptng) 200;

#define TNG_IF_OBSCURED(ptng) if (region_obscured(TNG_GADGET(ptng)->rep, TNG_GADGET(ptng)->rep->r) == COMPLETELY_OBSCURED)
#define TNG_IF_UNOBSCURED(ptng) if (region_obscured(TNG_GADGET(ptng)->rep, TNG_GADGET(ptng)->rep->r) == UNOBSCURED)

#define TNG_IF_FOREIGN_OBSCURED(ptng) if (foreign_region_obscured(TNG_GADGET(ptng)->rep, TNG_GADGET(ptng)->rep->r) == COMPLETELY_OBSCURED)
#define TNG_IF_FOREIGN_UNOBSCURED(ptng) if (foreign_region_obscured(TNG_GADGET(ptng)->rep, TNG_GADGET(ptng)->rep->r) == UNOBSCURED)
#define TNG_FOREIGN_OBSCURED(ptng) (foreign_region_obscured(TNG_GADGET(ptng)->rep, TNG_GADGET(ptng)->rep->r))

// Macros for creating supported TNGs....

EXTERN Gadget *gad_pushbutton_create_from_tng(void *ui_data, Point loc, TNG **pptng, TNGStyle *sty, int button_type,
   void *display_data, Point size);
#define TNG_CREATE_PUSHBUTTON(ui_data, loc, pptng, sty, button_type, display_data, size) gad_pushbutton_create_from_tng(ui_data, loc, pptng, sty, button_type, display_data, size)
   
EXTERN Gadget *gad_slider_create_from_tng(void *ui_data, Point loc, TNG **pptng, TNGStyle *sty, int alignment, int min, int max,
   int value, int increment, Point size);
#define TNG_CREATE_SLIDER(ui_data, loc, pptng, sty, alignment, min, max, value, increm, size) gad_slider_create_from_tng(ui_data, loc, pptng, sty, alignment, min, max, value, increm, size)

EXTERN Gadget *gad_textgadget_create_from_tng(void *ui_data, Point loc, TNG **pptng, TNGStyle *sty, ulong options, Point size);
#define TNG_CREATE_TEXT(ui_data, loc, pptng, sty, options, size)  gad_textgadget_create_from_tng(ui_data, loc, pptng, sty, options, size)

#endif // __TNGAPP_H

