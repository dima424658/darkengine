/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkcmbat.h,v 1.4 1998/10/27 20:26:11 dc Exp $
#pragma once  
#ifndef __DRKCMBAT_H
#define __DRKCMBAT_H

////////////////////////////////////////////////////////////
// DARK-SPECIFIC COMBAT FEATURES
//

EXTERN void DarkCombatInit(void);
EXTERN void DarkCombatTerm(void);

// if you want to see whose fault it is
EXTERN BOOL player_is_culprit_for_weapon(ObjID culprit, BOOL *held);

#endif // __DRKCMBAT_H
