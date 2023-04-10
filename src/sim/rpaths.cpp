/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/rpaths.cpp,v 1.14 1998/10/17 19:28:55 dc Exp $
//
// Room paths functions
//

#include <lg.h>
#include <command.h>
#include <config.h>

#include <portal.h>

#include <rooms.h>
#include <roomutil.h>

#include <aipathdb.h>
#include <aipthloc.h>

#include <rpaths.h>

// Must be last header
#include <dbmem.h>

cAIRoomDB *g_pAIRoomDB = NULL;

////////////////////////////////////////////////////////////////////////////////

#ifdef EDITOR
void AIRoomDBSpew();

static Command g_AIRoomDatabaseCommands[] =
{
   { "build_ai_room_database", FUNC_VOID, BuildAIRoomDatabase, "Build AI Room Path Database", HK_ALL },
   { "ai_room_db_spew",        FUNC_VOID, AIRoomDBSpew,        "Spew AI Room Path Database", HK_ALL },
};

static BOOL g_CommandsInstalled = FALSE;
#endif

////////////////////////////////////////////////////////////////////////////////

void InitAIRoomDatabase()
{
   Assert_(g_pAIRoomDB == NULL);

   g_pAIRoomDB = new cAIRoomDB;

#ifdef EDITOR
   if (!g_CommandsInstalled)
   {
      g_CommandsInstalled = TRUE;
      COMMANDS(g_AIRoomDatabaseCommands, HK_ALL);
   }
#endif
}

void TermAIRoomDatabase()
{
   Assert_(g_pAIRoomDB != NULL);

   delete g_pAIRoomDB;
   g_pAIRoomDB = NULL;
}

void ResetAIRoomDatabase()
{
   if (g_pAIRoomDB != NULL)
      TermAIRoomDatabase();
   InitAIRoomDatabase();
}

#ifdef EDITOR
void BuildAIRoomDatabase()
{
   Assert_(g_pAIRoomDB != NULL);

   g_pAIRoomDB->Clear();

   if (g_pRooms->GetNumRooms() == 0)
   {
      mprintf("BuildAIRoomDatabase: The room database must be built first!\n");
      return;
   }

   if (g_AIPathDB.NumCells() == 0)
   {
      mprintf("BuildAIRoomDatabase: The pathfinding database must be built first!\n");
      return;
   }

   g_pAIRoomDB->BuildRoomCellLists();

#if ROOM_PATHS
   if (config_is_defined("ai_room_paths"))
   {
      g_pAIRoomDB->GeneratePortalHints();
      g_pAIRoomDB->GenerateCosts();
   }
#endif
}
#endif // EDITOR

void SaveAIRoomDatabase(AIRoomDBReadWrite movefunc)
{
   Assert_(g_pAIRoomDB != NULL);

   g_pAIRoomDB->Write(movefunc);
}

void LoadAIRoomDatabase(AIRoomDBReadWrite movefunc)
{
   Assert_(g_pAIRoomDB != NULL);

   g_pAIRoomDB->Read(movefunc);
}


////////////////////////////////////////
#ifdef EDITOR
void AIRoomDBSpew()
{
   if (g_pAIRoomDB != NULL)
      g_pAIRoomDB->SpewDB();
}
#endif

////////////////////////////////////////////////////////////////////////////////

// @TBD (toml 03-17-98): hmmm
static int ReadWriteAdaptor(void * pfnOuter, void * buf, size_t e, size_t n)
{
   (*((AIRoomDBReadWrite)pfnOuter))(buf, e, n);
   return 0;
}

////////////////////////////////////////////////////////////////////////////////

cAIRoomDB::cAIRoomDB()
   : m_UseValidChecks(FALSE),
     m_DatabaseEmpty(TRUE)
{
   Clear();
}

////////////////////////////////////////

cAIRoomDB::~cAIRoomDB()
{
#if ROOM_PATHS
   ClearRoomPaths();
#endif
   ClearRoomCellLists();

   m_CellValidList.SetSize(0);
}

////////////////////////////////////////////////////////////////////////////////

