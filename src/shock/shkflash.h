/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkflash.h,v 1.4 2000/01/31 09:55:43 adurant Exp $
//
// gun flashes
//
#pragma once

This file has been moved to gunflash.h AMSD


#ifndef __SHKFLASH_H
#define __SHKFLASH_H

#include <matrixs.h>
#include <objtype.h>

// Create flash objects as specified by archetype flash links at the various VHots of the gun
// Each link identifies the flash object (destination) and the vhot number as data
EXTERN void CreateGunFlashes(ObjID archetypeID, ObjID gunID, 
                             mxs_ang facingOffset = 0,
                             BOOL isGhost = FALSE);

EXTERN void GunFlashInit(void);
EXTERN void GunFlashTerm(void);

#endif // __SHKFLASH_H
