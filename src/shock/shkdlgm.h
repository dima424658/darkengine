/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkdlgm.h,v 1.2 2000/01/31 09:55:36 adurant Exp $
#pragma once
#ifndef __SHKDLGM_H
#define __SHKDLGM_H

DEFINE_LG_GUID(LOOPID_DialogMode, 0xce);

EXTERN struct sLoopModeDesc DialogLoopMode;

EXTERN struct sLoopInstantiator* GetDialogLoopInst();

#endif 
