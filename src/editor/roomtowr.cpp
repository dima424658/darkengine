/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/roomtowr.cpp,v 1.4 1999/12/09 18:16:26 MAT Exp $

/* --=<= --/-/-/-/-/-/-/ <(< (( ((( /^^\ ))) )) >)> \-\-\-\-\-\-\-- =>=-- *\
   roomtowr.cpp

   modifying our freshly-generated world rep according to properties
   on room brushes

\* --=<= --\-\-\-\-\-\-\ <(< (( ((( \vv/ ))) )) >)> /-/-/-/-/-/-/-- =>=-- */


#include <mprintf.h>

#include <wrtype.h>
#include <wrfunc.h>
#include <bspsphr.h>
#include <room.h>
#include <rooms.h>

#include <roomtowr.h>

#include <dbmem.h>


/* --=<= --/-/-/-/-/-/-/ <(< (( ((( /^^\ ))) )) >)> \-\-\-\-\-\-\-- =>=-- *\

   data types

\* --=<= --\-\-\-\-\-\-\ <(< (( ((( \vv/ ))) )) >)> /-/-/-/-/-/-/-- =>=-- */

struct sCallbackEntry
{
   sCallbackEntry(tRoomToWRCallback pfnCallback, void *pData)
      : m_pfnCallback(pfnCallback),
        m_pData(pData)
   {
   }

   tRoomToWRCallback m_pfnCallback;
   void *m_pData;
};


/* --=<= --/-/-/-/-/-/-/ <(< (( ((( /^^\ ))) )) >)> \-\-\-\-\-\-\-- =>=-- *\

   constants

\* --=<= --\-\-\-\-\-\-\ <(< (( ((( \vv/ ))) )) >)> /-/-/-/-/-/-/-- =>=-- */

#define kToRightOfEdgeEpsilon .01


/* --=<= --/-/-/-/-/-/-/ <(< (( ((( /^^\ ))) )) >)> \-\-\-\-\-\-\-- =>=-- *\

   internal globals--these only mean anything while we're in RoomToWR

\* --=<= --\-\-\-\-\-\-\ <(< (( ((( \vv/ ))) )) >)> /-/-/-/-/-/-/-- =>=-- */

static cDynArray<sCallbackEntry> g_Callbacks;
static cDynArray<tRoomToWRStartCallback> g_StartCallbacks;
static cDynArray<tRoomToWREndCallback> g_EndCallbacks;
static cDynArray<int> g_CellList;
static uchar g_aaVertexOffset[6][4]
= {
   {0, 4, 6, 2},        // x
   {1, 3, 7, 5},
   {0, 1, 5, 4},        // y
   {2, 6, 7, 3},
   {0, 2, 3, 1},        // z
   {4, 5, 7, 6},
};
static cRoom *g_pRoom;
static mxs_vector g_RoomCenter;
static mxs_vector g_RoomDims;
static float g_fRoomRadius;
static Location g_RoomLocation;
static mxs_plane g_aRoomPlane[6];
static mxs_vector g_aRoomVertexPool[8];


/* --=<= --/-/-/-/-/-/-/ <(< (( ((( /^^\ ))) )) >)> \-\-\-\-\-\-\-- =>=-- *\

   weird little helper functions

\* --=<= --\-\-\-\-\-\-\ <(< (( ((( \vv/ ))) )) >)> /-/-/-/-/-/-/-- =>=-- */

// Does edge p1p2 cross the plane?  If so, stuff pIntersection with
// the point and return TRUE.
static inline BOOL EdgePolyIntersection(mxs_vector *p1, mxs_vector *p2,
                                        mxs_vector *pPlaneNorm,
                                        float fPlaneConstant,
                                        mxs_vector *pIntersection)
{
   float fDist1 = mx_dot_vec(p1, pPlaneNorm) + fPlaneConstant;
   float fDist2 = mx_dot_vec(p2, pPlaneNorm) + fPlaneConstant;

   // Are both endpoints on the same side of the plane?
   if ((fDist1 > 0 && fDist2 > 0) || (fDist1 < 0 && fDist2 < 0))
      return FALSE;

   float fTime = fDist1 / (fDist1 - fDist2);
   mx_interpolate_vec(pIntersection, p1, p2, fTime);
   return TRUE;
}


// Is this point to the right of this edge, in a plane with this norm?
static inline BOOL ToRightOfEdge(mxs_vector *pPoint, mxs_vector *pE1,
                                mxs_vector *pE2, mxs_vector *pPlaneNorm)
{
   mxs_vector EdgeToPoint;
   mx_sub_vec(&EdgeToPoint, pPoint, pE1);
   mx_normeq_vec(&EdgeToPoint);

   mxs_vector Edge, Perp;
   mx_sub_vec(&Edge, pE2, pE1);
   mx_cross_vec(&Perp, &Edge, pPlaneNorm);
   mx_normeq_vec(&Perp);

   // Now we have two unit vectors, one perpendicular to the edge and
   // the other pointing from the first vertex of the edge to our
   // point.  How close a match are they?
   if (mx_dot_vec(&EdgeToPoint, &Perp) > -kToRightOfEdgeEpsilon)
      return TRUE;
   else
      return FALSE;
}


