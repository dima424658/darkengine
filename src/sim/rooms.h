/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/rooms.h,v 1.13 2000/01/31 10:00:38 adurant Exp $
//
// cRooms header
//
#pragma once

#ifndef __ROOMS_H
#define __ROOMS_H

#include <objtype.h>
#include <matrixs.h>

#include <dlist.h>
#include <hashpp.h>

#include <roombase.h>
#include <room.h>
#include <rportal.h>


/////////////////

typedef cHashTableFunctions<long> longHashFunctions;
typedef cHashTable<long, /*tPCallback*/ void *, longHashFunctions> cPortalCallbackTable;

typedef cHashTableFunctions<ObjID> ObjIDHashFunctions;
typedef cHashTable<ObjID, cRoom *, ObjIDHashFunctions> cRoomTable;

/////////////////

typedef cDynArray<cRoom *>       cRoomList;
typedef cDynArray<cRoomPortal *> cRoomPortalList;

/////////////////

EXTERN void SendRoomMessages(ObjID exit_room, ObjID enter_room, ObjID moving_obj);

EXTERN BOOL g_RoomsOK;

/////////////////

class cRooms
{
public:

   /////////////////
   //
   // Constructor / Destructor
   //
   cRooms(void);
   ~cRooms(void);

   void Read(RoomReadWrite movefunc);
   void Write(RoomReadWrite movefunc);

   /////////////////
   //
   // Accessors
   //
   int    GetNumRooms(void) const;

   cRoom *GetRoom(int roomNum) const;
   cRoom *GetRoomNamed(char *roomName) const;

   void   AddRoom(cRoom *newRoom);
   void   ClearRooms();

   /////////////////

   int          GetNumPortals(void) const;

   cRoomPortal *GetPortal(int portalID) const;

   void         AddPortal(cRoomPortal *pPortal, int index = -1);
   void         ClearPortals();

   /////////////////

   void   AddWatch(tWatchCallback callback, int *handle);
   void   RemoveWatch(int handle);
   void   ClearWatch(int handle);
   int    GetNumWatches(void) const;

   /////////////////
   //
   // Object-Room Updators
   //
   void   UpdateRoomData(ObjID objID, const mxs_vector &new_loc);
   void   UpdateObjRoom(ObjID objID, cRoom *pRoomFrom, cRoom *pRoomTo);

   BOOL   FindObjRoom(ObjID objID, const mxs_vector &loc);
   cRoom *RoomFromPoint(const mxs_vector &loc);

   void   SetObjRoom(ObjID objID, cRoom *room);
   cRoom *GetObjRoom(ObjID objID) const;


   /////////////////
   //
   // Callback Functions
   //
   void AddPortalCallback(ObjID roomFrom, ObjID roomTo, tPortalCallback callback);
   void AddEnterCallback(ObjID enterRoom, tPortalCallback callback);
   void AddExitCallback(ObjID exitRoom, tPortalCallback callback);
   void AddTransitionCallback(tPortalCallback callback);
   static void AddAutomapCallback(tWatchCallback callback);

   void RemovePortalCallback(ObjID roomFrom, ObjID roomTo);
   void RemoveEnterCallback(ObjID enterRoom);
   void RemoveExitCallback(ObjID exitRoom);
   void RemoveTransitionCallback(tPortalCallback);
   static void RemoveAutomapCallback(tWatchCallback callback);

private:

   cRoomList  m_RoomList;
   cRoomTable m_RoomTable;

   cRoomPortalList m_RoomPortalList;

   cPortalCallbackTable m_PortalCallbackTable;
   cPortalCallbackTable m_EnterCallbackTable;
   cPortalCallbackTable m_ExitCallbackTable;

   cDynArray<tPortalCallback> m_TransitionCallbackList;

   cDynArray<tWatchCallback>  m_WatchCallbackList;

   static tWatchCallback      m_AutomapCallback;
};

////////////////////////////////////////////////////////////////////////////////

inline int cRooms::GetNumRooms(void) const
{
   return m_RoomList.Size();
}

////////////////////////////////////////

