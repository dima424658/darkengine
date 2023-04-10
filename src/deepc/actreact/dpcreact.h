/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCREACT_H
#define __DPCREACT_H

////////////////////////////////////////////////////////////
// ACT/REACT REACTIONS FOR DEEP COVER
//

#define REACTION_SET_MODEL "set_model"
#define REACTION_LIGHT_OFF "light_off"
#define REACTION_LIGHT_ON  "light_on"
#define REACTION_RADIATE   "radiate"
#define REACTION_POISON    "toxin"

EXTERN void DPCReactionsInit(void);
EXTERN void DPCReactionsPostLoad(void);


#endif // __DPCREACT_H
