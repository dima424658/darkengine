/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/damgscrp.h,v 1.1 1998/01/07 13:03:34 mahk Exp $
#pragma once  
#ifndef __DAMGSCRP_H
#define __DAMGSCRP_H

#include <damgscrm.h>

#define OnDamage()         SCRIPT_MESSAGE_HANDLER_SIGNATURE_(Damage,sDamageScrMsg)
#define DefaultOnDamage()  SCRIPT_CALL_BASE(Damage)

#define OnSlain()         SCRIPT_MESSAGE_HANDLER_SIGNATURE_(Slain,sSlayMsg)                 
#define DefaultOnSlain()  SCRIPT_CALL_BASE(Slain)

#define DAMAGE_MESSAGE_DEFAULTS() \
          SCRIPT_DEFINE_MESSAGE_DEFAULT_(Damage,sDamageScrMsg) \
          SCRIPT_DEFINE_MESSAGE_DEFAULT_(Slain,sSlayMsg)

#define DAMAGE_MESSAGE_MAP_ENTRIES() \
          SCRIPT_MSG_MAP_ENTRY(Damage)  \
          SCRIPT_MSG_MAP_ENTRY(Slain)

#endif // __DAMGSCRP_H




