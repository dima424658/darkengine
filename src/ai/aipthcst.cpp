/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipthcst.cpp,v 1.19 2000/02/28 17:27:55 toml Exp $
//
// Simple raycaster through the AI pathfinding database
//
// Links out of a cell are sorted by distance from the edge of the polygon for the benefit of the pathcaster (toml 06-04-99)

#include <config.h>
#include <matrix.h>
#include <vec2.h>
#include <dynarray.h>
#include <matrixc.h>
#include <aipathdb.h>
#include <aipthedg.h>
#include <aipthloc.h>

#include <aipthcst.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////////////////////////////////////////////////////////////////

static cMxsVector g_startPoint;
static tAIPathCellID g_startCell;
static cMxsVector g_endPoint;
static  tAIPathOkBits g_OkBits;

static float g_castLength;      // This actually includes our epsilon.
static cMxsVector g_castNorm;   // unit vector in direction of cast
static cMxsVector g_castPerp;   // 90 degrees clockwise of g_castNorm

static BOOL g_fIgnoreOBBs;

static IAIPathfindControl * g_pControl;

// our epsilon for growing the segments which represent our links
#define kEpsilon 0.0007

// helper for AIPathCast--finds a cell which intersects our cast
// returns:
#define kEndedInCell    0
#define kDeadEnd        -1


///////////////////////////////////////////////////////////////////////////////

static int FindNextCell(tAIPathCellID cellID, BOOL fAllowBlockedOBBs)
{
   // We're really only using the x and y components of these guys.
   const mxs_vector * pEdgeStart;
   const mxs_vector * pEdgeEnd; 

   cMxsVector edgeStartRel, edgeEndRel, interceptRel;
   tAIPathCellID nextCellID;
   const sAIPathCellLink *pLink;

   float xStart, xEnd, startRatio, endRatio, yIntercept;

   const sAIPathCell *pCell = g_AIPathDB.GetCell(cellID);
   int vertexCount = pCell->vertexCount;

   // Verify the point is in fact not just inside the cell. (toml 06-03-99)
   // @TBD (toml 06-04-99):  how much slower does this make things? Could store prevprev cell in outer function and test only in event of dead end
   if (IsPtInCellXYPlane(g_endPoint, pCell))
      return kEndedInCell;
   
   unsigned PathFlags;
   if (fAllowBlockedOBBs)
      PathFlags = kAIPF_Unpathable;
   else
      PathFlags = (kAIPF_Unpathable | kAIPF_BlockingOBB);

   int lastCell = pCell->firstCell + pCell->cellCount;
   for (int i = pCell->firstCell; i < lastCell; ++i) 
   {

      pLink = g_AIPathDB.GetCellLink(i);
      if (!AICanUseLink(cellID, pLink->dest, pLink->okBits, g_OkBits, PathFlags, g_pControl))
         continue;

      nextCellID = pLink->dest;

      // Does this edge lead to a cell we've already visited?
      if (g_AIPathDB.GetMark(nextCellID))
         continue;

      pEdgeStart = &g_AIPathDB.GetVertex(pLink->vertex_1);
      mx_mk_vec(&edgeStartRel,
                pEdgeStart->x - g_startPoint.x,
                pEdgeStart->y - g_startPoint.y, 0.0);
      xStart = edgeStartRel.x * g_castPerp.x + edgeStartRel.y * g_castPerp.y;

      // Now we have the x coordinates of our edge in cast space.  If
      // we're entirely to the left or the right of our cast we can
      // bail on this edge.
      if (xStart > 0.0)
         continue;

      pEdgeEnd = &g_AIPathDB.GetVertex(pLink->vertex_2);
      mx_mk_vec(&edgeEndRel,
                pEdgeEnd->x - g_startPoint.x,
                pEdgeEnd->y - g_startPoint.y, 0.0);
      xEnd = edgeEndRel.x * g_castPerp.x + edgeEndRel.y * g_castPerp.y;

      if (xEnd < 0.0)
         continue;

      // Our edge overlaps our cast on the relative x?  Ok.  Find the
      // y coordinate of our intersection, still in cast space.
      startRatio = xEnd / (xEnd - xStart);
      endRatio = 1.0 - startRatio;
      interceptRel.x = edgeStartRel.x * startRatio + edgeEndRel.x * endRatio;
      interceptRel.y = edgeStartRel.y * startRatio + edgeEndRel.y * endRatio;

      yIntercept = interceptRel.x * g_castNorm.x
                 + interceptRel.y * g_castNorm.y;

      // If we pass this test we know our ray intersects this edge.
      // Be mindful that g_castLength includes backing up significantly.
      if (yIntercept < -kEpsilon || yIntercept > g_castLength)
         continue;

      return nextCellID;
   }

   return kDeadEnd;
}


///////////////////////////////////////////////////////////////////////////////

