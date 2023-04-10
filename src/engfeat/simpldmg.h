/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/simpldmg.h,v 1.2 2000/01/31 09:45:38 adurant Exp $
#pragma once

#ifndef __SIMPLDMG_H
#define __SIMPLDMG_H

////////////////////////////////////////////////////////////
// THE SIMPLE DAMAGE MODEL
//
// Objects have hit points.  They die when the run out.  
// Unless some listener returns kDamageMsgStatusQuo for a Slay event, Slain objects
// are terminated.  Likewise, terminated objects are destroyed
// 

EXTERN void InitSimpleDamageModel(void);

//
// Damage types used the by simple model
//

enum eSimpleDamageKinds
{
   kDamageKindImpact,
};




#endif // __SIMPLDMG_H
