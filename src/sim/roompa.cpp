/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/roompa.cpp,v 1.10 1998/08/26 14:10:14 CCAROLLO Exp $
//
// cRoomPropAgent functions
//

//#define PROFILE_ON

#include <lg.h>
#include <timings.h>
#include <config.h>
#include <hashpp.h>
#include <hshpptem.h>

#include <roombase.h>
#include <rooms.h>
#include <room.h>
#include <rportal.h>
#include <roompa.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>

#define kMaxDistDiff 10.0

////////////////////////////////////////

cRoomPropAgent::cRoomPropAgent()
 : m_Funcs(NULL)
{

}

////////////////////////////////////////

cRoomPropAgent::~cRoomPropAgent()
{

}

////////////////////////////////////////////////////////////////////////////////

DECLARE_TIMER(AI_BF_Prop, Average);

#ifndef SHIP
int num_queued;
int num_visited;
#endif

void cRoomPropAgent::PropagateBF(const mxs_vector &startPos, const cRoom *startRoom)
{
   AUTO_TIMER(AI_BF_Prop);

   static BOOL spew = config_is_defined("RoomPropSpew");
   static BOOL spew_stats = config_is_defined("RoomPropSpewStats");

   const cRoom  *pHeadRoom;
   int active_room_head;
   int active_room_tail;
   int active_room_ptr;

   int enter_portal_index;

   const cRoomPortal   *pEnterPortal;
   const cRoomPortal   *pNextPortal;
   mxs_real  dist, new_dist, adj_dist;
   int       i, j, k;

   if (g_pRooms->GetNumRooms() > MAX_BF_ROOMS)
   {
      CriticalMsg("Too many rooms, increase MAX_BF_ROOMS!");
      return;
   }

   #ifndef SHIP
   num_queued = 1;
   num_visited = 1;
   #endif

   // Blast the room info array
   memset(m_BFRoomInfo, 0, sizeof(cBFRoomInfo) * MAX_BF_ROOMS);

   // Add the starting room to the list and tables
   active_room_head = active_room_tail = active_room_ptr = startRoom->GetRoomID();
   m_BFRoomInfo[active_room_head].Init(0.1, -1, -1, -1, -1);

   // Propagate
   while (active_room_ptr != -1)
   {
      // Get the room class
      pHeadRoom = g_pRooms->GetRoom(active_room_head);

      // We've queued it, so it should have a distance
      if ((dist = m_BFRoomInfo[active_room_head].GetDist()) == 0)
         CriticalMsg("No dist value during propagation!\n");

      enter_portal_index = m_BFRoomInfo[active_room_head].GetPreviousPortal();      

      if (enter_portal_index == -1)
         pEnterPortal = NULL;
      else
         pEnterPortal = pHeadRoom->GetPortal(enter_portal_index);

      #ifndef SHIP
      if (spew)
         mprintf("Examining %d\n", active_room_head);
      #endif

      // Call the enter-room callback
      if (pEnterPortal)
      {
         if (!m_Funcs->EnterCallback(pHeadRoom, pEnterPortal, pEnterPortal->GetCenter(), dist))
            goto remove_cur_room;

         if (m_BFRoomInfo[active_room_head].GetPreviousRoom2() >= 0)
         {
            m_BFRoomInfo[active_room_head].SwitchPreviousRooms();

            if (!m_Funcs->EnterCallback(pHeadRoom, pEnterPortal, startPos, dist))
               goto remove_cur_room;
            m_BFRoomInfo[active_room_head].SwitchPreviousRooms();
         }
      }
      else
      {
         if (!m_Funcs->EnterCallback(pHeadRoom, pEnterPortal, startPos, dist))
            goto remove_cur_room;
      }

      #ifndef SHIP
      num_visited++;
      #endif

      // Add all the portal-adjacent rooms to the ordered list
      for (i=0; i<pHeadRoom->GetNumPortals(); i++)
      {
         BOOL add_room = FALSE;

         // Don't add the room from which we came
         if (i == enter_portal_index)
            continue;

         pNextPortal = pHeadRoom->GetPortal(i);

         // Determine distance to new portal
         if (pEnterPortal)
         {
            new_dist = pHeadRoom->GetPortalDist(i, enter_portal_index);
            adj_dist = m_Funcs->PortalsCallback(pEnterPortal, pNextPortal, dist + new_dist);
            
            // Negative return means to stop propagating
            if (adj_dist > 0)
            {
               Assert_((adj_dist + 0.0001) >= (dist + new_dist));
               add_room = TRUE;
            }
         }
         else
         {
            new_dist = mx_dist_vec(&pNextPortal->GetCenter(), &startPos) + 0.1;
            adj_dist = m_Funcs->PortalsCallback(pEnterPortal, pNextPortal, new_dist);

            // Negative return means to stop propagating
            if (adj_dist > 0)
               add_room = TRUE;
         }

         if (add_room)
         {
            cRoom    *pNewRoom;
            int       new_room_id;
            mxs_real  room_dist;

            pNewRoom = pNextPortal->GetFarRoom();
            new_room_id = pNewRoom->GetRoomID();

            // Find portal index of portal on other side of the current portal
            j = pNextPortal->GetFarPortal()->GetIndex();

            // Check if we've already visited
            if ((room_dist = m_BFRoomInfo[new_room_id].GetDist()) > 0)
            {
               if ((room_dist + kMaxDistDiff) <= adj_dist)
                  continue;

               if (m_BFRoomInfo[new_room_id].GetPreviousRoom2() < 0)
               {
                  m_BFRoomInfo[new_room_id].SetPreviousRoom2(active_room_head);
                  m_BFRoomInfo[new_room_id].SetPreviousPortal2(j);
               }
            }
            else
            {
               #ifndef SHIP
               if (spew)
                  mprintf(" queueing %d\n", pNewRoom->GetRoomID());

               num_queued++;
               #endif

               // Init new room
               m_BFRoomInfo[new_room_id].Init(adj_dist, active_room_head, j, -1, -1);

               // Add it to active list (insertion sort)
               // Do it backwards, because we're more likely to insert toward the end of the list
               k = active_room_tail;

               while ((k != -1) && (m_BFRoomInfo[k].GetDist() > adj_dist))
                  k = m_BFRoomInfo[k].GetPrevActive();
 
               AssertMsg(k >= 0, "Inserting at head of BF prop active list?");

               // Inserting after node k
               if (m_BFRoomInfo[k].GetNextActive() < 0)
               {
                  // Appending to list
                  m_BFRoomInfo[k].SetNextActive(new_room_id);
                  m_BFRoomInfo[new_room_id].SetPrevActive(k);
                  active_room_tail = new_room_id;
               }
               else
               {
                  // Middle of list
                  m_BFRoomInfo[new_room_id].SetNextActive(m_BFRoomInfo[k].GetNextActive());
                  m_BFRoomInfo[new_room_id].SetPrevActive(k);
                  m_BFRoomInfo[m_BFRoomInfo[k].GetNextActive()].SetPrevActive(new_room_id);
                  m_BFRoomInfo[k].SetNextActive(new_room_id);
               }
            }
         }
      }

remove_cur_room:

      AssertMsg(active_room_ptr == active_room_head, "Active room is not at head of BF prop active list?");

      // from head of list
      active_room_head = m_BFRoomInfo[active_room_head].GetNextActive();
      if (active_room_head > 0)
         m_BFRoomInfo[active_room_head].SetPrevActive(-1);
      m_BFRoomInfo[active_room_ptr].SetNextActive(-1);

      active_room_ptr = active_room_head;
   }

   #ifndef SHIP
   if (spew_stats)
      mprintf("  %d rooms queued, %d visited\n", num_queued, num_visited);
   #endif
}

