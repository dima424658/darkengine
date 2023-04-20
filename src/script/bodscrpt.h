/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/bodscrpt.h,v 1.4 1998/04/07 13:58:06 CCAROLLO Exp $
#pragma once  
#ifndef __BODSCRPT_H
#define __BODSCRPT_H

#include <scrptmsg.h>
#include <objtype.h>
#include <objscrt.h>

//////////////////////////////////////
//
// Body Messages
//
//    Motion begin, motion end, motion flag reached

#define kBodyMsgVer 1

struct sBodyMsg : public sScrMsg
{
   enum eBodyAction
   {
      kMotionStart = 0x00000000,
      kMotionEnd,
      kMotionFlagReached,
      kToMakeBig = 0xFFFFFFFF
   };
   // what the deal is
   eBodyAction  ActionType;
   string MotionName;
   integer FlagValue;

   sBodyMsg()
   {
   }

   // real constructor
   sBodyMsg(const ObjID obj_to, const eBodyAction action, const char *motionName, const ulong flagValue=0)
      : ActionType(action), FlagValue(flagValue), MotionName(motionName)
   {
      static const char *body_msg_names[] = { "MotionStart", "MotionEnd", "MotionFlagReached"};

      // setup base scrMsg fields
      to = obj_to;
      free((void *)message);
      message = strdup(body_msg_names[action]);
   }
         
   DECLARE_SCRMSG_PERSISTENT();

};

#ifdef SCRIPT
#define OnMotionStart()                SCRIPT_MESSAGE_HANDLER_SIGNATURE_(MotionStart, sBodyMsg)
#define DefaultOnMotionStart()         SCRIPT_CALL_BASE(MotionStart)
#define OnMotionEnd()                  SCRIPT_MESSAGE_HANDLER_SIGNATURE_(MotionEnd, sBodyMsg)
#define DefaultOnMotionEnd()           SCRIPT_CALL_BASE(MotionEnd)
#define OnMotionFlagReached()          SCRIPT_MESSAGE_HANDLER_SIGNATURE_(MotionFlagReached, sBodyMsg)
#define DefaultOnMotionFlagReached()   SCRIPT_CALL_BASE(MotionFlagReached)
#endif


////////////////////////////////////////
//
// THE BASE BODY SCRIPT
//

#ifdef SCRIPT

#define BODY_MESSAGE_DEFAULTS() \
   SCRIPT_DEFINE_MESSAGE_DEFAULT_(MotionStart,    sBodyMsg) \
   SCRIPT_DEFINE_MESSAGE_DEFAULT_(MotionEnd,   sBodyMsg) \
   SCRIPT_DEFINE_MESSAGE_DEFAULT_(MotionFlagReached, sBodyMsg)

#define BODY_MESSAGE_MAP_ENTRIES() \
         SCRIPT_MSG_MAP_ENTRY(MotionStart) \
         SCRIPT_MSG_MAP_ENTRY(MotionEnd) \
         SCRIPT_MSG_MAP_ENTRY(MotionFlagReached)
#endif

#endif // __BODSCRPT_H