inline cRoom *cRooms::GetRoom(int roomNum) const
{
   AssertMsg2(roomNum < m_RoomList.Size(), "Attempt to get room 0 of 0", roomNum, m_RoomList.Size());

   return m_RoomList[roomNum];
}

////////////////////////////////////////

inline void cRooms::AddRoom(cRoom *newRoom)
{
   newRoom->SetRoomID((short)m_RoomList.Size());
   m_RoomTable.Insert(newRoom->GetObjID(), newRoom);
   m_RoomList.Append(newRoom);
}

////////////////////////////////////////////////////////////////////////////////

inline int cRooms::GetNumPortals(void) const
{
   return m_RoomPortalList.Size();
}

////////////////////////////////////////

inline cRoomPortal *cRooms::GetPortal(int portalID) const
{
   AssertMsg2(portalID < m_RoomPortalList.Size(), "Attempt to get portal 0 of 0\n", portalID, m_RoomPortalList.Size());
   return m_RoomPortalList[portalID];
}

////////////////////////////////////////

//
// @WARNING: Do NOT use the index parameter unless you are sure that 
//           you will not be leaving gaps in the indexing.  It exists
//           primarily for loading so we don't have to swizzle.
inline void cRooms::AddPortal(cRoomPortal *pPortal, int index)
{
   if (index < 0)
   {
      pPortal->SetPortalID(m_RoomPortalList.Size());
      m_RoomPortalList.Append(pPortal);
   }
   else
   {
      pPortal->SetPortalID(index);
      if (index >= m_RoomPortalList.Size())
         m_RoomPortalList.SetSize(index + 1);
      m_RoomPortalList[index] = pPortal;
   }
}

////////////////////////////////////////

inline void cRooms::ClearPortals(void)
{
   m_RoomPortalList.SetSize(0);
}

////////////////////////////////////////////////////////////////////////////////

inline void cRooms::ClearWatch(int handle)
{
   for (int i=0; i<m_RoomList.Size(); i++)
      m_RoomList[i]->RemoveAllWatchElements(handle);
}

////////////////////////////////////////

inline int cRooms::GetNumWatches(void) const
{
   return m_WatchCallbackList.Size();
}

////////////////////////////////////////////////////////////////////////////////

inline cRoom *cRooms::GetObjRoom(ObjID objID) const
{
   return m_RoomTable.Search(objID);
}

////////////////////////////////////////////////////////////////////////////////

inline void cRooms::AddPortalCallback(ObjID roomFrom, ObjID roomTo, tPortalCallback callback)
{
   m_PortalCallbackTable.Insert((((long)roomFrom) << 16) | roomTo, callback);
}

////////////////////////////////////////

inline void cRooms::AddEnterCallback(ObjID enterRoom, tPortalCallback callback)
{
   m_EnterCallbackTable.Insert((long)enterRoom, callback);
}

////////////////////////////////////////

inline void cRooms::AddExitCallback(ObjID exitRoom, tPortalCallback callback)
{
   m_ExitCallbackTable.Insert((long)exitRoom, callback);
}

////////////////////////////////////////

inline void cRooms::AddTransitionCallback(tPortalCallback callback)
{
   for (int i=0; i<m_TransitionCallbackList.Size(); i++)
   {
      if (m_TransitionCallbackList[i] == callback)
         return;
   }
   m_TransitionCallbackList.Append(callback);
}

////////////////////////////////////////

inline void cRooms::RemovePortalCallback(ObjID roomFrom, ObjID roomTo)
{
   m_PortalCallbackTable.Delete((((long)roomFrom) << 16) | roomTo);
}

////////////////////////////////////////

inline void cRooms::RemoveEnterCallback(ObjID enterRoom)
{
   m_EnterCallbackTable.Delete(enterRoom);
}

////////////////////////////////////////

inline void cRooms::RemoveExitCallback(ObjID exitRoom)
{
   m_ExitCallbackTable.Delete(exitRoom);
}

////////////////////////////////////////

inline void cRooms::RemoveTransitionCallback(tPortalCallback callback)
{
   for (int i=0; i<m_TransitionCallbackList.Size(); i++)
   {
      if (m_TransitionCallbackList[i] == callback)
      {
         m_TransitionCallbackList.DeleteItem(i);
         i--;
      }
   }
}

#endif



