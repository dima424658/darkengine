/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/twqscrpt.h,v 1.3 1998/04/07 13:59:17 CCAROLLO Exp $
// 

#pragma once
#ifndef __TWQSCRPT_H
#define __TWQSCRPT_H

#include <scrptmsg.h>

#include <objtype.h>
#include <tweqenum.h>

//////////////////////////////////////
//
// MESSAGES: Tweq Message and services
//    TweqComplete: takes Tweq type, Complete Forward/Back, Operation to do

#define kTweqMsgVer 2

struct sTweqMsg : public sScrMsg
{
   // data about what is going on in the message
   eTweqType      Type;
   eTweqOperation Op;
   eTweqDirection Dir;

   // the default empty constructor generates a null message
   // the idea is if there are problems with this, we find them because the type
   //   will be null, then we go determine whose "fault" it is, and fix it
   // scripts can check for null, but i think we'd like them not to?
   sTweqMsg()
      : Type(kTweqTypeNull)
   {
   }

   sTweqMsg(ObjID to, eTweqType type, eTweqOperation op, eTweqDirection dir)
      : sScrMsg(to,"TweqComplete"), Type(type), Op(op), Dir(dir)
   {
   }

   DECLARE_SCRMSG_PERSISTENT();
};

#ifdef SCRIPT
#define OnTweqComplete()               SCRIPT_MESSAGE_HANDLER_SIGNATURE_(TweqComplete,sTweqMsg)
#define DefaultOnTweqComplete()        SCRIPT_CALL_BASE(TweqComplete)
#endif

// TweqComplete is part of the EngineRootScript, so the MessageMap is in engscrpt.h

// Name for A/R backend since scripts and others might care
#define REACTION_TWEQ_CONTROL "tweq_control"

#endif  // __TWQSCRPT_H
