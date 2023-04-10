/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aipath.cpp,v 1.21 2000/02/28 17:27:53 toml Exp $
//
//
//

#include <lg.h>
#include <fltang.h>

#include <aitype.h>
#include <aidist.h>
#include <aipath.h>
#include <aipathdb.h>
#include <aipthcst.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>

static const int PathVersion = 1;

#define kBlockLookbackLimit 4

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIPath
//

cAIPath::cAIPath()
{
   m_pControl = NULL;
   m_curEdge = -1;
   m_fActive = FALSE;
   m_nBlockedLookback = 0;

   nLocalStressBits = 0;

   m_edges.SetSize(0);
}

///////////////////////////////////////

cAIPath::~cAIPath()
{
   m_edges.SetSize(0);
}

///////////////////////////////////////


STDMETHODIMP_(void) cAIPath::SetStressBits(tAIPathOkBits nStressBits)
{
   nLocalStressBits = nStressBits;
}

///////////////////////////////////////

STDMETHODIMP_(tAIPathOkBits) cAIPath::GetStressBits()
{
   return nLocalStressBits;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIPath::IsCellInPath(tAIPathCellID Cell)
{
   if (m_curEdge < 0)
      return FALSE;

   // yeah, I know, not the best search in the world, but list should be pretty short, right?

#define SEARCH_LIMITER 10

   for (int i = m_curEdge; (i < m_edges.Size()) && (i < (m_curEdge+SEARCH_LIMITER)); i++)
      if (m_edges[i].cell == Cell)
         return TRUE;

   return FALSE;
}

///////////////////////////////////////



STDMETHODIMP_(BOOL) cAIPath::Active() const
{
   return m_fActive;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIPath::Clear()
{
   m_fActive = FALSE;
   m_curEdge = -1;
   m_edges.SetSize(0);
}

///////////////////////////////////////

STDMETHODIMP_(HRESULT) cAIPath::Advance(tAIPathOkBits nStressBits)
{
   if (m_curEdge > 0)
   {
      m_curEdge--;

      tAIPathCellID cell = m_edges[m_curEdge].cell;
      ObjID door;

      // Test if can advanced to door, verify can pass
      // Jon 6/26/99: don't allow advance if the cell is active moving terrain (this can change dynamically)
      if (m_pControl &&
          ((g_AIPathDB.IsBelowDoor(cell) && (door = g_AIPathDB.GetCellDoor(cell)) != OBJ_NULL && !m_pControl->CanPassDoor(door))) ||
          g_AIPathDB.IsActiveMovingTerrain(cell))
      {
//         mprintf("Need to repathfind\n");

         if (Repathfind(nStressBits))
            return S_ADVANCE_REPATHFOUND;
         else
            return E_FAIL;
      }

      return S_OK;
   }

   return S_FALSE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIPath::Backup()
{
   if (m_curEdge < m_edges.Size() - 1)
   {
      m_curEdge++;
      return TRUE;
   }

   return FALSE;
}

///////////////////////////////////////

STDMETHODIMP_(int) cAIPath::GetPathEdgeCount() const
{
   return m_edges.Size();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIPath::GetPathEdge(int edgeIndex, cAIPathEdge *edge) const
{
   *edge = m_edges[edgeIndex];
}

///////////////////////////////////////

STDMETHODIMP_(const cAIPathEdge *) cAIPath::GetCurEdge() const
{
   if ((m_curEdge >= 0) && (m_curEdge < m_edges.Size()))
      return &m_edges[m_curEdge];

   return NULL;
}

///////////////////////////////////////

void cAIPath::Append(const cAIPath *p)
{
   cAIPathEdge edge;
   int i;

   for (i= 0; i < p->GetPathEdgeCount(); i++)
   {
      p->GetPathEdge(i, &edge);
      m_edges.Append(edge);
   }

   p->GetFinalDestination(&m_finalDest);
}

///////////////////////////////////////
#if 0
int cAIPath::Read(void * pContext, tAIReadWriteFunc read)
{
   int bytes = 0;
   cAIPathEdge edge;
   int num_edges;
   int version;
   int i;

   bytes += read(pContext, &version, sizeof(int), 1);
   if (version != PathVersion)
      return (-1);

   bytes += read(pContext, &m_finalDest, sizeof(cMxsVector), 1);
   bytes += read(pContext, &m_curEdge, sizeof(int), 1);
   bytes += read(pContext, &m_fActive, sizeof(BOOL), 1);

   m_edges.SetSize(0);
   bytes += read(pContext, &num_edges, sizeof(int), 1);
   for (i=0; i<num_edges; i++)
   {
      bytes += read(pContext, &edge, sizeof(cAIPathEdge), 1);
      m_edges.Append(edge);
   }
   return bytes;
}

///////////////////////////////////////

int cAIPath::Write(void * pContext, tAIReadWriteFunc write)
{
   int bytes = 0;
   int size = m_edges.Size();

   bytes += write(pContext, (void *)&PathVersion, sizeof(int), 1);

   bytes += write(pContext, &m_finalDest, sizeof(cMxsVector), 1);
   bytes += write(pContext, &m_curEdge, sizeof(int), 1);
   bytes += write(pContext, &m_fActive, sizeof(BOOL), 1);

   bytes += write(pContext, &size, sizeof(int), 1);
   for (int i=0; i<m_edges.Size(); i++)
      bytes += write(pContext, &m_edges[i], sizeof(cAIPathEdge), 1);
   return bytes;
}
#endif
///////////////////////////////////////

void cAIPath::SetCurrentPathEdgeAtStart()
{
   m_curEdge = m_edges.Size() - 1;

   if (m_edges.Size() > 0)
      SetActive(TRUE);
   else
      SetActive(FALSE);
}

///////////////////////////////////////
//
// IsCurEdgeVisible returns TRUE whenever
// any part of the "next path edge" is visible
// through the "current path edge" from the
// "fromPt".

#define kMaxPathVisDistSq sq(40.0)
#define kDoorPassAllowance sq(2.5)

BOOL cAIPath::IsNextEdgeVisible(const cMxsVector &fromPt, tAIPathCellID startCell, tAIPathOkBits nStressBits) const
{
   const cAIPathEdge * pNextEdge = GetNextPathEdge();

   // hmmm, we don't have a nextEdge, so I guess
   // we can see our next edge.
   if (!pNextEdge)
      return TRUE;

   // Is the next edge close enough to consider "visible"
   cMxsVector centerPt;
   pNextEdge->GetCenter(&centerPt);

   // Never look past doors that are not right in front of of you
   if (AIXYDistanceSq(fromPt, centerPt) > kDoorPassAllowance &&
      g_AIPathDB.IsBelowDoor(pNextEdge->cell))
      return FALSE;

   if (pflt(kMaxPathVisDistSq, AIXYDistanceSq(fromPt, centerPt)))
      return FALSE;

   return AIIsEdgeVisible(fromPt, startCell, *pNextEdge, kAIOK_Walk | nStressBits | nLocalStressBits, m_pControl);
}

///////////////////////////////////////
//
// IsDestVisible returns TRUE whenever
// the m_finalDest is visible through the
// "current path edge" from the fromPt.

STDMETHODIMP_(BOOL) cAIPath::IsDestVisible(const cMxsVector &fromPt, tAIPathCellID startCell, tAIPathOkBits nStressBits) const
{
   // @TBD (toml 05-24-98): may not be quite right. If we have edges remaining, we should check ther last one?
   if (!startCell)
      return FALSE;
   return AIPathcast(fromPt, startCell, m_finalDest, kAIOK_Walk | nStressBits | nLocalStressBits, m_pControl);
}

///////////////////////////////////////

STDMETHODIMP_(int) cAIPath::UpdateTargetEdge(const mxs_vector &fromPt, const mxs_angvec & fromFac, tAIPathCellID startCell, tAIPathOkBits nStressBits)
{
#ifdef DEBUG
   static int d;

   d++;
#endif

   // First, scan forward
   int     look = 0;
   HRESULT result = S_OK;
   int     prevCurEdge = m_curEdge;    

   while (IsNextEdgeVisible(fromPt, startCell, nStressBits|nLocalStressBits))
   {
      result = Advance(nStressBits|nLocalStressBits);
      if (result == S_OK)
         look++;
      else
         break;
   }

   if (result == S_ADVANCE_REPATHFOUND)
      return 0;

   // Now, if we can't look forward, scan backwards to verify we haven't become obstructed in some
   // way making a previous lookahead invalid
   if (look)
      return look;
   
   const cAIPathEdge * pCurEdge = GetCurEdge();
   if (!pCurEdge)
   {
      m_nBlockedLookback = 0;
      return 0;
   }

   do
   {
      if (AIIsEdgeVisible(fromPt, startCell, *pCurEdge, kAIOK_Walk | nStressBits | nLocalStressBits, m_pControl))
         break;

      if (!Backup())
      {
#ifdef DEBUG
         if (d == 1)
            Assert_(UpdateTargetEdge(fromPt, fromFac, startCell, nStressBits|nLocalStressBits) == 0);
         d--;
#endif
         m_nBlockedLookback = 0;
         
         // On reaching here, we are at the start of the path. Since the 
         // previous logic looked forward and found nothing, and this
         // logic looked back and found nothing, it is likely we cannot
         // "see" any part of the path. 
         //
         // What should happen is we need to repath. In practice, the
         // safest thing is to  just keep aiming as before. At some
         // point, a better recovery strategy must be devised. (toml 02-28-00)
         
         m_curEdge = prevCurEdge;
         
         return 0;
      }

      look--;
      pCurEdge = GetCurEdge();

      // Check to make sure we're not looking back too much
      if (m_nBlockedLookback < kBlockLookbackLimit)
      {
         mxs_vector delta;
         mxs_vector objRelativePt;

         pCurEdge->GetCenter(&delta);
         mx_subeq_vec(&delta, &fromPt);

         mx_rot_z_vec(&objRelativePt, &delta, mx_rad2ang(floatang(-mx_ang2rad(fromFac.tz)).value));

         if (objRelativePt.x < -2.5 && ffabsf(objRelativePt.y) < 1.5)
         {
            look++;
            m_curEdge--;
            m_nBlockedLookback++;
            return look;
         }
      }

   } while (1);
   
   m_nBlockedLookback = 0;

#ifdef DEBUG
   if (result != S_ADVANCE_REPATHFOUND && d == 1)
      AssertMsg(UpdateTargetEdge(fromPt, fromFac, startCell, nStressBits|nLocalStressBits) == 0, "Inconsistent results from cAIPath::UpdateTargetEdge()");
   d--;
#endif

   return look;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIPath::MoreEdges() const
{
   return (m_curEdge > 0);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIPath::GetFinalDestination(mxs_vector *pDestination) const
{
   *pDestination = m_finalDest;
}

///////////////////////////////////////

STDMETHODIMP_(float) cAIPath::ComputeXYLength(const mxs_vector &curLoc) const
{
   if (m_curEdge < 0)
      return AIXYDistance(curLoc, m_finalDest);

   if (m_curEdge >= m_edges.Size())
      return 5000.0;

   int current = m_curEdge;
   const mxs_vector * pPrevCenter = &g_AIPathDB.GetCenter(m_edges[current].cell);
   const mxs_vector * pCurCenter;

   float sumDist = AIXYDistance(curLoc, *pPrevCenter);

   while (--current >= 0)
   {
      pCurCenter = &g_AIPathDB.GetCenter(m_edges[current].cell);
      sumDist += AIXYDistance(*pPrevCenter, *pCurCenter);
      pPrevCenter = pCurCenter;
   }

   sumDist += AIXYDistance(*pPrevCenter, m_finalDest);

   return sumDist;
}


///////////////////////////////////////

BOOL cAIPath::Repathfind(tAIPathOkBits nStressBits)
{
   IAIPathfinder * pPathfinder;
   if (m_pControl->QueryInterface(IID_IAIPathfinder, (void **)&pPathfinder) == S_OK)
   {
      cAIPath * pNewPath = (cAIPath *)pPathfinder->Pathfind(m_finalDest, m_edges[0].cell, kAIOK_Walk | nStressBits | nLocalStressBits); // cast is safe assumption as of today(toml 10-01-98)
      SafeRelease(pPathfinder);

      if (!pNewPath)
         return FALSE;

      m_finalDest = pNewPath->m_finalDest;
      m_curEdge = pNewPath->m_curEdge;
      m_fActive = pNewPath->m_fActive;
      m_edges = pNewPath->m_edges;

      pNewPath->Release();

      return TRUE;
   }
   
   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