////////////////////////////////////////////////////////////////////////////////

#ifndef SHIP
int depth_accum;
int num_branches;
#endif

void cRoomPropAgent::PropagateRec(const cRoom *curRoom, const cRoomPortal *enterPortal, 
                                  const mxs_vector &enterPt, mxs_real dist, int depth)
{
   static cRoomPortal *next_portal;
   static mxs_real     distance;
   static mxs_real     adj_distance;  
   int enter_portal_index;
   int i;

   // Don't recurse more than max
   if (depth == kMaxDepth)
      return;

   // Let instantiator know we entered the room
   if(!m_Funcs->EnterCallback(curRoom, enterPortal, enterPt, dist))
      return;

   // Find index of entrance portal
   if (enterPortal == NULL)
      enter_portal_index = -1;
   else
   {
      for (i=0; i<curRoom->GetNumPortals(); i++)
      {
         next_portal = curRoom->GetPortal(i);
         if (next_portal->GetFarRoom() == enterPortal->GetNearRoom())
            break;
      }
      enter_portal_index = i;
   }

   AssertMsg(enter_portal_index != curRoom->GetNumPortals(), "Unable to find entrance portal");
   AssertMsg(enter_portal_index < curRoom->GetNumPortals(), "Invalid entrance portal");

   // Propagate outward through each portal
   for (i=0; i<curRoom->GetNumPortals(); i++)
   {
      // Make sure we're not backing up
      if (i == enter_portal_index)
         continue;

      next_portal = curRoom->GetPortal(i);
      if (enterPortal != NULL)
      {
         // Check if we can go between
         distance = curRoom->GetPortalDist(i, enter_portal_index);
         adj_distance = m_Funcs->PortalsCallback(enterPortal, next_portal, dist + distance);
         if (adj_distance > 0)
         {
            PropagateRec(next_portal->GetFarRoom(), next_portal, next_portal->GetCenter(), adj_distance, depth + 1);
         }
      }
      else
      {
         // We started in this room, so find starting distnace ("dist" is really location)
         distance = mx_dist_vec((mxs_vector *)&next_portal->GetCenter(), (mxs_vector *)&enterPt);
         adj_distance = m_Funcs->PortalsCallback(enterPortal, next_portal, distance);
         if (adj_distance > 0)
         {
            PropagateRec(next_portal->GetFarRoom(), next_portal, next_portal->GetCenter(), adj_distance, depth + 1);
         }
      }
   }

   #ifndef SHIP
   depth_accum += depth;
   num_branches++;
   #endif

   // Let instantiator know we're exiting the room
   m_Funcs->ExitCallback(curRoom);
}

