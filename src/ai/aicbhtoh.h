/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbhtoh.h,v 1.35 2000/02/24 21:20:11 bfarquha Exp $
//

#ifndef __AICBHTOH_H
#define __AICBHTOH_H

#include <aicombat.h>
#include <ctagset.h>
#include <aimultcb.h>

#pragma once
#pragma pack(4)

typedef unsigned eAISoundConcept;
class cAISeqAction;
struct sAITargetInfo;

// build/clean up our properties
BOOL AIInitCombatHtoHAbility(IAIManager *);
BOOL AITermCombatHtoHAbility(void);

///////////////////////////////////////////////////////////////////////////////

enum eAIHtoHCombatEvent
{
   kAIHCE_EventNull,
   kAIHCE_ImDamaged,
   kAIHCE_TargetDamaged,
   kAIHCE_ImContacted,
   kAIHCE_TargetContacted,
   kAIHCE_Block,
   kAIHCE_OppAttackStart,
   kAIHCE_OppAttackEnd,
   kAIHCE_OppBlockStart,
   kAIHCE_OppBlockEnd,
   kAIHCE_Hit,
   kAIHCE_Hitting,
   kAIHCE_Stunned,
   kAIHCE_TooClose,
};

//////////////////////////

#define kModeTypeMask (((1<<16)-1)<<15)

enum eModeType
{
   kTypeAttack=(1<<16),
      kTypeDefend=(1<<17),
      kTypeBlocking=(1<<18),
      kTypeMove=(1<<19),
      kTypeJitter=(1<<20),
      kTypeRespond=(1<<21),
      kTypeMisc=(1<<22),
      kTypeDamage=(1<<23),
      kTypeOpCode=(1<<24),
      kTypeInterrupt=(1<<25),
      kTypeTargetHigh=(1<<26) // Kinda hacked. We want an AI to always swing high if you're 2.5 ft to 5 ft above him.
};

#define kModeBaseMask ((1<<16)-1)

enum eModeBase
{
   _kUndecided,
      _kCircleCW,
      _kCircleCCW,
      _kCharge,
      _kChargeUnseen,
      _kBackoff,
      _kJumpback,
      _kAdvance,
      _kAdvanceBig,
      _kAvoid,
      _kBlock,
      _kDirectedBlock,
      _kDodge,
      _kIdle,
      _kNoMove,
      _kSwingQuick,
      _kSwingNormal,
      _kSwingMassive,
      _kSwingSpecial,
      _kTakeDamage,
      _kFrustrate,

      kNumHtoHModes,
      _kHtoHModesIntMax = 0xffffffff            // force it use an int
};

enum eMode
{
   kUndefined     = 0,

      kUndecided     = _kUndecided|kTypeMisc|kTypeInterrupt,
      kCircleCW      = _kCircleCW|kTypeMove|kTypeJitter|kTypeRespond,
      kCircleCCW     = _kCircleCCW|kTypeMove|kTypeJitter|kTypeRespond,
      kCharge        = _kCharge|kTypeMove|kTypeInterrupt,
      kChargeUnseen  = _kChargeUnseen|kTypeMove|kTypeInterrupt,
      kFrustrate     = _kFrustrate|kTypeInterrupt,
      kBackoff       = _kBackoff|kTypeMove,
      kJumpback      = _kJumpback|kTypeJitter|kTypeDefend,  // should this be defend?  should we have "avoid"
      kAdvance       = _kAdvance|kTypeMove,
      kAdvanceBig    = _kAdvanceBig|kTypeMove,
      kAvoid         = _kAvoid|kTypeMisc,
      kBlock         = _kBlock|kTypeDefend|kTypeBlocking,
      kDirectedBlock = _kDirectedBlock|kTypeDefend|kTypeBlocking|kTypeOpCode,
      kDodge         = _kDodge|kTypeDefend|kTypeJitter,
      kIdle          = _kIdle|kTypeRespond|kTypeInterrupt,
      kNoMove        = _kNoMove|kTypeRespond|kTypeInterrupt, // @HACK ai gun
      kSwingQuick    = _kSwingQuick|kTypeAttack,
      kSwingNormal   = _kSwingNormal|kTypeAttack,
      kSwingMassive  = _kSwingMassive|kTypeAttack|kTypeTargetHigh, // @HACK: Massives are assumed to be overhand, so they can hit high.
      kSwingSpecial  = _kSwingSpecial|kTypeAttack,
      kTakeDamage    = _kTakeDamage|kTypeDamage|kTypeOpCode,

      kHtoHModesIntMax = 0xffffffff            // force it use an int
};

//////////////////////////
//
// For use in the choosing of attacks
//

typedef struct sModeSelection
{
   eMode        mode;
   int          wgt;      //  ie. how good/cool/smart this maneuver is
} sModeSelection;

//////////////////////////

#define kAIHC_MaxSelections 8

typedef struct sModeSelections
{
   unsigned       flags;
   sModeSelection selections[kAIHC_MaxSelections];
} sModeSelections;


//////////////////////////

typedef struct sHtoHModeSelections
{
   struct sResponses
   {
      sModeSelections OpponentAttack;
      sModeSelections LowHitpoints;
      sModeSelections Opening;
      sModeSelections TooCloseToOpponent;
      sModeSelections RemainingTooCloseToOpponent;
      sModeSelections FailedBackup;
      sModeSelections Cornered;
   };

   struct sAttacks
   {
      sModeSelections NormalWhileIdle;
      sModeSelections NormalWhileActive;
      sModeSelections OpponentAttacking;
      sModeSelections OpponentBlocking;
      sModeSelections OpponentUnarmed;
   };

   sResponses responses;
   sAttacks   attacks;
} sHtoHModeSelections;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIHtoHSubcombat
//

