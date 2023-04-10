/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmtscpt.cpp,v 1.2 1998/07/17 11:13:38 CCAROLLO Exp $
//
// Physics moving terrain scripting message implementations
//

#include <lg.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <phmtscpt.h>

// Must be last header
#include <dbmem.h>

IMPLEMENT_SCRMSG_PERSISTENT(sMovingTerrainMsg)
{
   PersistenceHeader(sScrMsg, kMovingTerrainMsgVer);
   Persistent(waypoint);
   return TRUE;
}

IMPLEMENT_SCRMSG_PERSISTENT(sWaypointMsg)
{
   PersistenceHeader(sScrMsg, kWaypointMsgVer);
   Persistent(moving_terrain);
   return TRUE;
}

