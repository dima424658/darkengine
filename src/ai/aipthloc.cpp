/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipthloc.cpp,v 1.13 2000/03/01 13:02:54 bfarquha Exp $
//
//
//

// #define PROFILE_ON 1

#include <vec2.h>

#include <objpos.h>
#include <rpaths.h>
#include <rooms.h>

#include <aidebug.h>
#include <aipthcst.h>
#include <aipathdb.h>
#include <aipthloc.h>
#include <aiutils.h>
#include <aiwr.h>
#include <memall.h>
#include <dbmem.h>   // must be last header!

///////////////////////////////////////////////////////////////////////////////
//
// AIFindClosestCell
//
// The point of FindClosestCell is to figure out which path cell a point is
// in.  Since the pathcells are simply planes, and the location is a three
// dimensional point, the correlation between the two can be a little tricky.
//
// The optimal solution involves searching through the entire path database,
// finding the cells which enclose the xy position of the location, and
// then comparing all of those cells with the location and finding the
// best z fit.  This should absolutely find the real path cell the
// location is in, should there actually be one, which there isn't always.
//
// However, since the above exhaustive search can be rather slow for large
// pathcell databases, the user should provide a hintCell (-1 when he can't)
// which provides the last known cell the object was in.  FindClosestCell
// will first try the hintCell, and then any cell directly connected to
// the hint cell.
//
// Finally, if the hintCell search and the exhaustive search have not found
// a single xy-plane match, FindClosestCell reverts to a simple distance
// based search, comparing the rough centers of the pathcells with the
// location.

///////////////////////////////////////

#define FCC_TIMING

#ifdef FCC_TIMING
DECLARE_TIMER(FCC, Average);
DECLARE_TIMER(FCC_A, Average);
DECLARE_TIMER(FCC_B, Average);
DECLARE_TIMER(FCC_C, Average);
#endif

///////////////////////////////////////

// Assuming aipoint pt is inside the the XY plane of the
// cell, GetZAtXYOnCell will return the exact z location
// of that pt if that pt is mapped onto the pathcell.
// (thus the pt.z is ignored)
#define GetZAtXYOnCell(pCell, pt) g_AIPathDB.GetZAtXY(pCell, pt)

// externally callable function to do above thing

float AIGetZAtXYOnCell(const mxs_vector & pt, tAIPathCellID cellID)
{
   const sAIPathCell *pCell=g_AIPathDB.GetCell(cellID);
   return GetZAtXYOnCell(pCell,pt);
}

///////////////////////////////////////

// since we know we can get small return values
#define EPS_TOL (5e-6)

// IsPtInCellXYPlane checks if aipoint p is within the cell, considering
// only the xy-plane.
BOOL IsPtInCellXYPlane(const mxs_vector &p, const sAIPathCell * pCell)
{
   int i;
   register const cMxsVector * p1;
   const cMxsVector * p2;

   float edge_vec_x, edge_vec_y;   // 2d vectors we dont have...
   float vert_to_point_x, vert_to_point_y;

#ifdef COUNT_CELL_CALLS
   static int counter=0;
   counter++;
   if ((counter&0x3ff)==0) mprintf("x400..");
#endif

   const int iFirstVertex = pCell->firstVertex;
   const int iVertexLimit = iFirstVertex + pCell->vertexCount;

   p2=&(g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[iFirstVertex].id].pt);
   for (i = iFirstVertex; i < iVertexLimit; i++)
   {  // really should ping-pong these two - but im lazy
      p1 = p2;
      if (i == iVertexLimit-1)
         p2 = &(g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[iFirstVertex].id].pt);
      else
         p2 = &(g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[i+1].id].pt);

      // get the 2d edge vector
      edge_vec_x = p2->x - p1->x;
      edge_vec_y = p2->y - p1->y;

      // get the 2d edge vertex to point data
      vert_to_point_x = p.x - p1->x;
      vert_to_point_y = p.y - p1->y;

      if (edge_vec_y*vert_to_point_x - edge_vec_x*vert_to_point_y < - EPS_TOL)
         return FALSE;
   }
   return TRUE;
}


///////////////////////////////////////

#define kMaxCloseCells 256

tAIPathCellID g_CloseCells[kMaxCloseCells];
unsigned      g_nCloseCells;

///////////////////////////////////////

