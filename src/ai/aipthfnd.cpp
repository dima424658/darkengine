/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipthfnd.cpp,v 1.40 2000/03/06 22:36:31 bfarquha Exp $
//

// #define PROFILE_ON 1

#include <lg.h>
#include <timings.h>

#include <propface.h>
#include <propbase.h>
#include <property.h>

#include <aitype.h>
#include <aidebug.h>
#include <aipath.h>
#include <aipathdb.h>
#include <aiprops.h>
#include <aipthedg.h>
#include <aipthfnd.h>
#include <aipthloc.h>
#include <aiutils.h>
#include <rpaths.h>

///////////////////

#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

BOOL g_fAIUsePathZones = TRUE;

///////////////////////////////////////////////////////////////////////////////

static IBoolProperty * g_pAIUsesDoorsProperty;

#define AIUsesDoors(obj)   AIGetProperty(g_pAIUsesDoorsProperty, (obj), TRUE)

///////////////////////////////////////

static sPropertyDesc g_AIUsesDoorsPropertyDesc =
{
  "AI_UsesDoors",
  0,
  NULL, 0, 0,
  { AI_CORE_CAT, "Uses doors" },
   kPropertyChangeLocally,  // net_flags
};

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitPathfinder(IAIManager *)
{
   g_pAIUsesDoorsProperty = CreateBoolProperty(&g_AIUsesDoorsPropertyDesc, kPropertyImplVerySparse);
   return TRUE;
}

///////////////////////////////////////

