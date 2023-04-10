/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/rpaths.h,v 1.12 2000/01/31 10:00:43 adurant Exp $
//
// Room paths header
//
#pragma once

#ifndef __RPATHS_H
#define __RPATHS_H

#include <roomsys.h>
#include <dynarray.h>
#include <aipthtyp.h>

#if ROOM_PATHS
// include <aipath.h>
#endif

#include <mprintf.h>

////////////////////////////////////////////////////////////////////////////////

#if ROOM_PATHS

struct sRPortalAIPath
{
   ulong    cost;
   cAIPath *path;
};

typedef cDynArray<sRPortalAIPath> cRPortalAIPathSubList;
typedef cDynClassArray<cRPortalAIPathSubList> cRPortalAIPathList;

#endif

////////////////////////////////////////////////////////////////////////////////

typedef cDynArray<tAIPathCellID> cAICellList;

////////////////////////////////////////////////////////////////////////////////

typedef void (*AIRoomDBReadWrite) (void *buf, size_t elsize, size_t nelem);

////////////////////////////////////////////////////////////////////////////////

class cAIRoomDB;

EXTERN cAIRoomDB *g_pAIRoomDB;

EXTERN void InitAIRoomDatabase();
EXTERN void TermAIRoomDatabase();
EXTERN void ResetAIRoomDatabase();

EXTERN void SaveAIRoomDatabase(AIRoomDBReadWrite movefunc);
EXTERN void LoadAIRoomDatabase(AIRoomDBReadWrite movefunc);

EXTERN void BuildAIRoomDatabase();

////////////////////////////////////////////////////////////////////////////////

class cAIRoomDB
{
public:

   //////////////////
   //
   // Constructor / Destructor
   //
   cAIRoomDB();
   ~cAIRoomDB();

   void Clear();
   BOOL IsEmptyDB();

   void Read(RoomReadWrite movefunc);
   void Write(RoomReadWrite movefunc);

   void SpewDB();

   //////////////////

#if ROOM_PATHS
   void GenerateCosts();

   void AddRoomPath(short roomID, int portalID1, int portalID2, ulong cost, cAIPath *path);
   void InvalidateRoomPath(short roomID, int portalID1, int portalID2);

   BOOL           HasPath(short roomID, int portalID1, int portalID2);
   const cAIPath *GetPath(short roomID, int portalID1, int portalID2);
   ulong          GetCost(short roomID, int portalID1, int portalID2);

   void ClearRoomPaths();
#endif

   //////////////////

   void InitValidCellList();

   void ClearRoomCellLists();
   void BuildRoomCellLists();

   void MarkRoomCells(short roomID);
   void UnMarkRoomCells(short roomID);

   void MarkAllRoomCells();
   void UnMarkAllRoomCells();

   void UseValidChecks(BOOL state);

   BOOL IsCellValid(tAIPathCellID cellID);

   //////////////////

#if ROOM_PATHS
   void ClearPortalHints();
   void GeneratePortalHints();

   void SetPortalHint(int portalID, int hint);
   int  GetPortalHint(int portalID) const;
#endif

private:

   BOOL AddCellToRoom(short roomID, tAIPathCellID cellID);

   /////////////////////////////////////

   BOOL m_DatabaseEmpty;

   BOOL m_UseValidChecks;

#if ROOM_PATHS
   cDynClassArray<cRPortalAIPathList> m_RoomPortalPaths;
   cDynArray<int>  m_PortalHintList;
#endif

   cDynClassArray<cAICellList> m_RoomCellList;
   cDynArray<BOOL>             m_CellValidList;

};

////////////////////////////////////////////////////////////////////////////////

inline BOOL cAIRoomDB::IsEmptyDB()
{
   return m_DatabaseEmpty;
}

#if ROOM_PATHS
////////////////////////////////////////////////////////////////////////////////

