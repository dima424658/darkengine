/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrnew.h,v 1.3 1999/05/31 14:12:36 JON Exp $
//
//
//

#ifndef __AICBRNEW_H
#define __AICBRNEW_H

#include <aicbrtyp.h>

#include <aimultcb.h>
#include <aicbrloc.h>
#include <aicbrmds.h>
#include <aiprrngd.h>
#include <aisndtyp.h>

#include <physcast.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitNewRangedCombatAbility(IAIManager *);
BOOL AITermNewRangedCombatAbility();

////////////////////////////////////////

class cPhysModel;

////////////////////////////////////////

const int kAIRC_MaxInstalledModes = kAIRC_NumModes; // max number of modes 

const int kAIRC_MaxEvents = 10;        // maximum events in queue

///////////////////////////////////////////////////////////////////////////////

class cAINewRangedSubcombat  : public cAISubcombat
{
friend class cAIRangedBackup;
friend class cAIRangedClose;
friend class cAIRangedMode;
friend class cAIRangedIdle;
friend class cAIRangedShoot;
friend class cAIRangedWound;
friend class cAIRangedVantage;
friend class cAIRangedStepRight; 
friend class cAIRangedStepLeft;
friend class cAIRangedFlee;

public:
   cAINewRangedSubcombat();
   ~cAINewRangedSubcombat();

   void SetPhyscastCallback(tPhyscastObjTest callback);

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

   // Mode access functions
   // @TODO: add these to a new interface
   ObjID GetID(void) const;

   const sAITargetInfo* GetTargetInfo(void) const;
   eAIRangedCombatRange ClassifyRange(float range) const;
   const sAIRangedRanges* GetRangesSq(void) const;
   
   cAIRangedLocation* GetCurrentLocation(void);

   BOOL Pathcast(const mxs_vector& destVec, tAIPathCellID* pEndCell = NULL);

   ObjID GetDefaultProjectile(void) const;
   int ProjectileHittable(int targetMethod, ObjID projectileID, const Location& sourceLoc, ObjID targetID,
      mxs_vector* pTargetVec) const;
   int ProjectileHittable(int targetMethod, ObjID projectileID, ObjID sourceID, ObjID targetID,
      mxs_vector* pTargetVec) const;

protected:

   ///////////////////////////////////////////
   // my new functions
   //

   void UpdateData(void);

   int SelectNewMode(BOOL modesFailed[]);
   int SelectInterruptMode(BOOL modesFailed[], eAIRangedModePriority priority);

   ///////////////////////////////////////////

   void Reset();

   void RefreshProjectilesTime(int dt);
   void RefreshProjectilesProj(ObjID selected);

   void SelectProjectile(ObjID *projectile, sAIProjectileRel **proj_link_data, mxs_vector *targeting_location);
   BOOL ProjectileAvailable(sAIProjectileRel *proj_link_data);

   BOOL HasTargetLOS();

   cAIMotionAction* CreateFrustrationAction(void);

   /////////////////////////////////////
   // modes

   cAIRangedMode* m_pModes[kAIRC_MaxInstalledModes];   // @TODO: dynarray
   int m_numberModes;
   int m_currentMode;

   BOOL IsMoving(void) const;

   /////////////////////////////////////
   // sound

   BOOL PlaySound(eAISoundConcept CombatSound);

   /////////////////////////////////////
   // cached properties

   sAIRangedCombatProp *m_pRangedCombatProp;
   sAIRangedRanges *m_pRanges;

   /////////////////////////////////////
   // events

   void InformOfEvent(eAINewRangedCombatEvent event, BOOL bSignal, void* pData = NULL);
   void ClearEvents(void);
   BOOL FindEvent(eAINewRangedCombatEvent event);
   sAIRangedCombatEvent* GetEvent(eAINewRangedCombatEvent event);

   sAIRangedCombatEvent m_events[kAIRC_MaxEvents];
   int m_numEvents;

   /////////////////////////////////////
   // range info

   eAIRangedCombatRange m_rangeClass;
   sAIRangedRanges m_rangesSq;

   /////////////////////////////////////
   // movement data

   cAIRangedLocation* m_pCurrentLocation;

   /////////////////////////////////////
   // LOF

   BOOL ProjectileHittable(int targetMethod, ObjID projectileID, const Location& sourceLoc, 
                           ObjID targetID, int targetSubModel, mxs_vector* pTargetVec) const;

   /////////////////////////////////////
   // shooting data

   int      m_ConsecutiveMisses;       // misses in a row with all projectiles
   cAITimer m_FiringDelay;             // delay between selecting shots
   cAIRandomTimer m_ReactShootTimer;

   cAITimer m_RefreshProjectileTimer;  // how often the time-since-selected state gets updated

   tPhyscastObjTest m_physcastCallback;
};

///////////////////////////////////////////////////////////////////////////////

inline ObjID cAINewRangedSubcombat::GetID(void) const {return cAISubcombat::GetID();}

inline cAIRangedLocation* cAINewRangedSubcombat::GetCurrentLocation(void) {return m_pCurrentLocation;}

inline const sAITargetInfo* cAINewRangedSubcombat::GetTargetInfo(void) const {return cAISubcombat::GetTargetInfo();}

inline const sAIRangedRanges* cAINewRangedSubcombat::GetRangesSq(void) const {return &m_rangesSq;}

inline void cAINewRangedSubcombat::SetPhyscastCallback(tPhyscastObjTest callback) {m_physcastCallback = callback;}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAICombatRanged
//

class cAICombatNewRanged : public cAISimpleMultiCombat
{
public:
   cAICombatNewRanged();

private:
   cAINewRangedSubcombat m_RangedSubcombat;
};

///////////////////////////////////////

inline cAICombatNewRanged::cAICombatNewRanged()
{
   SetSubcombat(&m_RangedSubcombat);
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AICBRNEW_H */


