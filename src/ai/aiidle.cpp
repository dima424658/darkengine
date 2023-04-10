/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiidle.cpp,v 1.25 2000/02/11 18:27:46 bfarquha Exp $
//
//
//

// #define PROFILE_ON 1

#include <lg.h>
#include <mprintf.h>

#include <mtagvals.h>
#include <propface.h>
#include <propbase.h>
#include <property.h>
#include <propert_.h>
#include <prophash.h>
#include <rendobj.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <aiactloc.h>
#include <aiactmot.h>
#include <aiactmov.h>
#include <aiactseq.h>
#include <aiactori.h>
#include <aiapimov.h>
#include <aiapibhv.h>
#include <aidfndpt.h>
#include <aigests.h>
#include <aigoal.h>
#include <aiidle.h>
#include <aiprabil.h>
#include <aitagtyp.h>

#include <crwpnapi.h>
#include <crwpnlst.h>

// Must be last header
#include <dbmem.h>

//////////////////////////////////////////////////////////////////////////////

#define kMinFidgetDelay 4000
#define kMaxFidgetDelay 9000

static cTag g_AIFidgetTags;

///////////////////////////////////////////////////////////////////////////////
//
// Idle directions
//

#define kAIMaxIdleDirs  4

struct sAIIdleDirs
{
   unsigned minFreq;
   unsigned maxFreq;

   struct sDir
   {
      int dir;
      int weight;
   };

   sDir dirs[kAIMaxIdleDirs];
};

///////////////////////////////////////

#define IDLE_DIR_FIELDS(i) \
   { "Facing " #i ": direction",    kFieldTypeInt,  FieldLocation(sAIIdleDirs, dirs[i-1].dir)                                                       }, \
   { "               weight",       kFieldTypeInt,  FieldLocation(sAIIdleDirs, dirs[i-1].weight)                                                      }

static sFieldDesc _g_AIIdleDirsFieldDesc[] =
{
   { "Min. facing switch time",         kFieldTypeInt,  FieldLocation(sAIIdleDirs, minFreq)                                                       }, \
   { "Max. facing switch time",         kFieldTypeInt,  FieldLocation(sAIIdleDirs, maxFreq)                                                       }, \

   IDLE_DIR_FIELDS(1),
   IDLE_DIR_FIELDS(2),
   IDLE_DIR_FIELDS(3),
   // IDLE_DIR_FIELDS(4)

};

static sStructDesc _g_AIIdleDirsStructDesc = StructDescBuild(sAIIdleDirs, kStructFlagNone, _g_AIIdleDirsFieldDesc);

///////////////////////////////////////

#undef INTERFACE
#define INTERFACE IAIIdleDirsProperty
DECLARE_PROPERTY_INTERFACE(IAIIdleDirsProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIIdleDirs *);
};

///////////////////

static sPropertyDesc _g_IdleDirsPropertyDesc =
{
   "AI_IdleDirs",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_ABILITY_CAT, "Idling: Directions" },
   kPropertyChangeLocally, // net_flags
};

///////////////////

typedef cSpecificProperty<IAIIdleDirsProperty, &IID_IAIIdleDirsProperty, sAIIdleDirs *, cHashPropertyStore< cClassDataOps<sAIIdleDirs> > > cAIIdleDirsPropertyBase;

class cAIIdleDirsProperty : public cAIIdleDirsPropertyBase
{
public:
   cAIIdleDirsProperty()
      : cAIIdleDirsPropertyBase(&_g_IdleDirsPropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIIdleDirs);
};

///////////////////

static IAIIdleDirsProperty * g_pAIIdleDirsProperty;

#define AIGetIdleDirs(obj) AIGetProperty(g_pAIIdleDirsProperty, (obj), (sAIIdleDirs *)NULL)

///////////////////////////////////////////////////////////////////////////////
//
// Idle origin
//

struct sAIIdleOrigin
{
   mxs_vector location;
   float      facing;
};

///////////////////////////////////////

