/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifndef __CHEVKIND_H
#define __CHEVKIND_H
#pragma once

////////////////////////////////////////////////////////////
// KINDS OF CHAINED EVENTS
//

enum eChainedEventKind_
{
   kEventKindNull, 
   kEventKindCollision,  // Physics collision
   kEventKindImpact,     // Damage model impact 
   kEventKindDamage,
   kEventKindSlay,
   kEventKindTerminate,
   kEventKindResurrect, 
   kEventKindStim, 
}; 



#endif __CHEVKIND_H