// Given polygon A and polygon B, does any edge of polygon A pass
// through polygon B?  We don't catch cases where they are coplanar.
static BOOL PolyEdgeTest(uchar *pAVert, mxs_vector *pAPool, int iANumEdges,
                         mxs_vector *pAPlaneNorm, float fAPlaneConstant,
                         uchar *pBVert, mxs_vector *pBPool, int iBNumEdges,
                         mxs_vector *pBPlaneNorm, float fBPlaneConstant)
{
   // iterate over all edges in polygon A
   for (int i = 0; i < iANumEdges; ++i) {
      mxs_vector Intersection;

      // Does this edge intersect the plane of polygon B?
      if (EdgePolyIntersection(&pAPool[pAVert[i]],
                               &pAPool[pAVert[(i + 1) & iANumEdges]],
                               pBPlaneNorm, fBPlaneConstant, &Intersection)) {

         // If all of the edges of polygon B are to the right of our
         // intersection point, or all are to the left (because the
         // polygon is wound the other way), then the intersection is
         // inside polygon B and our polygons intersect.

         // The first edge goes from the last point in the poly to the
         // first.
         BOOL bToRight = ToRightOfEdge(&Intersection,
                                       &pBPool[pBVert[iBNumEdges - 1]],
                                       &pBPool[pBVert[0]], pBPlaneNorm);

         // Here's all the other edges.
         for (int j = 0; j < (iBNumEdges - 1); ++j) {
            if (bToRight != ToRightOfEdge(&Intersection, &pBPool[pBVert[j]],
                                        &pBPool[pBVert[j + 1]], pBPlaneNorm)) {
               break;
            }
         }

         if (j == (iBNumEdges - 1))
            return TRUE;
      }
   }

   return FALSE;
}


static BOOL PolygonsIntersect(uchar *pAVert, mxs_vector *pAPool,int iANumEdges,
                              mxs_vector *pAPlaneNorm, float fAPlaneConstant,
                              uchar *pBVert, mxs_vector *pBPool,int iBNumEdges,
                              mxs_vector *pBPlaneNorm, float fBPlaneConstant)
{
   if (PolyEdgeTest(pAVert, pAPool, iANumEdges, pAPlaneNorm, fAPlaneConstant,
                    pBVert, pBPool, iBNumEdges, pBPlaneNorm, fBPlaneConstant))
      return TRUE;

   if (PolyEdgeTest(pBVert, pBPool, iBNumEdges, pBPlaneNorm, fBPlaneConstant,
                    pAVert, pAPool, iANumEdges, pAPlaneNorm, fAPlaneConstant))
      return TRUE;

   return FALSE;
}


// set up info about room: center, planes, vertices
static void SetGlobalsForRoom(cRoom *pRoom)
{
   g_pRoom = pRoom;
   g_RoomCenter = pRoom->GetCenterPt();
   MakeLocationFromVector(&g_RoomLocation, &g_RoomCenter);
   g_RoomDims = pRoom->GetSize();
   g_fRoomRadius = mx_mag_vec(&g_RoomDims);

   int i;
   for (i = 0; i < 6; ++i) {
      tPlane P = pRoom->GetPlane(i);

      g_aRoomPlane[i].v = P.normal;
      g_aRoomPlane[i].d = P.d;
   }

   mxs_vector aAxis[3];
   for (i = 0; i < 3; ++i)
      mx_scale_vec(&aAxis[i], &g_aRoomPlane[i].v, g_RoomDims.el[i]);

   for (i = 0; i < 8; ++i) {
      mx_copy_vec(&g_aRoomVertexPool[i], &g_RoomCenter);
      mx_scale_addeq_vec(&g_aRoomVertexPool[i], &aAxis[0], (i&1)? -1 : 1);
      mx_scale_addeq_vec(&g_aRoomVertexPool[i], &aAxis[1], (i&2)? -1 : 1);
      mx_scale_addeq_vec(&g_aRoomVertexPool[i], &aAxis[2], (i&4)? -1 : 1);
   }
}


