/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/roomeax.h,v 1.3 2000/01/31 10:00:34 adurant Exp $
//
// Room EAX header
//
#pragma once

#ifndef __ROOMEAX_H
#define __ROOMEAX_H

#ifdef __cplusplus

#include <dynarray.h>
#include <tagfile.h>

#endif

////////////////////////////////////////

EXTERN void InitRoomEAX();
EXTERN void TermRoomEAX();
EXTERN void ResetRoomEAX();

EXTERN int  GetRoomAcoustics(int roomID);

////////////////////////////////////////

#ifdef __cplusplus

class cRoomEAX
{
public:

   cRoomEAX();
   ~cRoomEAX();

   void Read(ITagFile *file);
   void Write(ITagFile *write) const;

   void Reset();
   void Build();

   void Remap(int from, int to);

   void SetAcoustics(int roomID, int roomType);
   int  GetAcoustics(int roomID) const;

private:

   cDynZeroArray<int> m_acousticsIndex;
};

////////////////////////////////////////

EXTERN cRoomEAX *g_pRoomEAX;

////////////////////////////////////////

inline void cRoomEAX::SetAcoustics(int roomID, int roomType)
{
   if (roomID < 0)
      return;

   if (roomID >= m_acousticsIndex.Size())
      m_acousticsIndex.SetSize(roomID + 1);

   m_acousticsIndex[roomID] = roomType;
}

inline int cRoomEAX::GetAcoustics(int roomID) const
{
   if ((roomID < 0) || (roomID >= m_acousticsIndex.Size()))
      return -1;
   else
      return m_acousticsIndex[roomID];
}

#endif

#endif




