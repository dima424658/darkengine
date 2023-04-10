/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipthfnd.h,v 1.17 2000/01/05 15:54:38 BFarquha Exp $
//
//
//

#ifndef __AIPTHFND_H
#define __AIPTHFND_H

#include <aiapipth.h>
#include <aipthqry.h>
#include <aibascmp.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitPathfinder(IAIManager *);
BOOL AITermPathfinder();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIPathfinder
//

typedef cAIComponentBase<IAIPathfinder, &IID_IAIPathfinder> cAIPathfinderBase;

class cAIPathfinder : public cAIPathfinderBase,
                      cCTDelegating<IAIPathfindControl>,
                      cCTDelegating<IAIPathQuery>
{
public:
   cAIPathfinder();

   STDMETHOD (QueryInterface)(REFIID id, void ** ppI);

   STDMETHOD_(const char *, GetName)();

   //
   // IAIPathfinder methods
   //
   STDMETHOD_(IAIPath *, Pathfind)(const mxs_vector & toLocation,
                                   tAIPathCellID endCell = 0,
                                   tAIPathOkBits fOkBits = kAIOK_Walk);

   STDMETHOD_(IAIPath *, PathfindNear)(const mxs_vector & toLocation,
                                   float fDist,
                                   tAIPathCellID endCell = 0,
                                   tAIPathOkBits fOkBits = kAIOK_Walk);

   STDMETHOD_(BOOL, Pathcast)(const mxs_vector & endLocation,
                              tAIPathCellID * pEndCell,
                              tAIPathOkBits OkBits = kAIOK_Walk);

   STDMETHOD_(BOOL, Pathcast)(const mxs_vector & endPoint,
                              tAIPathOkBits OkBits = kAIOK_Walk);

   STDMETHOD_(BOOL, IsEdgeVisible)(const cAIPathEdge & edge,
                                   tAIPathOkBits OkBits = kAIOK_Walk);

   STDMETHOD_(IAIPathfindControl *, AccessControl)();

   //
   // IAIPathfindControl methods
   //
   STDMETHOD_(void, GetRecentCells)(cAICellArray * pResult);

   STDMETHOD_(BOOL, CanPassDoor)(ObjID door);


   STDMETHOD_(BOOL, AppCanPass1)(tAIPathCellID from,
                                 tAIPathCellID to);

   STDMETHOD_(BOOL, AppCanPass2)(tAIPathCellID from,
                                 tAIPathCellID to);

   //
   // Find the edge of the database. Returns TRUE if hit it.
   //
   STDMETHOD_(BOOL, CastForBoundary)(const mxs_vector & end,
                                     mxs_vector * pHitLoc,
                                     mxs_vector * pBoundVec);

};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIPTHFND_H */
