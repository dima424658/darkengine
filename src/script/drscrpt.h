/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/script/drscrpt.h,v 1.10 2000/02/24 13:46:54 patmac Exp $
//

#ifndef __DRSCRPT_H
#define __DRSCRPT_H

#pragma once

#include <scrptmsg.h>
#include <objtype.h>
#include <objscrt.h>

#include <doorphys.h>

//////////////////////////////////////
//
// Services
//

DECLARE_SCRIPT_SERVICE(Door, 0xf6)
{
   // Start a door closing.  Returns FALSE if already closing or closed.
   STDMETHOD_(BOOL, CloseDoor)(object door_obj) PURE;

   // Start a door opening.  Returns FALSE if already opening or open.
   STDMETHOD_(BOOL, OpenDoor)(object door_obj) PURE;

   // return a doorphys state
   STDMETHOD_(int, GetDoorState)(object door_obj) PURE;

   // Toggle a door's motion (opens if closing or closed, closes if open
   // or opening).
   STDMETHOD(ToggleDoor)(object door_obj) PURE;

   // Enable/disable a door's blocking state
   STDMETHOD(SetBlocking)(object door_obj, BOOL state) PURE;

   // return if a door is blocking sound
   STDMETHOD_(BOOL, GetSoundBlocking)(object door_obj) PURE;
};

//////////////////////////////////////
//
// Messages
//
//    DoorOpen, DoorClose, DoorOpening, DoorClosing

#define kDoorMsgVer 1

struct sDoorMsg : public sScrMsg
{
   enum eDoorAction
   {
      kOpen = 0x00000000,
      kClose,
      kOpening,
      kClosing,
      kHalt,
      kToMakeBig = 0xFFFFFFFF
   };

   // what the deal is
   eDoorAction  ActionType;
   eDoorAction  PrevActionType;   

   // TRUE iff this is a proxy door, so scripts may wish to curtail
   // their actions. If this is not a network game, isProxy will always
   // be FALSE.
   BOOL isProxy;

   sDoorMsg()
   {
   }

   // real constructor
   sDoorMsg(ObjID obj_to, eDoorAction action, eDoorAction prev_action,
            BOOL proxy)
      : ActionType(action),
        PrevActionType(prev_action),
        isProxy(proxy)
   {
      static const char *room_msg_names[5] = { "DoorOpen", "DoorClose", "DoorOpening", "DoorClosing", "DoorHalt" };

      // setup base scrMsg fields
      to = obj_to;
      free((void *)message);
      message = strdup(room_msg_names[action]);
      // We are specifically allowing these messages to go to proxies:
      flags |= kSMF_MsgSendToProxy;
   }
         
   DECLARE_SCRMSG_PERSISTENT();

};

#ifdef SCRIPT
#define OnDoorOpen()            SCRIPT_MESSAGE_HANDLER_SIGNATURE_(DoorOpen, sDoorMsg)
#define DefaultOnDoorOpen()     SCRIPT_CALL_BASE(DoorOpen)
#define OnDoorClose()           SCRIPT_MESSAGE_HANDLER_SIGNATURE_(DoorClose, sDoorMsg)
#define DefaultOnDoorClose()    SCRIPT_CALL_BASE(DoorClose)
#define OnDoorOpening()         SCRIPT_MESSAGE_HANDLER_SIGNATURE_(DoorOpening, sDoorMsg)
#define DefaultOnDoorOpening()  SCRIPT_CALL_BASE(DoorOpening)
#define OnDoorClosing()         SCRIPT_MESSAGE_HANDLER_SIGNATURE_(DoorClosing, sDoorMsg)
#define DefaultOnDoorClosing()  SCRIPT_CALL_BASE(DoorClosing)
#define OnDoorHalt()            SCRIPT_MESSAGE_HANDLER_SIGNATURE_(DoorHalt, sDoorMsg)
#define DefaultOnDoorHalt()     SCRIPT_CALL_BASE(DoorHalt)
#endif

////////////////////////////////////////
//
// The base room script
//

#ifdef SCRIPT

BEGIN_SCRIPT(Door, RootScript)

   MESSAGE_DEFS: 
  
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(DoorOpen,    sDoorMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(DoorClose,   sDoorMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(DoorOpening, sDoorMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(DoorClosing, sDoorMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(DoorHalt,    sDoorMsg)

      SCRIPT_BEGIN_MESSAGE_MAP()
         SCRIPT_MSG_MAP_ENTRY(DoorOpen)
         SCRIPT_MSG_MAP_ENTRY(DoorClose)
         SCRIPT_MSG_MAP_ENTRY(DoorOpening)
         SCRIPT_MSG_MAP_ENTRY(DoorClosing)
         SCRIPT_MSG_MAP_ENTRY(DoorHalt)
      SCRIPT_END_MESSAGE_MAP()

END_SCRIPT(Door)

#endif

#endif
