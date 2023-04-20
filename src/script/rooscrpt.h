/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/rooscrpt.h,v 1.4 1999/04/13 19:02:43 Justin Exp $
// 

#pragma once

#ifndef __ROOSCRPT_H
#define __ROOSCRPT_H

#include <scrptmsg.h>

#include <objtype.h>

//////////////////////////////////////
//
// MESSAGES: Room services
//    RoomEnter, RoomExit, for Player, RemotePlayer, Creature, or Object

#define kRoomMsgVer 1

struct sRoomMsg : public sScrMsg
{
   enum eRoomChange
   {
      kEnter,
      kExit,
      kRoomTransit,
      kRoomChangeSpaceWasterJoy=0xffffffff
   };

   enum eObjType
   {
      kPlayer,
      kRemotePlayer,
      kCreature,
      kObject,
      kNull,
      kObjTypeSpaceWasterJoy=0xffffffff
   };

   // ObjId's for the act
   ObjID       FromObjId;
   ObjID       ToObjId;
   ObjID       MoveObjId;

   // data about what is going on in the message
   eObjType    ObjType;
   eRoomChange TransitionType;

   // the default empty constructor generates a null message
   // the idea is if there are problems with this, we find them because the type
   //   will be null, then we go determine whose "fault" it is, and fix it
   // scripts can check for null, but i think we'd like them not to?
   sRoomMsg()
      : ObjType(kNull)
   {
      // We have to deal with these messages on the proxies, since
      // script networking doesn't yet cope with extended msgs
      flags |= kSMF_MsgSendToProxy;
   }

   sRoomMsg(ObjID froom, ObjID toroom, ObjID moveobj, eObjType objt, eRoomChange transt)
      : FromObjId(froom), ToObjId(toroom), MoveObjId(moveobj), ObjType(objt), TransitionType(transt)
   {
      static const char *room_msg_names[4][2]=
      {
         { "PlayerRoomEnter", "PlayerRoomExit" },
         { "RemotePlayerRoomEnter", "RemotePlayerRoomExit" },
         { "CreatureRoomEnter", "CreatureRoomExit" },
         { "ObjectRoomEnter", "ObjectRoomExit" },
      };
      // now setup the base scrMsg fields
      to=(transt==kEnter)?toroom:froom;
      free((void *)message);
      message=strdup(room_msg_names[objt][transt]);
      // We have to deal with these messages on the proxies, since
      // script networking doesn't yet cope with extended msgs
      flags |= kSMF_MsgSendToProxy;
   }

   sRoomMsg(ObjID froom, ObjID toroom, ObjID moveobj, eObjType objt)
      : sScrMsg(moveobj,"ObjRoomTransit"),
         FromObjId(froom), ToObjId(toroom), MoveObjId(moveobj), ObjType(objt), TransitionType(kRoomTransit)
   {
      // We have to deal with these messages on the proxies, since
      // script networking doesn't yet cope with extended msgs
      flags |= kSMF_MsgSendToProxy;
   }

   DECLARE_SCRMSG_PERSISTENT();
};

#ifdef SCRIPT
#define OnPlayerRoomEnter()               SCRIPT_MESSAGE_HANDLER_SIGNATURE_(PlayerRoomEnter,sRoomMsg)
#define DefaultOnPlayerRoomEnter()        SCRIPT_CALL_BASE(PlayerRoomEnter)
#define OnPlayerRoomExit()                SCRIPT_MESSAGE_HANDLER_SIGNATURE_(PlayerRoomExit,sRoomMsg)
#define DefaultOnPlayerRoomExit()         SCRIPT_CALL_BASE(PlayerRoomExit)
#define OnRemotePlayerRoomEnter()         SCRIPT_MESSAGE_HANDLER_SIGNATURE_(RemotePlayerRoomEnter,sRoomMsg)
#define DefaultOnRemotePlayerRoomEnter()  SCRIPT_CALL_BASE(RemotePlayerRoomEnter)
#define OnRemotePlayerRoomExit()          SCRIPT_MESSAGE_HANDLER_SIGNATURE_(RemotePlayerRoomExit,sRoomMsg)
#define DefaultOnRemotePlayerRoomExit()   SCRIPT_CALL_BASE(RemotePlayerRoomExit)
#define OnCreatureRoomEnter()             SCRIPT_MESSAGE_HANDLER_SIGNATURE_(CreatureRoomEnter,sRoomMsg)
#define DefaultOnCreatureRoomEnter()      SCRIPT_CALL_BASE(CreatureRoomEnter)
#define OnCreatureRoomExit()              SCRIPT_MESSAGE_HANDLER_SIGNATURE_(CreatureRoomExit,sRoomMsg)
#define DefaultOnCreatureRoomExit()       SCRIPT_CALL_BASE(CreatureRoomExit)
#define OnObjectRoomEnter()               SCRIPT_MESSAGE_HANDLER_SIGNATURE_(ObjectRoomEnter,sRoomMsg)
#define DefaultOnObjectRoomEnter()        SCRIPT_CALL_BASE(ObjectRoomEnter)
#define OnObjectRoomExit()                SCRIPT_MESSAGE_HANDLER_SIGNATURE_(ObjectRoomExit,sRoomMsg)
#define DefaultOnObjectRoomExit()         SCRIPT_CALL_BASE(ObjectRoomExit)
#define OnObjRoomTransit()                SCRIPT_MESSAGE_HANDLER_SIGNATURE_(ObjRoomTransit,sRoomMsg)
#define DefaultOnObjRoomTransit()         SCRIPT_CALL_BASE(ObjRoomTransit)
#endif

// DECLARE_SCRIPT_SERVICE(Rooms, 0xf0)

///////////////
// the base room script

#ifdef SCRIPT

BEGIN_SCRIPT(Room, RootScript)

   MESSAGE_DEFS:

      SCRIPT_DEFINE_MESSAGE_DEFAULT_(PlayerRoomEnter       ,sRoomMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(PlayerRoomExit        ,sRoomMsg)   
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(RemotePlayerRoomEnter ,sRoomMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(RemotePlayerRoomExit  ,sRoomMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(CreatureRoomEnter     ,sRoomMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(CreatureRoomExit      ,sRoomMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(ObjectRoomEnter       ,sRoomMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(ObjectRoomExit        ,sRoomMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(ObjRoomTransit        ,sRoomMsg)

      SCRIPT_BEGIN_MESSAGE_MAP()
         SCRIPT_MSG_MAP_ENTRY(PlayerRoomEnter)
         SCRIPT_MSG_MAP_ENTRY(PlayerRoomExit)
         SCRIPT_MSG_MAP_ENTRY(RemotePlayerRoomEnter)
         SCRIPT_MSG_MAP_ENTRY(RemotePlayerRoomExit)
         SCRIPT_MSG_MAP_ENTRY(CreatureRoomEnter)
         SCRIPT_MSG_MAP_ENTRY(CreatureRoomExit)
         SCRIPT_MSG_MAP_ENTRY(ObjectRoomEnter)
         SCRIPT_MSG_MAP_ENTRY(ObjectRoomExit)
         SCRIPT_MSG_MAP_ENTRY(ObjRoomTransit)
      SCRIPT_END_MESSAGE_MAP()

END_SCRIPT(Room)

#endif  // SCRIPT

#endif  // __ROOSCRPT_H
