/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/pickscrm.h,v 1.1 1999/12/03 14:58:56 adurant Exp $
#pragma once  
#ifndef __PICKSCRM_H
#define __PICKSCRM_H
#include <scrptmsg.h>

////////////////////////////////////////////////////////////
// "PickStateChange" script message
// 
// This will get sent when the state on an "advanced" pick lock 
// changes.

struct sPickStateScrMsg: public sScrMsg
{
   int prevstate;
   int currentstate;

   sPickStateScrMsg(ObjID victim = 0, int pstate = 0, int cstate = 0)
      : sScrMsg(victim, "PickStateChange"), prevstate(pstate), currentstate(cstate) {};
}; 

#ifdef SCRIPT
#define OnPickStateChange() SCRIPT_MESSAGE_HANDLER_SIGNATURE_(PickStateChange,sPickStateScrMsg)
#define DefaultOnPickStateChange() SCRIPT_CALL_BASE(PickStateChange)

#define PICKSTATECHANGE_MESSAGE_DEFAULTS() \
        SCRIPT_DEFINE_MESSAGE_DEFAULT_(PickStateChange,sPickStateScrMsg)

#define PICKSTATECHANGE_MESSAGE_MAP_ENTRIES() \
           SCRIPT_MSG_MAP_ENTRY(PickStateChange)

#endif // SCRIPT

#endif // __PICKSCRM_H
