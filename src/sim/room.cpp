/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/room.cpp,v 1.5 1998/06/22 16:08:22 CCAROLLO Exp $
//
// cRoom functions
//

#include <lg.h>

#include <math.h>
#include <matrix.h>

#include <roomsys.h>
#include <rooms.h>
#include <room.h>
#include <rportal.h>
#include <roomutil.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>


////////////////////////////////////////

cRoom::cRoom(const cStr &name, ObjID objID, tPlane planeList[], mxs_vector &center)
 : m_objID(objID)
{
   mx_copy_vec(&m_Center, &center);

   for (int i=0; i<6; i++)
   {
      mx_copy_vec(&m_Plane[i].normal, &planeList[i].normal);
      m_Plane[i].d = planeList[i].d;
   }

   m_PortalList.SetSize(0);
   m_PortalDist.SetSize(0);

   m_WatchList.SetSize(g_pRooms->GetNumWatches());
}

////////////////////////////////////////

cRoom::cRoom(RoomReadWrite movefunc)
{
   cRoomPortal *pRPortal;
   int   size, size2;
   int   i, j;
   int   val;
   float fval;

   movefunc(&m_objID, sizeof(ObjID), 1);
   movefunc(&m_roomID, sizeof(short), 1);

   movefunc(&m_Center, sizeof(mxs_vector), 1);

   for (i=0; i<6; i++)
      movefunc(&m_Plane[i], sizeof(tPlane), 1);

   movefunc(&size, sizeof(int), 1);

   for (i=0; i<size; i++)
   {
      pRPortal = new cRoomPortal(movefunc);
      AddPortal(pRPortal);
      g_pRooms->AddPortal(pRPortal, pRPortal->GetPortalID());
   }

   for (i=0; i<size; i++)
   {
      for (j=0; j<size; j++)
      {
         movefunc(&fval, sizeof(float), 1);
         m_PortalDist[i][j] = fval;
      }
   }

   movefunc(&size, sizeof(int), 1);
   m_WatchList.SetSize(size);

   for (i=0; i<size; i++)
   {
      movefunc(&size2, sizeof(int), 1);
      m_WatchList[i].SetSize(size2);

      for (j=0; j<size2; j++)
      {
         movefunc(&val, sizeof(int), 1);
         m_WatchList[i][j] = val;
      }
   }
}

////////////////////////////////////////

cRoom::~cRoom(void)
{
   int i;

   for (i=0; i<m_PortalList.Size(); i++)
   {
      delete m_PortalList[i];
      m_PortalDist[i].SetSize(0);
   }

   for (i=0; i<g_pRooms->GetNumWatches(); i++)
      m_WatchList[i].SetSize(0);

   m_PortalList.SetSize(0);
   m_PortalDist.SetSize(0);

   m_WatchList.SetSize(0);
}


////////////////////////////////////////

void cRoom::Write(RoomReadWrite movefunc)
{
   int size, size2;
   int i, j;
   int   val;
   float fval;

   movefunc(&m_objID, sizeof(ObjID), 1);
   movefunc(&m_roomID, sizeof(short), 1);

   movefunc(&m_Center, sizeof(mxs_vector), 1);

   for (i=0; i<6; i++)
      movefunc(&m_Plane[i], sizeof(tPlane), 1);

   size = m_PortalList.Size();
   movefunc(&size, sizeof(int), 1);

   for (i=0; i<size; i++)
      m_PortalList[i]->Write(movefunc);

   for (i=0; i<size; i++)
   {
      for (j=0; j<size; j++)
      {
         fval = m_PortalDist[i][j];
         movefunc(&fval, sizeof(float), 1);
      }
   }

   size = m_WatchList.Size();
   movefunc(&size, sizeof(int), 1);

   for (i=0; i<size; i++)
   {
      size2 = m_WatchList[i].Size();
      movefunc(&size2, sizeof(int), 1);

      for (j=0; j<size2; j++)
      {
         val = m_WatchList[i][j];
         movefunc(&val, sizeof(int), 1);
      }
   }
}

////////////////////////////////////////

const mxs_vector cRoom::GetSize() const
{
   mxs_vector size;

   for (int i=0; i<3; i++)
      size.el[i] = -(mx_dot_vec(&m_Plane[i].normal, &m_Center) + m_Plane[i].d);

   return size;
}

////////////////////////////////////////

