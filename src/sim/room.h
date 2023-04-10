/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/room.h,v 1.12 2000/03/18 16:25:08 adurant Exp $
//
// cRoom header
//
#pragma once

#ifndef __ROOM_H
#define __ROOM_H

#include <str.h>
#include <dynarray.h>
#include <dlist.h>

#include <label.h>
#include <objtype.h>
#include <iobjsys.h>
#include <comtools.h>
#include <appagg.h>

#include <matrixs.h>

#include <roombase.h>
#include <rportal.h>
#include <roomprop.h>

#include <objedit.h> //objwarnname

#define AssertValidPortal(pn, ps)  AssertMsg2(pn < ps, "Attempt to access portal 0 of 0", pn + 1, ps)

typedef cDynArray<mxs_real> cDynArrayReal;
typedef cDynArray<ObjID>    cDynArrayObj;

class cRoom
{
public:

   /////////////////
   //
   // Constructor / Destructor
   //
   cRoom(const cStr &name, ObjID objID, tPlane planeList[], mxs_vector &center);
   cRoom(RoomReadWrite movefunc);
   ~cRoom(void);

   void Write(RoomReadWrite movefunc);

   /////////////////
   //
   // Accessors
   //
   void               SetName(const char *name);
   const char        *GetName() const;

   ObjID              GetObjID() const;

   void               SetRoomID(short roomID);
   short              GetRoomID() const;

   void               SetCenterPt(const mxs_vector &center);
   const mxs_vector  &GetCenterPt() const;

   const tPlane      &GetPlane(int planeNum) const;

   const mxs_vector   GetSize() const;

   int                GetNumPortals() const;
   cRoomPortal       *GetPortal(int portalNum) const;

   void               AddPortal(cRoomPortal *newPortal);

   mxs_real           GetPortalDist(int portalOne, int portalTwo) const;

   BOOL               PointInside(const mxs_vector &pt) const;
   BOOL               LineIntersect(const mxs_vector &from, const mxs_vector &to) const;

   /////////////////
   //
   // Watches
   //
   void   AddWatch(int handle);
   void   RemoveWatch(int handle);

   void   AddWatchElement(int handle, ObjID objID);
   void   RemoveWatchElement(int handle, int index);
   void   RemoveAllWatchElements(int handle);

   void   GetObjList(const ObjID **objList, int *size, int handle = kAllObjWatchHandle) const;

   /////////////////
   //
   // Accessors to cRoomPortal, for convenience
   //
   const tPlane &GetPortalPlane(int portalNum) const;

   int           GetPortalNumEdges(int portalNum) const;
   const tPlane &GetPortalEdge(int portalNum, int planeNum) const;

   cRoom  *GetPortalFarRoom(int portalNum) const;
   cRoom  *GetPortalNearRoom(int portalNum) const;

   const mxs_vector &GetPortalCenter(int portalNum) const;

   BOOL  PortalRaycast(int portalNum, mxs_vector &start, mxs_vector &dir) const;
   BOOL  PortalGetRaycastProj(int portalNum, mxs_vector &start, mxs_vector &dir, mxs_vector *point) const;


private:
   ObjID m_objID;
   short m_roomID;

   mxs_vector m_Center;

   tPlane m_Plane[6];

   cDynArray<cRoomPortal *>       m_PortalList;
   cDynClassArray<cDynArrayReal>  m_PortalDist;

   cDynClassArray<cDynArrayObj>   m_WatchList;
};

////////////////////////////////////////////////////////////////////////////////

inline void cRoom::SetName(const char *name)
{
   AutoAppIPtr(ObjectSystem);
   pObjectSystem->NameObject(GetObjID(), name);
}

////////////////////////////////////////

inline const char *cRoom::GetName(void) const
{
   AutoAppIPtr(ObjectSystem);
 
   return pObjectSystem->GetName(GetObjID());
}

////////////////////////////////////////

inline ObjID cRoom::GetObjID(void) const
{
   return m_objID;
}

////////////////////////////////////////

inline void cRoom::SetRoomID(short roomID)
{
   m_roomID = roomID;
}

////////////////////////////////////////

inline short cRoom::GetRoomID() const
{
   return m_roomID;
}

////////////////////////////////////////

inline void cRoom::SetCenterPt(const mxs_vector &center)
{
   mx_copy_vec(&m_Center, (mxs_vector *)&center);
}

////////////////////////////////////////

inline const mxs_vector &cRoom::GetCenterPt(void) const
{
   return m_Center;
}

////////////////////////////////////////

inline const tPlane &cRoom::GetPlane(int planeNum) const
{
   return m_Plane[planeNum];
}

////////////////////////////////////////

inline int cRoom::GetNumPortals(void) const
{
   return m_PortalList.Size();
}

////////////////////////////////////////

inline cRoomPortal *cRoom::GetPortal(int portalNum) const
{
   AssertValidPortal(portalNum, m_PortalList.Size());
   return m_PortalList[portalNum];
}

////////////////////////////////////////

inline mxs_real cRoom::GetPortalDist(int portalOne, int portalTwo) const
{
   AssertValidPortal(portalOne, m_PortalList.Size());
   AssertValidPortal(portalTwo, m_PortalList.Size());

   return m_PortalDist[portalOne][portalTwo];
}

////////////////////////////////////////////////////////////////////////////////

inline void cRoom::RemoveWatch(int handle)
{
   m_WatchList[handle].SetSize(0);
}

////////////////////////////////////////

inline void cRoom::AddWatchElement(int handle, ObjID objID)
{
   AssertMsg2(handle < m_WatchList.Size(), "Attempt to add to watch list 0 of 0", handle, m_WatchList.Size());

#ifndef SHIP
   for (int i=0; i<m_WatchList[handle].Size(); i++)
   {
      if (m_WatchList[handle][i] == objID)
      {
         Warning(("Attempt to twice add watch! ([%d] Room %d (%s)", handle, m_roomID, ObjWarnName(objID)));
         return;
      }
   }
                 
#endif

   m_WatchList[handle].Append(objID);
}

////////////////////////////////////////

inline void cRoom::RemoveWatchElement(int handle, int index)
{
   AssertMsg2(handle < m_WatchList.Size(), "Attempt to remove from watch list 0 of 0", handle, m_WatchList.Size());
   AssertMsg2(index < m_WatchList[handle].Size(), "Attempt to remove watch element 0 of 0",
                                                   index, m_WatchList[handle].Size());

   // Swap item to be removed with last item so no array packing necessary
   m_WatchList[handle].FastDeleteItem(index);
}

////////////////////////////////////////

inline void cRoom::RemoveAllWatchElements(int handle)
{
   AssertMsg2(handle < m_WatchList.Size(), "Attempt to remove from watch list 0 of 0", handle, m_WatchList.Size());

   m_WatchList[handle].SetSize(0);
}

////////////////////////////////////////

inline void cRoom::GetObjList(const ObjID **objList, int *size, int handle) const
{
   AssertMsg2(handle < m_WatchList.Size(), "Attempt to retrieve watch list 0 of 0", handle, m_WatchList.Size());

   *objList = m_WatchList[handle];
   *size = m_WatchList[handle].Size();
}

////////////////////////////////////////////////////////////////////////////////

#endif



