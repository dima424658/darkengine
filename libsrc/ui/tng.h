// $Header: x:/prj/tech/libsrc/ui/RCS/tng.h 1.15 1998/06/18 13:30:45 JAEMZ Exp $

#ifndef __TNG_H
#define __TNG_H
#pragma once

// Includes
#include <lg.h>  // every file should have this
#include <error.h>
#include <2dres.h>


// Default resource id's 
#define RES_guiIcons 400		// (0x190)
#define REF_IMG_iconArrowUp 0x1900000
#define REF_IMG_iconArrowUp1 0x1900001
#define REF_IMG_iconArrowDn 0x1900002
#define REF_IMG_iconArrowDn1 0x1900003
#define REF_IMG_iconArrowLt 0x1900004
#define REF_IMG_iconArrowLt1 0x1900005
#define REF_IMG_iconArrowRt 0x1900006
#define REF_IMG_iconArrowRt1 0x1900007
#define REF_IMG_iconChkbox 0x1900008
#define REF_IMG_iconChkbox1 0x1900009
#define REF_IMG_iconRadbut 0x190000a
#define REF_IMG_iconRadbut1 0x190000b
#define REF_IMG_bitmapMarble 0x190000c
#define REF_IMG_imgName1 0x190000d
#define RES_sysFont6 401		// (0x191)
#define RES_sysFont8 402		// (0x192)
#define RES_courFont8 403		// (0x193)
#define RES_timesFont12 404		// (0x194)
#define RES_timesFont18 405		// (0x195)
#define RES_timesFont24 406		// (0x196)
#define RES_ithcFont9 407		// (0x197)
#define RES_romanFont35 408		// (0x198)
#define RES_colorFont13 409		// (0x199)


// Typedefs

typedef struct {
	Id font;					// text font id
	Ref bitmapRef;			// bitmapped backdrop (or NULL)
   Point frobsize;      // size of non-resource frobs
	uchar backColor;		// background color (if bitmapRef == backColor == 0,
								// viewport is transparent)
	uchar textColor;		// text color
	uchar altBackColor;	// alternate background color for some gadgets
	uchar altTextColor;	// alternate text color for some gadgets
	uchar bordColor[4];	// if TNG_BEVEL, u.l. & l.r. bevel colors
								//	(1st pair normal, 2nd pair focus)
								//	if TNG_FATBORDER, 4-pixel border using these cols
} TNGStyle;

#define TNG_BEVEL			0x0001	// bevel border
#define TNG_FATBORDER	0x0002	// fat border
#define TNG_BMAPBACK		0x0004	// bitmap background
#define TNG_TILEBACK		0x0008	// tiled bitmap background
#define TNG_NOBACK		0x0010	// fully transparent background

// #define TNG_TRANSP      0x0100
#define TNG_ALTCOLORS	0x8000	// use alternate background & text colors
#define TNG_FOCUS			0x4000	// has focus (use focus bevel)
#define TNG_INVERTBEVEL	0x2000	// use inverted bevel
#define TNG_NOBORDER		0x1000	// turn off border altogether

#define TNG_BEVELMASK		0x6000	// bottom 2 bits for bevel drawing
#define TNG_BEVELSHIFT	13			// shift down to put in low bits

typedef bool (*TNGCallback)(void *ui_data, void *user_data);

typedef struct _TNG_CB {
   struct _TNG_CB *next_cb;
   void *user_data;
   ushort event_type;
   ushort condition;
   int id;
   TNGCallback cb;
} TNG_CB;

typedef struct _TNG {
   void *type_data;
   void *ui_data;
   void *cb_data;
   TNGStyle *style;
   TNG_CB *pcb;
   ushort flags;
   errtype (*draw_func)(struct _TNG *ptng, ushort partmask, Point loc);
   bool (*mousebutt)(struct _TNG *ptng, uchar type, Point rel);
   bool (*keycooked)(struct _TNG *ptng, ushort key);
   bool (*signal)(struct _TNG *ptng, ushort signal);
   bool (*keyscan)(struct _TNG *ptng, ushort scan);
   bool (*mousemove)(struct _TNG *ptng, Point loc);
} TNG;