class cAIHtoHSubcombat : public cAISubcombat
{
public:
   cAIHtoHSubcombat();
   cAIHtoHSubcombat(sHtoHModeSelections * pModeSels);

   // Standard component methods
   STDMETHOD_(const char *, GetName)();

   STDMETHOD_(void, Init)();

   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   // parse the magic game events which tell us a block happened
   STDMETHOD_(void, OnGameEvent)(void *magic);
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   STDMETHOD_(void, OnBeginFrame)(void);
   STDMETHOD_(void, OnDamage)(const sDamageMsg *pMsg, ObjID realCulpritID);
   STDMETHOD_(void, OnWeapon)(eWeaponEvent ev, ObjID victim, ObjID culprit);
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);

   // Ability flow
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

   // tell about events in the world (from weapon/damage listeners)
   void InformOfEvent(eAIHtoHCombatEvent event);

#ifdef INCORRECT_AI_NETWORKING
   // Network routines.  Only public so they can be seen by the Init method.
   void SendAIRehostMessageTo(ObjID targetPlayer);
   void SendAILoseControlMessageTo(ObjID targetPlayer);
   static void HandleAIRehostMessage(const sNetMsg_Generic *pMsg, ulong size,
                                     ObjID fromPlayer, void * /* pClientData */);
   static void HandleAILoseControlMessage(const sNetMsg_Generic *pMsg, ulong size,
                                          ObjID fromPlayer, void * /* pClientData */);
#endif

protected:
   /////////////////////////////////////////////////////////////
   //
   // The attack mode system
   //

   enum eActPriority
   {
      kRnd,
      kLow,
      kNorm,
      kHigh,
      kMust
   };

   //////////////////////////

   static sHtoHModeSelections gm_DefaultModeSelections;

   //////////////////////////

   virtual sModeSelection * SolveResponse(sModeSelections & selections);

private:
   /////////////////////////////////////////////////////////////
   //
   // Mode queries
   //

   BOOL ModeTest(eMode m, eModeType t) const  { return (m&t)!=0; }
   BOOL ModeGetBase(eMode m) const    { return (m&kModeBaseMask); }

   BOOL IsAttacking(void) const    { return ModeTest(m_mode,kTypeAttack); }
   BOOL IsBlocking(void) const     { return ModeTest(m_mode,kTypeBlocking); }
   BOOL IsAvoiding(void) const     { return ModeTest(m_mode,kTypeDefend); }
   BOOL CanRespond(void) const     { return ModeTest(m_mode,kTypeRespond); }

   ///////////////////////
   // various internal utilities

   HRESULT SuggestActionsForMode(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

   virtual eMode ChooseNewAttackMode(ObjID target, const cMxsVector & targetLoc, float fZDist);

   // Can't get at player, so what to do?
   cAIAction *DoFrustration(const cAIActions & previous);

   // these are basically helper functions
   void  UpdateOppState(eAIHtoHCombatEvent ev);
   BOOL  CheckInterruptForEvent(eMode * newMode);
   void  PlayCombatSound(eAISoundConcept CombatSound, BOOL always);
   BOOL  CheckStat(eAIRating stat, eActPriority pri, float fac);
   void  CacheVisualDamageTags(BOOL big_hit);
   float StatWeightMode(sModeSelection *choice);

   cAISeqAction * CreateFrustrationAction(ObjID target, const mxs_vector &);

   ////////////////////////////////////
   // actual variables

   // mode control info
   eMode m_mode;
   int   m_ModeDuration;

   // distance squared as of our last check
   float m_lastDistSq;

   // set to -1 on init.. when we get gameevent telling us to dBlock, we set it
   //  then the suggest action next frame comes along and deals for us...
   int   m_DoDirectionBlock;

   // current state on the Opponent we are engaged with
   // @TODO: make this a bit field, save some memory, sheesh
   BOOL m_OppAttacking;
   BOOL m_OppBlocking;
   BOOL m_OppUnarmed;  // sadly, this really means "doesnt have a sword out"
   BOOL m_OppBehindMe;

   // for knowing about whether we can/should charge
   int        m_lastChargeTime;
   mxs_vector m_lastChargePos;
   tSimTime   m_failedChargePathTime;

   // for knowing about weather or not we should signal to flee.
   int m_lastFleeTime;

   // woo woo, try (and fail) to be less dippy
   int   m_BackoffCount;
   int   m_BackoffFail;

   // internal event queue
   #define AIHCE_EventQueueLen 32
   eAIHtoHCombatEvent m_EventQueue[AIHCE_EventQueueLen];
   int                m_EventQueuePtr;

   // for queueing taking damage
   BOOL               m_TakeDamage;
   cTagSet            m_DamageTags;

   cAIRandomTimer     m_FrustrationTimer;

   sHtoHModeSelections * m_pModeSels;

   BOOL CheckModeInterrupt(eMode *);
   eMode SelectMode();
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAICombatHtoH
//

class cAICombatHtoH : public cAISimpleMultiCombat
{
public:
   cAICombatHtoH();

private:
   cAIHtoHSubcombat m_HtoHSubcombat;
};

///////////////////////////////////////

inline cAICombatHtoH::cAICombatHtoH()
{
   SetSubcombat(&m_HtoHSubcombat);
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AICBHTOH_H */
