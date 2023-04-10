/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/arscrm.h,v 1.2 1998/04/07 13:58:02 CCAROLLO Exp $
#pragma once  
#ifndef __ARSCRM_H
#define __ARSCRM_H

#include <arscrt.h>

//
// Act/React Script Messages
//


struct sStimMsg : public sScrMsg
{
   stimulus_kind stimulus; 
   real intensity;
   integer sensor;
   integer source;

   DECLARE_SCRMSG_PERSISTENT(); 

#ifndef SCRIPT
   // Turn a stim name into a message name. 
   static const char* message_name(stimulus_kind kind); 

   sStimMsg(); 
   sStimMsg(ObjID toWhom, stimulus_kind stim, real i, integer sen, integer src);

#endif 
}; 

#endif // __ARSCRM_H
