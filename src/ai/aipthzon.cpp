/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipthzon.cpp,v 1.7 2000/03/01 13:03:03 bfarquha Exp $
//
// AI zone-determination
//

#include <lg.h>

#include <phmterr.h>

#include <aiokbits.h>
#include <aipathdb.h>
#include <aipthzon.h>

IMPLEMENT_POOL(sZoneOkBitsMap);

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////////////////////////

void ResetPathfindZones()
{
   int i;

   for (i = 0; i < kAIZone_Num; i++)
   {
      g_AIPathDB.m_ZoneDatabases[i].m_ZonePairTable.DestroyAll();
      g_AIPathDB.m_ZoneDatabases[i].m_ZonePairTable.SetEmpty();
      g_AIPathDB.m_ZoneDatabases[i].m_nZones = 1;
      g_AIPathDB.m_ZoneDatabases[i].m_CellZones.SetSize(0);
   }
}

////////////////////////////////////////////////////////////////////////////////

inline tAIPathCellID FindUnzonedCell(eAIPathZoneType ZoneType)
{
   for (int i=1; i<g_AIPathDB.m_nCells + 1; i++)
   {
      if (g_AIPathDB.m_ZoneDatabases[ZoneType].m_CellZones[i] == AI_ZONE_INVALID)
          return i;
   }

   return 0;
}

////////////////////////////////////////

static inline void DoDeterminePathfindZones(eAIPathZoneType ZoneType, uint nOkBits)
{
   cDynArray<tAIPathCellID> activeCellList(g_AIPathDB.m_nCells + 1);
   cDynArray<bool> cellInActiveList(g_AIPathDB.m_nCells + 1);
   int num_active_cells;
   int start_unfinished_cells;
   int num_solo_cells = 0;
   BOOL non_zero_links;
   int i;

   tAIPathCellID  startCellID;

   // Initialize zone lists
   g_AIPathDB.m_ZoneDatabases[ZoneType].m_CellZones.SetSize(g_AIPathDB.m_nCells + 1);
   for (i=0; i<=g_AIPathDB.m_nCells; i++)
   {
      g_AIPathDB.m_ZoneDatabases[ZoneType].m_CellZones[i] = AI_ZONE_INVALID;
      cellInActiveList[i] = FALSE;
   }

   // Divide the database up into "zones", which are contiguous, walkable/swimmable sections
   while ((startCellID = FindUnzonedCell(ZoneType)) != 0)
   {
      num_active_cells = 0;
      start_unfinished_cells = 0;

      non_zero_links = FALSE;

      // Add our start cell to list
      activeCellList[num_active_cells++] = startCellID;
      cellInActiveList[startCellID] = TRUE;

      // Process each cell in our list
      while (start_unfinished_cells < num_active_cells)
      {
         tAIPathCellID  cellID = activeCellList[start_unfinished_cells];
         sAIPathCell   *pCell = &g_AIPathDB.m_Cells[cellID];

         const int first_cell_link = pCell->firstCell;
         const int last_cell_link = pCell->firstCell + pCell->cellCount;

         for (i=first_cell_link; i<last_cell_link; i++)
         {
            sAIPathCellLink *pLink = &g_AIPathDB.m_Links[i];

            //
            // wsf: for now, include "stressed" cells in a zone.
            // Ideally, we'd like to compute two zone databases. One with stressed bits, and one without.
            // This would make pathfinding faster.
            // Nevertheless, including stressed cells will only slow normal pathfinding very little
            //
            // We'd really like to seperate out swimming/walking zones, so walkers don't bother looking
            // through swimming links.
            // If this link has ok bits, and it either has no condition bits, or they match any of
            // the ones passed in, then we can use it.
            if ((pLink->okBits & nOkBits) &&
                 (!(pLink->okBits & kConditionMask) ||
                  (pLink->okBits & nOkBits & kConditionMask)))
            {
               tAIPathCellID  newCell = pLink->dest;
               sAIPathCell   *pNewCell = &g_AIPathDB.m_Cells[newCell];

               if ((g_AIPathDB.m_ZoneDatabases[ZoneType].m_CellZones[newCell] == AI_ZONE_INVALID) &&
                   !cellInActiveList[newCell])
               {
                  activeCellList[num_active_cells++] = newCell;
                  cellInActiveList[newCell] = TRUE;
               }
            }

            if (!non_zero_links && pLink->okBits != 0)
               non_zero_links = TRUE;
         }

         start_unfinished_cells++;
      }

      if ((num_active_cells == 1) && !non_zero_links)
      {
         g_AIPathDB.m_ZoneDatabases[ZoneType].m_CellZones[activeCellList[0]] = AI_ZONE_SOLO;
         cellInActiveList[activeCellList[0]] = FALSE;
         num_solo_cells++;
      }
      else
      {
         // Mark block of cells as zone
         for (i=0; i<num_active_cells; i++)
         {
            g_AIPathDB.m_ZoneDatabases[ZoneType].m_CellZones[activeCellList[i]] = g_AIPathDB.m_ZoneDatabases[ZoneType].m_nZones;
            cellInActiveList[activeCellList[0]] = FALSE;
         }

         g_AIPathDB.m_ZoneDatabases[ZoneType].m_nZones++;
         if (g_AIPathDB.m_ZoneDatabases[ZoneType].m_nZones >= MAX_AI_PATH_ZONES)
         {
            CriticalMsg("Too many path zones!\n");
            return;
         }

         if ((g_AIPathDB.m_ZoneDatabases[ZoneType].m_nZones % 16) == 0)
            mprintf(".");
      }
   }

   mprintf("%d zones (type %d) found (%d solo)", g_AIPathDB.m_ZoneDatabases[ZoneType].m_nZones - 1, ZoneType, num_solo_cells);
}


