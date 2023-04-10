/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipath.h,v 1.13 2000/02/28 17:27:54 toml Exp $
//
//
//

#ifndef __AIPATH_H
#define __AIPATH_H

#include <dynarray.h>

#include <aitype.h>
#include <aiapipth.h>
#include <aipthedg.h>
#include <aipthtyp.h>

#pragma pack(4)
#pragma once

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIPath
//

class cAIPath : public cCTUnaggregated<IAIPath, &IID_IAIPath, kCTU_Default>
{
public:
   cAIPath();
   ~cAIPath();

   void SetControl(IAIPathfindControl * pControl);

   STDMETHOD_(BOOL, Active)() const;
   STDMETHOD_(void, Clear)();
   STDMETHOD_(HRESULT, Advance)(tAIPathOkBits nStressBits);
   STDMETHOD_(BOOL, Backup)();
   STDMETHOD_(int,  GetPathEdgeCount)() const;
   STDMETHOD_(void, GetPathEdge)(int edgeIndex, cAIPathEdge *) const; // 0..count-1;
   STDMETHOD_(const cAIPathEdge *, GetCurEdge)() const;

   // returns number looked ahead or back
   STDMETHOD_(int, UpdateTargetEdge)(const mxs_vector &fromPt, const mxs_angvec & fromFac, tAIPathCellID startCell, tAIPathOkBits nStressBits);

   // Compute the 2d distance to destination
   STDMETHOD_(float, ComputeXYLength)(const mxs_vector &curLoc) const;

   void SetCurrentPathEdgeAtStart();
   void SetActive(BOOL newValue);

   void AddPathEdge(const cAIPathEdge &nextEdge);

   const cAIPathEdge * GetNextPathEdge() const;

   STDMETHOD_(BOOL, MoreEdges)() const;
   BOOL IsNextEdgeVisible(const cMxsVector &fromPt, tAIPathCellID startCell, tAIPathOkBits nStressBits) const;
   STDMETHOD_(BOOL, IsDestVisible)(const cMxsVector &fromPt, tAIPathCellID startCell, tAIPathOkBits nStressBits) const;

   STDMETHOD_(void, GetFinalDestination)(mxs_vector *pDestination) const;
   void SetFinalDestination(const cMxsVector &destination);

   STDMETHOD_(void, SetStressBits)(tAIPathOkBits nStressBits);

   STDMETHOD_(tAIPathOkBits, GetStressBits)();

   STDMETHOD_(BOOL, IsCellInPath)(tAIPathCellID);

private:
   void Append(const cAIPath *); // private
   BOOL Repathfind(tAIPathOkBits nStressBits);

   cMxsVector   m_finalDest;
   int          m_curEdge;
   BOOL         m_fActive;
   int          m_nBlockedLookback;
   tAIPathOkBits nLocalStressBits;

   cDynArray_<cAIPathEdge, 16> m_edges;

   IAIPathfindControl * m_pControl;
};


///////////////////////////////////////

inline void cAIPath::SetControl(IAIPathfindControl * pControl)
{
   Assert_(!m_pControl);
   m_pControl = pControl;
}

///////////////////////////////////////

inline void cAIPath::SetActive(BOOL newValue)
{
   m_fActive = newValue;
}

///////////////////////////////////////

inline void cAIPath::SetFinalDestination(const cMxsVector &destination)
{
   m_finalDest = destination;
}

///////////////////////////////////////

inline void cAIPath::AddPathEdge(const cAIPathEdge &nextEdge)
{
   m_edges.Append(nextEdge);
}

///////////////////////////////////////

inline const cAIPathEdge * cAIPath::GetNextPathEdge() const
{
   if (m_curEdge > 0)
      return &m_edges[m_curEdge-1];

   return NULL;
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIPATH_H */

