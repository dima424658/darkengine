/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/gadget/RCS/drawelem.h 1.15 1998/08/04 14:09:32 JAEMZ Exp $

#ifndef __DRAWELEM_H
#define __DRAWELEM_H
#pragma once

#include <res.h>
#include <guistyle.h>

////////////////////////////////////////////////////////////
// DrawElem states
//
//

typedef enum _DrawElemState
{
   dsNORMAL,   // just a regular-guy drawelem 
   dsDEPRESSED, // mouse is down on us
   dsDISABLED, // "greyed out" 
   dsATTENTION, // mouse is over us but not down
} DrawElemState;

#define DRAWELEM_ALL_STATES 0xFFFF

typedef void (*DrawElemCallback)(struct _DrawElement* drawme, DrawElemState state);

#define MAX_BTYPES   8
EXTERN Id lgad_btype_ids[MAX_BTYPES];

// Total hackly thing
EXTERN Id lgad_internal_id;
EXTERN char *(*lgad_string_get)(int num, char *buf, int bufsize);

typedef struct _DrawElement {
   short draw_type;
   void *draw_data;
   void *draw_data2;
   ushort fcolor;
   ushort bcolor;
   ushort draw_flags;
   struct _DrawElement *inner;
   ushort statemask;  // what states does this drawelem handle (0 == default for all states)
   struct _DrawElement* next; // drawelems for other states (optional)
} DrawElement;

#define DRAWTYPE_NONE   	0 // do nothing!
#define DRAWTYPE_TEXT		1 // interpret as char *
#define DRAWTYPE_BITMAP		2 // interpret as grs_bitmap *
#define DRAWTYPE_RES		   3 // interpret as resource ID
#define DRAWTYPE_VAR		   4 // interpret as int *
#define DRAWTYPE_VARSTRING	5 // interpret as string array
#define DRAWTYPE_VARRES	   6 // interpret as ID array
#define DRAWTYPE_FORMAT		7 // interpret as formatted string
#define DRAWTYPE_TEXTREF	8 // interpret as string ID
#define DRAWTYPE_FORMATREF 9 // interpret as string ID for formatted string 
#define DRAWTYPE_CALLBACK  10 // interpret data as a DrawElemCallback 
#define DRAWTYPE_RESOFFSET 11 // like DRAWTYPE_RES but uses the callback data as an offset to the Ref, data2 is how many bitmaps are valid
#define DRAWTYPE_VARTEXTREF     12 // like VARSTRING and VARRES but the array is one of string refs
#define DRAWTYPE_BITMAPOFFSET 13 // like BITMAP but data is a bitmap**, callback data is offset, data2 is how many bitmaps are valid

/* DrawElements represent a specific way to draw yourself.  There are currently 4 types, and when it 
comes time to draw, having a DrawElement means that it is specified what one needs to do to draw.
DRAWTYPE_NONE does what you might expect, namely, nothing.
DRAWTYPE_TEXT draws text in the default UI font unless draw_data2 is set, in which case it takes 
   that as a resource ID of the font to use instead. 
DRAWTYPE_BITMAP interprets draw_data as a bitmap to put in the relevant place.
DRAWTYPE_BITMAPOFFSET  interprets draw_data as an array of bitmap pointers, and uses callback data
    as the offset, and data2 as the limit, the number of bitmap states
DRAWTYPE_RES interprets draw_data as a resource ID to look into and find the right art to draw there.
DRAWTYPE_VAR just uses the value of the variable being controlled by the gadget to use as a display, 
   otherwise similar to DRAWTYPE_TEXT.  draw_data2 is reserved by the gadget to provide said data.
DRAWTYPE_VARSTRING uses draw_data as a char ** and indexes into that array depending on the value 
   of draw_data2 (as an int *), which is filled in by the gadget tracking that gadgets primary value.
DRAWTYPE_VARRES uses draw_data as an array of resource ID s to iterate through, much like VARSTRING does.
DRAWTYPE_VARTEXTREF uses draw_data as an array of string text Refs to iterate through, ala the other VAR types
DRAWTYPE_FORMAT is like DRAWTYPE_TEXT except that the text is first passed through lg_sprintf and the 
   draw_data2 parameter is interpreted as an (int *) and its contents passed to it as the content argument.
DRAWTYPE_TEXTREF is just like DRAWTYPE_TEXT but it takes in a string Ref
DRAWTYPE_FORMATREF is like DRAWTYPE_FORMAT except it isn't a char *, it's a string Ref.
DRAWTYPE_CALLBACK uses the draw_data as a DrawCallback, passing it's draw_data2 as the VoyBox pointer.
*/

