/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/diffscrm.h,v 1.1 1998/08/02 23:37:03 mahk Exp $
#pragma once  
#ifndef __DIFFSCRM_H
#define __DIFFSCRM_H
#include <scrptmsg.h>

////////////////////////////////////////////////////////////
// "Difficulty" script message
// 
// This will get sent *before* the sim starts, when difficulty is being 
// processed.  To get this message, you have to have the "Diff Script" 
// property. 
//

struct sDiffScrMsg: public sScrMsg
{
   int difficulty; 

   sDiffScrMsg(ObjID towhom, int diff)
      : sScrMsg(towhom, "Difficulty"), difficulty(diff)
   {
   }
}; 

#ifdef SCRIPT
#define OnDifficulty() SCRIPT_MESSAGE_HANDLER_SIGNATURE_(Difficulty,sDiffScrMsg)
#define DefaultOnDifficulty() SCRIPT_CALL_BASE(Difficulty)

#define DIFFICULTY_MESSAGE_DEFAULTS() \
        SCRIPT_DEFINE_MESSAGE_DEFAULT_(Difficulty,sDiffScrMsg)

#define DIFFICULTY_MESSAGE_MAP_ENTRIES() \
           SCRIPT_MSG_MAP_ENTRY(Difficulty)

#endif // SCRIPT

#endif // __DIFFSCRM_H
