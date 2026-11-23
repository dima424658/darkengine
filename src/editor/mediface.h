/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/mediface.h,v 1.3 2000/01/29 13:12:27 adurant Exp $
#pragma once

#ifndef __MEDIFACE_H
#define __MEDIFACE_H

#include <rect.h>

EXTERN BOOL CreateMotionEditorInterface(void);
EXTERN void DestroyMotionEditorInterface(void);
EXTERN void MainInterfaceResetBotSlider(int num_frames);
EXTERN const Rect *MotionEditorBounds(void);

#ifdef __cplusplus
void DrawMotionEditor();
#endif

#endif
