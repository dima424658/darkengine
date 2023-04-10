/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifndef __UITEXTED_H
#define __UITEXTED_H
/*
 * $Source: r:/t2repos/thief2/libsrc/gadget/uitexted.h,v $
 * $Revision: 1.5 $
 * $Author: JAEMZ $
 * $Date: 1998/01/12 12:41:29 $
 *
 */

#include <gadget.h>
#include <uibutton.h>
#include <region.h>
#include <event.h>

struct _TextGadg;
//**************************************
// UI TEXT EDITOR GADGET
// MAHK 1/95
// TO GO INTO THE UI LIBRARY SOME DAY
// ------------------------------------ 
// This text editor gadget provides support for 
// automatically displaying and editing text in a region. 
//
// Every text editor lives in its own region.  The text editor
// dispatches USER_DEFINED gadget events to the region under certain
// circumstances.  The events have the following format.


struct _TextGadgEvent; 
typedef struct _TextGadgEvent
{
   UIEVFRONT
   ushort signaller; // ignore if not equal to TEXTED_SIGNALLER
   ushort action; // defined below
   ushort data;    // action-specific data 
   UIEVBACK(2*sizeof(short));
} TextGadgEvent;

// Text gadget actions
#define TEXTGADG_SPECKEY 0 // A special key was pressed, "data" field contains cooked keycode
#define TEXTGADG_SELECT  1 // Text was "selected" (Not yet supported)
#define TEXTGADG_BUTTON  2 // button events
#define TEXTGADG_SPECKEY_PRE 3 // A special key was pressed before the parsing


//------------------------------------------------------------
// Text Editor Gadget Structure


struct _buffer
{
   char* buf;
   int len;
};

typedef struct _TextGadg
{
   ButtonGadg butt;          // gadget's box
   short edit;           // edit flags
   ulong flags;          // special flags, see below. 
   struct _buffer text;  // text being displayed/edited
   int cursor;           // character index of "cursor"
   int last_char;        // character index of last char in buffer
   int rep_count;        // current editor repeat count
   bool at_end;           // are we out of space?
   short* speckeys;        // special keys
} TextGadg;

// -----
// Flags
// -----

#define TEXTGADG_BORDER_FLAG     0x0002   // should there be a border?  
#define TEXTGADG_FOCUS_FLAG      0x0004   // should the gadget grab focus on click? 
#define TEXTGADG_MOUSE_DOWNS     0x0008   // should we send mouse downs
#define TEXTGADG_PRESCREEN_FLAG  0x0010   // Prescreen the events

#define TEXTGADG_ALIGN_MASK      0xF000   // text alignment
#define TEXTGADG_ALIGN_LEFT      0x1000   // left alignment
#define TEXTGADG_ALIGN_RIGHT     0x2000   // right alignment
#define TEXTGADG_ALIGN_TOP       0x4000   // top alignment
#define TEXTGADG_ALIGN_BOTTOM    0x8000   // bottom alignment

#define TEXTGADG_EDIT_EDITABLE   0x0001   // can you edit things
#define TEXTGADG_EDIT_OVERMODE   0x0002   // are we in insert mode
#define TEXTGADG_EDIT_BRANDNEW   0x0004   // most keystrokes wipe initial text
#define TEXTGADG_EDIT_NOSPACES   0x0008   // prohibit whitespace? 


#define tgadg_edit_flg(t,flg)   (t->edit&TEXTGADG_EDIT_##flg##)

//----------
// Accessors
//----------

#define TextGadgRegion(pgadg)    (VB(pgadg)->r)
#define TextGadgEditing(pgadg)   ((pgadg)->edit)
#define TextGadgText(pgadg)      ((pgadg)->text.buf)
#define TextGadgTextLen(pgadg)   ((pgadg)->text.len)
#define TextGadgCursor(pgadg)    ((pgadg)->cursor)
#define TextGadgFlags(pgadg)     ((pgadg)->flags)

#define TextGadgSetCursor(pgadg,intval)    ((pgadg)->cursor = (intval))
#define TextGadgSetFlag(pgadg,flg)  ((pgadg)->edit |= (flg))
#define TextGadgClrFlag(pgadg,flg)  ((pgadg)->edit &= ~(flg))

// Set the list of cooked keycodes that generate "special key" events.
// Must be a null terminated array of keycodes

#define TextGadgSetSpecialKeys(pgadg,keys) ((pgadg)->speckeys = (keys))

//-----------------------------------------------------
// TextGadgInit()
//
// Initializes a text gadget
// parent: gadget's parent region
// gadg:   gadget to be initialized
// area:   coordinates of gadget's bounding rectangle
// z:      gadget's region z coordinate
// buf:    buffer containing initial value of string to be edited.
// buflen: size of buf's memory
// flags:  flags, as defined above

EXTERN errtype TextGadgInit(Region* parent, TextGadg* gadg, Rect* area, int z, char* buf, int buflen, ulong flags);

//-----------------------------------------------------
// TextGadgDestroy()
//
// invalidates a text gadget, performing any necessary shutdown.

EXTERN errtype TextGadgDestroy(TextGadg* gadg);

// notify textgadg that you've changed it's text.

EXTERN errtype TextGadgUpdate(TextGadg* gadg);

// grab focus with a textgadg, unfocusing all others

EXTERN errtype TextGadgFocus(TextGadg* gadg);

EXTERN errtype TextGadgUnfocus(TextGadg* gadg);


#endif // __UITEXTED_H

