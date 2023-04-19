/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/gadget/RCS/gadblist.h 1.2 1996/08/26 02:02:36 mahk Exp $

#ifndef __GADBLIST_H
#define __GADBLIST_H
#include <gadget.h>
#include <drawelem.h>
#include <uibutton.h>
////////////////////////////////////////////////////////////
// API for buttonlist gadgets.
// recently ported over from the TN installer, woo hoo!
////////////////////////////////////////////////////////////

//------------------------------------------------------------
// STRUCTURE DEFINITION
//

typedef bool (*LGadButtonListCallback)(ushort action, int button, void* data, LGadBox *vb);

typedef struct _LGadButtonList
{
   ButtonList blist;
   LGadButtonListCallback call;
   bool free_self;
} LGadButtonList;

//------------------------------------------------------------
// FLAGS

#define BUTTONLIST_RADIO_FLAG BLIST_RADIO_FLAG 

//------------------------------------------------------------
// SPECIAL ACCESSORS
// 

#define LGadRadioButtonSelection(bl) RadioButtonSelection(&(bl)->blist)
#define LGadRadioButtonSelect(bl,n) RadioButtonSetSelection(&(bl)->blist,n)

//---------------------------------------------------------
// DESCRIPTIOR STRUCTURE


typedef struct _LGadButtonListDesc
{
   int num_buttons;
   Rect* button_rects;  // rectangles of buttons
   DrawElement* button_elems; // elems of buttons
   LGadButtonListCallback cb; 
   int paltype;
   ulong flags;
} LGadButtonListDesc;

EXTERN LGadButtonList* LGadCreateButtonListDesc(LGadButtonList* list, LGadRoot* vr,
   LGadButtonListDesc* desc);

EXTERN void LGadDestroyButtonList(LGadButtonList* list);


#endif // __GADBLIST_H

