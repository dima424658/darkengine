/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/rportal.h,v 1.4 2000/01/31 10:00:45 adurant Exp $
//
// cRoomPortal header
//
#pragma once

#ifndef __RPORTAL_H
#define __RPORTAL_H

#include <matrixs.h>
#include <dynarray.h>

#include <roombase.h>
#include <room.h>

class cRoomPortal
{
public:

   /////////////////
   //
   // Constructor / Destructor
   //
   cRoomPortal(tPlane &portalPlane, tPlane edgePlaneList[], 
               int numEdgePlanes, mxs_vector &centerPt,
               cRoom *nearRoom, cRoom *farRoom);
   cRoomPortal(RoomReadWrite movefunc);
   ~cRoomPortal(void);      

   void Write(RoomReadWrite movefunc);

   /////////////////
   //
   // Accessors
   //
   const tPlane &GetPlane(void) const;
   
   int           GetNumEdges(void) const;
   const tPlane &GetEdgePlane(int planeNum) const;
   
   cRoom  *GetFarRoom(void) const;
   cRoom  *GetNearRoom(void) const;

   const mxs_vector &GetCenter(void) const;

   void SetPortalID(int portalID);
   int  GetPortalID(void) const;

   void SetIndex(int index);
   int  GetIndex(void) const;

   void         SetFarPortal(cRoomPortal *pRPortal);
   cRoomPortal *GetFarPortal() const;

   /////////////////
   //
   // Raycasting and projecting
   //
   BOOL Raycast(mxs_vector &start, mxs_vector &dir) const;
   BOOL GetRaycastProj(mxs_vector &start, mxs_vector &dir, mxs_vector *point) const;

private:
   
   int  m_PortalID;
   int  m_Index;

   tPlane             m_PortalPlane;
   cDynArray<tPlane>  m_EdgePlane;

   int  m_FarRoomID;
   int  m_NearRoomID;

   mxs_vector m_CenterPt;

   int  m_FarPortalID;
};

////////////////////////////////////////////////////////////////////////////////

inline const tPlane &cRoomPortal::GetPlane(void) const
{
   return m_PortalPlane;
}

////////////////////////////////////////

inline int cRoomPortal::GetNumEdges(void) const
{
   return m_EdgePlane.Size();
}

////////////////////////////////////////

inline const tPlane &cRoomPortal::GetEdgePlane(int planeNum) const
{
   AssertMsg2(planeNum < m_EdgePlane.Size(), "Attempt to get edge plane 0 of 0\n", planeNum, m_EdgePlane.Size());
   return m_EdgePlane[planeNum];
}

////////////////////////////////////////

inline const mxs_vector &cRoomPortal::GetCenter(void) const
{
   return m_CenterPt;
}

////////////////////////////////////////

inline void cRoomPortal::SetPortalID(int portalID)
{
   m_PortalID = portalID;
}

////////////////////////////////////////

inline int cRoomPortal::GetPortalID(void) const
{
   return m_PortalID;
}

////////////////////////////////////////

inline void cRoomPortal::SetIndex(int index)
{
   m_Index = index;
}

////////////////////////////////////////


inline int cRoomPortal::GetIndex(void) const
{
   return m_Index;
}

////////////////////////////////////////

inline void cRoomPortal::SetFarPortal(cRoomPortal *pRPortal)
{
   m_FarPortalID = pRPortal->GetPortalID();
}

////////////////////////////////////////

#endif

