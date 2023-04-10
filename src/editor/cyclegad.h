/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/cyclegad.h,v 1.14 2000/01/29 13:11:25 adurant Exp $
#pragma once

#ifndef __CYCLEGAD_H
#define __CYCLEGAD_H

#include <gadget.h>
#include <gadblist.h>
#include <gadtext.h>
#include <drawelem.h>

////////////////////////////////////////////////////////////
// GENERIC "cycle" gadget
// 
// Has a "left" and "right" arrow, and a value in the center
//
////////////////////////////////////////////////////////////


//
// callback argument: which part of the gadget was poked at
//

typedef enum eCyclePart
{
   kCycleLeftPart,
   kCycleRightPart,
   kCycleCenterPart,
} eCyclePart;

//
// The callback itself.  Note that the app must change the variable.  
// action is decoded using the following macros:
//

typedef enum _CycleActionKinds
{
   kCycleActionKey,
   kCycleActionMouse,
   kCycleActionUpdate,
} CycleActionKinds;

#define CYCLE_KIND_SHF 16
#define CYCLE_KIND_MASK 0xFFFF
#define CYCLE_DATA_MASK ((1 << CYCLE_KIND_SHF)-1)

#define CYCLE_ACTION_KIND(action) (((action) >> CYCLE_KIND_SHF) & CYCLE_KIND_MASK)
#define CYCLE_ACTION_DATA(action) ((action) & CYCLE_DATA_MASK)

#define CYCLE_MAKE_ACTION(kind,data) (((kind) << CYCLE_KIND_SHF) | (data))

typedef bool (*CycleCallback)(struct _CycleGadg* gadg, ulong action, eCyclePart part, void* var, struct _CycleGadgDesc *desc);

//
// Id's for arrow display elements
//

typedef enum eCycleArrow
{
   kCycleLeftArrow,
   kCycleRightArrow,
   kCycleNumArrows,
} eCycleArrow;


//
// Types of vars supported
//

//----------------------------------------
// And, of course, how could we not have a descriptor
//

#define CYCLE_EDIT_FLAG      0x80000000  // can we edit the text of the box
#define CYCLE_NONPOTENT_FLAG 0x40000000  // should we get non-potent callbacks

#define CYCLE_TEXTBOX_MASK 0xFFFF
#define CYCLE_TEXTBOX_SHF 0 
#define CYCLE_FOCUS_FLAG (TEXTBOX_FOCUS_FLAG << CYCLE_TEXTBOX_SHF) // grab focus on click
#define CYCLE_TDOWNS_FLAG (TEXTBOX_MOUSE_DOWNS << CYCLE_TEXTBOX_SHF) // should we get down events in the textbox

#define CYCLE_EDIT_SHF 16
#define CYCLE_EDIT_MASK  0xFF

#define CYCLE_NOSPACE_FLAG (TEXTBOX_EDIT_NOSPACES << CYCLE_EDIT_SHF) // disallow typed spaces
#define CYCLE_OVERSTRIKE_FLAG (TEXTBOX_EDIT_OVERMODE << CYCLE_EDIT_SHF) // overstrike mode

typedef struct _CycleGadgDesc
{
   Rect bounds; 
   void* var;
   DrawElement* arrow_drawelems;
   CycleCallback cb;
   char* format_string;  // printf/scanf format string
   void* user_data;
   ulong flags;
   struct _CycleGadg *next;
   struct _CycleGadg *prev;
} CycleGadgDesc;


//----------------------------------------
// The CycleGadg data structure
//

typedef struct _CycleGadg
{
   LGadRoot root;
   LGadButtonList list;
   //   LGadEditMenu menu;
   LGadTextBox text;
   CycleGadgDesc desc;
   Rect arrow_rects[kCycleNumArrows];
   bool free_self;
} CycleGadg;

//
// Create the sucker!
//

EXTERN CycleGadg* CreateCycleGadg(CycleGadg* gadg, LGadRoot* root, CycleGadgDesc* desc);
EXTERN void DestroyCycleGadg(CycleGadg* gadg);

EXTERN void UpdateCycleGadg(CycleGadg* gadg, bool redraw);

// Spoof an event.
EXTERN bool TriggerCycleGadg(CycleGadg* gadg, ulong action, eCyclePart part);

////////////////////////////////////////////////////////////
// STRING CYCLEGADG 
//
// a cyclegadg that cycles through a list of strings.
//

