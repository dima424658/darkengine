/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiapipth.h,v 1.8 2000/01/05 15:54:20 BFarquha Exp $
//
//
//

#ifndef __AIAPIPTH_H
#define __AIAPIPTH_H

#ifndef __cplusplus
#error "Private AI APIs require C++"
#endif

#include <dynarray.h>

#include <aiapicmp.h>
#include <aiokbits.h>
#include <aipthtyp.h>

#pragma once
#pragma pack(4)

F_DECLARE_INTERFACE(IAIPathfinder);
F_DECLARE_INTERFACE(IAIPathfindControl);
F_DECLARE_INTERFACE(IAIPath);

class cAIPathEdge;
typedef cDynArray<tAIPathCellID> cAICellArray;

///////////////////////////////////////////////////////////////////////////////
//
// Low-level functioins
//

IAIPath * AIPathfind(const mxs_vector & fromLocation, tAIPathCellID startCell,
                     const mxs_vector & toLocation,   tAIPathCellID endCell = 0,
                     tAIPathOkBits OkBits = kAIOK_Walk,
                     IAIPathfindControl * pControl = NULL);

// It is assumed that this is being called because normal pathfinding failed.
IAIPath * AIPathfindNear(const mxs_vector & fromLocation, tAIPathCellID startCell,
                     const mxs_vector & toLocation, float fDist, tAIPathCellID endCell = 0,
                     tAIPathOkBits OkBits = kAIOK_Walk,
                     IAIPathfindControl * pControl = NULL);

///////////////////////////////////////////////////////////////////////////////

BOOL AIPathcast(const mxs_vector & startLocation, tAIPathCellID startCell,
                const mxs_vector & endLocation,
                tAIPathCellID * pEndCell,
                tAIPathOkBits OkBits = kAIOK_Walk,
                IAIPathfindControl * pControl = NULL);

///////////////////////////////////////

inline BOOL AIPathcast(const mxs_vector & startPoint,
                       tAIPathCellID startCell,
                       const mxs_vector & endPoint,
                       tAIPathOkBits OkBits = kAIOK_Walk,
                       IAIPathfindControl * pControl = NULL)
{
   tAIPathCellID ignored;
   return AIPathcast(startPoint, startCell, endPoint, &ignored, OkBits, pControl);
}

///////////////////////////////////////

void AISetPathcastIgnoreBlockingOBBs(BOOL);

///////////////////////////////////////

BOOL AIIsEdgeVisible(const mxs_vector & startLocation, tAIPathCellID startCell,
                     const cAIPathEdge & edge,
                     tAIPathOkBits fOkBits = kAIOK_Walk,
                     IAIPathfindControl * pControl = NULL);

///////////////////////////////////////

BOOL AICanUseLink(tAIPathCellID from,
                  tAIPathCellID to,
                  tAIPathOkBits LinkOkBits,
                  tAIPathOkBits AIOkBits,
                  unsigned      PathFlags,
                  IAIPathfindControl * pControl = NULL);

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAIPathfinder
//

DECLARE_INTERFACE_(IAIPathfinder, IAIComponent)
{
   STDMETHOD_(IAIPath *, Pathfind)(const mxs_vector & toLocation,
                                   tAIPathCellID endCell = 0,
                                   tAIPathOkBits fOkBits = kAIOK_Walk) PURE;

   STDMETHOD_(IAIPath *, PathfindNear)(const mxs_vector & toLocation,
                                   float fDist,
                                   tAIPathCellID endCell = 0,
                                   tAIPathOkBits fOkBits = kAIOK_Walk) PURE;

   STDMETHOD_(BOOL, Pathcast)(const mxs_vector & endLocation,
                              tAIPathCellID * pEndCell,
                              tAIPathOkBits OkBits = kAIOK_Walk) PURE;

   STDMETHOD_(BOOL, Pathcast)(const mxs_vector & endPoint,
                              tAIPathOkBits OkBits = kAIOK_Walk) PURE;

   STDMETHOD_(BOOL, IsEdgeVisible)(const cAIPathEdge & edge,
                                   tAIPathOkBits OkBits = kAIOK_Walk) PURE;

   STDMETHOD_(IAIPathfindControl *, AccessControl)() PURE;
};

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAIPathfindControl
//

DECLARE_INTERFACE_(IAIPathfindControl, IUnknown)
{
   STDMETHOD_(void, GetRecentCells)(cAICellArray * pResult) PURE;

   STDMETHOD_(BOOL, CanPassDoor)(ObjID door) PURE;


   STDMETHOD_(BOOL, AppCanPass1)(tAIPathCellID from,
                                 tAIPathCellID to) PURE;

   STDMETHOD_(BOOL, AppCanPass2)(tAIPathCellID from,
                                 tAIPathCellID to) PURE;
};

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAIPath
//

#define S_ADVANCE_REPATHFOUND (S_FALSE + 1)

DECLARE_INTERFACE_(IAIPath, IUnknown)
{
   STDMETHOD_(BOOL, Active)() const PURE;
   STDMETHOD_(void, Clear)() PURE;
   STDMETHOD_(HRESULT, Advance)(tAIPathOkBits nStressBits) PURE;
   STDMETHOD_(BOOL, Backup)() PURE;
   STDMETHOD_(int,  GetPathEdgeCount)() const PURE;
   STDMETHOD_(void, GetPathEdge)(int edgeIndex, cAIPathEdge *) const PURE; // 0..count-1;
   STDMETHOD_(const cAIPathEdge *, GetCurEdge)() const PURE;

   STDMETHOD_(void, GetFinalDestination)(mxs_vector *pDestination) const PURE;
   STDMETHOD_(BOOL, MoreEdges)() const PURE;
   STDMETHOD_(BOOL, IsDestVisible)(const cMxsVector &fromPt, tAIPathCellID startCell, tAIPathOkBits nStressBits) const PURE;

   // returns number looked ahead or back
   STDMETHOD_(int, UpdateTargetEdge)(const mxs_vector &fromPt, const mxs_angvec & fromFac, tAIPathCellID startCell, tAIPathOkBits nStressBits) PURE;

   // Compute the 2d distance to destination
   STDMETHOD_(float, ComputeXYLength)(const mxs_vector &curLoc) const PURE;

   // Label path as using "stress" bits.
   STDMETHOD_(void, SetStressBits)(tAIPathOkBits nStressBits) PURE;

   STDMETHOD_(tAIPathOkBits, GetStressBits)() PURE;

   STDMETHOD_(BOOL, IsCellInPath)(tAIPathCellID) PURE;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIAPIPTH_H */