BOOL AITermPathfinder()
{
   SafeRelease(g_pAIUsesDoorsProperty);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

// Set this when any flavor of pathfind fails due to controller.
BOOL g_bPathfindControllerFailed = FALSE;

BOOL AICanUseLink(tAIPathCellID from,
                  tAIPathCellID to,
                  tAIPathOkBits LinkOkBits,
                  tAIPathOkBits AIOkBits,
                  unsigned      PathFlags,
                  IAIPathfindControl * pControl)
{
   if ((unsigned)g_AIPathDB.m_Cells[to].pathFlags & PathFlags)
      return FALSE;

   if (pControl)
   {
      ObjID door;
      if (g_AIPathDB.IsBelowDoor(to) &&
          (door = g_AIPathDB.GetCellDoor(to)) != OBJ_NULL &&
          !pControl->CanPassDoor(door))
      {
         g_bPathfindControllerFailed = TRUE;
         return FALSE;
      }
      if (LinkOkBits & kAIOK_App1)
      {
         if (pControl->AppCanPass1(from, to) == FALSE)
            return FALSE;
      }

      if (LinkOkBits & kAIOK_App2)
      {
         if (pControl->AppCanPass2(from, to) == FALSE)
            return FALSE;
      }
   }

// wsf: this for including RED links.
#if 0
   // Special handling of HighStrike. If we've specified highstrike, then also test RED links.
   if ((AIOkBits & kAIOKCOND_HighStrike) && !LinkOkBits)
      return TRUE;
#endif

   // If we're considered to be a small creature, then make sure we use only small creature links.
   // LVLs and SmallCreature links are mutually exclusive.
   if ((AIOkBits & kAIOK_SmallCreature) && !(LinkOkBits & kAIOK_SmallCreature))
      return FALSE;

   // If this callback does not apply to this link, forget it.
   // This should be, by a vast margin, the common case.

   if (!AIOkBits)
      return TRUE;

   // 'stressed' and 'highstrike' never occur at the same time:
   // Test condition bits. If link requires a condition, but we don't match, then fail.
   if ((LinkOkBits&kConditionMask) && !(LinkOkBits&AIOkBits&kConditionMask))
      return FALSE;

    // Normal bits match.
   if (LinkOkBits&AIOkBits&~kConditionMask)
      return TRUE;

   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

static tAIPathCellID FindAnyCell(const mxs_vector & fromLocation, const mxs_vector & toLocation, float distCheck)
{
   #define kMaxSteps 2
   tAIPathCellID result = 0;
   mxs_vector    delta;
   mxs_vector    test;
   float         dist = mx_dist_vec(&toLocation, &fromLocation);
   float         stepVal;

   mx_sub_vec(&delta, &toLocation, &fromLocation);

   stepVal = distCheck / kMaxSteps;
   for (int i = 1; i < kMaxSteps + 1; i++)
   {
      if (dist > (stepVal * i))
      {
         mx_scale_vec(&test, &delta, (dist - (stepVal * i)) / dist);
         mx_addeq_vec(&test, &fromLocation);
         result = AIFindClosestCell(test, 0);
         if (result)
            break;
      }
   }
   return result;
}

///////////////////////////////////////////////////////////////////////////////
//
// Low-level functioins
//

#define kRetryDist 4.0

DECLARE_TIMER(AI_PF, Average);
DECLARE_TIMER(AI_PF_Setup, Average);
DECLARE_TIMER(AI_PF_FindCell, Average);
DECLARE_TIMER(AI_PF_Init, Average);




// Given edge points and destination, return point along edge that is closest to destination.
static inline void CalcClosestEdgePoint(mxs_vector *pFinalLocation, const mxs_vector *toLocation, const mxs_vector *pEdge1, const mxs_vector *pEdge2)
{
   float fScale;
   mxs_vector Point2;
   mxs_vector Location;

   mx_sub_vec(&Point2, pEdge2, pEdge1);
   mx_sub_vec(&Location, toLocation, pEdge1);


   // Mostly vertical:
   if (fabs(Point2.y) < 0.001)
      fScale = Location.y/Point2.y;
   else // test slope.  @TBD: (x1 != x2) : safe assumption?
      {
      float fSlope = Point2.y/Point2.x;

// long form:
//      float fC = -Location.y/fSlope;
//      float fXSolution = (fSlope*fC)/(fSlope*fSlope+1);

// short form:
      float fXSolution = -Location.y/(fSlope*fSlope+1);
      fScale = fXSolution/Point2.x;
      }

   if (fScale < 0)
      *pFinalLocation = *pEdge1;
   else if (fScale > 1)
      *pFinalLocation = *pEdge2;
   else
      {
      pFinalLocation->x = pEdge1->x + Point2.x*fScale;
      pFinalLocation->y = pEdge1->y + Point2.y*fScale;
      pFinalLocation->z = pEdge1->z + Point2.z*fScale;
      }
}


// Edge assumes to go from origin to pPoint.
// return dist2
static inline float CalcDist2ToEdge(const mxs_vector *pLocation, const mxs_vector *pPoint)
   {
   float fScale;

   float fXSolution;
   float fSlope;

   // Mostly vertical:
   if (fabs(pPoint->x) < 0.001)
      fScale = pLocation->y/pPoint->y;
   else // test slope.  @TBD: (x1 != x2) : safe assumption?
      {
      fSlope = pPoint->y/pPoint->x;
      fXSolution = pLocation->y/(fSlope*fSlope+1);
      fScale = fXSolution/pPoint->x;
      }

   if (fScale < 0) // return dist2 to origin
      return mx_mag2_vec(pLocation);
   else if (fScale > 1) // return dist2 to point
      {
      mxs_vector RelVec;
      mx_sub_vec(&RelVec, pLocation, pPoint);
      RelVec.z = 0;
      return mx_mag2_vec(&RelVec);
      }
   else // return dist to edge
      {
      // vertical.
      if (fabs(pPoint->x) < 0.001)
         return pLocation->x*pLocation->x;

      // use XSolution
      mxs_vector RelVec;
      RelVec.x = pLocation->x-fXSolution;
      RelVec.y = pLocation->y-fXSolution*fSlope;
      RelVec.z = 0;
      return mx_mag2_vec(&RelVec);
      }
   }



// Return shortest distance from location to point/edge of cell.
// Point is assumed to be outside of cell.
static inline float CellDistToPoint(tAIPathCellID cellId, const mxs_vector *pLocation, float fMinDist2)
{
   const cMxsVector * p1;
   const cMxsVector * p2;

   const sAIPathCell *pCell = g_AIPathDB.GetCell(cellId);
   const int iFirstVertex = pCell->firstVertex;
   const int iVertexLimit = iFirstVertex + pCell->vertexCount;

   mxs_vector EdgeVec;
   mxs_vector VertToPoint;

   float fClosestDist2 = 999999;
   float fDist2;

   p2=&(g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[iFirstVertex].id].pt);

   for (int i = iFirstVertex; (i < iVertexLimit) && (fClosestDist2 > fMinDist2); i++)
   {
      p1 = p2;
      if (i == iVertexLimit-1)
         p2 = &(g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[iFirstVertex].id].pt);
      else
         p2 = &(g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[i+1].id].pt);

      // get the 2d edge vector
      EdgeVec.x = p2->x - p1->x;
      EdgeVec.y = p2->y - p1->y;

      // get the 2d edge vertex to point data
      VertToPoint.x = pLocation->x - p1->x;
      VertToPoint.y = pLocation->y - p1->y;

      // dot prod for "outside/inside" test using perp to edge_vec.
      if ((EdgeVec.y*VertToPoint.x-EdgeVec.x*VertToPoint.y) > 0) // Inside edge, so ignore.
         continue;

      // Determine closest dist to this edge.
//      fDist2 = CalcDist2ToEdge(pLocation, &VertToPoint);
      fDist2 = CalcDist2ToEdge(&VertToPoint, &EdgeVec);
      if (fDist2 < fClosestDist2)
         fClosestDist2 = fDist2;
   }

   return fClosestDist2;
}


//
// wsf: Hello, future generations. Sorry for the clutter. I'm adding a "PathfindNear" ability, so
// that AI's can do hand-to-hand strikes even if they can't pathfind right next to you. To do this,
// I'm making a parallel (read "redundant") version of the AIPathfindInternal function, that will
// likely look a lot like AIPathfindInternal, with some special stuff added. Soo sorry.
//

//
// Find a path that gets us fDist close to our target.
//
//    1) Get all cells that are within fDist of target. Do this by setting up that pathdb
//       to store "strikeable" links - those that are greater than LVLs, but less than the
//       average strike distance (about 5.1 ft - roughly HugeZ from aimultcb). Danger here. If we chose a number that is too high
//       for the median strike distance, given the actual AI in question, then too many cells
//       will be considered. A median that is too small will result in opportunities being passed up. Oh well.
//       We err on the side of slight inefficiency.
//
//    2) Scan each of these cells. Use first cell that we can actually path to. ideally, we'd
//       path to cell closest to AI, but that's being hoggish. First find will yield good-enough
//       results.
//


