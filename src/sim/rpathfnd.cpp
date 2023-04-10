/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/rpathfnd.cpp,v 1.13 1998/10/03 11:27:13 TOML Exp $
//
// Room pathfinding functions
//

#if 0
#include <lg.h>
#include <timings.h>
#include <matrixs.h>
#include <objpos.h>
#include <playrobj.h>
#include <ged_line.h>
#include <gedit.h>

#include <timer.h>
#include <command.h>

#include <aipthloc.h>
#include <aitype.h>
#include <aipathdb.h>
#include <aipath.h>

#include <rpathfnd.h>
#include <roombase.h>
#include <roompa.h>
#include <rooms.h>
#include <roomutil.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>

cRoomPathFind *g_pRoomPathFindSys = NULL;

////////////////////////////////////////////////////////////////////////////////

void RoomPathSpew(void);

#ifndef SHIP
static Command RoomPathFindCommands[] =
{
   {"find_room_path",    FUNC_INT,    FindMyPath,        "Find room path from player to object",   HK_ALL},
   {"find_room_path_t",  FUNC_INT,    FindMyPathTimed,   "Find room path from player to object",   HK_ALL},
   {"new_pathfind",      TOGGLE_BOOL, &g_UseNewPathfind, "Use the new room pathfinding",           HK_ALL},
};
#endif

////////////////////////////////////////////////////////////////////////////////

void InitRoomPathFind()
{
   Assert_(g_pRoomPathFindSys == NULL);

   g_pRoomPathFindSys = new cRoomPathFind;

   #ifndef SHIP
   COMMANDS(RoomPathFindCommands, HK_ALL);
   #endif
}

////////////////////////////////////////

void TermRoomPathFind()
{
   Assert_(g_pRoomPathFindSys != NULL);

   delete g_pRoomPathFindSys;
   g_pRoomPathFindSys = NULL;
}

////////////////////////////////////////

void ClearRoomPathFind()
{
   Assert_(g_pRoomPathFindSys != NULL);

   g_pAIRoomDB->Clear();
}

////////////////////////////////////////

void FindMyPathTimed(ObjID objID)
{
   Assert_(g_pRoomPathFindSys);

   cAIPath  Path;
   cMxsVector from, to;
   int i;

   int new_start, new_end;
   int old_start, old_end;

   int from_hint, to_hint;

   from.Set(ObjPosGet(PlayerObject())->loc.vec);
   to.Set(ObjPosGet(objID)->loc.vec);

   from_hint = AIFindClosestCell(from, 0);
   to_hint = AIFindClosestCell(to, 0);

   g_UseNewPathfind = TRUE;

   new_start = tm_get_millisec();
   for (i=0; i<1000; i++)
      AIRoomPathFind(from, from_hint, to, to_hint, &Path, kAIOK_All);
   new_end = tm_get_millisec();

   g_UseNewPathfind = FALSE;

   old_start = tm_get_millisec();
   for (i=0; i<1000; i++)
      AIRoomPathFind(from, from_hint, to, to_hint, &Path, kAIOK_All);
   old_end = tm_get_millisec();

   g_UseNewPathfind = TRUE;

   mprintf(" new = %g milliseconds per pathfind\n", (float)(new_end - new_start) / 1000);
   mprintf(" old = %g milliseconds per pathfind\n", (float)(old_end - old_start) / 1000);
}

////////////////////////////////////////

void FindMyPath(ObjID objID)
{
#ifdef EDITOR
   Assert_(g_pRoomPathFindSys);

   cAIPath Path;
   cAIPathEdge edge;
   cMxsVector    from, to;
   int i;

   from.Set(ObjPosGet(PlayerObject())->loc.vec);
   to.Set(ObjPosGet(objID)->loc.vec);

   gedDeleteChannelLines(1<<LINE_CH_AI);

   if (AIRoomPathFind(from, 0, to, 0, &Path, kAIOK_All))
   {
      ged_line_load_channel = LINE_CH_AI;
      ged_line_view_channels |= (1<<LINE_CH_AI);

      cMxsVector efrom, eto;
      BOOL     drawn_any = FALSE;

      for (i=0; i < Path.GetPathEdgeCount() - 1; i++)
      {
         drawn_any = TRUE;

         Path.GetPathEdge(i, &edge);
         edge.GetCenter(&efrom);

         Path.GetPathEdge(i+1, &edge);
         edge.GetCenter(&eto);

         if (i == 1)
            gedLineAddPal(&to, &efrom, 0, 238);

         gedLineAddPal(&efrom, &eto, 0, 238);
      }

      if (!drawn_any)
      {
         if (Path.GetPathEdgeCount() == 1)
         {
            Path.GetPathEdge(0, &edge);
            edge.GetCenter(&efrom);

            gedLineAddPal(&from, &efrom, 0, 238);
            gedLineAddPal(&efrom, &to, 0, 238);
         }
         else
            gedLineAddPal(&from, &to, 0, 238);
      }
      else
         gedLineAddPal(&eto, &from, 0, 238);
   }

   gedit_full_redraw();

#endif // EDITOR
}

