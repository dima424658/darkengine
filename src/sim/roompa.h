/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/roompa.h,v 1.8 2000/01/31 10:00:36 adurant Exp $
//
// cRooms header
//
#pragma once

#ifndef __ROOMPA_H
#define __ROOMPA_H

#include <matrixs.h>
#include <hashpp.h>
#include <dlist.h>
#include <dlisttem.h>

#include <roombase.h>

/////////////////

const kMaxDepth = 10;

/////////////////

enum eEnterResult_
{
   kER_Nothing,
   kER_StartBad,
   kER_EndBad,
   kER_Success,
};
typedef int eEnterResult;

////////////////////////////////////////////////////////////////////////////////

class cRoomPropResult;

typedef cDListNode<cRoomPropResult, 0> cRoomPropResultNode;
typedef cDList<cRoomPropResult, 0> cRoomPropResultListBase;

class cRoomPropResult : public cRoomPropResultNode
{
public:
   cRoomPropResult(const cRoom *init_room);

   const cRoom *room;
};

inline cRoomPropResult::cRoomPropResult(const cRoom *init_room)
   : room(init_room)
{
};

////////////////////////////////////////

class cRoomPropResultList : public cRoomPropResultListBase
{
public:
   ~cRoomPropResultList() { DestroyAll(); }
};

////////////////////////////////////////

// 1024 * 16 = 16k
#define MAX_BF_ROOMS 1024

// array elements for A* search
class cBFRoomInfo
{
public:
   void Init(mxs_real _dist, int _previous_room, int _previous_portal, short _next_active, short _prev_active)
   {
      dist = _dist;
      previous_room = _previous_room + 1;
      previous_portal = _previous_portal + 1;
      next_active = _next_active + 1;
      prev_active = _prev_active + 1;
   };

   mxs_real GetDist() const { return dist; };
   void     SetDist(mxs_real _dist) { dist = _dist; };

   int  GetPreviousRoom() const { return previous_room - 1; };
   void SetPreviousRoom(int _previous_room) { previous_room = _previous_room + 1; };

   int  GetPreviousRoom2() const { return previous_room_2 - 1; };
   void SetPreviousRoom2(int _previous_room_2) { previous_room_2 = _previous_room_2 + 1; };

   void SwitchPreviousRooms()
   {
      static short temp;

      temp  = previous_room;
      previous_room = previous_room_2;
      previous_room_2 = temp;

      temp = previous_portal;
      previous_portal = previous_portal_2;
      previous_portal_2 = temp;
   };

   int  GetPreviousPortal() const { return previous_portal - 1; };
   void SetPreviousPortal(int _previous_portal) { previous_portal = _previous_portal + 1; };

   int  GetPreviousPortal2() const { return previous_portal  - 1; };
   void SetPreviousPortal2(int _previous_portal_2) { previous_portal_2 = _previous_portal_2 + 1; };

   short GetNextActive() const { return next_active - 1; };
   void  SetNextActive(short _next_active) { next_active = _next_active + 1; };

   short GetPrevActive() const { return prev_active - 1; };
   void  SetPrevActive(short _prev_active) { prev_active = _prev_active + 1; };

private:
   mxs_real dist;                  // distance to get to this room
   short    previous_room;         // how we got to this room
   short    previous_room_2;       // secondary previous room
   short    previous_portal;       // the portal we used to get here
   short    previous_portal_2;     // secondary previous portal
   short    next_active;           // the next room id in the active list
   short    prev_active;           // the previous room id in the active list
};

////////////////////////////////////////////////////////////////////////////////

class cRoomPropAgent;

class cRoomPAFuncs
{
public:
   cRoomPropAgent *GetPropAgent;

   virtual int      EnterCallback(const cRoom *room, const cRoomPortal *enterPortal, const mxs_vector &enterPt, mxs_real dist) = 0;
   virtual void     ExitCallback(const cRoom *room) = 0;
   virtual mxs_real PortalsCallback(const cRoomPortal *enterPortal, const cRoomPortal *exitPortal, mxs_real dist) = 0;

   cRoomPropAgent  *m_RoomPropAgent;
};

////////////////////////////////////////////////////////////////////////////////

class cRoomPropAgent
{
public:

   /////////////////
   //
   // Constructor / Destructor
   //
   cRoomPropAgent();
   ~cRoomPropAgent();


   /////////////////
   //
   // Functions
   //
   void SetFunctions(cRoomPAFuncs *funcs);

   void PropagateBF(const mxs_vector &startPos, const cRoom *startRoom);
   void PropagateDF(const mxs_vector &startPos, const cRoom *startRoom);

   void GetRoomPath(const cRoom *endRoom, cRoomPropResultList *pResultList) const;
   const cBFRoomInfo *GetRoomBrushInfo() const;

private:

   void PropagateRec(const cRoom *curRoom, const cRoomPortal *enterPortal, const mxs_vector &startPt, mxs_real dist, int depth);

   cRoomPAFuncs    *m_Funcs;

   cBFRoomInfo m_BFRoomInfo[MAX_BF_ROOMS];
};

////////////////////////////////////////////////////////////////////////////////

inline const cBFRoomInfo *cRoomPropAgent::GetRoomBrushInfo() const
{
   return m_BFRoomInfo;
}

////////////////////////////////////////////////////////////////////////////////

inline void cRoomPropAgent::SetFunctions(cRoomPAFuncs *funcs)
{
   m_Funcs = funcs;
   m_Funcs->m_RoomPropAgent = this;
}

////////////////////////////////////////////////////////////////////////////////

#endif