void cAIRoomDB::Clear()
{
#if ROOM_PATHS
   ClearRoomPaths();
#endif
   ClearRoomCellLists();

   InitValidCellList();

   m_DatabaseEmpty = TRUE;
}

////////////////////////////////////////////////////////////////////////////////

void cAIRoomDB::Read(AIRoomDBReadWrite movefunc)
{
   int size, size2;
   int i, j;
   tAIPathCellID cval;
#if ROOM_PATHS
   ulong ulval;
   int   ival;
   BOOL  bval;
   int size3;
   int k;
#endif

   movefunc(&m_DatabaseEmpty, sizeof(BOOL), 1);

   movefunc(&size, sizeof(int), 1);
   m_CellValidList.SetSize(size);

   movefunc(&size, sizeof(int), 1);
   m_RoomCellList.SetSize(size);

   for (i=0; i<size; i++)
   {
      movefunc(&size2, sizeof(int), 1);
      m_RoomCellList[i].SetSize(size2);

      for (j=0; j<size2; j++)
      {
         movefunc(&cval, sizeof(tAIPathCellID), 1);
         m_RoomCellList[i][j] = cval;
      }
   }
#if ROOM_PATHS
   movefunc(&size, sizeof(int), 1);
   m_RoomPortalPaths.SetSize(size);

   for (i=0; i<size; i++)
   {
      movefunc(&size2, sizeof(int), 1);
      m_RoomPortalPaths[i].SetSize(size2);

      for (j=0; j<size2; j++)
      {
         movefunc(&size3, sizeof(int), 1);
         m_RoomPortalPaths[i][j].SetSize(size3);

         for (k=0; k<size3; k++)
         {
            movefunc(&ulval, sizeof(ulong), 1);
            m_RoomPortalPaths[i][j][k].cost = ulval;

            movefunc(&bval, sizeof(BOOL), 1);

            if (bval)
            {
               m_RoomPortalPaths[i][j][k].path = new cAIPath;
               m_RoomPortalPaths[i][j][k].path->Read(movefunc, ReadWriteAdaptor);
            }
            else
               m_RoomPortalPaths[i][j][k].path = NULL;
         }
      }
   }

   InitValidCellList();

   movefunc(&size, sizeof(int), 1);
   m_PortalHintList.SetSize(size);

   for (i=0; i<size; i++)
   {
      movefunc(&ival, sizeof(int), 1);
      m_PortalHintList[i] = ival;
   }
#endif
}

////////////////////////////////////////

void cAIRoomDB::Write(AIRoomDBReadWrite movefunc)
{
   int size, size2;
   int i, j;
   tAIPathCellID cval;
#if ROOM_PATHS
   ulong ulval;
   int   ival;
   BOOL  bval;
   int size3;
   int k;
#endif


   movefunc(&m_DatabaseEmpty, sizeof(BOOL), 1);

   size = m_CellValidList.Size();
   movefunc(&size, sizeof(int), 1);

   size = m_RoomCellList.Size();
   movefunc(&size, sizeof(int), 1);

   for (i=0; i<size; i++)
   {
      size2 = m_RoomCellList[i].Size();
      movefunc(&size2, sizeof(int), 1);

      for (j=0; j<size2; j++)
      {
         cval = m_RoomCellList[i][j];
         movefunc(&cval, sizeof(tAIPathCellID), 1);
      }
   }

#if ROOM_PATHS
   size = m_RoomPortalPaths.Size();
   movefunc(&size, sizeof(int), 1);

   for (i=0; i<size; i++)
   {
      size2 = m_RoomPortalPaths[i].Size();
      movefunc(&size2, sizeof(int), 1);

      for (j=0; j<size2; j++)
      {
         size3 = m_RoomPortalPaths[i][j].Size();
         movefunc(&size3, sizeof(int), 1);

         for (k=0; k<size3; k++)
         {
            ulval = m_RoomPortalPaths[i][j][k].cost;
            movefunc(&ulval, sizeof(ulong), 1);

            bval = (m_RoomPortalPaths[i][j][k].path != NULL) && (j != k);
            movefunc(&bval, sizeof(BOOL), 1);

            if (bval)
               m_RoomPortalPaths[i][j][k].path->Write(movefunc, ReadWriteAdaptor);
         }
      }
   }

   size = m_PortalHintList.Size();
   movefunc(&size, sizeof(int), 1);

   for (i=0; i<size; i++)
   {
      ival = m_PortalHintList[i];
      movefunc(&ival, sizeof(int), 1);
   }     
#endif
}