#define TNG_MOUSE_MOTION    0x0001
#define TNG_MOUSE_LDOWN     0x0002
#define TNG_MOUSE_LUP       0x0004
#define TNG_MOUSE_RDOWN     0x0008
#define TNG_MOUSE_RUP       0x0010
#define TNG_MOUSE_CDOWN     0x0020
#define TNG_MOUSE_CUP       0x0040
#define TNG_ANY_MOUSE_DOWN  TNG_MOUSE_LDOWN | TNG_MOUSE_RDOWN | TNG_MOUSE_CDOWN
#define TNG_ANY_MOUSE_UP    TNG_MOUSE_LUP | TNG_MOUSE_RUP | TNG_MOUSE_CUP
#define TNG_ANY_MOUSE_BUTTON   TNG_ANY_MOUSE_UP | TNG_ANY_MOUSE_DOWN 
#define TNG_ANY_MOUSE       TNG_MOUSE_MOTION | TNG_ANY_MOUSE_BUTTON

#define TNG_SIGNAL_CHANGED    0x0001
#define TNG_SIGNAL_SELECT     0x0002
#define TNG_SIGNAL_DESELECT   0x0004
#define TNG_SIGNAL_SCROLL     0x0008
#define TNG_SIGNAL_DECREMENT  0x0010
#define TNG_SIGNAL_INCREMENT  0x0020
#define TNG_SIGNAL_EXPOSE     0x0040

#define TNG_EVENT_KBD_RAW        0x00000001
#define TNG_EVENT_KBD_COOKED     0x00000002
#define TNG_EVENT_MOUSE          0x00000004
#define TNG_EVENT_MOUSE_MOVE     0x00000008
#define TNG_EVENT_JOY            0x00000010
#define TNG_EVENT_SIGNAL         0x00000020
#define TNG_ALL_EVENTS           0xFFFFFFFF

//#define TNG_DEFAULT_FONT "data/font5x6p.fon"

// Prototypes
EXTERN void TNGDrawBase(TNG *ptng, Point coord, Point size);
EXTERN errtype TNGInit(TNG *ptng, TNGStyle *sty, void *ui_data);
EXTERN void TNGDrawBitmapRef(Ref ref, Point pt);
EXTERN void TNGDrawTileMapRef(Ref ref, Point pt);
EXTERN errtype TNGDrawText(Id id, char *text, int x, int y);

EXTERN errtype tng_install_callback(TNG *ptng, ushort event_type, ushort cond, TNGCallback cbfn, void *user_data, int *pid);
EXTERN errtype tng_uninstall_callback(TNG *ptng, int id);
EXTERN bool tng_cb_mousebutt(TNG *ptng, uchar type, Point loc);
EXTERN bool tng_cb_keycooked(TNG *ptng, ushort key);
EXTERN bool tng_cb_signal(TNG *ptng, ushort signal);
EXTERN bool tng_cb_keyscan(TNG *ptng, ushort scan);
EXTERN bool tng_cb_mousemove(TNG *ptng, Point loc);
EXTERN errtype tng_cb_draw(TNG *ptng, ushort partmask, Point loc);

// Macros
#define TNG_STYLE(x) (x)->tng_data->style
#define TNG_FLAGS(x) (x)->tng_data->flags
#define TNG_UI_DATA(x) (x)->tng_data->ui_data;

#define TNG_SIGNAL(ptng, sig) (ptng)->signal((ptng),(sig))

#define TNG_DRAW_TEXT(ptng, text, x, y) TNGDrawText((ptng)->style->font, text, x, y)

#define IF_SET_RV(x) if (!retval) { retval = x; }
#define IF_RV(x) if (!retval) { x; }

// externals

EXTERN Point tngZeroPt;

#include <tngapp.h>

#endif // __TNG_H




