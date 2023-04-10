/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiinform.cpp,v 1.10 2000/02/11 18:27:47 bfarquha Exp $
//
//


// #define PROFILE_ON 1

#include <appsfx.h>
#include <propbase.h>
#include <property.h>
#include <propert_.h>
#include <prophash.h>
#include <psndinfo.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <aiactinv.h>
#include <aiactps.h>
#include <aiactseq.h>
#include <aialert.h>
#include <aiapiact.h>
#include <aiapiiai.h>
#include <aiapisnd.h>
#include <aiapisns.h>
#include <aiaware.h>
#include <aidebug.h>
#include <aigoal.h>
#include <aiinform.h>
#include <aiprops.h>
#include <aipsdscr.h>
#include <aisndtyp.h>
#include <aitagtyp.h>
#include <aiteams.h>

#include <aiprcore.h>

#include <config.h>

#include <dbmem.h>

F_DECLARE_INTERFACE(IAIInformResponseProperty);

#define kDefMaxPass 3
#define kDefTransferDelay 8000
#define kMinInformDist 20
#define kDefaultExpiration 90000

///////////////////////////////////////////////////////////////////////////////
//
// Inform response property
//

#define kAIMaxInfRspActs 8

struct sAIInformResponse
{
   BOOL         fIgnore;
   BOOL         fNoInvestigate;
   eAIPriority  priority;
   DWORD        reserved[4];
   sAIPsdScrAct acts[kAIMaxInfRspActs];
};

///////////////////////////////////////

#undef INTERFACE
#define INTERFACE IAIInformResponseProperty
DECLARE_PROPERTY_INTERFACE(IAIInformResponseProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIInformResponse *);
};

static IAIInformResponseProperty * g_pAIInformResponseProperty;

///////////////////

#define INFORM_CAT AI_ABILITY_CAT": Inform"

static sPropertyDesc _g_InformResponsePropertyDesc =
{
   "AI_InfRsp",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_ABILITY_CAT, "Inform: Response options" },
   kPropertyChangeLocally,  // net_flags
};

///////////////////

typedef cSpecificProperty<IAIInformResponseProperty, &IID_IAIInformResponseProperty, sAIInformResponse *, cHashPropertyStore< cClassDataOps<sAIInformResponse> > > cAIInformResponsePropertyBase;

class cAIInformResponseProperty : public cAIInformResponsePropertyBase
{
public:
   cAIInformResponseProperty()
      : cAIInformResponsePropertyBase(&_g_InformResponsePropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIInformResponse);
};

///////////////////