////////////////////////////////////////

#ifdef EDITOR
void cAIRoomDB::SpewDB()
{
   if (m_DatabaseEmpty)
   {
      mprintf("AI Room Database empty\n");
      return;
   }

   int i, j;
#if ROOM_PATHS
   int k;
#endif

   mprintf("Room-Cell Lists\n");
   mprintf("---------------\n");
   for (i=0; i<m_RoomCellList.Size(); i++)
   {
      mprintf("[Room %d Cell List]\n", i);
      for (j=0; j<m_RoomCellList[i].Size(); j++)
         mprintf(" %d\n", m_RoomCellList[i][j]);

      mprintf("\n");
   }
   mprintf("\n");

#if ROOM_PATHS
   if (config_is_defined("ai_room_paths"))
   {
      mprintf("Room-Portal Paths\n");
      mprintf("-----------------\n");
      for (i=0; i<m_RoomPortalPaths.Size(); i++)
      {
         mprintf("[Room %d Portal Paths]\n", i);
 
         for (j=0; j<m_RoomPortalPaths[i].Size(); j++)
         {
            for (k=0; k<m_RoomPortalPaths[i][j].Size(); k++)
            {
               if (j == k)
                  continue;

               mprintf("  - %d to %d - (cost %d)\n", j, k, m_RoomPortalPaths[i][j][k].cost);
               if (m_RoomPortalPaths[i][j][k].path != NULL)
                  m_RoomPortalPaths[i][j][k].path->DumpCenters();
            }
         }
      }
      mprintf("\n");

      mprintf("Portal Hints\n");
      mprintf("------------\n");
      for (i=0; i<g_pRooms->GetNumPortals(); i++)
         mprintf(" [%d] %d\n", i, GetPortalHint(i));
      mprintf("\n");
   }
#endif
}
#endif

////////////////////////////////////////////////////////////////////////////////

#if ROOM_PATHS
void cAIRoomDB::AddRoomPath(short roomID, int portalID1, int portalID2, ulong cost, cAIPath *path)
{
   int max_portal = (portalID1 > portalID2) ? portalID1 : portalID2;

   // Grow arrays as needed
   if (roomID >= m_RoomPortalPaths.Size())
      m_RoomPortalPaths.SetSize(roomID + 1);

   if (max_portal >= m_RoomPortalPaths[roomID].Size())
   {
      m_RoomPortalPaths[roomID].SetSize(max_portal + 1);
      for (int i=0; i<m_RoomPortalPaths[roomID].Size(); i++)
         m_RoomPortalPaths[roomID][i].SetSize(max_portal + 1);
   }

   m_RoomPortalPaths[roomID][portalID1][portalID2].cost = cost;
   m_RoomPortalPaths[roomID][portalID1][portalID2].path = path;
}

////////////////////////////////////////////////////////////////////////////////

void cAIRoomDB::ClearRoomPaths()
{
   int i, j, k;

   for (i=0; i<m_RoomPortalPaths.Size(); i++)
   {
      for (j=0; j<m_RoomPortalPaths[i].Size(); j++)
      {
         for (k=0; k<m_RoomPortalPaths[i][j].Size(); k++)
         {
            if ((m_RoomPortalPaths[i][j][k].path != NULL) && (j != k))
               delete m_RoomPortalPaths[i][j][k].path;
         }
         m_RoomPortalPaths[i][j].SetSize(0);
      }
      m_RoomPortalPaths[i].SetSize(0);
   }
   
   m_RoomPortalPaths.SetSize(0);
}

#endif

////////////////////////////////////////////////////////////////////////////////