static IAIPath * AIPathfindNearInternal(const mxs_vector & fromLocation, tAIPathCellID startCell,
                     const mxs_vector & toLocation, float fDist,  tAIPathCellID endCell,
                     tAIPathOkBits OkBits,
                     IAIPathfindControl * pControl, eAIPathZoneType ZoneType)
{
   AUTO_TIMER(AI_PF);
   #define MAX_PATH_COST 10000.0

   static BOOL recursing;

   g_bPathfindControllerFailed = FALSE;

   if (!g_fAIPathFindInited)
   {
      mprintf("No pathfinding database.\n");
      return NULL;
   }

   sAIPathCell * pCell;
   sAIPathCell * pNewCell;
   tAIPathCellID i;

#if 0
   AISpewPathFind (("AIPathFind: request [%g, %g, %g] ---> [%g, %g, %g]\n",
      fromLocation.x, fromLocation.y, fromLocation.z,
      toLocation.x, toLocation.y, toLocation.z));
#endif

   TIMER_Start(AI_PF_Setup);

   TIMER_Start(AI_PF_FindCell);

   // dist is measured squared.
   fDist *= fDist;

   // find start & end cells
   startCell = AIFindClosestCell(fromLocation, startCell);
   endCell = AIFindClosestCell(toLocation, endCell);

   TIMER_MarkStop(AI_PF_FindCell);

   // We're hosed. Unlike the normal FindPath, we don't try to get nearby cells for starting/ending. Just fail.
   if (!startCell || !endCell)
      {
      Warning (("AIPathFindNear: couldn't find %s cells.\n", ((startCell == endCell) ? "start and end" : (endCell) ? "start" : "end")));
      TIMER_MarkStop(AI_PF_Setup);
      return 0;
      }

   AISpewPathFind (("PathfindNear %d -> %d\n", startCell, endCell));

   if (startCell == endCell)
   {
      AISpewPathFind (("AIPathFindNear: start & end cells equal, returning TRUE w/o pathfinding.\n"));
      TIMER_MarkStop(AI_PF_Setup);
      cAIPath * pStraightPath = new cAIPath;
      pStraightPath->SetFinalDestination(toLocation);
      return pStraightPath;
   }

   // do zone-checking
   if (g_fAIUsePathZones &&
       !g_AIPathDB.CanPathfindBetweenZones(ZoneType, g_AIPathDB.GetCellZone(ZoneType, startCell),
                                           g_AIPathDB.GetCellZone(ZoneType, endCell),
                                           OkBits))
   {
      AIWatch2(Path, g_AICurrentAI, "Can't pathfind between zones! Cells: %d -> %d\n", startCell, endCell);
      goto fail_retry;
   }

   // Grab cells that are within acceptable distance of destination point. Distance from cell
   // to point is done by vertex/edge distance tests. Cells may be connected via kAIOKCOND_HighStrike
   // links.

   //
   // Slightly hacky, but only allow kAIOKCOND_HighStrike for the cells that immediately neighbor
   // then end cell. Since most other configurations would give us paths where the target is truly
   // unstrikable.
   //

   // initialize the path search list...
   ///////////////////////////////////////

   // @TODO:OPTIMIZE
   // Reseting the entire pathfinding database's inOpenList is really
   // a waste, and creates a bad minimum cost to call this function.
   // a better thing to do would be to reset just the cells that
   // were touched at the end of this function so that the database is setup
   // for the next run ahead of time.  i think that would be faster.
   // you could also experiment with different storage mechanisms
   // that wouldn't require an inOpenList.
   // the advantage of the inOpenList is that it makes it very
   // quick to answer the question, "is it already expanded?"
   // and if so, what was the cost to reach it?  i have an idea,
   // what if inOpenList wasn't marked as TRUE, but rather with
   // an ID unique to this pathfind request.  inOpenList == FALSE
   // would be replace with inOpenList != uniqueID.  hmmm.  bestCostToReach
   // would still need to handled.  so....
   //       inOpenList == 0  means bestCostToReach is valid, but cell not in openList
   //       inOpenList == n > 0 && != uniqueID means bestCostToReach is invalid, never in openList this pathfind request
   //       inOpenList == uniqueID means bestCostToReach is valid, and the cell is in the openList

   TIMER_Start(AI_PF_Init);

   g_AIPathDB.MarkBegin();
   memset(g_AIPathDB.m_BestCostToReach, 0xFF, (g_AIPathDB.m_nCells  + 1) * sizeof(g_AIPathDB.m_BestCostToReach[0]));

   TIMER_MarkStop(AI_PF_Init);

   TIMER_MarkStop(AI_PF_Setup);



   // do the A* search
   ///////////////////////////////////////

   {
      tAIPathCellID headOfOpenList = 0;    // ptr into g_AIPathDB.m_Cells array
      tAIPathCellID bestNode = 0;          // best in open list to expand
      tAIPathCellID bestNodesPrevious;     // who's head of best in the list to expand
      tAIPathCellID newNode;               // new node being added to the open list
      float      totalCost;             // best cost to reach plus heuristic cost
      float      bestTotalCost;
      float      costToReachNewNode;
      cMxsVector    center;
      tAIPathOkBits nUseHighStrike; // do this for the immediate neighbors of endCell only.

      // create the open list, with the start cell
      ///////////////////////////////////////

      headOfOpenList = startCell;
      g_AIPathDB.m_Cells[headOfOpenList].bestNeighbor = 0;
      g_AIPathDB.m_Cells[headOfOpenList].linkFromNeighbor = 0;
      g_AIPathDB.m_Cells[headOfOpenList].next = 0;

      g_AIPathDB.m_BestCostToReach[headOfOpenList] = 0;

      // while we still have options to explore in the open list,
      // and we haven't found the end cell, keep expanding
      // new options in the open list.....
      ///////////////////////////////////////

      while (headOfOpenList != 0 && bestNode != endCell)
      {
         // find the best node to expand from the open list
         tAIPathCellID previous;

         AISpewPathFind (("Open List: "));

         // @TODO:OPTIMIZE
         // the linked list that uses the "next" field is currently unordered.
         // if it were made to use an insertion sort, it would slow down
         // the insertion of news cells in the "expand best node" loop below
         // _but_ the following loop to find the best node could be dropped
         // all together.  It's probably worth it.

         i = headOfOpenList;
         bestTotalCost = MAX_PATH_COST;
         previous = 0;
         while (i != 0)
         {
            AISpewPathFind (("%d ", i));

            g_AIPathDB.GetCenter(i, &center);

            totalCost = g_AIPathDB.m_BestCostToReach[i] + (ulong)AIDistance(center, toLocation);
            if (totalCost < bestTotalCost)
            {
               bestTotalCost = totalCost;
               bestNodesPrevious = previous;
               bestNode = i;
            }
            previous = i;
            i = g_AIPathDB.m_Cells[i].next;
         }

         AISpewPathFind (("[best %d]\n", bestNode));

         // expand the best node
         pCell = g_AIPathDB.AccessCell(bestNode);

         sAIPathCellLink * pLink;
         const int lastCell = pCell->firstCell + pCell->cellCount;
         for (i = pCell->firstCell; i < lastCell; i++)
         {
            pLink = &g_AIPathDB.m_Links[i];

            // If we're linking to endCell, flip HighStrike bit:
            nUseHighStrike = 0;
            if (pLink->dest == endCell)
               nUseHighStrike = kAIOKCOND_HighStrike; // do this for the immediate neighbors of endCell only.

            if (AICanUseLink(bestNode, pLink->dest, pLink->okBits, OkBits|nUseHighStrike, (kAIPF_Unpathable | kAIPF_BlockingOBB), pControl))
            {
               newNode            = pLink->dest;
               costToReachNewNode = g_AIPathDB.m_BestCostToReach[bestNode] + pLink->cost;
               pNewCell           = g_AIPathDB.AccessCell(newNode);

               if (g_pAIRoomDB->IsCellValid(newNode))
               {
                  // It's a valid node for this search

                  if (costToReachNewNode < g_AIPathDB.m_BestCostToReach[newNode])
                  {
                     // if this is a cell neighboring the endCell, check to see if
                     // it is indeed close to actual destination.
                     if (!nUseHighStrike || (CellDistToPoint(bestNode, &toLocation, fDist) < fDist))
                     // hey we found a lower cost way to reach this node, or
                     // we're visiting it for the first time,
                     // and the okbits says this ai is ok to fit through the link.
                     {
                        if (!g_AIPathDB.GetMark(newNode))
                        {
                           // not currently in the active list, so let's
                           // put this node (back) in.
                           AISpewPathFind (("  adding %d\n", newNode));

                           pNewCell->next = pCell->next;
                           pCell->next    = (tAIPathCellIDPacked)newNode;
                           g_AIPathDB.Mark(newNode);
                        }

                        // update pts of this node to bestNode so we can remember
                        // the path eventually.

                        g_AIPathDB.m_BestCostToReach[newNode] = costToReachNewNode;
                        pNewCell->bestNeighbor     = (tAIPathCellIDPacked) bestNode;
                        pNewCell->linkFromNeighbor = (tAIPathCell2CellLinkIDPacked) i;
                     }
                  }
               }
            }
         }

         // remove the best node from the open list
         if (bestNodesPrevious != 0)
         {
            // remove from the middle of the list
            g_AIPathDB.m_Cells[bestNodesPrevious].next = pCell->next;
         }
         else
         {
            // remove from the beginning of the list
            headOfOpenList = pCell->next;
         }
         pCell->next = 0;
         g_AIPathDB.Unmark(bestNode);
      }

      if (bestNode != endCell)
      {
         g_AIPathDB.MarkEnd();
         goto fail_retry;
      }
   }

   g_AIPathDB.MarkEnd();

   {
   // make the final path
   ///////////////////////////////////////
   cAIPath * pPath = new cAIPath;

   {
      tAIPathCellID   pathBack; // index into the g_AIPathDB.m_Cells array
      cAIPathEdge     newEdge;  // new edge to add into final path
      mxs_vector FinalLocation;

      // Final location is point on just previous to endCell cell, closest to endCell.
      CalcClosestEdgePoint(&FinalLocation, &toLocation,
            &g_AIPathDB.m_Vertices[g_AIPathDB.m_Links[g_AIPathDB.m_Cells[endCell].linkFromNeighbor].vertex_1].pt,
            &g_AIPathDB.m_Vertices[g_AIPathDB.m_Links[g_AIPathDB.m_Cells[endCell].linkFromNeighbor].vertex_2].pt);

      pPath->SetActive(FALSE);
      pPath->SetFinalDestination(FinalLocation);

      AISpewPathFind (("final path: "));

      pathBack = endCell;

      while (pathBack != startCell)
      {
         AISpewPathFind (("%d ", pathBack));

         newEdge.cell = g_AIPathDB.m_Cells[pathBack].bestNeighbor;
         newEdge.vertexA = g_AIPathDB.m_Links[g_AIPathDB.m_Cells[pathBack].linkFromNeighbor].vertex_1;
         newEdge.vertexB = g_AIPathDB.m_Links[g_AIPathDB.m_Cells[pathBack].linkFromNeighbor].vertex_2;

         tAIPathOkBits TestBits = g_AIPathDB.m_Links[g_AIPathDB.m_Cells[pathBack].linkFromNeighbor].okBits;
         pPath->AddPathEdge(newEdge);

         pathBack = g_AIPathDB.m_Cells[pathBack].bestNeighbor;

#ifdef DEBUG
         if (pathBack == 0)
         {
            Warning(("AIPathFind: whoah! failed bad.\n"));
            SafeRelease(pPath);
            return NULL;
         }
#endif
      }
      AISpewPathFind (("\n"));
   }

   pPath->SetCurrentPathEdgeAtStart();
   pPath->SetControl(pControl);

   return pPath;
   }
fail_retry:
#if 0
   // Try starting at a recently valid cell
   if (pControl && !recursing)
   {
      cAICellArray recent;
      pControl->GetRecentCells(&recent);
      tAIPathCellID newStart = 0;
      tAIPathCellID newEnd = endCell;
      for (int j = 0; j < recent.Size(); j++)
      {
         if (!(g_AIPathDB.m_Cells[recent[j]].pathFlags & (kAIPF_Unpathable | kAIPF_BlockingOBB)))
            newStart = recent[j];
      }

      if (newStart)
      {
         if (newStart == startCell && AIDistanceSq(fromLocation, toLocation) < sq(kRetryDist))
         {
            newEnd = FindAnyCell(fromLocation, toLocation, kRetryDist);
         }

         recursing = TRUE;
         return AIPathfindNearInternal(g_AIPathDB.GetCenter(newStart), newStart,
                           g_AIPathDB.GetCenter(newEnd), fDist, newEnd,
                           OkBits, pControl);
         recursing = FALSE;
      }
   }
#endif
   return NULL;
}