static sFieldDesc _g_AIIdleOriginFieldDesc[] =
{
   { "Original Location", kFieldTypeVector,  FieldLocation(sAIIdleOrigin, location)                                                       }, \
   { "Original Facing",   kFieldTypeFloat,  FieldLocation(sAIIdleOrigin, facing)                                                       }, \

};

static sStructDesc _g_AIIdleOriginStructDesc = StructDescBuild(sAIIdleOrigin, kStructFlagNone, _g_AIIdleOriginFieldDesc);

///////////////////////////////////////

#undef INTERFACE
#define INTERFACE IAIIdleOriginProperty
DECLARE_PROPERTY_INTERFACE(IAIIdleOriginProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIIdleOrigin *);
};

///////////////////

static sPropertyDesc _g_IdleOriginPropertyDesc =
{
   "AI_IdleOrgn",
   kPropertyInstantiate,
   NULL, 0, 0,  // constraints, version
   { AI_STATE_CAT, "Idling: Origin" },
   kPropertyChangeLocally, // net_flags
};

///////////////////

typedef cSpecificProperty<IAIIdleOriginProperty, &IID_IAIIdleOriginProperty, sAIIdleOrigin *, cHashPropertyStore< cClassDataOps<sAIIdleOrigin> > > cAIIdleOriginPropertyBase;

class cAIIdleOriginProperty : public cAIIdleOriginPropertyBase
{
public:
   cAIIdleOriginProperty()
      : cAIIdleOriginPropertyBase(&_g_IdleOriginPropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIIdleOrigin);
};

///////////////////

static IAIIdleOriginProperty * g_pAIIdleOriginProperty;

#define AIGetIdleOrigin(obj) AIGetProperty(g_pAIIdleOriginProperty, (obj), (sAIIdleOrigin *)NULL)
#define AIClearIdleOrigin(obj)   g_pAIIdleOriginProperty->Delete(obj)

inline void AISetIdleOrigin(ObjID obj, const mxs_vector & location, float facing)
{
   sAIIdleOrigin origin;
   origin.location = location;
   origin.facing = facing;
   g_pAIIdleOriginProperty->Set(obj, &origin);
}

///////////////////////////////////////

static sPropertyDesc _g_IdleReturnToOriginPropertyDesc =
{
   "AI_IdlRetOrg",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_ABILITY_CAT, "Idle: Returns to origin" },
   kPropertyChangeLocally, // net_flags
};

static IBoolProperty * g_pAIIdleReturnToOriginProperty;

#define AIReturnsToIdleOrigin(obj) AIGetProperty(g_pAIIdleReturnToOriginProperty, (obj), (BOOL)TRUE)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitIdleAbility(IAIManager *)
{
   g_AIFidgetTags.Set("IdleGesture", kMTV_set);

   StructDescRegister(&_g_AIIdleDirsStructDesc);
   StructDescRegister(&_g_AIIdleOriginStructDesc);

   g_pAIIdleDirsProperty = new cAIIdleDirsProperty;
   g_pAIIdleOriginProperty = new cAIIdleOriginProperty;
   g_pAIIdleReturnToOriginProperty = CreateBoolProperty(&_g_IdleReturnToOriginPropertyDesc, kPropertyImplDense);

   return TRUE;
}

///////////////////////////////////////

