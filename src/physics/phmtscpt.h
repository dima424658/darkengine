/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmtscpt.h,v 1.4 1999/01/21 20:10:42 JUSTIN Exp $
//
// Physics moving terrain scripting message declarations
//

#ifndef __PHMTSCPT_H
#define __PHMTSCPT_H

#pragma once

#include <scrptmsg.h>
#include <objtype.h>
#include <objscrt.h>

#define kMovingTerrainMsgVer 1
#define kWaypointMsgVer 1

struct sMovingTerrainMsg : public sScrMsg
{
   ObjID waypoint;

   sMovingTerrainMsg()
   {
   }

   sMovingTerrainMsg(ObjID mt_obj, ObjID waypoint_obj)
      : sScrMsg(mt_obj, "MovingTerrainWaypoint"), 
        waypoint(waypoint_obj)
   {
      // Waypoint messages get sent, even if the object's a proxy:
      flags |= kSMF_MsgSendToProxy;
   }

   DECLARE_SCRMSG_PERSISTENT();
};

struct sWaypointMsg : public sScrMsg
{
   ObjID moving_terrain;

   sWaypointMsg()
   {
   }

   sWaypointMsg(ObjID mt_obj, ObjID waypoint_obj)
      : sScrMsg(waypoint_obj, "WaypointReached"),
        moving_terrain(mt_obj)
   {
   }

   DECLARE_SCRMSG_PERSISTENT();
};

#ifdef SCRIPT

#define OnMovingTerrainWaypoint()         SCRIPT_MESSAGE_HANDLER_SIGNATURE_(MovingTerrainWaypoint, sMovingTerrainMsg)
#define DefaultOnMovingTerrainWaypoint()  SCRIPT_CALL_BASE(MovingTerrainWaypoint)

#define OnWaypointReached()               SCRIPT_MESSAGE_HANDLER_SIGNATURE_(WaypointReached, sWaypointMsg)
#define DefaultOnWaypointReached()        SCRIPT_CALL_BASE(WaypointReached)

#endif

////////////////////////////////////////
//
// The base moving terrain script
//

#ifdef SCRIPT

BEGIN_SCRIPT(MovingTerrain, RootScript)

   MESSAGE_DEFS:

      SCRIPT_DEFINE_MESSAGE_DEFAULT_(MovingTerrainWaypoint, sMovingTerrainMsg)

      SCRIPT_BEGIN_MESSAGE_MAP()
         SCRIPT_MSG_MAP_ENTRY(MovingTerrainWaypoint)
      SCRIPT_END_MESSAGE_MAP()

END_SCRIPT(MovingTerrain)

BEGIN_SCRIPT(Waypoint, RootScript)

   MESSAGE_DEFS:

      SCRIPT_DEFINE_MESSAGE_DEFAULT_(WaypointReached, sWaypointMsg)

      SCRIPT_BEGIN_MESSAGE_MAP()
         SCRIPT_MSG_MAP_ENTRY(WaypointReached)
      SCRIPT_END_MESSAGE_MAP()

END_SCRIPT(Waypoint)

#endif

#endif // __PHMTSCPT_H












