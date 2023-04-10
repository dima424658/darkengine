/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostwep.h,v 1.3 2000/01/29 13:41:14 adurant Exp $
#pragma once

#ifndef _GHOSTWEP_H_
#define _GHOSTWEP_H_

#include <ghosttyp.h>

// probably need some defines, here
EXTERN void  GhostAttachWeapon(sGhostRemote *pCreat,int nWeapon);
EXTERN int   GhostGetWeaponIdCharging(ObjID nId, BOOL charging);
EXTERN int   GhostGetWeaponId(ObjID nId);
EXTERN ObjID GhostGetWeaponArch(int id);
#endif
