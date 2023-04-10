/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/mtscrpt.h,v 1.1 1999/12/23 16:54:08 BFarquha Exp $
#pragma once
#ifndef __MTSCRPT_H
#define __MTSCRPT_H

#include <scrptmsg.h>
#include <media.h>

//////////////////////////////////////
//
// Medium Transition Messages
//

#define kMediumTransMsgVer 1

struct sMediumTransMsg : public sScrMsg
{
   int nFromType;
   int nToType;

   sMediumTransMsg()
      : nFromType(-1),
        nToType(-1)
   {}

   // real constructor
   sMediumTransMsg(const ObjID obj_to, int nFrom, int nTo)
      : nFromType(nFrom),
        nToType(nTo)
   {
      // setup base scrMsg fields
      to = obj_to;
      free((void *)message);
      message = strdup("MediumTransition");
   }

   DECLARE_SCRMSG_PERSISTENT();

};

#ifdef SCRIPT
#define OnMediumTransition()           SCRIPT_MESSAGE_HANDLER_SIGNATURE_(MediumTransition, sMediumTransMsg)
#endif


////////////////////////////////////////
//
// THE BASE MT SCRIPT
//

#ifdef SCRIPT

#define MEDIUMTRANSITION_MESSAGE_DEFAULTS() \
   SCRIPT_DEFINE_MESSAGE_DEFAULT_(MediumTransition,    sMediumTransMsg)

#define MEDIUMTRANSITION_MESSAGE_MAP_ENTRIES() \
         SCRIPT_MSG_MAP_ENTRY(MediumTransition)
#endif

#endif // __MTSCRPT_H