void cRoom::AddPortal(cRoomPortal *newPortal)
{
   int  end_index;
   int  i;

   end_index = m_PortalList.Size();

   // Grow the number of elements in the grid, and the length of each element
   m_PortalDist.SetSize(end_index + 1);
   for (i=0; i<end_index + 1; i++)
      m_PortalDist[i].SetSize(end_index + 1);

   // Set each element's dist to new portal, and new portal's dist to them
   for (i=0; i<end_index; i++)
   {
      m_PortalDist[i][end_index] = mx_dist_vec((mxs_vector *)&GetPortalCenter(i), (mxs_vector *)&newPortal->GetCenter());
      m_PortalDist[end_index][i] = m_PortalDist[i][end_index];
   }
   m_PortalDist[end_index][end_index] = 0.0;

   // And add to main list
   newPortal->SetIndex(m_PortalList.Size());

   m_PortalList.Append(newPortal);
}

////////////////////////////////////////

BOOL cRoom::PointInside(const mxs_vector &pt) const
{
   int i;
   for (i=0; i<6; i++)
   {
      if (PointPlaneDist(m_Plane[i], pt) > ON_PLANE_EPSILON)
         break;
   }
   return (i == 6);
}

BOOL cRoom::LineIntersect(const mxs_vector &from, const mxs_vector &to) const
{
   int farthest_front_index = -1;
   int nearest_back_index = -1;
   mxs_real farthest_front, nearest_back;
   mxs_real from_dist, to_dist;
   mxs_real loc;

   for (int i=0; i<6; i++)
   {
      from_dist = PointPlaneDist(m_Plane[i], from);
      to_dist = PointPlaneDist(m_Plane[i], to);

      // If both points are in front of the plane, ignore the whole thing
      if ((from_dist > ON_PLANE_EPSILON) && (to_dist > ON_PLANE_EPSILON))
         return FALSE;

      // If both points are behind, ingore just this plane
      if ((from_dist < ON_PLANE_EPSILON) && (to_dist < ON_PLANE_EPSILON))
         continue;

      loc = fabs(from_dist / (from_dist - to_dist));

      if (from_dist > 0)
      {
         if ((farthest_front_index == -1) || (loc > farthest_front))
         {
            farthest_front_index = i;
            farthest_front = loc;
         }
      } 
      else
      {
         if ((nearest_back_index == -1) || (loc < nearest_back))
         {
            nearest_back_index = i;
            nearest_back = loc;
         }
      }
   }

   return ((farthest_front_index == -1) || (nearest_back_index == -1) || (nearest_back > farthest_front));
}

////////////////////////////////////////////////////////////////////////////////

void cRoom::AddWatch(int handle)
{
   AssertMsg2(handle <= m_WatchList.Size(), "Attempt to grow watch list from %d to %d", handle, m_WatchList.Size());

   if (handle == m_WatchList.Size())
      m_WatchList.SetSize(handle + 1);
   m_WatchList[handle].SetSize(0);
}

////////////////////////////////////////////////////////////////////////////////

const tPlane &cRoom::GetPortalPlane(int portalNum) const
{
   AssertValidPortal(portalNum, m_PortalList.Size());
   return m_PortalList[portalNum]->GetPlane();
}

////////////////////////////////////////

int cRoom::GetPortalNumEdges(int portalNum) const 
{
   AssertValidPortal(portalNum, m_PortalList.Size());
   return m_PortalList[portalNum]->GetNumEdges();
}   

////////////////////////////////////////

const tPlane &cRoom::GetPortalEdge(int portalNum, int planeNum) const
{
   AssertValidPortal(portalNum, m_PortalList.Size());
   return m_PortalList[portalNum]->GetEdgePlane(planeNum);
}

////////////////////////////////////////

cRoom *cRoom::GetPortalFarRoom(int portalNum) const
{
   AssertValidPortal(portalNum, m_PortalList.Size());
   return m_PortalList[portalNum]->GetFarRoom();
}

////////////////////////////////////////

cRoom *cRoom::GetPortalNearRoom(int portalNum) const
{
   AssertValidPortal(portalNum, m_PortalList.Size());
   return m_PortalList[portalNum]->GetNearRoom();
}

////////////////////////////////////////

const mxs_vector &cRoom::GetPortalCenter(int portalNum) const
{
   AssertValidPortal(portalNum, m_PortalList.Size());
   return m_PortalList[portalNum]->GetCenter();
}

////////////////////////////////////////

BOOL cRoom::PortalRaycast(int portalNum, mxs_vector &start, mxs_vector &dir) const
{
   AssertValidPortal(portalNum, m_PortalList.Size());
   return m_PortalList[portalNum]->Raycast(start, dir);
}

////////////////////////////////////////

BOOL cRoom::PortalGetRaycastProj(int portalNum, mxs_vector &start, mxs_vector &dir, mxs_vector *point) const
{
   AssertValidPortal(portalNum, m_PortalList.Size());
   return m_PortalList[portalNum]->GetRaycastProj(start, dir, point);
}

////////////////////////////////////////////////////////////////////////////////


