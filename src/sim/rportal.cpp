/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/rportal.cpp,v 1.8 1998/10/08 17:42:53 CCAROLLO Exp $
//
// cRoomPortal functions
//

#include <lg.h>

#include <math.h>

#include <matrixs.h>

#include <roomsys.h>
#include <rooms.h>
#include <rportal.h>
#include <roomutil.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////////////////////////

cRoomPortal::cRoomPortal(tPlane &portalPlane, tPlane edgePlaneList[],
                         int numEdgePlanes, mxs_vector &centerPt,
                         cRoom *nearRoom, cRoom *farRoom)
 : m_PortalPlane(portalPlane),
   m_CenterPt(centerPt)
{
   AssertMsg(nearRoom != NULL, "Invalid portal Near Room");
   AssertMsg(farRoom  != NULL, "Invalid portal Far Room");

   m_NearRoomID = nearRoom->GetRoomID();
   m_FarRoomID = farRoom->GetRoomID();

   m_EdgePlane.SetSize(numEdgePlanes);

   for (int i=0; i<numEdgePlanes; i++)
      m_EdgePlane[i] = edgePlaneList[i];
}

////////////////////////////////////////

cRoomPortal::cRoomPortal(RoomReadWrite movefunc)
{
   tPlane *pPlane;
   int size, i;

   movefunc(&m_PortalID, sizeof(int), 1);
   movefunc(&m_Index, sizeof(int), 1);

   movefunc(&m_PortalPlane, sizeof(tPlane), 1);

   movefunc(&size, sizeof(int), 1);
   m_EdgePlane.SetSize(size);

   for (i=0; i<size; i++)
   {
      pPlane = &m_EdgePlane[i];
      movefunc(pPlane, sizeof(tPlane), 1);
   }

   movefunc(&m_FarRoomID, sizeof(int), 1);
   movefunc(&m_NearRoomID, sizeof(int), 1);

   movefunc(&m_CenterPt, sizeof(mxs_vector), 1);

   movefunc(&m_FarPortalID, sizeof(int), 1);
}

////////////////////////////////////////

cRoomPortal::~cRoomPortal(void)
{
   m_EdgePlane.SetSize(0);
}

////////////////////////////////////////

void cRoomPortal::Write(RoomReadWrite movefunc)
{
   tPlane *pPlane;
   int size;
   int i;

   movefunc(&m_PortalID, sizeof(int), 1);
   movefunc(&m_Index, sizeof(int), 1);

   movefunc(&m_PortalPlane, sizeof(tPlane), 1);

   size = m_EdgePlane.Size();
   movefunc(&size, sizeof(int), 1);

   for (i=0; i<size; i++)
   {
      pPlane = &m_EdgePlane[i];
      movefunc(pPlane, sizeof(tPlane), 1);
   }

   movefunc(&m_FarRoomID, sizeof(int), 1);
   movefunc(&m_NearRoomID, sizeof(int), 1);

   movefunc(&m_CenterPt, sizeof(mxs_vector), 1);

   movefunc(&m_FarPortalID, sizeof(int), 1);
}

////////////////////////////////////////////////////////////////////////////////

cRoom *cRoomPortal::GetFarRoom(void) const
{
   return g_pRooms->GetRoom(m_FarRoomID);
}

////////////////////////////////////////

cRoom *cRoomPortal::GetNearRoom(void) const
{
   return g_pRooms->GetRoom(m_NearRoomID);
}

////////////////////////////////////////

cRoomPortal *cRoomPortal::GetFarPortal(void) const
{
   return g_pRooms->GetPortal(m_FarPortalID);
}

////////////////////////////////////////////////////////////////////////////////

BOOL cRoomPortal::Raycast(mxs_vector &start, mxs_vector &dir) const
{
   mxs_vector int_pt;

   // Find the intersection point
   if (!RayPlaneIntersection(m_PortalPlane, start, dir, &int_pt))
      return FALSE;

   // Check that it's "inside" each edge plane
   for (int i=0; i<m_EdgePlane.Size(); i++)
   {
      if (PointPlaneDist(m_EdgePlane[i], int_pt) > ON_PLANE_EPSILON)
         return FALSE;
   }

   return TRUE;
}

////////////////////////////////////////

BOOL cRoomPortal::GetRaycastProj(mxs_vector &start, mxs_vector &dir, mxs_vector *point) const
{
   mxs_real   pt_dist;

   // Find the intersection point of the ray and the plane
   if (!RayPlaneIntersection(m_PortalPlane, start, dir, point))
      return FALSE;

   // If it's "outside" any portals, project it onto it
   for (int i=0; i<m_EdgePlane.Size(); i++)
   {
      pt_dist = PointPlaneDist(m_EdgePlane[i], *point);
      if (pt_dist > ON_PLANE_EPSILON)
         mx_scale_addeq_vec(point, (mxs_vector *)&m_EdgePlane[i].normal, -pt_dist);
   }

   return TRUE;
}