static IAIPath * AIPathfindInternal(const mxs_vector & fromLocation, tAIPathCellID startCell,
                     const mxs_vector & toLocation,   tAIPathCellID endCell,
                     tAIPathOkBits OkBits,
                     IAIPathfindControl * pControl, eAIPathZoneType ZoneType)
{
   AUTO_TIMER(AI_PF);
   #define MAX_PATH_COST 10000.0

   static BOOL recursing;

   g_bPathfindControllerFailed = FALSE;

   if (!g_fAIPathFindInited)
   {
      mprintf("No pathfinding database.\n");
      return NULL;
   }

   sAIPathCell * pCell;
   sAIPathCell * pNewCell;
   tAIPathCellID i;
   int           j;

#if 0
   AISpewPathFind (("AIPathFind: request [%g, %g, %g] ---> [%g, %g, %g]\n",
      fromLocation.x, fromLocation.y, fromLocation.z,
      toLocation.x, toLocation.y, toLocation.z));
#endif

   TIMER_Start(AI_PF_Setup);

   TIMER_Start(AI_PF_FindCell);
   // find start & end cells
   startCell = AIFindClosestCell(fromLocation, startCell);
   endCell = AIFindClosestCell(toLocation, endCell);

   TIMER_MarkStop(AI_PF_FindCell);

   if (!startCell && pControl)
   {
      cAICellArray recent;
      pControl->GetRecentCells(&recent);
      for (j = 0; j < recent.Size(); j++)
      {
         if (!(g_AIPathDB.m_Cells[recent[j]].pathFlags & (kAIPF_Unpathable | kAIPF_BlockingOBB)))
            startCell = recent[j];
      }
   }

   if (!startCell)
   {
      startCell = FindAnyCell(toLocation, fromLocation, kRetryDist);
   }

   if (!endCell)
   {
      endCell = FindAnyCell(fromLocation, toLocation, kRetryDist);
   }

   if (startCell == 0 || endCell == 0)
   {
      Warning (("AIPathFind: couldn't find %s cells.\n", ((startCell == endCell) ? "start and end" : (endCell) ? "start" : "end")));
      TIMER_MarkStop(AI_PF_Setup);
      return NULL;
   }

   AISpewPathFind (("Pathfind %d -> %d\n", startCell, endCell));

   if (startCell == endCell)
   {
      AISpewPathFind (("AIPathFind: start & end cells equal, returning TRUE w/o pathfinding.\n"));
      TIMER_MarkStop(AI_PF_Setup);
      cAIPath * pStraightPath = new cAIPath;
      pStraightPath->SetFinalDestination(toLocation);
      return pStraightPath;
   }

   // do zone-checking
   if (g_fAIUsePathZones &&
       !g_AIPathDB.CanPathfindBetweenZones(ZoneType, g_AIPathDB.GetCellZone(ZoneType, startCell),
                                           g_AIPathDB.GetCellZone(ZoneType, endCell),
                                           OkBits))
   {
      AIWatch2(Path, g_AICurrentAI, "Can't pathfind between zones! Cells: %d -> %d\n", startCell, endCell);
      goto fail_retry;
   }

   // initialize the path search list...
   ///////////////////////////////////////

   // @TODO:OPTIMIZE
   // Reseting the entire pathfinding database's inOpenList is really
   // a waste, and creates a bad minimum cost to call this function.
   // a better thing to do would be to reset just the cells that
   // were touched at the end of this function so that the database is setup
   // for the next run ahead of time.  i think that would be faster.
   // you could also experiment with different storage mechanisms
   // that wouldn't require an inOpenList.
   // the advantage of the inOpenList is that it makes it very
   // quick to answer the question, "is it already expanded?"
   // and if so, what was the cost to reach it?  i have an idea,
   // what if inOpenList wasn't marked as TRUE, but rather with
   // an ID unique to this pathfind request.  inOpenList == FALSE
   // would be replace with inOpenList != uniqueID.  hmmm.  bestCostToReach
   // would still need to handled.  so....
   //       inOpenList == 0  means bestCostToReach is valid, but cell not in openList
   //       inOpenList == n > 0 && != uniqueID means bestCostToReach is invalid, never in openList this pathfind request
   //       inOpenList == uniqueID means bestCostToReach is valid, and the cell is in the openList

   TIMER_Start(AI_PF_Init);

   g_AIPathDB.MarkBegin();
   memset(g_AIPathDB.m_BestCostToReach, 0xFF, (g_AIPathDB.m_nCells  + 1) * sizeof(g_AIPathDB.m_BestCostToReach[0]));

   TIMER_MarkStop(AI_PF_Init);

   TIMER_MarkStop(AI_PF_Setup);

   // do the A* search
   ///////////////////////////////////////

   {
      tAIPathCellID headOfOpenList = 0;    // ptr into g_AIPathDB.m_Cells array
      tAIPathCellID bestNode = 0;          // best in open list to expand
      tAIPathCellID bestNodesPrevious;     // who's head of best in the list to expand
      tAIPathCellID newNode;               // new node being added to the open list
      float      totalCost;             // best cost to reach plus heuristic cost
      float      bestTotalCost;
      float      costToReachNewNode;
      cMxsVector    center;

      // create the open list, with the start cell
      ///////////////////////////////////////

      headOfOpenList = startCell;
      g_AIPathDB.m_Cells[headOfOpenList].bestNeighbor = 0;
      g_AIPathDB.m_Cells[headOfOpenList].linkFromNeighbor = 0;
      g_AIPathDB.m_Cells[headOfOpenList].next = 0;

      g_AIPathDB.m_BestCostToReach[headOfOpenList] = 0;

      // while we still have options to explore in the open list,
      // and we haven't found the end cell, keep expanding
      // new options in the open list.....
      ///////////////////////////////////////

      while (headOfOpenList != 0 && bestNode != endCell)
      {
         // find the best node to expand from the open list
         tAIPathCellID previous;

         AISpewPathFind (("Open List: "));

         // @TODO:OPTIMIZE
         // the linked list that uses the "next" field is currently unordered.
         // if it were made to use an insertion sort, it would slow down
         // the insertion of news cells in the "expand best node" loop below
         // _but_ the following loop to find the best node could be dropped
         // all together.  It's probably worth it.

         i = headOfOpenList;
         bestTotalCost = MAX_PATH_COST;
         previous = 0;
         while (i != 0)
         {
            AISpewPathFind (("%d ", i));

            g_AIPathDB.GetCenter(i, &center);

            totalCost = g_AIPathDB.m_BestCostToReach[i] + (ulong)AIDistance(center, toLocation);
            if (totalCost < bestTotalCost)
            {
               bestTotalCost = totalCost;
               bestNodesPrevious = previous;
               bestNode = i;
            }
            previous = i;
            i = g_AIPathDB.m_Cells[i].next;
         }

         AISpewPathFind (("[best %d]\n", bestNode));

         // expand the best node
         pCell = g_AIPathDB.AccessCell(bestNode);

         sAIPathCellLink * pLink;
         const int lastCell = pCell->firstCell + pCell->cellCount;
         for (i = pCell->firstCell; i < lastCell; i++)
         {
            pLink = &g_AIPathDB.m_Links[i];

            if (AICanUseLink(bestNode, pLink->dest, pLink->okBits, OkBits, (kAIPF_Unpathable | kAIPF_BlockingOBB), pControl))
            {
               newNode            = pLink->dest;
               costToReachNewNode = g_AIPathDB.m_BestCostToReach[bestNode] + pLink->cost;
               pNewCell           = g_AIPathDB.AccessCell(newNode);

               if (g_pAIRoomDB->IsCellValid(newNode))
               {
                  // It's a valid node for this search

                  if (costToReachNewNode < g_AIPathDB.m_BestCostToReach[newNode])
                  {
                     // hey we found a lower cost way to reach this node, or
                     // we're visiting it for the first time,
                     // and the okbits says this ai is ok to fit through the link.

                     if (!g_AIPathDB.GetMark(newNode))
                     {
                        // not currently in the active list, so let's
                        // put this node (back) in.
                        AISpewPathFind (("  adding %d\n", newNode));

                        pNewCell->next = pCell->next;
                        pCell->next    = (tAIPathCellIDPacked)newNode;
                        g_AIPathDB.Mark(newNode);
                     }

                     // update pts of this node to bestNode so we can remember
                     // the path eventually.

                     g_AIPathDB.m_BestCostToReach[newNode] = costToReachNewNode;
                     pNewCell->bestNeighbor     = (tAIPathCellIDPacked) bestNode;
                     pNewCell->linkFromNeighbor = (tAIPathCell2CellLinkIDPacked) i;
                  }
               }
            }
         }

         // remove the best node from the open list
         if (bestNodesPrevious != 0)
         {
            // remove from the middle of the list
            g_AIPathDB.m_Cells[bestNodesPrevious].next = pCell->next;
         }
         else
         {
            // remove from the beginning of the list
            headOfOpenList = pCell->next;
         }
         pCell->next = 0;
         g_AIPathDB.Unmark(bestNode);
      }

      if (bestNode != endCell)
      {
         g_AIPathDB.MarkEnd();
         goto fail_retry;
      }
   }

   g_AIPathDB.MarkEnd();

   {
   // make the final path
   ///////////////////////////////////////
   cAIPath * pPath = new cAIPath;

   {
      tAIPathCellID   pathBack; // index into the g_AIPathDB.m_Cells array
      cAIPathEdge     newEdge;  // new edge to add into final path

      pPath->SetActive(FALSE);
      pPath->SetFinalDestination(toLocation);

      AISpewPathFind (("final path: "));

      pathBack = endCell;
      while (pathBack != startCell)
      {
         AISpewPathFind (("%d ", pathBack));

         newEdge.cell = g_AIPathDB.m_Cells[pathBack].bestNeighbor;
         newEdge.vertexA = g_AIPathDB.m_Links[g_AIPathDB.m_Cells[pathBack].linkFromNeighbor].vertex_1;
         newEdge.vertexB = g_AIPathDB.m_Links[g_AIPathDB.m_Cells[pathBack].linkFromNeighbor].vertex_2;

         tAIPathOkBits TestBits = g_AIPathDB.m_Links[g_AIPathDB.m_Cells[pathBack].linkFromNeighbor].okBits;
         pPath->AddPathEdge(newEdge);

         pathBack = g_AIPathDB.m_Cells[pathBack].bestNeighbor;

#ifdef DEBUG
         if (pathBack == 0)
         {
            Warning(("AIPathFind: whoah! failed bad.\n"));
            SafeRelease(pPath);
            return NULL;
         }
#endif
      }
      AISpewPathFind (("\n"));
   }

   pPath->SetCurrentPathEdgeAtStart();
   pPath->SetControl(pControl);

   return pPath;
   }

fail_retry:
#if 0
   // Try starting at a recently valid cell
   if (pControl && !recursing)
   {
      cAICellArray recent;
      pControl->GetRecentCells(&recent);
      tAIPathCellID newStart = 0;
      tAIPathCellID newEnd = endCell;
      for (int j = 0; j < recent.Size(); j++)
      {
         if (!(g_AIPathDB.m_Cells[recent[j]].pathFlags & (kAIPF_Unpathable | kAIPF_BlockingOBB)))
            newStart = recent[j];
      }

      if (newStart)
      {
         if (newStart == startCell && AIDistanceSq(fromLocation, toLocation) < sq(kRetryDist))
         {
            newEnd = FindAnyCell(fromLocation, toLocation, kRetryDist);
         }

         recursing = TRUE;
         return AIPathfindInternal(g_AIPathDB.GetCenter(newStart), newStart,
                           g_AIPathDB.GetCenter(newEnd), newEnd,
                           OkBits, pControl);
         recursing = FALSE;
      }
   }
#endif
   return NULL;
}