// Does the current room intersect this world rep cell?
static BOOL RoomIntersectsCell(int iCellIndex)
{
   PortalCell *pCell = WR_CELL(iCellIndex);

   // If either of these guys contains a point inside the other, they
   // intersect and we're done.  This is a little goofy since the room
   // system and Portal have different epsilons for their tests.
   if (g_pRoom->PointInside(pCell->sphere_center))
      return TRUE;
   if (PortalTestInCell(iCellIndex, &g_RoomLocation))
      return TRUE;

   // Quick rejection?
   float fDist = mx_dist_vec(&pCell->sphere_center, &g_RoomCenter);
   if (fDist > (g_fRoomRadius + pCell->sphere_radius))
      return FALSE;

   // iterate over polygons of cell and faces of room and compare
   // everything to everything exhaustingly
   PortalPolygonCore *pPoly = pCell->poly_list;
   int iCellVertexIndex = 0;

   int i, j;
   for (i = 0; i < pCell->num_polys; ++i) {
      for (j = 0; j < 6; ++j) {
         if (PolygonsIntersect(
                // poly from cell
                pCell->vertex_list + iCellVertexIndex, pCell->vpool,
                pPoly->num_vertices, &pCell->plane_list[pPoly->planeid].normal,
                pCell->plane_list[pPoly->planeid].plane_constant,

                // poly from room
                g_aaVertexOffset[j], g_aRoomVertexPool,
                4, &g_aRoomPlane[j].v,
                g_aRoomPlane[j].d))
            return TRUE;
      }
      iCellVertexIndex += pPoly->num_vertices;
      ++pPoly;
   }

   return FALSE;
}


// There are two steps to finding which world rep cells intersect a
// given room.  First, we use the BSP-sphere intersector for a rough
// cut.  Then we check each cell against the room individually.
static void SetCellsIntersectingRoom(cRoom *pRoom)
{
   int i;

   // find all cells intersecting the bounding sphere of our room
   g_CellList.SetSize(BSPSPHR_OUTPUT_LIMIT);

   SetGlobalsForRoom(pRoom);
   g_CellList.SetSize(portal_cells_intersecting_sphere(&g_RoomLocation,
                             g_fRoomRadius, g_CellList.AsPointer()));

   if (!g_CellList.Size() || g_CellList.Size() == (BSPSPHR_OUTPUT_LIMIT - 1)) {
      // use all cells
      g_CellList.SetSize(wr_num_cells);
      for (i = 0; i < wr_num_cells; ++i)
         g_CellList[i] = i;
   }

   // There may be cells which intersect the sphere, but not the
   // room.  For that matter, we may have cells which don't even
   // intersect the sphere, since the intersection is done
   // quick-and-dirty style.  So we iterate over the list and knock
   // out any cells which aren't really intersecting our room.
   for (i = 0; i < g_CellList.Size(); ++i) {
      if (!RoomIntersectsCell(g_CellList[i])) {
         g_CellList.FastDeleteItem(i);
         --i;
      }
   }
}


/* --=<= --/-/-/-/-/-/-/ <(< (( ((( /^^\ ))) )) >)> \-\-\-\-\-\-\-- =>=-- *\

   weird little exposed functions

\* --=<= --\-\-\-\-\-\-\ <(< (( ((( \vv/ ))) )) >)> /-/-/-/-/-/-/-- =>=-- */

void RoomToWRAddCallback(tRoomToWRCallback pfnCallback, void *pData)
{
   for (int i = 0; i < g_Callbacks.Size(); ++i)
      if (g_Callbacks[i].m_pfnCallback == pfnCallback
       && g_Callbacks[i].m_pData == pData) {
         Warning(("RoomToWR: Callback already added.  Leave me alone.\n"));
         return;
      }

   g_Callbacks.Append(sCallbackEntry(pfnCallback, pData));
}


void RoomToWRAddStartCallback(tRoomToWRStartCallback pfnCallback)
{
   for (int i = 0; i < g_StartCallbacks.Size(); ++i)
      if (g_StartCallbacks[i] == pfnCallback) {
         Warning(("RoomToWR: Start callback already added.\n"));
         return;
      }

   g_StartCallbacks.Append(pfnCallback);
}


void RoomToWRAddEndCallback(tRoomToWREndCallback pfnCallback)
{
   for (int i = 0; i < g_EndCallbacks.Size(); ++i)
      if (g_EndCallbacks[i] == pfnCallback) {
         Warning(("RoomToWR: End callback already added.\n"));
         return;
      }

   g_EndCallbacks.Append(pfnCallback);
}


// For every room, we find out what WR cells intersect it and call out
// to whatever clients are interested.
void RoomToWRProcess()
{
   int i, iCell, iCB;

   for (iCB = 0; iCB < g_StartCallbacks.Size(); ++iCB)
      g_StartCallbacks[iCB]();

   int iNumRooms = g_pRooms->GetNumRooms();
   for (i = 0; i < iNumRooms; ++i) {
      cRoom *pRoom = g_pRooms->GetRoom(i);
      if (pRoom->GetObjID() == OBJ_NULL)
         continue;

      SetCellsIntersectingRoom(pRoom);

      for (iCell = 0; iCell < g_CellList.Size(); ++iCell)
         for (iCB = 0; iCB < g_Callbacks.Size(); ++iCB)
            g_Callbacks[iCB].m_pfnCallback(g_CellList[iCell], pRoom,
                                           g_Callbacks[iCB].m_pData);
   }

   g_CellList.SetSize(0);

   for (iCB = 0; iCB < g_EndCallbacks.Size(); ++iCB)
      g_EndCallbacks[iCB]();
}