#define INFORM_RESPONSE_FIELDS(i) \
   AIPSDSCR_ARG_FIELD("Response: Step " #i, sAIInformResponse, acts[i-1])

static sFieldDesc _g_AIInformResponseFieldDesc[] =
{
   { "Ignore informs",     kFieldTypeBool,  FieldLocation(sAIInformResponse, fIgnore)                                                       },
   { "No default investigate",     kFieldTypeBool,  FieldLocation(sAIInformResponse, fNoInvestigate)                                                       },
   //{ "Priority", kFieldTypeEnum,   FieldLocation(sAIInformResponse, priority), kFieldFlagNone,     0, FieldNames(g_ppszAIPriority) },

   // @Note (toml 07-17-98): only showing 6 of 8 fields
   INFORM_RESPONSE_FIELDS(1),
   INFORM_RESPONSE_FIELDS(2),
#if 0
   INFORM_RESPONSE_FIELDS(3),
   INFORM_RESPONSE_FIELDS(4),
   INFORM_RESPONSE_FIELDS(5),
   INFORM_RESPONSE_FIELDS(6),
#endif
};

static sStructDesc _g_AIInformResponseStructDesc = StructDescBuild(sAIInformResponse, kStructFlagNone, _g_AIInformResponseFieldDesc);

///////////////////////////////////////

#define AIGetInformResponse(obj) AIGetProperty(g_pAIInformResponseProperty, obj, (sAIInformResponse *)NULL)

BOOL AIHasInformResponse(ObjID id)
{
   return g_pAIInformResponseProperty->IsRelevant(id);
}


///////////////////////////////////////

static sPropertyDesc _g_InformOthersPropertyDesc =
{
   "AI_InfOtr",
   0,
   NULL, 0, 0,  // constraints, version
   { INFORM_CAT, "Inform others" },
   kPropertyChangeLocally,  // net_flags
};

static IBoolProperty * g_pAIInformOthersProperty;

#define AIInformsOthers(obj) AIGetProperty(g_pAIInformOthersProperty, (obj), (BOOL)TRUE)

///////////////////////////////////////

static sPropertyDesc _g_InformPassPropertyDesc =
{
   "AI_InfMxP",
   0,
   NULL, 0, 0,  // constraints, version
   { INFORM_CAT, "Max. Passes" },
   kPropertyChangeLocally,  // net_flags
};

static IIntProperty * g_pAIInformPassProperty;

#define AIInformMaxPass(obj) AIGetProperty(g_pAIInformPassProperty, (obj), kDefMaxPass)

///////////////////////////////////////

static sPropertyDesc _g_InformDelayPropertyDesc =
{
   "AI_InfDly",
   0,
   NULL, 0, 0,  // constraints, version
   { INFORM_CAT, "Inform delay" },
   kPropertyChangeLocally,  // net_flags
};

static IIntProperty * g_pAIInformDelayProperty;

#define AIInformDelay(obj) AIGetProperty(g_pAIInformDelayProperty, (obj), kDefTransferDelay)

///////////////////////////////////////

static sPropertyDesc _g_InformMinDistPropertyDesc =
{
   "AI_InfDst",
   0,
   NULL, 0, 0,  // constraints, version
   { INFORM_CAT, "Min. dist to informed" },
   kPropertyChangeLocally,  // net_flags
};

static IIntProperty * g_pAIInformMinDistProperty;

#define AIInformDist(obj) AIGetProperty(g_pAIInformMinDistProperty, (obj), kMinInformDist)

///////////////////////////////////////

static sPropertyDesc _g_InformExpirationPropertyDesc =
{
   "AI_InfExp",
   0,
   NULL, 0, 0,  // constraints, version
   { INFORM_CAT, "Expiration" },
   kPropertyChangeLocally,  // net_flags
};

static IIntProperty * g_pAIInformExpirationProperty;

#define AIInformExpirationTime(obj) AIGetProperty(g_pAIInformExpirationProperty, (obj), kDefaultExpiration)

///////////////////////////////////////////////////////////////////////////////



BOOL AIInitInformAbility(IAIManager *)
{
   StructDescRegister(&_g_AIInformResponseStructDesc);

   g_pAIInformResponseProperty = new cAIInformResponseProperty;
   g_pAIInformOthersProperty = CreateBoolProperty(&_g_InformOthersPropertyDesc, kPropertyImplDense);

   g_pAIInformPassProperty = CreateIntProperty(&_g_InformPassPropertyDesc, kPropertyImplDense);
   g_pAIInformDelayProperty = CreateIntProperty(&_g_InformDelayPropertyDesc, kPropertyImplDense);
   g_pAIInformMinDistProperty = CreateIntProperty(&_g_InformMinDistPropertyDesc, kPropertyImplDense);
   g_pAIInformExpirationProperty = CreateIntProperty(&_g_InformExpirationPropertyDesc, kPropertyImplDense);

   return TRUE;
}

///////////////////////////////////////

BOOL AITermInformAbility()
{
   SafeRelease(g_pAIInformResponseProperty);
   SafeRelease(g_pAIInformOthersProperty);

   SafeRelease(g_pAIInformPassProperty);
   SafeRelease(g_pAIInformDelayProperty);
   SafeRelease(g_pAIInformMinDistProperty);
   SafeRelease(g_pAIInformExpirationProperty);

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

IAIInform * AIGetInform(IInternalAI * pAI)
{
   tAIIter        iter;
   IAIComponent * pComponent;
   IAIInform *    pInform = NULL;

   pComponent = pAI->GetFirstComponent(&iter);

   while (pComponent)
   {
      pComponent->QueryInterface(IID_IAIInform, (void **)&pInform);
      pComponent->Release();
      if (pInform)
         break;
      pComponent = pAI->GetNextComponent(&iter);
   }

   pAI->GetComponentDone(&iter);
   return pInform;
}


///////////////////////////////////////////////////////////////////////////////
//
//
//

sAIInform::sAIInform(IInternalAI * pAI, eAISoundConcept concept, ObjID ofWhom)
 : concept(concept),
   ofWhom(ofWhom)
{
   const sAIAwareness * pAwareness = pAI->AccessSenses()->GetAwareness(ofWhom);
   if (pAwareness)
   {
      if (pAwareness->ValidLastPos())
         loc     = pAwareness->lastPos;
      else
         loc     = kInvalidLoc;
      alertValue = pAwareness->level;
   }
   else
   {
      loc        = kInvalidLoc;
      alertValue = kAIAL_High;
   }

   locTime       = AIGetTime();
   maxPass       = AIInformMaxPass(pAI->GetObjID());
   transferDelay = AIInformDelay(pAI->GetObjID());
   expiration    = AIGetTime() + AIInformExpirationTime(pAI->GetObjID());
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIInform
//

cDynArray<cAIInform *> cAIInform::gm_InformAbilities;

///////////////////////////////////////

#define kAIBroadcastRate kAIT_2Sec
#define kAIRetellDelay   AICustomTime(12500)

cAIInform::cAIInform()
 : cCTDelegating<IAIInform>((cAIAbility *)this),
   m_Timer(kAIBroadcastRate),
   m_RetellDelay(kAIRetellDelay),
   m_fTriggered(FALSE)
{
   gm_InformAbilities.Append(this);
}

///////////////////////////////////////

cAIInform::~cAIInform()
{
   g_pAIForceImmediateInform->Delete(GetID());
   g_pAIInformSeenFrom->Delete(GetID());

   for (int i = 0; i < gm_InformAbilities.Size(); i++)
   {
      if (gm_InformAbilities[i] == this)
      {
         gm_InformAbilities.FastDeleteItem(i);
         break;
      }
   }
}

///////////////////////////////////////

STDMETHODIMP cAIInform::QueryInterface(REFIID id, void ** ppI)
{
   if (id == IID_IAIInform)
   {
      *ppI = (IAIInform *)this;
      cAIAbility::AddRef();
      return S_OK;
   }
   return cAIAbility::QueryInterface(id, ppI);
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIInform::GetName()
{
   return "Inform ability";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIInform::Init()
{
   if (m_pAI->AccessSenses() && m_pAI->AccessSoundEnactor())
      SetNotifications(kAICN_Sound | kAICN_ActionProgress | kAICN_Alertness);
   cAIAbility::Init();
}

///////////////////////////////////////

#define SUB_VER 1

STDMETHODIMP_(BOOL) cAIInform::Save(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Inform, 0, SUB_VER, pTagFile))
   {
      BaseAbilitySave(pTagFile);

      m_Timer.Save(pTagFile);
      AITagMove(pTagFile, &m_Current);
      AITagMove(pTagFile, &m_Received);
      m_RetellDelay.Save(pTagFile);
      AITagMoveDynarray(pTagFile, &m_Passes);
      AITagMove(pTagFile, &m_fTriggered);

      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////


STDMETHODIMP_(BOOL) cAIInform::Load(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Inform, 0, SUB_VER, pTagFile))
   {
      BaseAbilityLoad(pTagFile);
      m_Timer.Load(pTagFile);
      AITagMove(pTagFile, &m_Current);
      AITagMove(pTagFile, &m_Received);
      m_RetellDelay.Load(pTagFile);
      AITagMoveDynarray(pTagFile, &m_Passes);
      AITagMove(pTagFile, &m_fTriggered);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIInform::OnSound(const sSoundInfo * pSoundInfo, const sAISoundType * pType)
{
   if (pSoundInfo->SrcObject == GetID() || AITeamCompare(GetID(), pSoundInfo->SrcObject) != kAI_Teammates)
      return;
   if (pType->type == kAIST_Inform)
   {

      for (int i = 0; i < gm_InformAbilities.Size(); i++)
      {
         if (gm_InformAbilities[i]->GetID() == pSoundInfo->SrcObject)
         {
            if (!m_fTriggered)
            {
               BOOL notTold = TRUE;
               for (int j = 0; j < gm_InformAbilities[i]->m_Passes.Size(); j++)
               {
                  if (gm_InformAbilities[i]->m_Passes[j] == GetID())
                     notTold = FALSE;
               }
               if (notTold)
               {
                  AIWatch2(Inform, GetID(), "Informed of %s by %d!\n", AIGetConceptName(gm_InformAbilities[i]->m_Current.concept), gm_InformAbilities[i]->GetID());
                  gm_InformAbilities[i]->m_Passes.Append(GetID());
                  Receive(&(gm_InformAbilities[i]->m_Current));
               }
            }
            break;
         }
      }
   }
}

///////////////////////////////////////

DECLARE_TIMER(cAIInform_OnActionProgress, Average);

STDMETHODIMP_(void) cAIInform::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAIInform_OnActionProgress);

   if (m_pAIState->GetMode() != kAIM_Normal || !m_pAI->AccessSoundEnactor())
      return;

   // If we're in a good position to interrupt...
   if (IsOwn(pAction) && !pAction->InProgress())
   {
      m_fTriggered = FALSE;
      SignalGoal();
   }

   if (m_Current.concept)
   {
      if (AIGetTime() > m_Current.expiration ||
          m_Passes.Size() >= m_Current.maxPass)
      {
         AIWatch1(Inform, GetID(), "Done informing: %s", (AIGetTime() > m_Current.expiration) ? "expired" : "maximum passes");
         m_Current.concept = 0;
         m_Passes.SetSize(0);
      }
   }

   if (m_Current.concept &&
       ((m_RetellDelay.Expired() && m_Timer.Expired()) || g_pAIForceImmediateInform->IsRelevant(GetID())) &&
       pAction->GetResult() > kAIR_NoResult)
   {
      const sAIAwareness * pAwarenessOfRecent = m_pAI->AccessSenses()->GetAwareness(m_Current.ofWhom);

      if (!pAwarenessOfRecent ||
          (m_Current.transferDelay < pAwarenessOfRecent->TimeSinceContact()) ||
          g_pAIForceImmediateInform->IsRelevant(GetID()))
      {
         cAIAwareArray awarenesses;
         m_pAI->AccessSenses()->GetAllAwareness(&awarenesses);
         float minInformDistSq = AIInformDist(GetID());
         minInformDistSq = sq(minInformDistSq);

         g_pAIForceImmediateInform->Delete(GetID());

         for (int i = 0; i < awarenesses.Size(); i++)
         {
            if ((awarenesses[i]->flags & kAIAF_Seen) &&
                GetID() != awarenesses[i]->object &&
                AITeamCompare(GetID(), awarenesses[i]->object) == kAI_Teammates)
            {
               BOOL notTold = TRUE;
               for (int j = 0; j < m_Passes.Size(); j++)
               {
                  if (m_Passes[j] == awarenesses[i]->object)
                     notTold = FALSE;
               }

               if (notTold && m_pAIState->DistSq(*GetObjLocation(awarenesses[i]->object)) < minInformDistSq)
               {
                  AIWatch3(Inform, awarenesses[i]->object, "AI %d informing %d (at least) of \"%s\"", GetID(), awarenesses[i]->object, AIGetConceptName(m_Current.concept));
                  AIWatch2(Inform, GetID(), "Informing %d (at least) of \"%s\"", awarenesses[i]->object, AIGetConceptName(m_Current.concept));

                  const sAIAwareness * pAwareness = m_pAI->AccessSenses()->GetAwareness(m_Current.ofWhom);
                  if (pAwareness && pAwareness->ValidLastPos())
                     m_Current.loc = pAwareness->lastPos;

                  if (m_pAI->AccessSoundEnactor()->RequestConcept(m_Current.concept))
                  {
                     m_RetellDelay.Reset();
                     break;
                  }
                  else
                     AIWatch1(Inform, GetID(), "Failed to say \"%s\"", AIGetConceptName(m_Current.concept));
               }
            }
         }
      }
      m_Timer.Reset();
   }

   cAIAbility::OnActionProgress(pAction);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIInform::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   if (!IsOwn(pGoal))
   {
      m_fTriggered = FALSE;
      ClearNotifications(kAICN_GoalChange);
      g_pAIInformSeenFrom->Delete(GetID());
   }
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIInform::OnAlertness(ObjID ofWhom, eAIAwareLevel previous,
                                           eAIAwareLevel current, const sAIAlertness * pRaw)
{
   if (current <= previous)
      return;

   switch (current)
   {
      case kAIAL_Lowest:
      case kAIAL_Low:
         break;

      case kAIAL_Moderate:
      {
#if 0
         sAIInform modInform(m_pAI, kAISC_RecentOther, ofWhom);
         Post(&modInform);
#endif
         break;
      }

      case kAIAL_High:
      {
         if (AITeamCompare(GetID(), ofWhom) == kAI_Opponents)
         {
            sAIInform Inform(m_pAI, kAISC_RecentIntruder, ofWhom);
            Post(&Inform);
         }
         else if (GetID() != ofWhom && AITeamCompare(GetID(), ofWhom) == kAI_Teammates)
         {
            sAIInform Inform(m_pAI, kAISC_RecentBody, ofWhom);
            Post(&Inform);
         }
         else
         {
            sAIInform Inform(m_pAI, kAISC_RecentOther, ofWhom);
            Post(&Inform);
         }
         break;
      }
   }
}

///////////////////////////////////////

STDMETHODIMP cAIInform::SuggestGoal(cAIGoal * pPrevGoal, cAIGoal ** ppGoal)
{
   if (!m_fTriggered)
   {
      *ppGoal = NULL;
      return S_FALSE;
   }

   *ppGoal = new cAIInvestigateGoal(this);

   (*ppGoal)->priority  = kAIP_Normal;
   (*ppGoal)->object    = m_Received.ofWhom;
   if (m_Received.bUseLocOnly)
      (*ppGoal)->location  = m_Received.OriginLoc;
   else
      (*ppGoal)->location  = m_Received.loc;

   SetNotifications(kAICN_GoalChange);

   SignalAction();

   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cAIInform::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
   const sAIInformResponse * pResponse = AIGetInformResponse(GetID());

   cAISeqAction * pSeqAction = new cAISeqAction(this);

   if (pResponse)
   {
      cAIPsdScrAction * pPsdScrAction = CreatePsdScrAction();

      pPsdScrAction->Set(pResponse->acts, kAIMaxInfRspActs);
      pSeqAction->Add(pPsdScrAction);
      pPsdScrAction->Release();
   }

   if (!pResponse || !pResponse->fNoInvestigate)
   {
      cAIInvestAction * pInvestAction = CreateInvestAction();
      pInvestAction->SetNearPath(TRUE, 10); // hard-coded to path within 10 ft. if can't actually path to.

      if (pGoal->object)
         pInvestAction->Set(pGoal->location, pGoal->object, kAIS_VeryFast);
      else
         {
         pInvestAction->SetCheckPathDist(FALSE); // Don't check dist to dest. Just go there.
         pInvestAction->Set(pGoal->location, kAIS_VeryFast);
         }

      pInvestAction->SetDuration(15000);
      pSeqAction->Add(pInvestAction);
      pInvestAction->Release();
   }

   pNew->Append(pSeqAction);

   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIInform::WantsInforms()
{
   const sAIInformResponse * pResponse = AIGetInformResponse(GetID());
   return (!pResponse || !pResponse->fIgnore);
}

///////////////////////////////////////

STDMETHODIMP cAIInform::Post(const sAIInform * pInform)
{
   if (!m_pAI->AccessSenses())
      return S_FALSE;

   if (!AIInformsOthers(GetID()))
      return S_FALSE;

   const sAIAwareness * pAwarenessOfSource = m_pAI->AccessSenses()->GetAwareness(pInform->ofWhom);
   if (!pAwarenessOfSource ||
       !(pAwarenessOfSource->flags & kAIAF_FirstHand))
      return S_FALSE;

   if (!m_Current.concept || m_Current.concept < pInform->concept)
   {
      AIWatch1(Inform, GetID(), "Inform posted \"%s\"", AIGetConceptName(pInform->concept));
      m_Current = *pInform;
      m_Current.OriginLoc = *GetObjLocation(GetID());
   }
   else
      AIWatch2(Inform, GetID(), "Inform posted \"%s\", but have \"%s\"", AIGetConceptName(pInform->concept), AIGetConceptName(m_Current.concept));

   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cAIInform::Receive(const sAIInform * pInform)
{
   if (InControl() || m_fTriggered)
      return S_FALSE;

   const sAIInformResponse * pResponse = AIGetInformResponse(GetID());

   if (pResponse && pResponse->fIgnore)
   {
      AIWatch(Inform, GetID(), "Ignoring inform based on property setting");
      return S_FALSE;
   }

   m_fTriggered = TRUE;
   m_Received = *pInform;

   if (g_pAIInformSeenFrom->IsRelevant(GetID()))
   {
      g_pAIInformSeenFrom->Delete(GetID());
      m_Received.ofWhom = 0;
      m_Received.bUseLocOnly = TRUE;
   }
   else
      m_Received.bUseLocOnly = FALSE;

   SignalGoal();

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
