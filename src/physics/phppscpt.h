/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phppscpt.h,v 1.1 1998/07/16 18:25:40 CCAROLLO Exp $
//
// Physics pressure plate scripting message declarations
//

#ifndef __PHPPSCPT_H
#define __PHPPSCPT_H

#pragma once

#include <comtools.h>
#include <appagg.h>

#include <scrptmsg.h>
#include <objtype.h>
#include <objscrt.h>

////////////////////////////////////////

EXTERN void PressurePlateMessage(ObjID objID, int message);

////////////////////////////////////////

#define kPressurePlateMsgVer 1

enum ePPState
{
   kPressurePlateInactive,
   kPressurePlateActive,
   kPressurePlateDeactivating,
   kPressurePlateActivating,
};

static const char *state_names[] =
{
   "PressurePlateInactive",
   "PressurePlateActive",
   "PressurePlateDeactivating",
   "PressurePlateActivating"
};

struct sPressurePlateMsg : public sScrMsg
{
   sPressurePlateMsg()
   {
   }

   sPressurePlateMsg(ObjID pp_obj, int state)
   {
      Assert_((state >= 0) && (state < 4));

      to = pp_obj;

      free((void *)message);
      message = strdup(state_names[state]);
   }

   DECLARE_SCRMSG_PERSISTENT();
};

////////////////////////////////////////

#ifdef SCRIPT
#define OnPressurePlateInactive()              SCRIPT_MESSAGE_HANDLER_SIGNATURE_(PressurePlateInactive, sPressurePlateMsg)
#define DefaultOnPressurePlateInactive()       SCRIPT_CALL_BASE(PressurePlateInactive)
#define OnPressurePlateActive()                SCRIPT_MESSAGE_HANDLER_SIGNATURE_(PressurePlateActive, sPressurePlateMsg)
#define DefaultOnPressurePlateActive()         SCRIPT_CALL_BASE(PressurePlateActive)
#define OnPressurePlateDeactivating()          SCRIPT_MESSAGE_HANDLER_SIGNATURE_(PressurePlateDeactivating, sPressurePlateMsg)
#define DefaultOnPressurePlateDeactivating()   SCRIPT_CALL_BASE(PressurePlateDeactivating)
#define OnPressurePlateActivating()            SCRIPT_MESSAGE_HANDLER_SIGNATURE_(PressurePlateActivating, sPressurePlateMsg)
#define DefaultOnPressurePlateActivating()     SCRIPT_CALL_BASE(PressurePlateActivating)
#endif

////////////////////////////////////////

#ifdef SCRIPT

BEGIN_SCRIPT(PressurePlate, RootScript)

   MESSAGE_DEFS:

      SCRIPT_DEFINE_MESSAGE_DEFAULT_(PressurePlateInactive,     sPressurePlateMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(PressurePlateActive,       sPressurePlateMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(PressurePlateDeactivating, sPressurePlateMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(PressurePlateActivating,   sPressurePlateMsg)

   SCRIPT_BEGIN_MESSAGE_MAP()
      SCRIPT_MSG_MAP_ENTRY(PressurePlateInactive)
      SCRIPT_MSG_MAP_ENTRY(PressurePlateActive)
      SCRIPT_MSG_MAP_ENTRY(PressurePlateDeactivating)
      SCRIPT_MSG_MAP_ENTRY(PressurePlateActivating)
   SCRIPT_END_MESSAGE_MAP()

END_SCRIPT(PressurePlate)

#endif

////////////////////////////////////////

#endif