inline tAIPathCellID AIFindClosestCellWithHint(const mxs_vector & location, tAIPathCellID hintCell)
{
   tAIPathCellID i;

   // First, let's try the hintCell itself.  If we're inside the xy-plane
   // of the hintCell.  We'll call it quits and move on with life.
   if (IsPtInCellXYPlane(location, g_AIPathDB.GetCell(hintCell)))
      return hintCell;

   // Ok, that didn't work.  Let's try the cells connected to the hintCell.
   const int lastCell = g_AIPathDB.m_Cells[hintCell].firstCell + g_AIPathDB.m_Cells[hintCell].cellCount;

   for (i = g_AIPathDB.m_Cells[hintCell].firstCell; i < lastCell; i++)
      if (g_AIPathDB.m_Links[i].okBits &&
          IsPtInCellXYPlane(location, g_AIPathDB.GetCell(g_AIPathDB.m_Links[i].dest)))
         return g_AIPathDB.m_Links[i].dest;

   return 0;
}

///////////////////////////////////////

#define kCloseCellZDist   8.0
#define kCloseCellXYDist 20.0

void __fastcall AIFindCloseCells(const mxs_vector & location)
{
   g_nCloseCells = 0;

   for (tAIPathCellID cell = 1; cell <= g_AIPathDB.m_nCells; cell++)
   {
      if (!g_pAIRoomDB->IsCellValid(cell))
         continue;

      const cMxsVector & center = g_AIPathDB.GetCenter(cell);
      if (aflt(kCloseCellXYDist, location.x - center.x) ||
          aflt(kCloseCellXYDist, location.y - center.y) ||
          aflt(kCloseCellZDist, location.z - center.z))
          continue;

      if (g_nCloseCells < kMaxCloseCells)
         g_CloseCells[g_nCloseCells++] = cell;
      else
         Warning(("Exceeded maximum close cells (" __FILE__ ")\n"));
   }
}

///////////////////////////////////////

inline tAIPathCellID AIFindClosestOfCloseCells(const mxs_vector & location)
{
   float       z, highestZ = -10000;

   const sAIPathCell * pCell;
   const sAIPathCell * pMatch = g_AIPathDB.GetCell(0);

   for (int i = 0; i < g_nCloseCells; i++)
   {
      pCell = g_AIPathDB.GetCell(g_CloseCells[i]);
      if (IsPtInCellXYPlane(location, pCell))
      {
         z = GetZAtXYOnCell(pCell, location);

         if (aflt(location.z - z, kAIEpsilon))
         {
            pMatch = pCell;
            break;
         }

         if (location.z > z && z > highestZ)
         {
            pMatch = pCell;
            highestZ = z;
         }
      }
   }

   tAIPathCellID result = pMatch - g_AIPathDB.GetCell(0);
   static BOOL recursing;

   if (!result && !recursing)
   {
      recursing = TRUE;
      #define kRetryRange 0.5
      // Could be an architectural hole, as created by ramp brushes and the like. Try tweaking the point
      mxs_vector retryVec = location;
      retryVec.x += kRetryRange;
      result = AIFindClosestOfCloseCells(retryVec);
      if (!result)
      {
         retryVec.x -= 2 * kRetryRange;
         result = AIFindClosestOfCloseCells(retryVec);
         if (!result)
         {
            retryVec.x += kRetryRange;
            retryVec.y += kRetryRange;
            result = AIFindClosestOfCloseCells(retryVec);
            if (!result)
            {
               retryVec.y -= 2 * kRetryRange;
               result = AIFindClosestOfCloseCells(retryVec);
            }
         }
      }
      recursing = FALSE;
   }

   // okay.  we found a cell, let's return the highest one we're above.
   return result;
}

///////////////////////////////////////

inline tAIPathCellID AIFindClosestCellExhaustive(const mxs_vector & location)
{
   float       z, highestZ = -10000;

   const sAIPathCell * pCell;
   const sAIPathCell * pMatch = g_AIPathDB.m_Cells;

   for (int i = 1; i <= g_AIPathDB.m_nCells; i++)
   {
      pCell = g_AIPathDB.GetCell(i);
      if (g_pAIRoomDB->IsCellValid(i) && IsPtInCellXYPlane(location, pCell))
      {
         z = GetZAtXYOnCell(pCell, location);

         if (aflt(location.z - z, kAIEpsilon))
         {
            pMatch = pCell;
            break;
         }

         if (location.z > z && z > highestZ)
         {
            pMatch = pCell;
            highestZ = z;
         }
      }
   }

   // okay.  we found a cell, let's return the highest one we're above.
   return pMatch - ((sAIPathCell * )g_AIPathDB.m_Cells);
}

