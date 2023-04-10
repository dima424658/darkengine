/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/motedit.h,v 1.2 2000/01/29 13:12:36 adurant Exp $
// only external interface to motion editor
#pragma once

#ifndef __MOTEDIT_H
#define __MOTEDIT_H

// variable for whether or not motion editor is currently being used
EXTERN BOOL g_InMotionEditor;

// per-frame call to motion editor, where dt is elapsed millisec
EXTERN void MotEditUpdate(float dt);

// start motion editor
EXTERN void MotEditInit();

// close motion editor
EXTERN void MotEditClose();

// command to toggle motion editor on/off
EXTERN void motedit_cmd(void);

#endif
