/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/gadget/RCS/gadtext.h 1.4 1997/02/11 15:01:46 mahk Exp $

#ifndef __GADTEXT_H
#define __GADTEXT_H

#include <uitexted.h>
#include <guistyle.h>

//
// api constants; replicated from textgadg implementation.
// 


typedef enum _LGadTextBoxEvent
{
   TEXTBOX_SPECKEY = TEXTGADG_SPECKEY, // a special key was pressed
   TEXTBOX_SELECT = TEXTGADG_SELECT,   // text was selected (not yet supported)
   TEXTBOX_BUTTON = TEXTGADG_BUTTON,   // gadget was clicked on. 
} LGadTextBoxEvent;

typedef bool (*LGadTextBoxCallback)(struct _LGadTextBox* tbox, LGadTextBoxEvent event, int eventdata, void* user_data); 


#define TEXTBOX_BORDER_FLAG    TEXTGADG_BORDER_FLAG      // should there be a border?  
#define TEXTBOX_FOCUS_FLAG     TEXTGADG_FOCUS_FLAG   // should the gadget grab focus on click? 
#define TEXTBOX_MOUSE_DOWNS    TEXTGADG_MOUSE_DOWNS // should we get mouse downs? 

// Alignment is not supported yet, so...
//#define TEXTBOX_ALIGN_MASK     TEXTGADG_ALIGN_MASK       // text alignment
//#define TEXTBOX_ALIGN_LEFT     TEXTGADG_ALIGN_LEFT       // left alignment
//#define TEXTBOX_ALIGN_RIGHT    TEXTGADG_ALIGN_RIGHT      // right alignment
//#define TEXTBOX_ALIGN_TOP      TEXTGADG_ALIGN_TOP        // top alignment
//#define TEXTBOX_ALIGN_BOTTOM   TEXTGADG_ALIGN_BOTTOM     // bottom alignment
                                                       
#define TEXTBOX_EDIT_EDITABLE  TEXTGADG_EDIT_EDITABLE    // can you edit things
#define TEXTBOX_EDIT_OVERMODE  TEXTGADG_EDIT_OVERMODE    // are we in insert mode
#define TEXTBOX_EDIT_BRANDNEW  TEXTGADG_EDIT_BRANDNEW    // most keystrokes wipe initial text
#define TEXTBOX_EDIT_NOSPACES  TEXTGADG_EDIT_NOSPACES    // prohibit whitespace? 

////////////////////////////////////////
// Accessors
////////////////////////////////////////

#define LGadTextBoxRegion(pgadg)               TextGadgRegion(&(pgadg)->gadg)            
#define LGadTextBoxEditing(pgadg)              TextGadgEditing(&(pgadg)->gadg)           
#define LGadTextBoxText(pgadg)                 TextGadgText(&(pgadg)->gadg)              
#define LGadTextBoxTextLen(pgadg)              TextGadgTextLen(&(pgadg)->gadg)           
#define LGadTextBoxCursor(pgadg)               TextGadgCursor(&(pgadg)->gadg)            
#define LGadTextBoxFlags(pgadg)                TextGadgFlags(&(pgadg)->gadg)             
                                                                                         
#define LGadTextBoxSetCursor(pgadg,intval)     TextGadgSetCursor(&(pgadg)->gadg,intval)  
#define LGadTextBoxSetFlag(pgadg,flg)          TextGadgSetFlag(&(pgadg)->gadg,flg)       
#define LGadTextBoxClrFlag(pgadg,flg)          TextGadgClrFlag(&(pgadg)->gadg,flg)       
#define LGadTextBoxSetSpecialKeys(pgadg,keys) TextGadgSetSpecialKeys(&(pgadg)->gadg,keys)

//
// Yay! A descriptor struct
//

typedef struct _LGadTextBoxDesc 
{
   Rect bounds;
   char* editbuf;
   int editbuflen;
   ulong flags;
   LGadTextBoxCallback cb;
   void* data;
   guiStyle* style;
} LGadTextBoxDesc; 

//
// and now, the lgadgtextbox itself
//

typedef struct _LGadTextBox
{
   TextGadg gadg;
   LGadTextBoxCallback cb;
   void* data; // from desc
   bool free_self;
} LGadTextBox;

//
// Creation and destruction
//


EXTERN LGadTextBox* LGadCreateTextBoxDesc(LGadTextBox* box, LGadRoot* root, LGadTextBoxDesc* desc);
EXTERN void LGadDestroyTextBox(LGadTextBox* box);

//
// Notify the textbox that its text has changed
//

#define LGadUpdateTextBox(box) TextGadgUpdate(&(box)->gadg)

//
// Grab/Release focus with the text box
//

#define LGadFocusTextBox(box) TextGadgFocus(&(box)->gadg)
#define LGadUnfocusTextBox(box) TextGadgUnfocus((box != NULL) ? &(box)->gadg: NULL)


#endif // __GADTEXT_H
