/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aibassns.h,v 1.26 2000/01/24 12:12:38 adurant Exp $
//
// Base implementation of an AI sense
//

#ifndef __AIBASSNS_H
#define __AIBASSNS_H

#include <fixedstk.h>

#include <aiapisns.h>
#include <aibascmp.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

typedef long LinkID;
struct sAIVisionCone;
struct sAIVisionDesc;
struct sAIAwareCapacitor;  
struct sAIVisibilityControl;
F_DECLARE_INTERFACE(IAIInform);

///////////////////////////////////////////////////////////////////////////////

void AIInitSenses();
void AITermSenses();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISenses
//

class cAIObjectSounds;
struct sAIQueuedSound;

///////////////////////////////////////

class cAISenses : public cAIComponentBase<IAISenses, &IID_IAISenses>
{
public:
   cAISenses();
   ~cAISenses();

   STDMETHOD_(const char *, GetName)();

   STDMETHOD_(void, Init)();
   
   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   STDMETHOD_(void, OnBeginFrame)();
   STDMETHOD_(void, OnModeChange)(eAIMode previous, eAIMode mode);
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);
   STDMETHOD_(void, OnSound)(const sSoundInfo *, const sAISoundType * pType);
   STDMETHOD_(void, OnFoundBody)(ObjID body);
   STDMETHOD_(void, OnFoundSuspicious)(ObjID suspobj);
   STDMETHOD_(void, OnDamage)(const sDamageMsg * pMsg, ObjID realCulpritID);
   STDMETHOD_(void, OnFastObj)(ObjID firer, ObjID projectile, const mxs_vector & velocity);
   
   STDMETHOD (Update)();
   STDMETHOD (Update)(ObjID objId, unsigned flags = 0);
   STDMETHOD (ClearSense)(ObjID objId);
   STDMETHOD_(const sAIAwareness *, GetHighAwareness)(unsigned flags, unsigned maxLastContact = INT_MAX);
   STDMETHOD_(const sAIAwareness *, GetAwareness)(ObjID objId);
   STDMETHOD_(int, GetAllAwareness)(cAIAwareArray * pResult);

   STDMETHOD_(int, Hears)(const sSoundInfo *, const sAISoundType * pType);

   STDMETHOD_(void, KeepFresh)(ObjID obj, unsigned duration);

   static void UpdateVisibility(ObjID objId);

private:

   int GetFreeKnowledgePeriod();

   ////////////////////////////////////
   
   static int CalculateLightRating(ObjID objId, const sAIVisibilityControl * pVisCtrl);
   friend void AIUpdateVisibility(ObjID objId);

   ////////////////////////////////////
   
   void SetAwareness(ObjID objId, LinkID linkID, const sAIAwareness *);
   void TestForget(LinkID linkID, const sAIAwareness *);

   void ProcessPositionSounds();
   BOOL FunctionallySofter(const sSoundInfo * pSoundInfoNew, const sSoundInfo * pSoundInfoOld);

   ////////////////////////////////////
   
   struct sVisionArgs
   {
      ObjID                 obj;
      unsigned              flags;
      float                 distSq;
      Location              fromLoc;
      const Location *      pTargetLoc;
      Location              lookLoc;
      const sAIVisionDesc * pVisionDesc;
   };
   
   BOOL                  UpdateSightcast(const sVisionArgs & args, sAIAwareness *);
   eAIAwareLevel         Look(const sVisionArgs & args, sAIAwareness * pAwareness);
   const sAIVisionCone * FindVisionCone(const sVisionArgs & args) const;
   int                   GetVisibility(const sVisionArgs & args, const sAIVisionCone * pCone);

   ////////////////////////////////////
   
   eAIAwareLevel Listen(ObjID objId, sAIAwareness * pAwareness, mxs_vector * pSoundPos);

   LinkID GetAwarenessLink(ObjID objId, sAIAwareness ** ppAwareness);
   HRESULT Pulse(sAIAwareness * pAwareness, eAIAwareLevel visionPulse, const mxs_vector & visionPos, eAIAwareLevel soundPulse, const mxs_vector & soundPos);

   ////////////////////////////////////
   
   void AwarenessDelay(sAIAwareness * pAwareness, eAIAwareLevel * pPulse, const mxs_vector & pulseLoc);

   ////////////////////////////////////
   
   static float FindDistanceToClosestWallSq(ObjID);
   virtual floatang GetVisionFacing() const;
   
   ////////////////////////////////////
   
   const sAIAwareCapacitor * GetCapacitor();
   
   void GetBestInWorld(const mxs_vector &, mxs_vector *);
   
   ////////////////////////////////////
   
   void InformFrom(ObjID from);
   
   ////////////////////////////////////
   
   cAITimer                  m_Timer;
   cAITimer                  m_OthersTimer;
   cAITimer                  m_MaxInformTimer;
   
   cAITimer                  m_SenseCombatTimer;
   
   ////////////////////////////////////
   
   cAITimer                  m_FreshTimer;
   
   ////////////////////////////////////
   
   // Last time this AI was damaged.
   ulong                     m_LastPlayerDamage;  
   // Last time a projectile was fired by any player, where it will almost hit me.
   ulong                     m_LastPlayerProjectile;
   // Location from which above projectile was launched.
   mxs_vector                m_LastProjectileLoc;
   
   ////////////////////////////////////
   
   cAIObjectSounds * const   m_pObjectSounds;
   sAIQueuedSound *          m_pPositionSound;
   
   ////////////////////////////////////
   
   IAIInform *               m_pInform;
   
   ////////////////////////////////////
   
   enum eFlags
   {
      kSensedCombat = 0x01,
      kInUpdate     = 0x02
   };
   
   BOOL                      m_flags;
   
   ////////////////////////////////////
   //
   // Reaction delay
   //
   
   cAITimer m_ModerateDelay;
   cAITimer m_ModerateDelayReuse;
   cAITimer m_HighDelay;
   cAITimer m_HighDelayReuse;
   
   ////////////////////////////////////
   
   static BOOL               gm_FrameBudget;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIBASSNS_H */
