/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/damgscrm.h,v 1.3 1998/06/29 16:45:15 mahk Exp $
#pragma once  
#ifndef __DAMGSCRM_H
#define __DAMGSCRM_H

#include <objtype.h>

//
// Damage Model Script Messages
//

struct sDamageScrMsg : public sScrMsg 
{
   integer kind;   // what kind of damage
   integer damage; // how much
   ObjID culprit; // who did it 

   DECLARE_SCRMSG_PERSISTENT(); 

#ifndef SCRIPT
   sDamageScrMsg (ObjID victim = 0, ObjID clprit = 0, int knd =0, int amount = 0)
      : sScrMsg(victim,"Damage"), culprit(clprit), kind(knd), damage(amount) 
   {}
#endif 


}; 

struct sSlayMsg : public sScrMsg
{
   ObjID culprit; // Mr. Green
   integer kind;  // With the candlestick damage type

   DECLARE_SCRMSG_PERSISTENT(); 

#ifndef SCRIPT
   sSlayMsg(ObjID victim = 0, ObjID clprit = 0, int k = 0)
      : sScrMsg(victim, "Slain") ,culprit(clprit), kind(k) {}; 
#endif 
};


#endif // __DAMGSCRM_H