BOOL cAIRoomDB::AddCellToRoom(short roomID, tAIPathCellID cellID)
{
   if (m_RoomCellList.Size() < g_pRooms->GetNumRooms())
      m_RoomCellList.SetSize(g_pRooms->GetNumRooms());

   // Don't allow duplicates
   for (int i=0; i<m_RoomCellList[roomID].Size(); i++)
   {
      if (m_RoomCellList[roomID][i] == cellID)
         return FALSE;
   }

   m_RoomCellList[roomID].Append(cellID);
   return TRUE;
}

////////////////////////////////////////

void cAIRoomDB::InitValidCellList()
{
   m_CellValidList.SetSize(g_AIPathDB.NumCells() + 2);

   for (int i=0; i<=g_AIPathDB.NumCells(); i++)
      m_CellValidList[i] = FALSE;
}

////////////////////////////////////////

void cAIRoomDB::ClearRoomCellLists()
{
   for (int i=0; i<m_RoomCellList.Size(); i++)
      m_RoomCellList[i].SetSize(0);

   m_RoomCellList.SetSize(g_pRooms->GetNumRooms());
}

////////////////////////////////////////

#ifdef EDITOR

void cAIRoomDB::BuildRoomCellLists()
{
   Location start, end, hit;
   cRoom      *pRoom;
   mxs_vector *vertex;
   mxs_vector  interp_vertex;
   mxs_vector  proj_vertex;
   mxs_vector  xy_delta;
   mxs_vector  edge;
   mxs_real room_max_size_2;
   mxs_real best_z;
   mxs_real dist;
   int best_z_side;
   int num_vertices;
   int i, j, k;
   int add_cell = 0;

   mprintf("Generating cell-room lists...\n");

   InitValidCellList();
   ClearRoomCellLists();

   // Iterate over all rooms
   for (i=0; i<g_pRooms->GetNumRooms(); i++)
   {
      if ((i % 4) == 0)
         mprintf(" %d%% completed\r", i * 100 / g_pRooms->GetNumRooms());

      pRoom = g_pRooms->GetRoom(i);

      // Save off its max size
      room_max_size_2 = mx_mag2_vec(&pRoom->GetSize());

      // Find which side is the 'ceiling'
      for (j=0; j<6; j++)
      {
         if ((j == 0) || (pRoom->GetPlane(j).normal.z > best_z))
         {
            best_z_side = j;
            best_z = pRoom->GetPlane(j).normal.z;
         }
      }

      for (j=1; j<=g_AIPathDB.NumCells(); j++)
      {
         num_vertices = g_AIPathDB.GetCell(j)->vertexCount;

         for (k=0; k<(num_vertices * 2); k++)
         {
            // Interpolate halfway for every other k
            if (k & 1)
            {
               mx_sub_vec(&edge, &g_AIPathDB.GetCellVertex(j, ((k/2)+1)%num_vertices),
                                 &g_AIPathDB.GetCellVertex(j, k/2));

               // Don't interpolate between short edges
               if (mx_mag2_vec(&edge) < 1.0)
                  continue;

               mx_scale_add_vec(&interp_vertex, &g_AIPathDB.GetCellVertex(j, k/2), &edge, 0.5);
               vertex = &interp_vertex;
            }
            else
            {
               vertex = (mxs_vector *)&g_AIPathDB.GetCellVertex(j, k / 2);
            }

            mx_mk_vec(&xy_delta, pRoom->GetCenterPt().x - vertex->x,
                                 pRoom->GetCenterPt().y - vertex->y, 0.0);

            if (mx_mag2_vec(&xy_delta) < room_max_size_2)
            {
               if (pRoom->PointInside(*vertex))
               {
                  if (AddCellToRoom(pRoom->GetRoomID(), j))
                     add_cell++;

                  break;
               }

               // Project the point upward onto the room's ceiling
               dist = -PointPlaneDist(pRoom->GetPlane(best_z_side), *vertex);
               mx_scale_add_vec(&proj_vertex, vertex, &pRoom->GetPlane(best_z_side).normal, dist);

               // Raycast to see if we can get to that point
               MakeLocationFromVector(&start, vertex);
               MakeLocationFromVector(&end, &proj_vertex);

               start.vec.z += 0.001;
               ComputeCellForLocation(&start);
               if (start.cell != CELL_INVALID)
               {
                  if (!PortalRaycast(&start, &end, &hit, 0))
                     proj_vertex = hit.vec;
               }                 
 
               // And check that line's intersection with our room
               if (pRoom->LineIntersect(*vertex, proj_vertex))
               {
                  if (AddCellToRoom(pRoom->GetRoomID(), j))
                     add_cell++;

                  break;
               }
            }
         }
      }
   }

   if (g_pRooms->GetNumRooms() > 0)
      mprintf(" %d cells, %g average cells per room\n", g_AIPathDB.NumCells(), ((float)add_cell) / g_pRooms->GetNumRooms());

   m_DatabaseEmpty = FALSE;
}
#endif

