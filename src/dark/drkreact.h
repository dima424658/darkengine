/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkreact.h,v 1.3 2000/01/31 09:40:03 adurant Exp $
#pragma once

#ifndef __DRKREACT_H
#define __DRKREACT_H

////////////////////////////////////////////////////////////
// ACT/REACT REACTIONS FOR DARK
//

#define REACTION_SET_MODEL "set_model"
#define REACTION_LIGHT_OFF "light_off"
#define REACTION_LIGHT_ON  "light_on"

EXTERN void InitDarkReactions(void);
EXTERN void DarkReactionsPostLoad(void);


#endif // __DRKREACT_H
