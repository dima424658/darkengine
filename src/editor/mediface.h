/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/mediface.h,v 1.3 2000/01/29 13:12:27 adurant Exp $
#pragma once

#ifndef __MEDIFACE_H
#define __MEDIFACE_H

#include <gadget.h>

EXTERN LGadRoot *main_root;

#define MOTEDIT_BUTTON_HEIGHT 40

EXTERN void CreateMotionEditorInterface();
EXTERN void DestroyMotionEditorInterface();

EXTERN void MainInterfaceResetBotSlider(int num_frames);
EXTERN void MainInterfaceRefreshFlagBox();
EXTERN void MainInterfaceReset();

#endif
