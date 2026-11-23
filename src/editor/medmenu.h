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

struct sFlagValue
{
   ulong value;
};

EXTERN sStructDesc frame_flag_desc;
EXTERN BOOL FrameSliderUpdate(void);

#endif
