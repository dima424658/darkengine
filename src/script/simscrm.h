/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/simscrm.h,v 1.1 1998/07/13 11:31:43 mahk Exp $
#pragma once  
#ifndef __SIMSCRM_H
#define __SIMSCRM_H

#include <scrptmsg.h>

////////////////////////////////////////////////////////////
// SIM SCRIPT MESSAGES  
//
// Get sent to all objects at the start and end of the sim
//

struct sSimMsg : public sScrMsg
{
   boolean starting;  // is the sim starting 

   DECLARE_SCRMSG_PERSISTENT();  

#ifndef SCRIPT
   sSimMsg(ObjID towhom = 0, boolean s = TRUE) : sScrMsg(towhom,"Sim"), starting(s)
   {
   }
#endif 
};

#ifdef SCRIPT
#define OnSim() SCRIPT_MESSAGE_HANDLER_SIGNATURE_(Sim,sSimMsg)
#define DefaultOnSim() SCRIPT_CALL_BASE(Sim)
#endif SCRIPT



#endif // __SIMSCRM_H







