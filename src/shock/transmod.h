/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/transmod.h,v 1.4 2000/01/31 09:59:41 adurant Exp $
#pragma once

#ifndef __TRANSMOD_H
#define __TRANSMOD_H

// These functions are actually implemented in shklding.cpp 
EXTERN void TransModeSwitchLevel(const char *level, int marker, uint flags);
EXTERN void TransModeLoadGame(int slot);



#endif
