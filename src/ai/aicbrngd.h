/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrngd.h,v 1.21 1999/12/02 18:54:35 BFarquha Exp $
//
//
//

#ifndef __AICBRNGD_H
#define __AICBRNGD_H

#include <aimultcb.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitRangedCombatAbility(IAIManager *);
BOOL AITermRangedCombatAbility();

///////////////////////////////////////////////////////////////////////////////

struct sAIProjectileRel;
struct sAIRangedCombatProp;

////////////////////////////////////////

enum eAIRangedCombatEvent
{
   kAIRCE_EventNull,
   kAIRCE_ImDamaged,
   kAIRCE_TargetDamaged,
   kAIRCE_Hit,
   kAIRCE_Stunned,
};

struct sPointBias
{
   ObjID objID;
   float bias;
};

#define MAX_POINT_BIASES  3

///////////////////////////////////////

#define RE_EVAL_CUR_DEST  (0.5)
#define RE_EVAL_CUR_LOC   (0.5)

#define CUR_LOC_SCORE_BIAS (0.7)

#define PROJ_REFRESH_RATE     kAIT_10Hz
#define LOC_DECAY_RATE        kAIT_10Hz
#define TACTICAL_UPDATE_RATE  kAIT_10Hz

#define DIST_AT_DEST_2  (5.0)

#define REALLY_BAD  -10000

///////////////////////////////////////

enum eAIRCMoveLocKind
{
   kAIRC_NoMove,

   kAIRC_Vantage,
   kAIRC_Cover,
   kAIRC_Target,
   kAIRC_Sidestep
};

///////////////////////////////////////////////////////////////////////////////

class cAIRangedSubcombat  : public cAISubcombat
{
public:
   cAIRangedSubcombat();

   STDMETHOD_(void, Init)();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();

   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);
   STDMETHOD_(BOOL, PostLoad)();

   // Notifications
   STDMETHOD_(void, OnBeginFrame)();
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   STDMETHOD_(void, OnModeChange)(eAIMode previous, eAIMode mode);
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);
   STDMETHOD_(void, OnDamage)(const sDamageMsg *pMsg, ObjID realCulpritID);

   // Ability flow
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

   // tell about events in the world (from weapon/damage listeners)
   void InformOfEvent(eAIRangedCombatEvent event) {};

private:

   void Reset();

   void RefreshProjectilesTime(int dt);
   void RefreshProjectilesProj(ObjID selected);

   void SelectProjectile(ObjID *projectile, sAIProjectileRel **proj_link_data, mxs_vector *targeting_location);
   BOOL ProjectileAvailable(sAIProjectileRel *proj_link_data);
   BOOL ProjectileHittable(int targeting_method, ObjID projectile, ObjID source,
                           ObjID target, mxs_vector *targeting_location);

   BOOL HasTargetLOS();

   void DecayLocation();

   void UpdateTacticalScore();

   void EvalCurDestPt();
   void EvalCurLocation();

   float EvaluateLoc(ObjID markerObj, const mxs_vector &pt_loc);

   /////////////////////////////////////

   virtual cAIAction *      CheckSuggestDamageAction(const cAIActions & previous);
   virtual cAIAction *      CheckSuggestMoveAction(sAIRangedCombatProp * pRangedProp, const cAIActions & previous);
   virtual cAIAction *      CheckSuggestAttackAction(sAIRangedCombatProp * pRangedProp, const cAIActions & previous);
   virtual cAIAction *      CheckSuggestIdleAction(const cAIActions & previous);

   BOOL                     CheckReevaluateMove();
   virtual eAIRCMoveLocKind SelectMarkerMoveLoc(mxs_vector * pLoc);
   virtual eAIRCMoveLocKind SelectMoveLoc(mxs_vector * pLoc);
   virtual cAIAction *      CreateActionForMoveKind(eAIRCMoveLocKind moveKind, const mxs_vector & loc);

   /////////////////////////////////////

   // @TODO: alignment

   int      m_ConsecutiveMisses;       // misses in a row with all projectiles
   cAITimer m_FiringDelay;             // delay between selecting shots

   cAITimer m_RefreshProjectileTimer;  // how often the time-since-selected state gets updated
   cAITimer m_LocationDecayTimer;      // how often we decay our current location
   cAITimer m_TacticalUpdateTimer;     // how often we capacitate our tactical score

   ObjID m_CurMarkerPt;     // The marker point we're at, OBJ_NULL if we're not at one
   ObjID m_CurDestPt;       // Our current destination

   enum eMoveStatus
   {
      kNoMoveStatus,
      kMovingToDest,                             // If we're moving to a destination
      kAtLoc,                                    // If we're stationary

      kMS_IntMax = 0xffffffff
   };

   eMoveStatus m_MoveStatus;

   float m_DestinationScore;     // The score of our destination, when we selected it
   float m_CurDestScore;         // The current score of our destination

   float m_LocationScore;   // The score of our location when we got there
   float m_CurLocScore;     // The score of our current location
   float m_CurLocDecayAmt;  // The amount that our current location is decayed

   BOOL  m_Attacking;        // Are we in the process of attacking?
   BOOL  m_MovingAttacking;  // Are we attacking while in the process of moving?

   float m_TacticalScore;      // How we're doing tactically (sort of morale)
   float m_BaseTacticalScore;  // The baseline tactical score that we return to

   sPointBias m_PointBiasList[MAX_POINT_BIASES];

   sAIRangedCombatProp *GetRangedCombatProp();
   sAIRangedCombatProp *m_RangedCombatProp;

   cAIRandomTimer m_ReactShootTimer;

   BOOL m_fDamaged;
   float m_fDamage;

#if 0
   ObjID    GetProjectile();
   mxs_real GetProjectileRange(ObjID projectile);

   cAITimer m_LocoPingTimer;
#endif
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAICombatRanged
//

class cAICombatRanged : public cAISimpleMultiCombat
{
public:
   cAICombatRanged();

private:
   cAIRangedSubcombat m_RangedSubcombat;
};

///////////////////////////////////////

inline cAICombatRanged::cAICombatRanged()
{
   SetSubcombat(&m_RangedSubcombat);
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AICBTRNG_H */