BOOL AITermIdleAbility()
{
   SafeRelease(g_pAIIdleDirsProperty);
   SafeRelease(g_pAIIdleOriginProperty);
   SafeRelease(g_pAIIdleReturnToOriginProperty);

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIIdle
//

cAIIdle::cAIIdle()
 : m_GestureTimer(kMinFidgetDelay, kMaxFidgetDelay),
   m_DirTimer(0, 0),
   m_DefendTimer(kAIT_1Sec),
   m_TurnTimeout(AICustomTime(4000)),
   m_OriginTimer(AICustomTime(2*60*1000))
{
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAIIdle::GetName()
{
   return "Idling ability";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIIdle::Init()
{
   cAINonCombatAbility::Init();

   SetNotifications(kAICN_Defend | kAICN_SimStart | kAICN_GoalChange | kAICN_ActionProgress);
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIIdle::Save(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Idle, 0, 2, pTagFile))
   {
      BaseAbilitySave(pTagFile);
      m_GestureTimer.Save(pTagFile);
      m_DirTimer.Save(pTagFile);
      m_DefendTimer.Save(pTagFile);
      m_TurnTimeout.Save(pTagFile);
      m_OriginTimer.Save(pTagFile);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIIdle::Load(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Idle, 0, 2, pTagFile))
   {
      BaseAbilityLoad(pTagFile);
      m_GestureTimer.Load(pTagFile);
      m_DirTimer.Load(pTagFile);
      m_DefendTimer.Load(pTagFile);
      m_TurnTimeout.Load(pTagFile);
      m_OriginTimer.Load(pTagFile);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIIdle::OnSimStart()
{
   cAINonCombatAbility::OnSimStart();

   const sAIIdleDirs * pDirs = AIGetIdleDirs(GetID());

   if (pDirs && pDirs->maxFreq)
   {
      AssertMsg1(pDirs->minFreq < 0xffff && pDirs->maxFreq < 0xffff, "Idle direction frequency values for AI %d too large", GetID());
      m_DirTimer.Set(pDirs->minFreq, pDirs->maxFreq);
   }
   else
   {
      m_DirTimer.Set(0, 0);
   }

   // Switch to the idle weapon.
   if (CreatureWeaponObjSwizzle)
   {
      ((*CreatureWeaponObjSwizzle)(GetID(), kWeaponModeIdle));
   }
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIIdle::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   cAINonCombatAbility::OnGoalChange(pPrevious, pGoal);
   if (IsOwn(pGoal))
   {
      m_GestureTimer.Reset();
      m_OriginTimer.Force();
   }
   else if (IsLosingControl(pPrevious, pGoal) && !AIGetIdleOrigin(GetID()))
   {
      AISetIdleOrigin(GetID(), *m_pAIState->GetLocation(), RADIANS(m_pAIState->GetFacingAng().value));
   }
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIIdle::OnDefend(const sAIDefendPoint * pPoint)
{
   m_DefendTimer.Force();
   SignalGoal();
   SignalAction();
}

///////////////////////////////////////

DECLARE_TIMER(cAIIdle_OnActionProgress, Average);

STDMETHODIMP_(void) cAIIdle::OnActionProgress(IAIAction * pAction)
{
   AUTO_TIMER(cAIIdle_OnActionProgress);

   BOOL fSignal = FALSE;

   if (IsOwn(pAction))
   {
      BOOL fInProgress = pAction->InProgress();

      if (pAction->GetType() == kAIAT_Sequence)
      {
         if (!fInProgress)
         {
            m_OriginTimer.Reset();
            if (pAction->GetResult() == kAIR_Success)
            {
               AIClearIdleOrigin(GetID());
            }
            fSignal = TRUE;
         }
      }
      else
      {
         fSignal = (!fInProgress ||
                    (pAction->GetType() != kAIAT_Locomote && // i.e., running to defend (toml 11-04-98)
                     pAction->GetResult() == kAIR_NoResultSwitch &&
                     (m_GestureTimer.Expired() || m_TurnTimeout.Expired())));

         if (!fSignal && pAction->GetType() == kAIAT_Move)
         {
            const sAIMoveGoal & goal = ((cAIMoveAction *)pAction)->GetMoveGoal();
            floatarc arc;
            arc.SetByCenterAndSpan(goal.dir, DEGREES(20));
            fSignal = arc.contains(m_pAIState->GetFacingAng());
         }
      }
   }

   if (fSignal)
      SignalAction();
}

///////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIIdle::UpdateGoal(cAIGoal * pGoal)
{
   return cAINonCombatAbility::UpdateGoal(pGoal);
}

///////////////////////////////////////

STDMETHODIMP cAIIdle::SuggestGoal(cAIGoal * pPrevious, cAIGoal ** ppGoal)
{
   if (!AIGetIdleOrigin(GetID()))
      AISetIdleOrigin(GetID(), *m_pAIState->GetLocation(), RADIANS(m_pAIState->GetFacingAng().value));

   if (m_pAIState->GetMode() > kAIM_Normal)
   {
      *ppGoal = NULL;
      return S_FALSE;
   }

   *ppGoal = new cAIIdleGoal(this);
   (*ppGoal)->priority = kAIP_VeryLow;
   (*ppGoal)->object = GetID();
   (*ppGoal)->LocFromObj();

   SignalAction();
   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cAIIdle::SuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew)
{
// @TBD (toml 07-06-98):
   cAIAction * pAction = NULL;

   const sAIIdleOrigin * pOrigin;
   if (m_OriginTimer.Expired() &&
       AIReturnsToIdleOrigin(GetID()) &&
       (pOrigin = AIGetIdleOrigin(GetID())) != NULL)
   {
      cAISeqAction * pSeqAction = new cAISeqAction(this);
      cAILocoAction * pLocoOrigin = CreateLocoAction();
      cAIOrientAction * pOrientOrigin = CreateOrientAction();

      pLocoOrigin->Set(pOrigin->location);
      pOrientOrigin->Set(floatang(DEGREES(pOrigin->facing)));

      pSeqAction->Add(pLocoOrigin);
      pSeqAction->Add(pOrientOrigin);

      pLocoOrigin->Release();
      pOrientOrigin->Release();

      pAction = pSeqAction;
      m_DefendTimer.Reset();
      m_DirTimer.Reset();
      m_OriginTimer.Reset();
   }

   if (!pAction && m_DefendTimer.Expired())
   {
      const sAIDefendPoint * pDefend = AIGetDefendPoint(GetID());
      if (pDefend && !pDefend->fInRange)
      {
         cAILocoAction * pGotoDefend = CreateLocoAction();

         pGotoDefend->Set(pDefend->object,
                          (pDefend->returnSpeed) ? pDefend->returnSpeed : kAIS_Fast,
                          sq(pDefend->ranges[pDefend->iActive].radius),
                          pDefend->ranges[pDefend->iActive].height / 2);

         pAction = pGotoDefend;
         m_DefendTimer.Reset();
         m_DirTimer.Reset();
      }
   }

   if (!pAction && m_DirTimer.Expired())
   {
      const sAIIdleDirs * pDirs = AIGetIdleDirs(GetID());
      if (pDirs)
      {
         cDynArray<int> picks;

         for (int i = 0; i < kAIMaxIdleDirs; i++)
         {
            for (int j = pDirs->dirs[i].weight; j; j--)
               picks.Append(i);
         }
         if (picks.Size())
         {
            int pick = picks[AIRandom(0, picks.Size()-1)];
            cAIMoveAction * pTurn = CreateMoveAction();

            sAIMoveSuggestion * pSuggestion = new sAIMoveSuggestion;

            pSuggestion->SetWeightedBias(kAIMS_Loco, 100);
            pSuggestion->speed       = kAIS_Stopped;
            pSuggestion->dirArc.SetByCenterAndSpan(DEGREES(pDirs->dirs[pick].dir), DEGREES(1.0));

            pTurn->Add(pSuggestion);
            m_TurnTimeout.Reset();
            pAction = pTurn;
         }

      }
      m_DirTimer.Reset();
   }

   if (!pAction && rendobj_object_visible(GetID()) &&
        m_GestureTimer.Expired() && AIShouldIdleFidget(GetID()) &&
        m_pAIState->GetMode() > kAIM_Efficient)
   {
      IAIMoveEnactor * pMoveEnactor = m_pAI->AccessMoveEnactor();
      if (pMoveEnactor &&
          pMoveEnactor->SupportsMotionActions() &&
          pMoveEnactor->GetStatus() == kAIME_Idle)
      {
         pAction = m_pAI->AccessBehaviorSet()->CreateGestureAction(kAIGT_Fidget, this);
      }
   }

   if (pAction)
   {
      m_GestureTimer.Reset();
      pNew->Append(pAction);
   }

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