// We assume that the cell IDs we're passed in are correct for the
// points, and perform no sanity checking.
BOOL AIPathcastVia(const mxs_vector & startPoint, tAIPathCellID startCell,
                   const mxs_vector & endPoint, 
                   tAIPathCellID viaCell,
                   tAIPathCellID * pEndCell,
                   tAIPathOkBits OkBits,
                   IAIPathfindControl * pControl)
{
   if (!startCell || !IsPtInCellXYPlane(startPoint, g_AIPathDB.GetCell(startCell)))
   {
      *pEndCell = startCell;
      return FALSE;
   }
   
   if (IsPtInCellXYPlane(endPoint, g_AIPathDB.GetCell(startCell)))
   {
      *pEndCell = startCell;
      return TRUE;
   }

   g_AIPathDB.MarkBegin();
   
   mxs_vector cast;
   BOOL rv;

   // copy our input into globals
   g_startPoint = startPoint;
   g_startCell = startCell;
   g_endPoint = endPoint;
   g_OkBits = OkBits;
   g_pControl = pControl;

   // We're strictly 2D today, thanks.
   g_startPoint.z = 0.0;
   g_endPoint.z = 0.0;

   mx_sub_vec(&cast, &g_endPoint, &g_startPoint);
   g_castLength = mx_norm_vec(&g_castNorm, &cast) + kEpsilon;
   mx_mk_vec(&g_castPerp, g_castNorm.y, -g_castNorm.x, 0.0);

   tAIPathCellID currentCell = startCell;
   tAIPathCellID prevCell = 0;
   
   BOOL fAllowBlockedOBBs = g_fIgnoreOBBs || g_AIPathDB.IsBlockedOBB(startCell);

#ifdef TEST_PATHCAST
   if (config_is_defined("AIPathCastSpew"))
      mprintf("AI Cast from (%g %g %G) to (%g %g %g)\n",
              startPoint.x, startPoint.y, startPoint.z,
              endPoint.x, endPoint.y, endPoint.z);
#endif // DBG_ON

   BOOL fViaSucceed = (viaCell) ? FALSE : TRUE;

   while (1) 
   {
#ifdef TEST_PATHCAST
      if (config_is_defined("AIPathCastSpew")) 
      {
         mprintf("%d ", currentCell);
         mxs_vector center = g_AIPathDB.GetCenter(currentCell);
         if (config_is_defined("AIPathCastSpewCenters"))
            mprintf("(%g %g %g)", center.x, center.y, center.z);
      }
#endif // DBG_ON

      g_AIPathDB.Mark(currentCell);

      if (currentCell == viaCell)
         fViaSucceed = TRUE;
         
      prevCell = currentCell;
      currentCell = FindNextCell(currentCell, fAllowBlockedOBBs);

      if (currentCell == kEndedInCell) 
      {
         *pEndCell = prevCell;
         rv = TRUE && fViaSucceed;
         break;
      }

      if (currentCell == kDeadEnd) 
      {
         *pEndCell = prevCell;
         rv = FALSE;
      
         break;
      }
      
      if (fAllowBlockedOBBs)
         fAllowBlockedOBBs = (g_fIgnoreOBBs || g_AIPathDB.IsBlockedOBB(currentCell));
   }

   g_AIPathDB.MarkEnd();

#ifdef TEST_PATHCAST
   if (config_is_defined("AIPathCastSpew"))
      mprintf("\n");
#endif // DBG_ON

   return rv;
}

///////////////////////////////////////////////////////////////////////////////

BOOL AIPathcast(const mxs_vector & startPoint, tAIPathCellID startCell,
                const mxs_vector & endPoint, 
                tAIPathCellID * pEndCell,
                tAIPathOkBits OkBits,
                IAIPathfindControl * pControl)
{
   return AIPathcastVia(startPoint, startCell, endPoint, 0, pEndCell, OkBits, pControl);
}

///////////////////////////////////////////////////////////////////////////////

void AISetPathcastIgnoreBlockingOBBs(BOOL b)
{
   g_fIgnoreOBBs = b;
}

///////////////////////////////////////////////////////////////////////////////

BOOL AIIsEdgeVisible(const mxs_vector & startLocation, tAIPathCellID startCell,
                     const cAIPathEdge & edge, 
                     tAIPathOkBits fOkBits,
                     IAIPathfindControl * pControl)
{
   if (!startCell)
      return FALSE;

   cMxsVector testPt;
   
   // If the edge in question is linked directly from the start cell, we assume
   // it is fully visible (toml 06-03-99)
   if (startCell == edge.cell)
      return TRUE;

   #define kCloseEnough sq(1.0)
   if (Vec2PointSegmentSquared((Vec2 *)&startLocation, (Vec2 *)(&g_AIPathDB.GetVertex(edge.vertexA)),  (Vec2 *)(&g_AIPathDB.GetVertex(edge.vertexB))) < kCloseEnough)
      return TRUE;

   edge.GetCenter(&testPt);
   
   tAIPathCellID endCell;
   
   if (!AIPathcastVia(startLocation, startCell, testPt, edge.cell, &endCell, fOkBits, pControl))
      return FALSE;
      
//   if (endCell != startCell && endCell != edge.cell)
//      return FALSE;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
