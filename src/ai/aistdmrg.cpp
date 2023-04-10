/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aistdmrg.cpp,v 1.15 1999/12/01 16:06:56 BFarquha Exp $
//
// @TBD (toml 01-08-98): in ai, remember that movement regulators are only run on non sleep and efficient modes
//
//

// #define PROFILE_ON 1

#include <lg.h>
#include <timings.h>
#include <objpos.h>
#include <vec2.h>

#include <aiapiiai.h>
#include <aiavoid.h>
#include <aidist.h>
#include <aipathdb.h>
#include <aistdmrg.h>
#include <aiutils.h>
#include <aiapipth.h>

// Must be last header
#include <dbmem.h>

#define kAIAvoidWallsDist ((float)2.2)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIWallsCliffsMovReg
//
// Responsible for making sure the AI doesn't run into walls
//

STDMETHODIMP_(const char *) cAIWallsCliffsMovReg::GetName()
{
   return "Walls/cliffs movement regulator";
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIWallsCliffsMovReg::NewRegulations()
{
   return TRUE;
}

///////////////////////////////////////

DECLARE_TIMER(AI_AWCMR, Average);

STDMETHODIMP cAIWallsCliffsMovReg::SuggestRegulations(cAIMoveSuggestions & suggestions)
{
   // If our timer hasn't expired, just reuse previously calculated data
   if (!m_Timer.Expired())
   {
      suggestions.Add(m_Suggestions);
      return S_FALSE;
   }

   m_Timer.Reset();

   AUTO_TIMER(AI_AWCMR);

   // Kill my previous suggestions...
   m_Suggestions.DestroyAll();

#define kNumDirs   8
#define k45 0.7071067811865

   static mxs_vector dirVecs[kNumDirs] =
   {
      { 1   * kAIAvoidWallsDist, 0   * kAIAvoidWallsDist, 0 },
      { k45 * kAIAvoidWallsDist, k45 * kAIAvoidWallsDist, 0 },
      { 0   * kAIAvoidWallsDist, 1   * kAIAvoidWallsDist, 0 },
      {-k45 * kAIAvoidWallsDist, k45 * kAIAvoidWallsDist, 0 },
      {-1   * kAIAvoidWallsDist, 0   * kAIAvoidWallsDist, 0 },
      {-k45 * kAIAvoidWallsDist,-k45 * kAIAvoidWallsDist, 0 },
      { 0   * kAIAvoidWallsDist,-1   * kAIAvoidWallsDist, 0 },
      { k45 * kAIAvoidWallsDist,-k45 * kAIAvoidWallsDist, 0 },
   };
   static floatang narrowField(DEGREES(40)), wideField(DEGREES(70));

   Position *    pPos = ObjPosGet(GetID());
   tAIPathCellID cell = m_pAIState->GetPathCell();

   Assert_(pPos);

   int        i;
   mxs_vector testPt;
   floatang   ang;

   //AISetPathcastIgnoreBlockingOBBs(TRUE);

   for (i = 0; i < kNumDirs; i++)
   {
      mx_rot_z_vec(&testPt, &dirVecs[i], pPos->fac.tz);
      mx_addeq_vec(&testPt, m_pAIState->GetPathLocation());

      if (!AIPathcast(*m_pAIState->GetPathLocation(), cell, testPt, 0, (IAIPathfindControl *)NULL))
      {
         ang.set(pPos->loc.vec.x, pPos->loc.vec.y, testPt.x, testPt.y);

         sAIMoveSuggestion * pSuggestion = new sAIMoveSuggestion;

         pSuggestion->SetWeightedBias(kAIMS_Danger, 50);
         pSuggestion->dirArc.SetByCenterAndSpan(ang, narrowField);
         pSuggestion->speed  = kAIS_Fast;

         m_Suggestions.Append(pSuggestion);

         pSuggestion = new sAIMoveSuggestion;

         pSuggestion->SetWeightedBias(kAIMS_Danger, 20);
         pSuggestion->dirArc.SetByCenterAndSpan(ang, wideField);
         pSuggestion->speed  = kAIS_Fast;

         m_Suggestions.Append(pSuggestion);
      }
   }

   //AISetPathcastIgnoreBlockingOBBs(FALSE);

   suggestions.Add(m_Suggestions);
   return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAILVLMovReg
//
// Responsible for making sure the AI doesn't run into unused LVL cells
//

STDMETHODIMP_(const char *) cAILVLMovReg::GetName()
{
   return "LVL movement regulator";
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAILVLMovReg::NewRegulations()
{
   return TRUE;
}



// Truth be told, this code can be found in aipthfnd, too.

// Edge assumes to go from origin to pPoint.
// return dist2
static inline float CalcDist2ToEdge(const mxs_vector *pLocation, const mxs_vector *pPoint, mxs_vector *pIntersect)
{
   float fScale;

   float fXSolution;
   float fSlope;

   // Mostly vertical:
   if (fabs(pPoint->x) < 0.001)
      fScale = pLocation->y/pPoint->y;
   else // test slope.  @TBD: (x1 != x2) : safe assumption?
   {
      fSlope = pPoint->y/pPoint->x;
      fXSolution = -pLocation->y/(fSlope*fSlope+1);
      fScale = fXSolution/pPoint->x;
   }

   if (fScale < 0) // return dist2 to origin
   {
      pIntersect->x = 0;
      pIntersect->y = 0;
      return mx_mag2_vec(pLocation);
   }
   else if (fScale > 1) // return dist2 to point
   {
      mxs_vector RelVec;
      mx_sub_vec(&RelVec, pLocation, pPoint);
      RelVec.z = 0;
      *pIntersect = *pPoint;
      return mx_mag2_vec(&RelVec);
   }
   else // return dist to edge
   {
      // vertical.
      if (fabs(pPoint->x) < 0.001)
         {
         pIntersect->y = 0;
         pIntersect->x = -pLocation->x;
         return pLocation->x*pLocation->x;
         }

      // use XSolution
      mxs_vector RelVec;
      RelVec.x = pLocation->x-fXSolution;
      RelVec.y = pLocation->y-fXSolution*fSlope;
      RelVec.z = 0;
      pIntersect->x = fXSolution;
      pIntersect->y = fXSolution*fSlope;
      return mx_mag2_vec(&RelVec);
   }
}


static BOOL GetCellEdgeIntersection(mxs_vector *pPos, tAIPathCellID CellId, float fRadius, mxs_vector *pIntersect)
{
   sAIPathCell *pCell = g_AIPathDB.AccessCell(CellId);

   const int iFirstVertex = pCell->firstVertex;
   const int iVertexLimit = iFirstVertex + pCell->vertexCount;
   const cMxsVector * p1;
   const cMxsVector * p2;

   float fDist2;
   mxs_vector EdgeVec;
   mxs_vector VertToPoint;
   mxs_vector Intersect;
   float fRad2 = fRadius*fRadius;
   float fClosestDist2 = 999999;

   // Just give a default angle:
   memset(pIntersect, 0, sizeof(mxs_vector));
   pIntersect->x = 1;

   p2=&(g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[iFirstVertex].id].pt);

   // As soon as we find a point within our radius, or have exhausted all segments, then we're done.
   for (int i = iFirstVertex; (i < iVertexLimit) && (fClosestDist2 > fRad2); i++)
   {
      p1 = p2;
      if (i == iVertexLimit-1)
         p2 = &(g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[iFirstVertex].id].pt);
      else
         p2 = &(g_AIPathDB.m_Vertices[g_AIPathDB.m_CellVertices[i+1].id].pt);

      // get the 2d edge vector
      EdgeVec.x = p2->x - p1->x;
      EdgeVec.y = p2->y - p1->y;

      // get the 2d edge vertex to point data
      VertToPoint.x = pPos->x - p1->x;
      VertToPoint.y = pPos->y - p1->y;

      // dot prod for "outside/inside" test using perp to edge_vec.
      if ((EdgeVec.y*VertToPoint.x-EdgeVec.x*VertToPoint.y) > 0) // Inside edge, so ignore.
         continue;

      // Determine closest dist to this edge.
      fDist2 = CalcDist2ToEdge(&VertToPoint, &EdgeVec, &Intersect); // intersect returned relative to p1.
      if (fDist2 < fClosestDist2)
      {
         fClosestDist2 = fDist2;
         *pIntersect = Intersect;
         pIntersect->x += p1->x; // put intersect back into world coords.
         pIntersect->y += p1->y;
      }
   }

   return !!(fClosestDist2 < fRad2);
}



STDMETHODIMP cAILVLMovReg::SuggestRegulations(cAIMoveSuggestions & suggestions)
{
   // If our timer hasn't expired, just reuse previously calculated data
   if (!m_Timer.Expired())
   {
      suggestions.Add(m_Suggestions);
      return S_FALSE;
   }

   m_Timer.Reset();

   // Kill my previous suggestions...
   m_Suggestions.DestroyAll();

   mxs_vector testPt;
   static floatang narrowField(DEGREES(40)), wideField(DEGREES(70));

   Position *    pPos = ObjPosGet(GetID());
   tAIPathCellID cell = m_pAIState->GetPathCell();

   Assert_(pPos);

   int        i;
   floatang   ang;
   sAIPathCell *pCell;
   sAIPathCellLink * pLink;

   //AISetPathcastIgnoreBlockingOBBs(TRUE);

   // For each cell linked to our current cell, ask if that cell is in the path. If not, and the cell is linked
   // via LVL, then avoid it.

   if (!m_pAIState->GetCurrentPath())
      return S_FALSE;

   pCell = g_AIPathDB.AccessCell(m_pAIState->GetPathCell());
   if (!pCell)
      return S_FALSE;

   const int lastCell = pCell->firstCell + pCell->cellCount;
   for (i = pCell->firstCell; i < lastCell; i++)
   {
      pLink = &g_AIPathDB.m_Links[i];

      if ((pLink->okBits & kAIOKCOND_Stressed) && !m_pAIState->GetCurrentPath()->IsCellInPath(pLink->dest))
      {
         // If we are not close enough to this cell, forget it.
         if (!GetCellEdgeIntersection(&pPos->loc.vec, pLink->dest, kAIAvoidWallsDist, &testPt))
            continue;

         ang.set(pPos->loc.vec.x, pPos->loc.vec.y, testPt.x, testPt.y);

      // Generate suggestion to avoid this cell. We need to find dist to nearest edge of cell, then use that as our X/Y vector.
         sAIMoveSuggestion * pSuggestion = new sAIMoveSuggestion;

         pSuggestion->SetWeightedBias(kAIMS_Danger, 50);
         pSuggestion->dirArc.SetByCenterAndSpan(ang, narrowField);
         pSuggestion->speed  = kAIS_Fast;

         m_Suggestions.Append(pSuggestion);

         pSuggestion = new sAIMoveSuggestion;

         pSuggestion->SetWeightedBias(kAIMS_Danger, 20);
         pSuggestion->dirArc.SetByCenterAndSpan(ang, wideField);
         pSuggestion->speed  = kAIS_Fast;

         m_Suggestions.Append(pSuggestion);
      }
   }

   //AISetPathcastIgnoreBlockingOBBs(FALSE);

   suggestions.Add(m_Suggestions);
   return S_OK;
}



///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIObjectsMovReg
//
// Responsible for making sure the AI doesn't run into walls
//

STDMETHODIMP_(const char *) cAIObjectsMovReg::GetName()
{
   return "Objects movement regulator";
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIObjectsMovReg::NewRegulations()
{
   return TRUE;
}

///////////////////////////////////////

#define kZConsiderRepel  6.0
#define kDistObjRepelIn  1.5
#define kDistObjRepelOut 4.5
#define kDistObjRepelRange (kDistObjRepelOut - kDistObjRepelIn)

static int CalculateObjectBias(float distance)
{
   int value = 0;

   if (distance >= kDistObjRepelOut)
      value = 0;
   else if (distance < kDistObjRepelIn)
      value = 100;
   else
   {
      value = (1.0 - ((distance - kDistObjRepelIn) / kDistObjRepelRange)) * 100.0;
   }

   if (value > 100.0)
      value = 100.0;
   if (value < 0.0)
      value = 0.0;

   return value;
}

///////////////////////////////////////

struct sRepelObj
{
   ObjID object;
   float distance;
};

///////////////////

DECLARE_TIMER(AI_OMR, Average);

STDMETHODIMP cAIObjectsMovReg::SuggestRegulations(cAIMoveSuggestions & suggestions)
{
   // If our timer hasn't expired, just reuse previously calculated data
   if (!m_Timer.Expired())
   {
      suggestions.Add(m_Suggestions);
      return S_FALSE;
   }

   m_Timer.Reset();

   AUTO_TIMER(AI_OMR);

   // Kill my previous suggestions...
   m_Suggestions.DestroyAll();

   // Generate new Suggestions
   const cMxsVector & loc = *m_pAIState->GetLocation();
   const ObjID *      pAvoidObjs;
   ObjID              avoidObj;
   ObjID              self = GetID();
   int                nAvoidObjs;
   mxs_vector         objLoc;
   float              distance;

   AIGetAllObjRepel(&pAvoidObjs, &nAvoidObjs);

   if (pAvoidObjs)
   {
      for (nAvoidObjs--; nAvoidObjs >= 0; nAvoidObjs--)
      {
         avoidObj = pAvoidObjs[nAvoidObjs];
         if (avoidObj == self)
            continue;
         if (GetObjLocation(pAvoidObjs[nAvoidObjs], &objLoc) &&
             ffabsf(objLoc.z - loc.z) < kZConsiderRepel / 2 &&
             (distance = AIXYDistance(loc, objLoc)) < kDistObjRepelOut)
         {
            sAIMoveSuggestion * pSuggestion = new sAIMoveSuggestion;

            pSuggestion->SetWeightedBias(kAIMS_Danger, CalculateObjectBias(distance));
            pSuggestion->dirArc.SetByCenterAndSpan(m_pAIState->AngleTo(objLoc),
                                                   floatang(DEGREES(55)));
            pSuggestion->speed = kAIS_Normal;

            m_Suggestions.Append(pSuggestion);
         }
      }
      suggestions.Add(m_Suggestions);
   }

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////

