/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactmrk.cpp,v 1.12 2000/02/11 18:27:17 bfarquha Exp $
//
// AI Action - Move to Marker
//

// #define PROFILE_ON 1

#include <property.h>
#include <propbase.h>

#include <relation.h>
#include <lnkquery.h>
#include <linkbase.h>

#include <config.h>

#include <objpos.h>
#include <wrtype.h>
#include <wrfunc.h>
#include <port.h>

#include <aiprutil.h>
#include <aiactmrk.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

static IRelation * g_pAIRetreatPtDibsLinks;
static IRelation * g_pAIVantagePtDibsLinks;

///////////////////////////////////////

BOOL AIInitMarkerAction()
{
   sRelationDesc     retreatPtDibsDesc = { "AIRetreatPtDibs", kRelationNoEdit | kRelationNetworkLocalOnly, 0, 0 };
   sRelationDesc     vantagePtDibsDesc = { "AIVantagePtDibs", kRelationNoEdit | kRelationNetworkLocalOnly, 0, 0 };
   sRelationDataDesc noDataDesc        = { "None", 0 };

   g_pAIRetreatPtDibsLinks = CreateStandardRelation(&retreatPtDibsDesc, &noDataDesc, (kQCaseSetSourceKnown | kQCaseSetDestKnown));
   g_pAIVantagePtDibsLinks = CreateStandardRelation(&vantagePtDibsDesc, &noDataDesc, (kQCaseSetSourceKnown | kQCaseSetDestKnown));

   return TRUE;
}

///////////////////////////////////////

BOOL AITermMarkerAction()
{
   SafeRelease(g_pAIVantagePtDibsLinks);
   SafeRelease(g_pAIRetreatPtDibsLinks);

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMoveToMarkerAction
//

cAIMoveToMarkerAction::~cAIMoveToMarkerAction()
{
}


BOOL cAIMoveToMarkerAction::Set(eAIMarkerType markerType, sMarkerParams &markerParams)
{
#if 0
   // Alias to the marker type in which we're interested
   switch (markerType)
   {
      case eRetreatPoint:
         m_pCurProperty = g_pAIRetreatPointProperty;
         m_pCurRelation = g_pAIRetreatPtDibsLinks;
         break;

      case eVantagePoint:
         m_pCurProperty = g_pAIVantagePointProperty;
         m_pCurRelation = g_pAIVantagePtDibsLinks;
         break;

      case eMeleePoint:
         Warning(("cAIMoveToMarkerAction::Set: eMeleePoint is not supported\n"));
         return FALSE;
   }
#endif
   return FALSE;

   // Iterate over all objects of that type
   sPropertyObjIter iter;
   ObjID            iterPoint;
   int              pointValue;

   ObjID pointDibs = OBJ_NULL;

   ObjID bestPointDibs = OBJ_NULL;
   ObjID bestPoint     = OBJ_NULL;
   float bestRating    = -10000;
   float bestPtDist;

   m_pCurProperty->IterStart(&iter);
   while (m_pCurProperty->IterNext(&iter, &iterPoint))
   {
      if (!OBJ_IS_CONCRETE(iterPoint))
         continue;

      m_pCurProperty->Get(iterPoint, &pointValue);

      // Check if someone already has dibs on it
      sLink link;
      ILinkQuery *query = m_pCurRelation->Query(LINKOBJ_WILDCARD, iterPoint);
      if (!query->Done())
      {
         query->Link(&link);
         pointDibs = link.source;
      }
      else
         pointDibs = OBJ_NULL;
      SafeRelease(query);

      if ((pointDibs == OBJ_NULL) || (pointDibs == m_pAIState->GetID()))
      {
         // Either it's free or we already have dibs on it
         Assert_(ObjPosGet(iterPoint));
         Assert_(ObjPosGet(markerParams.target));

         mxs_vector pointLoc  = ObjPosGet(iterPoint)->loc.vec;
         mxs_vector targetLoc = ObjPosGet(markerParams.target)->loc.vec;

         mxs_real point_dist_ai     = mx_dist_vec(&pointLoc, m_pAIState->GetLocation());
         mxs_real point_dist_target = mx_dist_vec(&pointLoc, &targetLoc);

         if ((point_dist_target < markerParams.maxDistance) &&
             (point_dist_target > markerParams.minDistance))
         {
            mxs_real dist_ai_rating;
            mxs_real dist_targ_rating;
            mxs_real los_rating;
            mxs_real value_rating;
            mxs_real total_rating;

            dist_ai_rating = -fabs(point_dist_target - markerParams.idealDistance) * markerParams.toTargetBias;
            dist_targ_rating = -point_dist_ai * markerParams.toAIBias;

            // Check for a LOS
            Location start, end, hit;

            MakeHintedLocationFromVector(&start, &pointLoc, &ObjPosGet(iterPoint)->loc);
            MakeLocationFromVector(&end, &targetLoc);

            if (PortalRaycast(&start, &end, &hit, 0))
               los_rating = markerParams.LOSBias;
            else
               los_rating = 0;

            value_rating = pointValue * markerParams.markerValueBias;

            total_rating = dist_ai_rating + dist_targ_rating + los_rating + value_rating;

            if (config_is_defined("RangedSpew"))
               mprintf(" - %d - rating: %g\n", iterPoint, total_rating);

            if (total_rating > bestRating)
            {
               bestPoint = iterPoint;
               bestPointDibs = pointDibs;
               bestRating = total_rating;
               bestPtDist = point_dist_ai;
            }
         }
      }
   }

   m_pCurProperty->IterStop(&iter);

   if ((bestPoint != OBJ_NULL) && ((bestPointDibs != m_pAIState->GetID()) || (bestPtDist > 5.0)))
   {
      // Remove any previous dibs
      ILinkQuery *query = m_pCurRelation->Query(m_pAIState->GetID(), LINKOBJ_WILDCARD);
      for (; !query->Done(); query->Next())
         m_pCurRelation->Remove(query->ID());

      // Link to the new point
      m_pCurRelation->Add(m_pAIState->GetID(), bestPoint);

      SafeRelease(query);

      m_speed = markerParams.speed;

      return TRUE;
   }
   else
      return FALSE;
}

STDMETHODIMP_(eAIResult) cAIMoveToMarkerAction::Update()
{
   Assert_(m_pLocoAction);

   if (InProgress())
   {
      result = m_pLocoAction->Update();
      return result;
   }

   return kAIR_Success;
}

DECLARE_TIMER(cAIMoveToMarkerAction_Enact, Average);

STDMETHODIMP_(eAIResult) cAIMoveToMarkerAction::Enact(ulong deltaTime)
{
   AUTO_TIMER(cAIMoveToMarkerAction_Enact);

   Assert_(m_pCurProperty);
   Assert_(m_pCurRelation);

   if (!Started())
   {
      // Find our dibs point
      ILinkQuery *query = m_pCurRelation->Query(m_pAIState->GetID(), LINKOBJ_WILDCARD);
      if (!query->Done())
      {
         sLink link;
         query->Link(&link);

         m_pLocoAction = CreateLocoAction(ownerData);
         m_pLocoAction->Set(ObjPosGet(link.dest)->loc.vec, m_speed);

         SafeRelease(query);
      }

      SetStarted(TRUE);
   }

   if (m_pLocoAction)
      return m_pLocoAction->Enact(deltaTime);
   else
      return kAIR_Success;
}