////////////////////////////////////////

#ifndef SHIP
int   num_in_avg = 0;
float avg_depth = 0.0;
float max_depth = 0.0;
float min_depth = 1000.0;
#endif

void cRoomPropAgent::PropagateDF(const mxs_vector &startPos, const cRoom *startRoom)
{
   #ifndef SHIP
   depth_accum = 0;
   num_branches = 0;
   #endif

   PropagateRec(startRoom, NULL, startPos, 0, 0);

   #ifndef SHIP

   float cur_depth = ((float)depth_accum) / ((float)num_branches);

   if (config_is_defined("RoomPropSpew"))
      mprintf("  %d branches with %g average depth\n", num_branches, cur_depth);

   num_in_avg++;
   avg_depth += cur_depth;

   if (cur_depth < min_depth)
      min_depth = cur_depth;
   if (cur_depth > max_depth)
      max_depth = cur_depth;

   if (config_is_defined("RoomPropSpewStats"))
   {
      mprintf("  avg_depth = %g\n", avg_depth / num_in_avg);
      mprintf("  min_depth = %g\n", min_depth);
      mprintf("  max_depth = %g\n", max_depth);
      mprintf("\n");
   }
   #endif
}

////////////////////////////////////////////////////////////////////////////////

void cRoomPropAgent::GetRoomPath(const cRoom *endRoom, cRoomPropResultList *pResultList) const
{
   const cRoom *prev_room = endRoom;
   short prev_room_id = endRoom->GetRoomID();
   cRoomPropResult *result;   
   int   depth;

   pResultList->DestroyAll();

   depth = 0;
   while (prev_room_id != -1)
   {
      depth++;
      AssertMsg(depth < 70, "Abnormally long room path length");

      prev_room = g_pRooms->GetRoom(prev_room_id);

      result = new cRoomPropResult(prev_room);

      pResultList->Prepend(result);     
 
      prev_room_id = m_BFRoomInfo[prev_room_id].GetPreviousRoom();;
   }
}
     







