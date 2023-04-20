/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/script/atkscrpt.h,v 1.1 1998/10/20 21:16:41 CCAROLLO Exp $
//

#ifndef __ATKSCRPT_H
#define __ATKSCRPT_H

#pragma once

#include <scrptmsg.h>
#include <objtype.h>
#include <objscrt.h>

//////////////////////////////////////
//
// Messages
//

#define kAttackMsgVer 1

enum eAttackMsgType
{
   kAMT_StartWindup,
   kAMT_StartAttack,
   kAMT_EndAttack
};

struct sAttackMsg : public sScrMsg
{
   ObjID weapon;

   sAttackMsg()
   {
   }

   sAttackMsg(ObjID to_, ObjID weapon_, int type)
      : weapon(weapon_)
   {
      static const char *attack_msg_names[] = { "StartWindup", "StartAttack", "EndAttack" };

      to = to_;
      free((void *)message);
      message = strdup(attack_msg_names[type]);
   }
   
   DECLARE_SCRMSG_PERSISTENT();
};

#ifdef SCRIPT
#define OnStartWindup()        SCRIPT_MESSAGE_HANDLER_SIGNATURE_(StartWindup, sAttackMsg)
#define DefaultOnStartWindup() SCRIPT_CALL_BASE(StartWindup)
#define OnStartAttack()        SCRIPT_MESSAGE_HANDLER_SIGNATURE_(StartAttack, sAttackMsg)
#define DefaultOnStartAttack() SCRIPT_CALL_BASE(StartAttack)
#define OnEndAttack()          SCRIPT_MESSAGE_HANDLER_SIGNATURE_(EndAttack, sAttackMsg)
#define DefaultOnEndAttack()   SCRIPT_CALL_BASE(EndAttack)
#endif

#ifdef SCRIPT

BEGIN_SCRIPT(Attack, RootScript)

   MESSAGE_DEFS:

      SCRIPT_DEFINE_MESSAGE_DEFAULT_(StartWindup, sAttackMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(StartAttack, sAttackMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(EndAttack,   sAttackMsg)

      SCRIPT_BEGIN_MESSAGE_MAP()
         SCRIPT_MSG_MAP_ENTRY(StartWindup)
         SCRIPT_MSG_MAP_ENTRY(StartAttack)
         SCRIPT_MSG_MAP_ENTRY(EndAttack)
      SCRIPT_END_MESSAGE_MAP()

END_SCRIPT(Attack)

#endif

#endif
