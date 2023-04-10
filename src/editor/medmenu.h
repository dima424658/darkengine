/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/medmenu.h,v 1.3 2000/01/29 13:12:30 adurant Exp $
#pragma once

#ifndef __MEDMENU_H
#define __MEDMENU_H

#include <sdesbase.h>
#include <mediface.h>
#include <motion.h>

#define STANDARD_ME_MENU_BORDER BORDER(DRAWFLAG_BORDER_OUTLINE)
#define STANDARD_ME_MENU_FLAGS (MENU_GRAB_FOCUS|MENU_OUTER_DISMISS)

#define STANDARD_ME_MENU_MARGIN 2

struct sFlagValue
{
   ulong value;
};

EXTERN sStructDesc frame_flag_desc;

// Easy macro for standardized menus
#define StandardEditMenu(m,w,h,el,tit,varel) \
   LGadCreateEditMenuArgs(&(m),main_root,STANDARD_ME_MENU_MARGIN+200,MOTEDIT_BUTTON_HEIGHT+STANDARD_ME_MENU_MARGIN,(w),(h), sizeof((el))/sizeof(DrawElement), 0, (el), \
       STANDARD_ME_MENU_FLAGS, STANDARD_ME_MENU_BORDER, (tit), (varel), 0); \
   ActiveMenuAdd(VB(&m));

EXTERN void ActiveMenuAdd(LGadBox *b);
EXTERN bool ActiveMenuKill(short action, void *data, LGadBox *b);

EXTERN BOOL FrameSliderUpdate(); // returns TRUE if slider value changed

EXTERN bool SaveLoadButtonFunc(short action,void *data, LGadBox *b);
EXTERN bool PlayOptButtonFunc(short action,void *data, LGadBox *b);
EXTERN bool FrameButtonFunc(short action,void *data, LGadBox *b);
EXTERN bool HeaderButtonFunc(short action,void *data, LGadBox *b);
EXTERN bool PlayButtonFunc(short action,void *data, LGadBox *b);
EXTERN bool TransformButtonFunc(short action,void *data, LGadBox *b);

EXTERN void DestroyActiveMenus();

#endif