///////////////////////////////////////

tAIPathCellID __fastcall AIFindClosestCell(const mxs_vector & origLoc,
                                           tAIPathCellID hintCell,
                                           ObjID objID,
                                           unsigned flags)
{
   if (!g_fAIPathFindInited)
      return 0;

#ifdef FCC_TIMING
   AUTO_TIMER(FCC);
#endif

   const BOOL fObjInHintRange = (objID > OBJ_NULL && objID < g_AIPathDB.m_nObjHints);
   tAIPathCellID closestCell;

   tAIPathCellID retVal = 0;

   static BOOL recursing;
   mxs_vector location;

   if (!recursing)
   {
      // First, get the true floor location plus some
      Location start, end, hit;

      ObjPos * pPos = ObjPosGet(objID);

      if (objID > OBJ_NULL && (pPos = ObjPosGet(objID)) != NULL)
      {
         MakeHintedLocationFromVector(&start, &origLoc, &pPos->loc);
         MakeHintedLocationFromVector(&end, &origLoc, &pPos->loc);
      }
      else
      {
         MakeLocationFromVector(&start, &origLoc);
         MakeLocationFromVector(&end, &origLoc);
      }

      end.vec.z -= 500.0;

      UpdateChangedLocation(&end);

      AIRaycast(&start, &end, &hit, kAIR_NoHintWarn | kAIR_VsObjects);

      location = hit.vec;
      location.z += 0.1;
   }
   else
      location = origLoc;


   BOOL using_rooms = FALSE;
   cRoom *pRoom;

#ifdef FCC_TIMING
   {
   AUTO_TIMER(FCC_A);
#endif

   if (hintCell == 0 && fObjInHintRange)
      hintCell = g_AIPathDB.m_ObjHints[objID];

   if (hintCell > 0 && hintCell <= g_AIPathDB.m_nCells)
   {
      // Ok.  We have a hintCell.  Let's play with that.
      closestCell = AIFindClosestCellWithHint(location, hintCell);

      if (closestCell)
      {
         if (fObjInHintRange)
            g_AIPathDB.m_ObjHints[objID] = closestCell;

         retVal = closestCell;
         goto find_closest_cell_done;
      }
   }
   else
      hintCell = 0;

#ifdef FCC_TIMING
   }
   {
   AUTO_TIMER(FCC_B);
#endif

   // Set up our room data
   if (!(flags & kAIFC_NoRooms) && !g_pAIRoomDB->IsEmptyDB())
   {
      if ((objID != OBJ_NULL) && ((pRoom = g_pRooms->GetObjRoom(objID)) != NULL))
         using_rooms = TRUE;
      else
      if ((pRoom = g_pRooms->RoomFromPoint(location)) != NULL)
         using_rooms = TRUE;
   }

   if (using_rooms)
   {
      g_pAIRoomDB->UseValidChecks(TRUE);
      g_pAIRoomDB->MarkRoomCells(pRoom->GetRoomID());
   }

   AIFindCloseCells(location);
   closestCell = AIFindClosestOfCloseCells(location);

   if (closestCell)
   {
      if (fObjInHintRange)
         g_AIPathDB.m_ObjHints[objID] = closestCell;

      retVal = closestCell;
      goto find_closest_cell_done;
   }

   if (flags & kAIFC_NoExhaustive)
      goto find_closest_cell_done;


#ifdef FCC_TIMING
   }
   {
   AUTO_TIMER(FCC_C);
#endif

   // Ok.  We either don't have a hintCell, or the hintCell didn't provide us
   // anything useful.  So, now, let's do an exhaustive search, trying to find
   // the highest cell that the location is within the xyplane of.
   closestCell = AIFindClosestCellExhaustive(location);

#ifdef FCC_TIMING
   }
#endif

   if (closestCell)
   {
      if (fObjInHintRange)
         g_AIPathDB.m_ObjHints[objID] = closestCell;

      retVal = closestCell;
      goto find_closest_cell_done;
   }

   // Okay.  Presumably, location isn't actually inside of a cell.  Let's throw
   // out a benign warning so we know when this is happening, because it could
   // potentially cause problems.  But since we're not actually in a cell, and
   // we may need to know which cell to start pathfinding from, let's try to
   // find the one we're closest to.

find_closest_cell_done:

   if (using_rooms)
   {
      g_pAIRoomDB->UnMarkRoomCells(pRoom->GetRoomID());
      g_pAIRoomDB->UseValidChecks(FALSE);
   }

   if (using_rooms && !retVal && !recursing && !(flags & kAIFC_NoRooms))
   {
      Warning(("AI (maybe %d) may be outside room database at (%d, %d, %d)\n", AIDebugGet(), (int)location.x, (int)location.y, (int)location.z));
      AIWatch3(Path, g_AICurrentAI, "AI may be outside room database at (%d, %d, %d)", (int)location.x, (int)location.y, (int)location.z);

      // In case the AI has exited the room database, or has been teleported, try without the room restrictions.
      recursing = TRUE;
      retVal = AIFindClosestCell(location, -1, 0, flags | kAIFC_NoRooms | kAIFC_NoExhaustive);
      recursing = FALSE;
   }

#ifdef DEBUG
   if (!retVal)
      Warning(("AI (maybe %d) or AI target (maybe %d) may be outside AI database at (%d, %d, %d)\n", AIDebugGet(), objID, (int)location.x, (int)location.y, (int)location.z));
#endif

   return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
//
//

tAIPathCellID AILocateBestLocoCell(const mxs_vector & fromLoc,
                                   const tAIPathCellID fromCell,
                                   const mxs_vector & to) // hints meaningful?
{
   tAIPathCellID castResult = 0;
   if (fromCell)
      AIPathcast(fromLoc, fromCell, to, &castResult);
   tAIPathCellID result = AIFindClosestCell(to, castResult, -1, kAIFC_NoExhaustive);
   if (!result)
      result = castResult;

   if (!result)
   {
      Warning(("An AI is outside the world at (%g, %g, %g)\n", fromLoc.x, fromLoc.y, fromLoc.z));
      result = AIFindClosestCell(to, 0, -1);
   }
   return result;
}

///////////////////////////////////////////////////////////////////////////////

struct sCloseCellEdge
{
   sCloseCellEdge()
    : cell(0), edge(-1), distSq(999999.0) {}

   tAIPathCellID cell;
   int           edge;
   float         distSq;
};

float g_ClosestCellEdgeThreshSq = sq(2.0);

///////////////////

void FindCloseCells(tAIPathCellID from, const mxs_vector & toLoc, sCloseCellEdge * pCurClosest)
{
   if (!g_AIPathDB.GetMark(from))
   {
      int i, closest;
      float distSq, closestDistSq = 999999.0;

      g_AIPathDB.Mark(from);

      const sAIPathCell * pCell = g_AIPathDB.GetCell(from);

      for (i = 0; i < pCell->vertexCount; i++)
      {
         const mxs_vector & v1 = g_AIPathDB.GetCellVertex(from, i);
         const mxs_vector & v2 = g_AIPathDB.GetCellVertex(from, i + 1);

         distSq = Vec2PointSegmentSquared((Vec2 *)&toLoc, (Vec2 *)&v1, (Vec2 *)&v2);
         if (distSq < closestDistSq)
         {
            closestDistSq = distSq;
            closest = i;
         }
      }

      if (closestDistSq < g_ClosestCellEdgeThreshSq)
      {
         if (closestDistSq < pCurClosest->distSq)
         {
            pCurClosest->cell = from;
            pCurClosest->edge = closest;
            pCurClosest->distSq = closestDistSq;
         }
         const int lastCell = g_AIPathDB.m_Cells[from].firstCell + g_AIPathDB.m_Cells[from].cellCount;

         for (i = g_AIPathDB.m_Cells[from].firstCell; i < lastCell; i++)
         {
            if (g_AIPathDB.m_Links[i].okBits)
               FindCloseCells(g_AIPathDB.m_Links[i].dest, toLoc, pCurClosest);
         }
      }
   }
}

///////////////////////////////////////

BOOL AIFindClosestConnectedCellSearch(tAIPathCellID from, const mxs_vector & fromLoc,
                                      const mxs_vector & toLoc,
                                      tAIPathCellID * pResultCell, mxs_vector * pClosestLoc)
{
   sCloseCellEdge closest;

   AIWatch7(RecoverPath, AIDebugGet(), "Attempting to recover path (%d, [%d, %d, %d], [%d, %d, %d]", from, (int)fromLoc.x, (int)fromLoc.y, (int)fromLoc.z, (int)toLoc.x, (int)toLoc.y, (int)toLoc.z);

   g_AIPathDB.MarkBegin();
   FindCloseCells(from, toLoc, &closest);
   g_AIPathDB.MarkEnd();

   if (closest.cell)
   {
      AIWatch2(RecoverPath, AIDebugGet(), "Best match was (%d, %d)", closest.cell, closest.edge);

      *pResultCell = closest.cell;
      if (IsPtInCellXYPlane(toLoc, &g_AIPathDB.m_Cells[closest.cell]))
      {
         *pClosestLoc = toLoc;
         pClosestLoc->z = AIGetZAtXYOnCell(toLoc, closest.cell);
         AIWatch4(RecoverPath, AIDebugGet(), "Point in best cell, returning (%d, [%d, %d, %d])", closest.cell, (int)pClosestLoc->x, (int)pClosestLoc->y, (int)pClosestLoc->z);
         return TRUE;
      }

      mxs_vector AC, ABNorm;

      mx_sub_vec(&AC, &toLoc, &g_AIPathDB.GetCellVertex(closest.cell, closest.edge));
      mx_sub_vec(&ABNorm, &g_AIPathDB.GetCellVertex(closest.cell, closest.edge + 1), &g_AIPathDB.GetCellVertex(closest.cell, closest.edge));
      mx_normeq_vec(&ABNorm);
      mx_scale_vec(pClosestLoc, &ABNorm, mx_dot_vec(&AC, &ABNorm));
      mx_addeq_vec(pClosestLoc, &g_AIPathDB.GetCellVertex(closest.cell, closest.edge));

      const mxs_vector & center = g_AIPathDB.GetCenter(closest.cell);
      Vec2Sub((Vec2 *)pClosestLoc, (Vec2 *)pClosestLoc, (Vec2 *)&center);
      Vec2Scale((Vec2 *)pClosestLoc, (Vec2 *)pClosestLoc, 0.999);
      Vec2Add((Vec2 *)pClosestLoc, (Vec2 *)pClosestLoc, (Vec2 *)&center);

      // Assert_(IsPtInCellXYPlane(*pClosestLoc, &g_AIPathDB.m_Cells[closest.cell]));
      pClosestLoc->z = AIGetZAtXYOnCell(*pClosestLoc, closest.cell);
      AIWatch4(RecoverPath, AIDebugGet(), "Returning (%d, [%d, %d, %d])", closest.cell, (int)pClosestLoc->x, (int)pClosestLoc->y, (int)pClosestLoc->z);

      return TRUE;
   }

   AIWatch(RecoverPath, AIDebugGet(), "Failed to recover path");
   return FALSE;
}

///////////////////////////////////////

BOOL AIFindClosestConnectedCell(tAIPathCellID from, const mxs_vector & fromLoc,
                                const mxs_vector & toLoc,
                                tAIPathCellID * pResultCell, mxs_vector * pClosestLoc, tAIPathOkBits nStressBits)
{
   AISetPathcastIgnoreBlockingOBBs(TRUE);
   if (!AIPathcast(fromLoc, from, toLoc, pResultCell, kAIOK_Normal | nStressBits, (IAIPathfindControl *)NULL))
   {
      AISetPathcastIgnoreBlockingOBBs(FALSE);

      return AIFindClosestConnectedCellSearch(from, fromLoc, toLoc, pResultCell, pClosestLoc);
   }
   else
   {
      pClosestLoc->x = toLoc.x;
      pClosestLoc->y = toLoc.y;
      pClosestLoc->z = AIGetZAtXYOnCell(toLoc, *pResultCell) + kAIEpsilon;
   }

   AISetPathcastIgnoreBlockingOBBs(FALSE);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

unsigned AIGetCellZone(eAIPathZoneType ZoneType, tAIPathCellID cell)
{
   return g_AIPathDB.GetCellZone(ZoneType, cell);
}

///////////////////////////////////////////////////////////////////////////////

const mxs_vector & AIGetCellCenter(const tAIPathCellID cell, mxs_vector * pCenter)
{
   return g_AIPathDB.GetCenter(cell, pCenter);
}

///////////////////////////////////////

const mxs_vector & AIGetCellCenter(const tAIPathCellID cell)
{
   return g_AIPathDB.GetCenter(cell);
}

///////////////////////////////////////////////////////////////////////////////