#ifndef SHIP
BOOL g_fAIPDB_UseDoubleFind = TRUE;
#endif


// First try non-stressed path, then if it fails, do stressed.
IAIPath * AIPathfind(const mxs_vector & fromLocation, tAIPathCellID startCell,
                     const mxs_vector & toLocation,   tAIPathCellID endCell,
                     tAIPathOkBits OkBits,
                     IAIPathfindControl * pControl)
{
   IAIPath *pPath = AIPathfindInternal(fromLocation, startCell,
                                       toLocation, endCell,
                                       OkBits,
                                       pControl, kAIZone_Normal);

   // Path failed. Trying "stressed"
#ifndef SHIP
   if (g_fAIPDB_UseDoubleFind)
#endif

   // If we're not a small creature, try "stressed" links.
   if (!pPath && !(OkBits&kAIOK_SmallCreature))
      {
      Warning(("Doing Alternate Pathfind!\n"));
      pPath = AIPathfindInternal(fromLocation, startCell,
                                       toLocation, endCell,
                                       OkBits | kAIOKCOND_Stressed,
                                       pControl, kAIZone_NormalLVL);
      if (pPath)
         pPath->SetStressBits(kAIOKCOND_Stressed);
      }
   return pPath;
}


// It is assumed that this is being called because normal pathfinding failed.
IAIPath * AIPathfindNear(const mxs_vector & fromLocation, tAIPathCellID startCell,
                     const mxs_vector & toLocation, float fDist, tAIPathCellID endCell,
                     tAIPathOkBits OkBits,
                     IAIPathfindControl * pControl)
{
   IAIPath *pPath = AIPathfindNearInternal(fromLocation, startCell,
                                       toLocation, fDist, endCell,
                                       OkBits,
                                       pControl, kAIZone_HighStrike);

   // Path failed. Trying "stressed"
#ifndef SHIP
   if (g_fAIPDB_UseDoubleFind)
#endif

   // If we're not a small creature, try "stressed" links.
   if (!pPath && !(OkBits&kAIOK_SmallCreature))
      {
      Warning(("Doing Alternate Pathfind!\n"));
      pPath = AIPathfindNearInternal(fromLocation, startCell,
                                       toLocation, fDist, endCell,
                                       OkBits | kAIOKCOND_Stressed,
                                       pControl, kAIZone_HighStrikeLVL);
      if (pPath)
         pPath->SetStressBits(kAIOKCOND_Stressed);
      }
   return pPath;
}