#define DRAWFLAG_BORDER_BITS     0x0003
#define DRAWFLAG_BTYPE_BITS      0x000C
#define DRAWFLAG_INTERNAL_BITS   0x00F0
#define DRAWFLAG_FORMAT_BITS     0x1F00
#define DRAWFLAG_BORDER_SHIFT    0
#define DRAWFLAG_BTYPE_SHIFT     2
#define DRAWFLAG_INTERNAL_SHIFT  4
#define DRAWFLAG_FORMAT_SHIFT    8

#define DRAWFLAG_BORDER_NORMAL   0x0
#define DRAWFLAG_BORDER_OUTLINE  0x1
#define DRAWFLAG_BORDER_BEVEL    0x2
#define DRAWFLAG_BORDER_BITMAP   0x3
#define BORDER(x) ((x) << DRAWFLAG_BORDER_SHIFT)

#define DRAWFLAG_BTYPE_TREK      0x0
#define DRAWFLAG_BTYPE_TREK2     0x1
#define DRAW_BTYPE(x) (BORDER(DRAWFLAG_BORDER_BITMAP)|((x) << DRAWFLAG_BTYPE_SHIFT))

#define BORDER_TREK              DRAW_BTYPE(DRAWFLAG_BTYPE_TREK)

#define DRAWFLAG_INT_SOLID       0x0
#define DRAWFLAG_INT_TREK        0x1
#define DRAWFLAG_INT_TRANSP      0x2
#define INTERNAL(x) ((x) << DRAWFLAG_INTERNAL_SHIFT)

#define DRAWFLAG_FORMAT_CENTER   0x0
#define DRAWFLAG_FORMAT_LEFT     0x1
#define DRAWFLAG_FORMAT_RIGHT    0x2
#define DRAWFLAG_FORMAT_WRAP     0x4
#define DRAWFLAG_FORMAT_TABBED   0x10
#define FORMAT(x) ((x) << DRAWFLAG_FORMAT_SHIFT)

#define TREK_STANDARD_FLAGS   (BORDER_TREK|INTERNAL(DRAWFLAG_INT_TREK))

/* Okay, usage of this gets a little cryptic.  Basically, each byte of the draw_flags are used to control the
draw behavior of a different element -- bottom byte for border, second for internal, third for format.  There are
various defines for each type's control, these are the full DRAWFLAG_$TYPE$ defines.  For each type there is
a shift and a flags for getting at them.  Code that wants to look at it can just & the flags with the 
DRAWFLAG_$TYPE$_BITS define and then shift down by the DRAWFLAG_$TYPE$_SHIFT, and then compare directly to the
defines.  For setting them, there are $TYPE$ macros that allow you to set those values easy, by doing the right
shift for you. */


EXTERN void ElementSetStyle(guiStyle* style); // set the style for use by subsequent functions
EXTERN void ElementDraw(DrawElement *d, DrawElemState state, short x, short y, short w, short h);
EXTERN void ElementSize(DrawElement *d, short *px, short *py);
EXTERN void ElementOffset(DrawElement *d, short *pw, short *ph);
EXTERN void ElementOffsetAlternate(DrawElement *d, short *pw, short *ph);
EXTERN void ElementClear(DrawElement *d);
EXTERN void ElementExtrasSize(DrawElement *d, short *pw, short *ph);
EXTERN void ElementSetGlobalTabs(int numTabs, int *pTabPixOffsets);
EXTERN void ElementGetGlobalTabs( int *pnTabs, int **ppTabs );

#define DRAWELEM_STRLEN    511 // maximum string size for a drawelem
#define MIN_MARGIN         3 // minimum spacing of text to an edge

#endif