////////////////////////////////////////////////////////////////////////////////

cRoomPathFind::cRoomPathFind()
{
   m_RoomPropAgent.SetFunctions(this);
}

////////////////////////////////////////

cRoomPathFind::~cRoomPathFind()
{

}

////////////////////////////////////////////////////////////////////////////////

BOOL cRoomPathFind::FindPath(const mxs_vector &from_vec, tAIPathCellID startCell, 
                             const mxs_vector &to_vec, tAIPathCellID endCell)
{
   cRoom *start_room;

   start_room = g_pRooms->RoomFromPoint(from_vec);

   if (start_room == NULL)
      return FALSE;

   m_FromObj = -1;
   m_ToObj = -1;

   mx_copy_vec(&m_FromVec, &from_vec);
   mx_copy_vec(&m_ToVec, &to_vec);

   m_StartCell = startCell;
   m_EndCell = endCell;

   m_PathFound = FALSE;

   m_RoomPropAgent.PropagateBF(from_vec, start_room);

   return m_PathFound;
}

////////////////////////////////////////

BOOL cRoomPathFind::FindPath(ObjID fromObjID, ObjID toObjID)
{
   mxs_vector *start_pos;
   cRoom      *start_room;

   if (g_pRooms == NULL)
      return FALSE;

   m_FromObj = fromObjID;
   m_ToObj = toObjID;

   mx_copy_vec(&m_FromVec, &ObjPosGet(fromObjID)->loc.vec);
   mx_copy_vec(&m_ToVec, &ObjPosGet(toObjID)->loc.vec);

   start_pos = &ObjPosGet(fromObjID)->loc.vec;
   start_room = g_pRooms->GetObjRoom(fromObjID);

   if (start_room == NULL)
      return FALSE;

   m_PathFound = FALSE;

   m_RoomPropAgent.PropagateBF(*start_pos, start_room);

   return m_PathFound;
}

////////////////////////////////////////////////////////////////////////////////

DECLARE_TIMER(AI_NPF, Average);

BOOL cRoomPathFind::AINewPathFind(const cMxsVector & fromLocation, tAIPathCellID startCell,
                                  const cMxsVector & toLocation, tAIPathCellID endCell,
                                  cAIPath * pPath)
{
   AUTO_TIMER(AI_NPF);

   if (!g_fAIPathFindInited)
      return FALSE;

   pPath->Clear();

   g_pAIRoomDB->UseValidChecks(TRUE);

   if (FindPath(fromLocation, startCell, toLocation, endCell))
   {
      cRoomPropResult     *pPathRoom;
      cRoomPropResult     *pPathNextRoom;
      cAIPath WorkingPath;
      const cRoom *cur_room;
      const cRoom *next_room;
      cRoomPortal *prev_portal;
      cRoomPortal *next_portal;
      cMxsVector from_point, to_point;
      int i;

      pPathRoom = m_RoomPropList.GetLast();
      pPathNextRoom = pPathRoom->GetPrevious();

      prev_portal = NULL;
      while (pPathNextRoom != NULL)
      {
         cur_room = pPathRoom->room;
         next_room = pPathNextRoom->room;

         // Find the portal between this room and the next
         for (i=0; i<cur_room->GetNumPortals(); i++)
         {
            next_portal = cur_room->GetPortal(i);
            if (next_portal->GetFarRoom() == next_room)
               break;
         }

         AssertMsg(i != cur_room->GetNumPortals(), "Unable to find portal between rooms in pathfind\n");

         // Is it the last leg?
         if (prev_portal == NULL)
         {
            pPath->Append(&m_EndLeg);
         }
         else
         {
            // @TBD: We're doing this backward.  Potiential problems...
            if (g_pAIRoomDB->HasPath(cur_room->GetRoomID(), next_portal->GetIndex(), prev_portal->GetIndex()))
               pPath->Append(g_pAIRoomDB->GetPath(cur_room->GetRoomID(), next_portal->GetIndex(), prev_portal->GetIndex()));
            else
            {
               g_pAIRoomDB->UseValidChecks(TRUE);
               g_pAIRoomDB->MarkRoomCells(cur_room->GetRoomID());

               from_point.Set(prev_portal->GetCenter());
               to_point.Set(next_portal->GetCenter());
               if (AIPathFind(to_point, 0, from_point, 0, &WorkingPath, kAIOK_All))
               {
                  g_pAIRoomDB->UnMarkRoomCells(cur_room->GetRoomID());
                  pPath->Append(&WorkingPath);
               }
               else
               {
                  CriticalMsg("Unable to find path through room in path building\n");
                  g_pAIRoomDB->UnMarkRoomCells(cur_room->GetRoomID());
                  g_pAIRoomDB->UseValidChecks(FALSE);
                  return FALSE;
               }
            }
         }

         pPathRoom = pPathNextRoom;
         pPathNextRoom = pPathRoom->GetPrevious();

         prev_portal = next_portal->GetFarPortal();;
      }

      cur_room = pPathRoom->room;

      // The first leg
      if (prev_portal == NULL)
      {
         // Within a single room, so "start leg" is whole thing
         pPath->Append(&m_StartLeg);
      } 
      else
      {
         pPath->Append(&m_StartLeg);
      }

      g_pAIRoomDB->UseValidChecks(FALSE);

      pPath->SetFinalDestination(toLocation);
      pPath->SetCurrentPathEdgeAtStart();
      return TRUE;
   }
   else
   {
      g_pAIRoomDB->UseValidChecks(FALSE);
      return FALSE;
   }
}

