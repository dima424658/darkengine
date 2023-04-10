/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// gun flashes
//
#pragma once

#ifndef __DPCFLASH_H
#define __DPCFLASH_H

#ifndef __MATRIXS_H
#include <matrixs.h>
#endif // __MATRIXS_H

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // !_OBJTYPE_H

// Create flash objects as specified by archetype flash links at the various VHots of the gun
// Each link identifies the flash object (destination) and the vhot number as data
EXTERN void CreateGunFlashes(ObjID archetypeID, ObjID gunID, 
                             mxs_ang facingOffset = 0,
                             BOOL isGhost = FALSE);

EXTERN void GunFlashInit(void);
EXTERN void GunFlashTerm(void);

#endif // __DPCFLASH_H