///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIPathfinder
//

cAIPathfinder::cAIPathfinder()
 : cCTDelegating<IAIPathfindControl>((cAIPathfinderBase *)this),
   cCTDelegating<IAIPathQuery>((cAIPathfinderBase *)this)
{
}

///////////////////////////////////////

STDMETHODIMP cAIPathfinder::QueryInterface(REFIID id, void ** ppI)
{
   if (id == IID_IAIPathfindControl)
   {
      *ppI = (IAIPathfindControl *)this;
      cAIPathfinderBase::AddRef();
      return S_OK;
   }
   if (id == IID_IAIPathQuery)
   {
      *ppI = (IAIPathQuery *)this;
      cAIPathfinderBase::AddRef();
      return S_OK;
   }
   return cAIPathfinderBase::QueryInterface(id, ppI);
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIPathfinder::GetName()
{
   return "Pathfinding component";
}

///////////////////////////////////////

STDMETHODIMP_(IAIPath *) cAIPathfinder::Pathfind(const mxs_vector & toLocation,
                                                 tAIPathCellID endCell,
                                                 tAIPathOkBits fOkBits)
{
   return AIPathfind(*m_pAIState->GetPathLocation(), m_pAIState->GetPathCell(),
                     toLocation, endCell, fOkBits, this);
}


///////////////////////////////////////

STDMETHODIMP_(IAIPath *) cAIPathfinder::PathfindNear(const mxs_vector & toLocation,
                                                     float fDist,
                                                     tAIPathCellID endCell,
                                                     tAIPathOkBits fOkBits)
{
   return AIPathfindNear(*m_pAIState->GetPathLocation(), m_pAIState->GetPathCell(),
                     toLocation, fDist, endCell, fOkBits, this);
}


///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIPathfinder::Pathcast(const mxs_vector & endLocation,
                                            tAIPathCellID * pEndCell,
                                            tAIPathOkBits fOkBits)
{
   return AIPathcast(*m_pAIState->GetPathLocation(), m_pAIState->GetPathCell(),
                     endLocation, pEndCell, fOkBits, this);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIPathfinder::Pathcast(const mxs_vector & endLocation,
                                            tAIPathOkBits fOkBits)
{
   tAIPathCellID ignored;
   return AIPathcast(*m_pAIState->GetPathLocation(), m_pAIState->GetPathCell(),
                     endLocation, &ignored, fOkBits, this);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIPathfinder::IsEdgeVisible(const cAIPathEdge & edge,
                                                 tAIPathOkBits OkBits)
{
   return AIIsEdgeVisible(*m_pAIState->GetPathLocation(), m_pAIState->GetPathCell(),
                          edge, OkBits, this);
}

///////////////////////////////////////

STDMETHODIMP_(IAIPathfindControl *) cAIPathfinder::AccessControl()
{
   return this;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIPathfinder::GetRecentCells(cAICellArray * pResult)
{
   m_pAI->GetRecentCells(pResult);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIPathfinder::CanPassDoor(ObjID door)
{
   if (!AIUsesDoors(GetID()))
      return FALSE;

   // @TBD: store ai property in state bit so we don't have to do prop lookup all the time.
   if (DoorIsBig(door) || !m_pAIState->NeedsLargeDoors())
      return AICanOpenDoor(GetID(), door);

   return FALSE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIPathfinder::AppCanPass1(tAIPathCellID from,
                                               tAIPathCellID to)
{
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIPathfinder::AppCanPass2(tAIPathCellID from,
                                               tAIPathCellID to)
{
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIPathfinder::CastForBoundary(const mxs_vector & end, mxs_vector * pHitLoc, mxs_vector * pBoundVec)
{
   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