////////////////////////////////////////////////////////////////////////////////

DECLARE_TIMER(AI_NPF_PropEnter, Average);
DECLARE_TIMER(AI_NPF_PropEnterObj, Average);
DECLARE_TIMER(AI_NPF_PropEnterVec, Average);
DECLARE_TIMER(AI_NPF_PropEnterVecSingle, Average);
DECLARE_TIMER(AI_NPF_PropEnterVecFindStart, Average);
DECLARE_TIMER(AI_NPF_PropEnterVecFindEnd, Average);


int cRoomPathFind::EnterCallback(const cRoom *room, const cRoomPortal *enterPortal, const mxs_vector &enterPt, mxs_real dist)
{
   static int          result;
   static const ObjID *obj_list;
   static int          obj_list_size;

   result = kER_Nothing;

   AUTO_TIMER(AI_NPF_PropEnter);

   if (m_ToObj != -1)
   {
      AUTO_TIMER(AI_NPF_PropEnterObj);

      room->GetObjList(&obj_list, &obj_list_size, kAllObjWatchHandle);

      while ((obj_list_size--) && !m_PathFound)
      {
         if (*obj_list == m_ToObj)
         {
            // Whoah, we found the object
            cMxsVector from, to;

            // Build our room-path
            m_RoomPropAgent.GetRoomPath(room, &m_RoomPropList);

            const cRoom *pFirstRoom = m_RoomPropList.GetFirst()->room;

            if (pFirstRoom == room)
            {
               // Within a single room
               from.Set(m_FromVec);
               to.Set(m_ToVec);

               g_pAIRoomDB->UseValidChecks(TRUE);
               g_pAIRoomDB->MarkRoomCells(room->GetRoomID());
               if (AIPathFind(from, m_StartCell, to, m_EndCell, &m_StartLeg, kAIOK_All))
                  m_PathFound = TRUE;
               g_pAIRoomDB->UnMarkRoomCells(room->GetRoomID());
               g_pAIRoomDB->UseValidChecks(FALSE);

               if (m_PathFound)
                  return kER_Success;
               else
                  return kER_Nothing;
            }

            Assert_(m_RoomPropList.GetFirst()->GetNext() != NULL);

            // Find the portal between the first two rooms            
            const cRoom *pNextRoom  = m_RoomPropList.GetFirst()->GetNext()->room;
            BOOL found = FALSE;

            for (int i=0; i<pFirstRoom->GetNumPortals(); i++)
            {
               if (pFirstRoom->GetPortal(i)->GetFarRoom() == pNextRoom)
               {
                  found = TRUE;
                  break;
               }
            }

            AssertMsg(found, "No portal between first two pathfind rooms?");

            // Make sure we can get to the first portal
            from.Set(m_FromVec);
            to.Set(pFirstRoom->GetPortal(i)->GetCenter());

            int to_hint = g_pAIRoomDB->GetPortalHint(pFirstRoom->GetPortal(i)->GetPortalID());

            mprintf("start from hint = %d\n", m_StartCell);
            mprintf("start to hint = %d\n", to_hint);

            g_pAIRoomDB->UseValidChecks(TRUE);

            g_pAIRoomDB->MarkRoomCells(pFirstRoom->GetRoomID());
            if (AIPathFind(from, m_StartCell, to, to_hint, &m_StartLeg, kAIOK_All))
            {
               int from_hint = g_pAIRoomDB->GetPortalHint(enterPortal->GetPortalID());

               g_pAIRoomDB->UnMarkRoomCells(pFirstRoom->GetRoomID());

               // Make sure we can get from the last portal to the target object
               from.Set(enterPt);
               to.Set(m_ToVec);

               mprintf("end from hint = %d\n", from_hint);
               mprintf("end to hint = %d\n", m_EndCell);

               g_pAIRoomDB->MarkRoomCells(room->GetRoomID());
               if (AIPathFind(from, from_hint, to, m_EndCell, &m_EndLeg, kAIOK_All))
                  m_PathFound = TRUE;
               else
                  result = kER_EndBad;
               g_pAIRoomDB->MarkRoomCells(room->GetRoomID());
            }
            else
            {
               result = kER_StartBad;
               g_pAIRoomDB->UnMarkRoomCells(pFirstRoom->GetRoomID());
            }

            g_pAIRoomDB->UseValidChecks(FALSE);
         }

         obj_list++;
      }
   }
   else
   {
      AUTO_TIMER(AI_NPF_PropEnterVec);

      if (room->PointInside(m_ToVec))
      {
         // Whoah, we found the point
         cMxsVector from, to;

         // Build our room-path
         m_RoomPropAgent.GetRoomPath(room, &m_RoomPropList);

         const cRoom *pFirstRoom = m_RoomPropList.GetFirst()->room;

         if (pFirstRoom == room)
         {
            AUTO_TIMER(AI_NPF_PropEnterVecSingle);

            // Within a single room
            from.Set(m_FromVec);
            to.Set(m_ToVec);

            g_pAIRoomDB->UseValidChecks(TRUE);
            g_pAIRoomDB->MarkRoomCells(room->GetRoomID());
            if (AIPathFind(from, m_StartCell, to, m_EndCell, &m_StartLeg, kAIOK_All))
               m_PathFound = TRUE;
            g_pAIRoomDB->UnMarkRoomCells(room->GetRoomID());
            g_pAIRoomDB->UseValidChecks(FALSE);

            if (m_PathFound)
               return kER_Success;
            else
               return kER_Nothing;  // @TBD: what failure case to we really want?
         }

         Assert_(m_RoomPropList.GetFirst()->GetNext() != NULL);

         // Find the portal between the first two rooms
         const cRoom *pNextRoom  = m_RoomPropList.GetFirst()->GetNext()->room;
         BOOL found = FALSE;

         for (int i=0; i<pFirstRoom->GetNumPortals(); i++)
         {
            if (pFirstRoom->GetPortal(i)->GetFarRoom() == pNextRoom)
            {
               found = TRUE;
               break;
            }
         }

         AssertMsg(found, "No portal between first two pathfind rooms?");

         TIMER_Start(AI_NPF_PropEnterVecFindStart);
         // Make sure we can get to the first portal
         from.Set(m_FromVec);
         to.Set(pFirstRoom->GetPortal(i)->GetCenter());

         int to_hint = g_pAIRoomDB->GetPortalHint(pFirstRoom->GetPortal(i)->GetPortalID());

         g_pAIRoomDB->UseValidChecks(TRUE);

         g_pAIRoomDB->MarkRoomCells(pFirstRoom->GetRoomID());
         if (AIPathFind(from, m_StartCell, to, to_hint, &m_StartLeg, kAIOK_All))
         {
            int from_hint = g_pAIRoomDB->GetPortalHint(enterPortal->GetPortalID());

            g_pAIRoomDB->UnMarkRoomCells(pFirstRoom->GetRoomID());

            TIMER_MarkStop(AI_NPF_PropEnterVecFindStart);

            // Make sure we can get from the last portal to the target object
            from.Set(enterPt);
            to.Set(m_ToVec);

            AUTO_TIMER(AI_NPF_PropEnterVecFindEnd);

            g_pAIRoomDB->MarkRoomCells(room->GetRoomID());
            if (AIPathFind(from, from_hint, to, m_EndCell, &m_EndLeg, kAIOK_All))
               m_PathFound = TRUE;
            else
               result = kER_EndBad;
            g_pAIRoomDB->UnMarkRoomCells(room->GetRoomID());

         }
         else
         {
            result = kER_StartBad;
            g_pAIRoomDB->UnMarkRoomCells(pFirstRoom->GetRoomID());
            TIMER_MarkStop(AI_NPF_PropEnterVecFindStart);
         }
        
         g_pAIRoomDB->UseValidChecks(FALSE);
      }
   }

   if (m_PathFound)
      result =  kER_Success;

   return result;
}

////////////////////////////////////////

void cRoomPathFind::ExitCallback(const cRoom *room)
{



}

////////////////////////////////////////

mxs_real cRoomPathFind::PortalsCallback(const cRoomPortal *enterPortal, const cRoomPortal *exitPortal, mxs_real dist)
{
   if (enterPortal == NULL)
   {
      return dist;
   }
   else
   {
      cRoom *pRoom = enterPortal->GetNearRoom();

      if (!g_pAIRoomDB->HasPath(pRoom->GetRoomID(), enterPortal->GetIndex(), exitPortal->GetIndex()))
         return -1;
   }

   return dist;
}

#endif