void DeterminePathfindZones()
{
   DoDeterminePathfindZones(kAIZone_Normal, kAIOK_Walk|kAIOK_Swim);
   DoDeterminePathfindZones(kAIZone_NormalLVL, kAIOK_Walk|kAIOK_Swim|kAIOKCOND_Stressed);
   DoDeterminePathfindZones(kAIZone_HighStrike, kAIOK_Walk|kAIOK_Swim|kAIOKCOND_HighStrike);
   DoDeterminePathfindZones(kAIZone_HighStrikeLVL, kAIOK_Walk|kAIOK_Swim|kAIOKCOND_HighStrike|kAIOKCOND_Stressed);
}


////////////////////////////////////////////////////////////////////////////////

inline tAIPathCellID FindCellOfZone(eAIPathZoneType ZoneType, tAIPathZone zone)
{
   for (tAIPathCellID i=1; i<=g_AIPathDB.m_nCells; i++)
   {
      if (g_AIPathDB.GetCellZone(ZoneType, i) == zone)
         return i;
   }
   return 0;
}

////////////////////////////////////////

inline BOOL BetterBits(tAIPathOkBits bits1, tAIPathOkBits bits2)
{
   // No bits definitely isn't better
   if (bits1 == 0)
      return FALSE;

   // Any bits are better than no bits
   if (bits2 == 0)
      return TRUE;

   // It's not better if it's the same
   if (bits1 == bits2)
      return FALSE;

   // It's better if it's a strict subset
   if ((bits1 & bits2) == bits1)
      return TRUE;

   // It's not better if it's a strict superset
   if ((bits1 & bits2) == bits2)
      return FALSE;

   // They're disjoint, so it's not better (@TBD: how to handle this case?)
   return FALSE;
}

////////////////////////////////////////

static inline void DoLinkPathfindZones(eAIPathZoneType ZoneType)
{
   cDynArray<tAIPathOkBits> cellBestBits(g_AIPathDB.m_nCells + 1);
   cDynArray<tAIPathCellID> activeCellList;

   tAIPathZone zone;
   for (zone=1; zone<g_AIPathDB.m_ZoneDatabases[ZoneType].m_nZones; zone++)
   {
      if ((zone % 32) == 0)
         mprintf(".");

      // initialize our arrays
      activeCellList.SetSize(0);
      for (int i=1; i<=g_AIPathDB.m_nCells; i++)
         cellBestBits[i] = 0;

      tAIPathCellID rootCell = FindCellOfZone(ZoneType, zone);

      if (rootCell > 0)
      {
         activeCellList.Append(rootCell);
         cellBestBits[rootCell] = kAIOK_Walk;
      }

      while (activeCellList.Size() > 0)
      {
         tAIPathCellID  cellID = activeCellList[0];
         sAIPathCell   *pCell = &g_AIPathDB.m_Cells[cellID];
         tAIPathZone    cellZone = g_AIPathDB.GetCellZone(ZoneType, cellID);

         const int first_cell_link = pCell->firstCell;
         const int last_cell_link = pCell->firstCell + pCell->cellCount;

         for (i=first_cell_link; i<last_cell_link; i++)
         {
            sAIPathCellLink *pLink = &g_AIPathDB.m_Links[i];
            tAIPathCellID    newCellID = pLink->dest;
            tAIPathOkBits    newBits = (cellBestBits[cellID] | pLink->okBits) & kNoConditionMask;
            tAIPathZone      newZone = g_AIPathDB.GetCellZone(ZoneType, newCellID);

            // wsf: We ignore the "stressed" bits for now.
            //       Ideally, we'd link two zones via "stressed" bits IFF
            //       all cells linking one zone to another have "stressed" bits.
            //
            // Add to active list if we've found a better way there
            if (BetterBits(newBits, cellBestBits[newCellID]))
            {
               activeCellList.Append(newCellID);
               cellBestBits[newCellID] = newBits;

               // If we're transitioning zone, see if we've found a better way there
               if ((cellZone != newZone) && (zone != newZone))
               {
                  if (BetterBits(newBits, g_AIPathDB.GetZoneOkBits(ZoneType, zone, newZone)))
                     g_AIPathDB.SetZoneOkBits(ZoneType, zone, newZone, newBits);
               }
            }
         }

         // Remove from head of list
         activeCellList.FastDeleteItem(0);
      }
   }

#if 0
   for (tAIPathZone i=1; i<g_AIPathDB.m_nZones; i++)
   {
      for (tAIPathZone j=1; j<g_AIPathDB.m_nZones; j++)
      {
         mprintf(" %d to %d -> %d\n", i, j, g_AIPathDB.GetZoneOkBits(i, j));
      }
   }
#endif
}

void LinkPathfindZones()
{
   for (int i = 0; i < kAIZone_Num; i++)
      DoLinkPathfindZones((eAIPathZoneType)i);
}


////////////////////////////////////////

void FixupPathfindZones()
{
   for (int i=0; i<g_AIPathDB.m_MovableCells.Size(); i++)
   {
      for (int j = 0; j < kAIZone_Num; j++)
         g_AIPathDB.m_ZoneDatabases[j].m_CellZones[g_AIPathDB.m_MovableCells[i].cellID] = AI_ZONE_ALL;
   }
}

////////////////////////////////////////////////////////////////////////////////










