/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/arscrp.h,v 1.1 1997/12/30 17:50:39 mahk Exp $
#pragma once  
#ifndef __ARSCRP_H
#define __ARSCRP_H

#include <arscrm.h>

#define OnStimulus(stim)                  SCRIPT_DEFINE_MESSAGE_(stim##Stimulus,sStimMsg)
#define DefaultOnStimulus(stim)           SCRIPT_CALL_BASE(stim##Stimulus)
#define STIMULUS_MESSAGE_MAP_ENTRY(stim)  SCRIPT_MSG_MAP_ENTRY(stim##Stimulus)

#endif // __ARSCRP_H