// flags
#define STRING_CYCLE_MENU 0x1  // should clicking on the gadget generate a menu.
#define STRING_CYCLE_WRAP 0x2  // should it wrap around

typedef struct _StringCycleGadgDesc
{
   char** strings; // the string vector
   int num;  // the size of the vector in elements
   ulong flags; // flags as above
} StringCycleGadgDesc;

EXTERN CycleGadg* CreateStringCycleGadg(CycleGadg* gadg, LGadRoot* root, 
    CycleGadgDesc* desc, StringCycleGadgDesc* sdesc);

EXTERN void DestroyStringCycleGadg(CycleGadg* gadg);

// extract the user data from a cyclegadg
EXTERN void* StringCycleGadgData(CycleGadg* gadg); 

// reset our strings and number 
EXTERN void RedescribeStringCycleGadg(CycleGadg* gadg, char** strings, int num);

////////////////////////////////////////////////////////////
// Generator macros for typed cycle gadg
////////////////////////////////////////////////////////////


#define CycleGadgType(type) \
typedef struct _##type##CycleGadgDesc \
{ \
  type min,max; \
  type delta; \
  bool wrap ; /* should we wrap from bottom/top */ \
  /* internal use only*/ \
  void* data; \
  CycleCallback cb;\
} type##CycleGadgDesc; \
\
EXTERN CycleGadg* Create##type##CycleGadg(CycleGadg* gadg, LGadRoot* root, CycleGadgDesc* desc, struct _##type##CycleGadgDesc* typedesc); \
EXTERN void Destroy##type##CycleGadg(CycleGadg* gadg); 

#ifndef _IMPLEMENT_CYCLEGADG
#define impCycleGadgType(type)


#else

#define impCycleGadgType(type) \
static bool type##_cycle_cb(CycleGadg* gadg, ulong act, eCyclePart part, void* var, CycleGadgDesc* desc) \
{ \
  bool update = part == kCycleCenterPart; \
  ushort kind = CYCLE_ACTION_KIND(act); \
  ushort action = CYCLE_ACTION_DATA(act); \
  type* realvar = (type*)var; \
  type##CycleGadgDesc* tdesc = (type##CycleGadgDesc*)desc->user_data;\
  if (kind == kCycleActionMouse && (action & BUTTONGADG_LCLICK)) \
  { \
    switch(part)\
    { \
      case kCycleLeftPart:  *realvar -= tdesc->delta; update = TRUE; break; \
      case kCycleRightPart: *realvar += tdesc->delta; update = TRUE; break; \
    } \
  } \
  if (update) \
  { \
     if (*realvar < tdesc->min) *realvar = (tdesc->wrap) ? tdesc->max-tdesc->delta : tdesc->min; \
     if (*realvar > tdesc->max) *realvar = (tdesc->wrap) ? tdesc->min : tdesc->max; \
     /* here we stuff user data to fool user */ \
     desc->user_data = tdesc->data;\
     desc->cb = tdesc->cb; \
     desc->cb(gadg,act,part,var,desc); \
     /* now unstuff it */\
     desc->cb = type##_cycle_cb;  \
     desc->user_data = tdesc;           \
  }     \
  return update;        \
} \
\
CycleGadg* Create##type##CycleGadg(CycleGadg* gadg, LGadRoot* root, CycleGadgDesc* desc, type##CycleGadgDesc* typedesc) \
{  \
   CycleGadgDesc desc2; \
   type##CycleGadgDesc* tdesc2 = Malloc(sizeof(*typedesc)); \
   desc2 = *desc; \
   *tdesc2 = *typedesc;                 \
   tdesc2->cb = desc->cb;       \
   tdesc2->data = desc->user_data;      \
   desc2.cb=type##_cycle_cb; \
   desc2.user_data = tdesc2; \
   return CreateCycleGadg(gadg,root,&desc2); \
} \
\
void Destroy##type##CycleGadg(CycleGadg* gadg)\
{ \
  DestroyCycleGadg(gadg); \
  Free(gadg->desc.user_data); \
}       

#endif


CycleGadgType(int);
impCycleGadgType(int);

CycleGadgType(float);
impCycleGadgType(float);

CycleGadgType(short);
impCycleGadgType(short);

CycleGadgType(fixang);
impCycleGadgType(fixang);

#endif // __CYCLEGAD_H
