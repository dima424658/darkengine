/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/rpathfnd.h,v 1.12 2000/01/31 10:00:42 adurant Exp $
//
// Room pathfinding header
//
#pragma once

#ifndef __RPATHFND_H
#define __RPATHFND_H

#if ROOM_PATHS

#include <aitype.h>

#include <rpaths.h>
#include <roompa.h>

typedef int ObjID;

////////////////////////////////////////

class cRoomPathFind : public cRoomPAFuncs
{
public:

   //////////////////
   //
   // Constructor / Destructor
   //
   cRoomPathFind();
   virtual ~cRoomPathFind();

   //////////////////

   void UseValidChecks(BOOL state);

   BOOL IsCellValid(tAIPathCellID cellID);

   void MarkRoomCells(int roomID);
   void UnMarkRoomCells(int roomID);

   //////////////////

   BOOL FindPath(ObjID fromObjID, ObjID toObjID);
   BOOL FindPath(const mxs_vector &from_vec, tAIPathCellID startCell, 
                 const mxs_vector &to_vec, tAIPathCellID endCell);

   //////////////////

   BOOL CanPathFind();

   BOOL AINewPathFind(const cMxsVector & fromLocation, tAIPathCellID startCell,
                      const cMxsVector & toLocation, tAIPathCellID endCell,
                      cAIPath * pPath);

   /////////////////
   //
   // Callbacks
   //
   int      EnterCallback(const cRoom *room, const cRoomPortal *enterPortal, const mxs_vector &enterPt, mxs_real dist);
   void     ExitCallback(const cRoom *room);
   mxs_real PortalsCallback(const cRoomPortal *enterPortal, const cRoomPortal *exitPortal, mxs_real dist);

private:

   ObjID  m_FromObj;
   ObjID  m_ToObj;

   mxs_vector m_FromVec;
   mxs_vector m_ToVec;

   BOOL m_PathFound;

   cRoomPropResultList m_RoomPropList;

   cRoomPropAgent m_RoomPropAgent;

   cAIPath   m_StartLeg;
   cAIPath   m_EndLeg;

   tAIPathCellID  m_StartCell;
   tAIPathCellID  m_EndCell;   
};

////////////////////////////////////////////////////////////////////////////////

inline BOOL cRoomPathFind::CanPathFind()
{
   return !g_pAIRoomDB->IsEmptyDB();
}

////////////////////////////////////////////////////////////////////////////////

inline void cRoomPathFind::UseValidChecks(BOOL state)
{
   g_pAIRoomDB->UseValidChecks(state);
}

////////////////////////////////////////

inline BOOL cRoomPathFind::IsCellValid(tAIPathCellID cellID)
{
   return g_pAIRoomDB->IsCellValid(cellID);
}

////////////////////////////////////////

inline void cRoomPathFind::MarkRoomCells(int roomID)
{
   g_pAIRoomDB->MarkRoomCells(roomID);
}

////////////////////////////////////////

inline void cRoomPathFind::UnMarkRoomCells(int roomID)
{
   g_pAIRoomDB->UnMarkRoomCells(roomID);
}

////////////////////////////////////////////////////////////////////////////////

void InitRoomPathFind();
void ClearRoomPathFind();
void TermRoomPathFind();
EXTERN void FindMyPath(ObjID objID);
EXTERN void FindMyPathTimed(ObjID objID);

////////////////////////////////////////

EXTERN cRoomPathFind *g_pRoomPathFindSys;

////////////////////////////////////////////////////////////////////////////////
#endif

#endif