////////////////////////////////////////
#if ROOM_PATHS

void cAIRoomDB::GenerateCosts()
{
   cRoom   *pRoom;
   cAIPath *pPath;
   cMxsVector from, to;
   tAIPathCellID from_hint, to_hint;

   int num_paths = 0;
   int i, j, k;

   mprintf("Pathfinding and calculating costs...\n");

   if (!g_fAIPathFindInited)
   {
      mprintf(" no pathfinding database\n");
      return;
   }

   UseValidChecks(TRUE);

   for (i=0; i<g_pRooms->GetNumRooms(); i++)
   {
      pRoom = g_pRooms->GetRoom(i);

      if ((i & 0x07) == 0x07)
         mprintf(" %d%% completed\r", i * 100 / g_pRooms->GetNumRooms());

      for (j=0; j<pRoom->GetNumPortals(); j++)
      {
         for (k=0; k<pRoom->GetNumPortals(); k++)
         {
            if (j == k)
               continue;

            from.Set(pRoom->GetPortal(j)->GetCenter());
            to.Set(pRoom->GetPortal(k)->GetCenter());

            from_hint = GetPortalHint(pRoom->GetPortal(j)->GetPortalID());
            to_hint = GetPortalHint(pRoom->GetPortal(k)->GetPortalID());

            pPath = new cAIPath;

            MarkRoomCells(pRoom->GetRoomID());
            if (AIPathFind(from, from_hint, to, to_hint, pPath, kAIOK_All))
            {
               AddRoomPath(pRoom->GetRoomID(), j, k, pPath->GetCost(), pPath);
               num_paths++;
            }
            else
            {
               AddRoomPath(pRoom->GetRoomID(), j, k, -1, NULL);
            }
            UnMarkRoomCells(pRoom->GetRoomID());
         }
      }
   }

   UseValidChecks(FALSE);

   mprintf(" %d paths, %g average paths per room\n", num_paths, (float)num_paths / (float)g_pRooms->GetNumRooms());

   m_DatabaseEmpty = FALSE;
}
#endif
////////////////////////////////////////////////////////////////////////////////

#if ROOM_PATHS
void cAIRoomDB::GeneratePortalHints()
{
   cRoomPortal   *pPortal;
   cMxsVector       point;
   tAIPathCellID  hint;
   int num_portals;
   int num_hinted;
   int i;

   mprintf("Generating portal hints...\n");

   UseValidChecks(TRUE);

   num_hinted = 0;
   num_portals = g_pRooms->GetNumPortals();
   for (i=0; i<num_portals; i++)
   {
      if ((i & 0x1F) == 0x1F)
         mprintf(" %d%% completed\r", i * 100 / num_portals);

      pPortal = g_pRooms->GetPortal(i);

      MarkRoomCells(pPortal->GetNearRoom()->GetRoomID());
      
      point.Set(pPortal->GetCenter());
      hint = AIFindClosestCell(point, 0);

      UnMarkRoomCells(pPortal->GetNearRoom()->GetRoomID());

      if (hint > 1)
         num_hinted++;

      SetPortalHint(i, hint);
   }

   UseValidChecks(FALSE);

   mprintf(" %d portals of %d hinted (%d%%)\n", num_hinted, num_portals, num_hinted * 100 / num_portals);

   m_DatabaseEmpty = FALSE;
}
#endif

////////////////////////////////////////////////////////////////////////////////