inline void cAIRoomDB::InvalidateRoomPath(short roomID, int portalID1, int portalID2)
{
   AssertMsg(HasPath(roomID, portalID1, portalID2), "Attempt to invalidate invalid path\n");

   m_RoomPortalPaths[roomID][portalID1][portalID2].path = NULL;
}

////////////////////////////////////////

inline BOOL cAIRoomDB::HasPath(short roomID, int portalID1, int portalID2)
{
   return ((roomID < m_RoomPortalPaths.Size()) &&
           (portalID1 < m_RoomPortalPaths[roomID].Size()) &&
           (portalID2 < m_RoomPortalPaths[roomID].Size()) &&
           (m_RoomPortalPaths[roomID][portalID1][portalID2].path != NULL));
}

////////////////////////////////////////

inline const cAIPath *cAIRoomDB::GetPath(short roomID, int portalID1, int portalID2)
{
   AssertMsg(HasPath(roomID, portalID1, portalID2), "Attempt to get invalid path\n");

   return m_RoomPortalPaths[roomID][portalID1][portalID2].path;
}

////////////////////////////////////////

inline ulong cAIRoomDB::GetCost(short roomID, int portalID1, int portalID2)
{
   AssertMsg(HasPath(roomID, portalID1, portalID2), "Attempt to get cost of invalid path\n");

   return m_RoomPortalPaths[roomID][portalID1][portalID2].cost;
}

#endif

////////////////////////////////////////////////////////////////////////////////

inline void cAIRoomDB::UseValidChecks(BOOL state)
{
   m_UseValidChecks = state;
}

inline void cAIRoomDB::MarkRoomCells(short roomID)
{
   AssertMsg1(roomID < m_RoomCellList.Size(), "Attempt to mark from invalid room id 0\n", roomID);
   AssertMsg(m_CellValidList.Size() > 0, "Attempt to mark unitialized valid cell list\n");
   
   for (int i=0; i<m_RoomCellList[roomID].Size(); i++)
      m_CellValidList[m_RoomCellList[roomID][i]] = TRUE;
}

////////////////////////////////////////

inline void cAIRoomDB::UnMarkRoomCells(short roomID)
{
   AssertMsg1(roomID < m_RoomCellList.Size(), "Attempt to unmark from invalid room id 0\n", roomID);
   AssertMsg(m_CellValidList.Size() > 0, "Attempt to mark unitialized valid cell list\n");

   for (int i=0; i<m_RoomCellList[roomID].Size(); i++)
      m_CellValidList[m_RoomCellList[roomID][i]] = FALSE;
}

////////////////////////////////////////

inline void cAIRoomDB::MarkAllRoomCells()
{
   for (int i=0; i<m_RoomCellList.Size(); i++)
      MarkRoomCells(i);
}

inline void cAIRoomDB::UnMarkAllRoomCells()
{
   for (int i=0; i<m_RoomCellList.Size(); i++)
      UnMarkRoomCells(i);
}

////////////////////////////////////////

inline BOOL cAIRoomDB::IsCellValid(tAIPathCellID cellID)
{
   return ((m_UseValidChecks == FALSE) || (cellID < m_CellValidList.Size() && m_CellValidList[cellID]));
}

////////////////////////////////////////////////////////////////////////////////

#if ROOM_PATHS
inline void cAIRoomDB::ClearPortalHints()
{
   m_PortalHintList.SetSize(0);
}

////////////////////////////////////////

inline void cAIRoomDB::SetPortalHint(int portalID, int hint)
{
   if (portalID >= m_PortalHintList.Size())
      m_PortalHintList.SetSize(portalID + 1);

   m_PortalHintList[portalID] = hint;
}

////////////////////////////////////////

inline int cAIRoomDB::GetPortalHint(int portalID) const
{
   if (portalID >= m_PortalHintList.Size())
      return 0;
   else
      return m_PortalHintList[portalID];
}
#endif
////////////////////////////////////////////////////////////////////////////////

#endif













