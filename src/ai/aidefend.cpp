/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aidefend.cpp,v 1.9 2000/02/11 18:27:37 bfarquha Exp $
//
//
//

// #define PROFILE_ON 1

#include <lg.h>

#include <linkbase.h>
#include <relation.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <aiapiact.h>
#include <aidefend.h>
#include <aidfndpt.h>
#include <aitagtyp.h>

#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// Defense point links
//

static IRelation * g_pAIDefendLinks;

//////////////////////////////////////

#define DEFEND_RANGE_FIELDS(i) \
   { "Range " #i ": Radius",        kFieldTypeInt,   FieldLocation(sAIDefendPoint, ranges[i - 1].radius)                                                                  }, \
   { "         Height",             kFieldTypeInt,   FieldLocation(sAIDefendPoint, ranges[i - 1].height)                                                                  }, \
   { "         Minimum alertness",  kFieldTypeEnum,  FieldLocation(sAIDefendPoint, ranges[i - 1].minAlert), kFieldFlagNone, 0, FieldNames(g_ppszAIAwareAlertLevels) },\
   { "         Maximum alertness",  kFieldTypeEnum,  FieldLocation(sAIDefendPoint, ranges[i - 1].maxAlert), kFieldFlagNone, 0, FieldNames(g_ppszAIAwareAlertLevels) }

static sFieldDesc _g_AIDefendPointFieldDesc[] =
{
   AI_SPEED_FIELD("Return speed", sAIDefendPoint, returnSpeed),

   DEFEND_RANGE_FIELDS(1),
   DEFEND_RANGE_FIELDS(2),
   DEFEND_RANGE_FIELDS(3)
};

static sStructDesc _g_AIDefendPointStructDesc = StructDescBuild(sAIDefendPoint, kStructFlagNone, _g_AIDefendPointFieldDesc);

//////////////////////////////////////
//
// Get the active defend point. Returns NULL if none are active.
//

const sAIDefendPoint * AIGetDefendPoint(ObjID objId)
{
   LinkID linkId = g_pAIDefendLinks->GetSingleLink(objId, LINKOBJ_WILDCARD);

   if (!linkId)
      return NULL;

   sAIDefendPoint * pPoint = (sAIDefendPoint *)g_pAIDefendLinks->GetData(linkId);
   if (!pPoint->object || pPoint->iActive == kAINoDefend)
      return NULL;

   return pPoint;
}

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitDefendAbility(IAIManager *)
{
   sRelationDesc     DefendPointDesc     = { "AIDefendObj", kRelationNetworkLocalOnly, 0, 0 };
   sRelationDataDesc DefendPointDataDesc = { "sAIDefendPoint", sizeof(sAIDefendPoint), kRelationDataAutoCreate };

   StructDescRegister(&_g_AIDefendPointStructDesc);

   g_pAIDefendLinks = CreateStandardRelation(&DefendPointDesc, &DefendPointDataDesc, (kQCaseSetSourceKnown|kQCaseSetDestKnown));

   return TRUE;
}

///////////////////////////////////////

BOOL AITermDefendAbility()
{
   SafeRelease(g_pAIDefendLinks);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDefend
//

cAIDefend::cAIDefend()
 : m_Timer(kAIT_2Sec),
   m_fDefending(FALSE)
{
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIDefend::GetName()
{
   return "Defend component";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIDefend::Init(void)
{
   SetNotifications(kAICN_ActionProgress);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIDefend::Save(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Defend, 0, 0, pTagFile))
   {
      SaveNotifications(pTagFile);

      m_Timer.Save(pTagFile);
      AITagMove(pTagFile, &m_fDefending);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIDefend::Load(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Defend, 0, 0, pTagFile))
   {
      LoadNotifications(pTagFile);

      m_Timer.Load(pTagFile);
      AITagMove(pTagFile, &m_fDefending);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIDefend::OnAlertness(ObjID source, eAIAwareLevel previous, eAIAwareLevel current, const sAIAlertness * pRaw)
{
   cAIComponent::OnAlertness(source, previous, current, pRaw);
   m_Timer.Reset();
   Seek();
}

///////////////////////////////////////

DECLARE_TIMER(cAIDefend_OnActionProgress, Average);

STDMETHODIMP_(void) cAIDefend::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAIDefend_OnActionProgress);

   if (pAction->GetResult() > kAIR_NoResult && m_Timer.Expired())
   {
      m_Timer.Reset();
      Seek();
   }
}

///////////////////////////////////////

void cAIDefend::Seek()
{
   LinkID linkId = g_pAIDefendLinks->GetSingleLink(GetID(), LINKOBJ_WILDCARD);

   if (!linkId)
   {
      if (m_fDefending)
      {
         m_pAI->NotifyDefend(NULL);
         m_fDefending = FALSE;
      }
      return;
   }

   sLink link;
   sAIDefendPoint * pPoint = (sAIDefendPoint *)g_pAIDefendLinks->GetData(linkId);
   g_pAIDefendLinks->Get(linkId, &link);

   if (!pPoint->object)
   {
      m_fDefending = FALSE;
      pPoint->iActive = kAINoDefend;
   }

   pPoint->object = link.dest;
   pPoint->linkID = linkId;

   GetObjLocation(pPoint->object, &pPoint->location);

   AssertMsg((m_fDefending && pPoint->iActive != kAINoDefend) || (!m_fDefending && pPoint->iActive == kAINoDefend), "Defend inconsistency");

   eAIAlertLevel alertness = m_pAIState->GetAlertness();

   // Find the first interesting range
   for (int i = 0; i < kAIMaxDfndRanges; i++)
   {
      if (pPoint->ranges[i].radius &&
          alertness >= pPoint->ranges[i].minAlert &&
          alertness <= pPoint->ranges[i].maxAlert)
      {
         break;
      }
   }

   int iPrevActive = pPoint->iActive;
   pPoint->iActive = i;

   if (i == kAINoDefend)
   {
      if (m_fDefending)
      {
         m_pAI->NotifyDefend(NULL);
         m_fDefending = FALSE;
      }
      return;
   }
   else
   {
      pPoint->fInRange = AIInsideCylinder(*m_pAIState->GetLocation(),
                                          pPoint->location,
                                          sq(pPoint->ranges[i].radius),
                                          pPoint->ranges[i].height / 2,
                                          &pPoint->distSq);
   }

   if (!m_fDefending || iPrevActive != pPoint->iActive)
   {
      m_pAI->NotifyDefend(pPoint);
   }

   m_fDefending = TRUE;
}

///////////////////////////////////////////////////////////////////////////////
