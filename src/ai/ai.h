/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/ai.h,v 1.46 2000/02/28 17:27:50 toml Exp $
//
//
//

#ifndef __AI_H
#define __AI_H

#include <comtools.h>
#include <fixedstk.h>

#include <aitype.h>
#include <aiapiact.h>
#include <aiapiiai.h>
#include <aigoal.h>
#include <aictlsig.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

typedef cFixedStack<tAIPathCellID, 16> cAICellStack;

///////////////////////////////////////////////////////////////////////////////
//
// Forward declarations
//

class cAIActions;
struct sAIEfficiency;
struct sAIFrustrated;
typedef unsigned tAIActionType;

///////////////////////////////////////////////////////////////////////////////
//
// Constants and enumerations
//

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAI
//

class cAI : public cCTUnaggregated<IInternalAI, &IID_IInternalAI, kCTU_Default>
{
public:
   /////////////////////////////////////////////////////////////
   //
   // Management
   //

   cAI();
   virtual ~cAI();
   STDMETHOD (QueryInterface)(REFIID id, void ** ppI);

   //
   // Get another AI
   //
   STDMETHOD_(IInternalAI *, AccessOtherAI)(ObjID);
   STDMETHOD_(void, AccessAIs)(IInternalAI ***, int *);

   //
   // Live/dead management
   //
   STDMETHOD_(void, Kill)();
   STDMETHOD_(void, Resurrect)();
   STDMETHOD_(BOOL, IsDying)(void);
   STDMETHOD_(BOOL, IsDead)();

   //
   // Freeze/unfreeze
   //
   STDMETHOD_(void, Freeze)(tSimTime duration);
   STDMETHOD_(void, Unfreeze)(void);

   //
   // Plug the AI together
   //
   STDMETHOD (Init)(ObjID id, IAIBehaviorSet *);
   STDMETHOD (End)();

   //
   // Access a component
   //
   STDMETHOD (GetComponent)(const char * pszComponent, IAIComponent **);

   //
   // Component iteration protocol
   //
   STDMETHOD_(IAIComponent *, GetFirstComponent)(tAIIter *);
   STDMETHOD_(IAIComponent *, GetNextComponent)(tAIIter *);
   STDMETHOD_(void, GetComponentDone)(tAIIter *);

   //
   // Access enactor components
   //
   STDMETHOD_(IAIMoveEnactor *, AccessMoveEnactor)();
   STDMETHOD_(IAISoundEnactor *, AccessSoundEnactor)();
   STDMETHOD_(IAISenses *, AccessSenses)();
   STDMETHOD_(IAIPathfinder *, AccessPathfinder)();

   //
   // Access the AI behavior set
   //
   STDMETHOD_(IAIBehaviorSet *, AccessBehaviorSet)();

   //
   // Notification of entering/exiting gamemode
   //
   virtual void OnSimStart();
   virtual void OnSimEnd();

   ////////////////////////////////////
   //
   // Projectile hook
   //
   STDMETHOD_(void, NotifyFastObj)(ObjID firer, ObjID projectile, const mxs_vector & velocity);

   ////////////////////////////////////
   //
   // Sound hook
   //
   STDMETHOD_(void, NotifySound)(const sSoundInfo *, const sAISoundType *);

   ////////////////////////////////////
   //
   // Tripwires
   //
   STDMETHOD_(void, NotifyTripwire)(ObjID objID, BOOL enter);

   ////////////////////////////////////
   //
   // Notify of found body
   //
   STDMETHOD_(void, NotifyFoundBody)(ObjID body);

   ////////////////////////////////////
   //
   // Notify of found something suspicious
   //
   STDMETHOD_(void, NotifyFoundSuspicious)(ObjID suspobj);

   ////////////////////////////////////

   STDMETHOD_(void, NotifyScriptCommand)(eAIScriptCmd command, void *);

   ////////////////////////////////////

   STDMETHOD_(void, NotifyProperty)(IProperty *, const sPropertyListenMsg *);

   ////////////////////////////////////

   STDMETHOD_(void, NotifyDefend)(const sAIDefendPoint *);

   ////////////////////////////////////

   STDMETHOD_(void, NotifySignal)(const sAISignal *);

   ////////////////////////////////////

   STDMETHOD_(void, NotifyWitnessCombat)();

   ////////////////////////////////////

   STDMETHOD_(void, NotifyCollision)(const sPhysListenMsg *);

   ////////////////////////////////////

   STDMETHOD_(BOOL, IsNetworkProxy)();

   ////////////////////////////////////

   STDMETHOD_(void, StartProxyAction)(IAIAction *pAction, ulong deltaTime);

   ////////////////////////////////////

   STDMETHOD_(IAIAction *, GetTheActionOfType)(tAIActionType type);
   STDMETHOD_(void, StopActionOfType)(tAIActionType type);

   ////////////////////////////////////

   STDMETHOD_(void, SetIsBrainsOnly)(BOOL);

   ////////////////////////////////////

   STDMETHOD_(void, SetGoal)(cAIGoal *pGoal);

   ////////////////////////////////////

   STDMETHOD_(BOOL, PreSave)();
   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, PostSave)();

   STDMETHOD_(BOOL, PreLoad)();
   STDMETHOD_(BOOL, Load)(ITagFile *);
   STDMETHOD_(BOOL, PostLoad)();

public:
   /////////////////////////////////////////////////////////////
   //
   // State accessors
   //

   //
   // Get the AIs ObjID
   //
   STDMETHOD_(ObjID, GetObjID)();

   ObjID GetID();

   //
   // Access the state of the AI
   //
   STDMETHOD_(const cAIState *, GetState)();

   //
   // Freshen the AI path cell
   //
   STDMETHOD_(tAIPathCellID, UpdatePathCell)();

   //
   // Get recently visited cells
   //
   STDMETHOD_(void, GetRecentCells)(cAICellArray * pResult);

   //
   // Get the desired Z for the AI when non-biped locomoted
   //
   STDMETHOD_(float, GetGroundOffset)();

   //
   // Utility to get the closest player.  Most useful in networked games.
   //
   STDMETHOD_(ObjID, GetClosestPlayer)();

   //
   // Get the desired floor Z for the AI when biped locomoted
   //
   float AdjustMvrGroundHeight(const Location * pLoc, float height, ObjID hitObj);

   /////////////////////////////////////////////////////////////
   //
   // Sense accessor
   //
   // Never returns NULL. Returned structure should never be considered
   // valid across calls to this function or across frames.
   //
   STDMETHOD_(const sAIAwareness *, GetAwareness)(ObjID target);

   ////////////////////////////////////
   //
   // Get the efficiency property for the AI
   //

   const sAIEfficiency * GetEfficiency();

   /////////////////////////////////////////////////////////////
   //
   // Reset the AIs thinking
   //

   void ResetGoals();

   /////////////////////////////////////////////////////////////
   //
   // Core Flow
   //

   ////////////////////////////////////
   //
   // Ability callback used by abilities to signal desire to be called in
   // decision process. Since mode/goal desires may noted and retained,
   // not all abilities arerun every frame.
   //

   ////////////////////////////////////
   //
   // Run one frame
   //

   virtual HRESULT OnNormalFrame();

   ////////////////////////////////////
   //
   // Check for efficiency transition
   //

   eAIMode UpdateEfficiency();

   ////////////////////////////////////
   //
   // Update cached, generally read only, state data
   //
   // Any code that moves or orients the AI object should call this function
   //

   virtual void UpdateState();

   ////////////////////////////////////
   //
   // Update local cached property data
   //

   virtual void UpdateProperties(eAIPropUpdateKind kind);

   ////////////////////////////////////
   //
   // Update sensory data
   //

   virtual HRESULT UpdateSenses();

   ////////////////////////////////////
   //
   // Update AI alertness
   //

   virtual void UpdateAlertness();

   ////////////////////////////////////
   //
   // Update frustration.
   //

   virtual void UpdateFrustration();

   ////////////////////////////////////
   //
   // Update current action, if any. Return
   //

   virtual eAIResult UpdateAction();

   ////////////////////////////////////
   //
   // Update current goal, if any
   //

   virtual eAIResult UpdateGoal();

   ////////////////////////////////////
   //
   // Check if any abilities have signaleld
   //

   BOOL CheckSignals(tAIAbilitySignals signals = kAI_SigAll);

   ////////////////////////////////////
   //
   // Clear out ability signals
   //

   void ClearSignals();

   ////////////////////////////////////
   //
   // Decide the mode
   //

   virtual void DecideMode();

   ////////////////////////////////////
   //
   //
   //

   virtual void DecideGoal();

   ////////////////////////////////////
   //
   //
   //

   virtual void DecideAction();

   ////////////////////////////////////
   //
   // Enact result of decision process
   //

   virtual void Enact();

   ////////////////////////////////////
   //
   // Query "rendered" status
   //

   BOOL WasRendered(unsigned time = 0) const;
   void UpdateRenderedStatus();

protected:

   ////////////////////////////////////
   //
   // The AI state
   //

   cAIState m_state;

   enum eFlags
   {
      kStarted = 0x01,
      kResetGoals = 0x02
   };

   unsigned m_flags;                             // these flags differ from state flags in that they are more
                                                 // concerned with the AI-as-state-machine than the AI-as-object (toml 10-31-98)

   STDMETHOD_(void, SetModeAndNotify)(eAIMode);
   void CheckGoalsReset();

   ////////////////////////////////////
   //
   // Utility methods
   //

   ////////////////////////////////////
   //
   // General component management and notifications
   //

   void ConnectComponent(IAIComponent * pComponent);

   STDMETHOD_(void, NotifySimStart)();
   STDMETHOD_(void, NotifySimEnd)();
   STDMETHOD_(eDamageResult, NotifyDamage)(const sDamageMsg * pMsg);
   STDMETHOD_(void, NotifyWeapon)(eWeaponEvent ev, ObjID victim, ObjID culprit);

   void NotifyBeginFrame();
   void NotifyEndFrame();

   void NotifyUpdateProperties(eAIPropUpdateKind kind);

   void NotifyActionProgress(IAIAction *);
   void NotifyGoalProgress(const cAIGoal *);
   void NotifyModeChange(eAIMode, eAIMode);
   void NotifyGoalChange(const cAIGoal *, const cAIGoal *);
   void NotifyActionChange(IAIAction *, IAIAction *);

   void NotifyAwareness(ObjID source, eAIAwareLevel current, const sAIAlertness * pRaw);

   void NotifyAlertness(ObjID source, eAIAwareLevel previous, eAIAwareLevel current, const sAIAlertness * pRaw);

   void NotifyHighAlert(ObjID source, eAIAwareLevel previous, eAIAwareLevel current, const sAIAlertness * pRaw);

   void NotifyFrustration(sAIFrustrated *);

   STDMETHOD_(void, NotifyGoalsReset)(const cAIGoal *);

   STDMETHOD_(void, NotifyGameEvent)(void *);

   ////////////////////////////////////

   struct sAINotifyInfo
   {
      IAIComponent * pComponent;
      unsigned *     pNotifications;
      BOOL Wants(unsigned notification)
      {
         return (*pNotifications & notification);
      }

   };

   cDynArray_<sAINotifyInfo, 1> m_NotifyInfo;

   ////////////////////////////////////
   
   unsigned m_TimeLastRendered;

   ////////////////////////////////////
   //
   // Update process timers
   //

   struct sAIUpdateSchedule
   {
      sAIUpdateSchedule()
       : efficiency (kAIT_1Sec)
      {
      }

      cAITimer efficiency;                       // 1 sec
      cAITimer goal;                             // std
   };

   // AI state update schedule
   sAIUpdateSchedule m_UpdateSchedule;

   ////////////////////////////////////

   cAITimer m_ResetTimer;

   ////////////////////////////////////
   //
   // Senses
   //

   IAISenses * m_pSenses;

   ////////////////////////////////////
   //
   // Senses
   //

   cDynArray<IAIMoveRegulator *> m_MoveRegulators;

   ////////////////////////////////////
   //
   // Abilities
   //

   struct sAIAbilityInfo
   {
      IAIAbility *      pAbility;
      unsigned *        pSignals;

      cAIGoal *         pGoal;
      sAIModeSuggestion modeSuggestion;

      BOOL CheckSignals(tAIAbilitySignals test = kAI_SigAll)
      {
         return (*pSignals & test);
      }
   };

   cDynArray<sAIAbilityInfo> m_Abilities;
   sAIAbilityInfo *          m_pCurAbilityInfo;

   ////////////////////////////////////
   //
   // Enactors
   //

   IAIMoveEnactor *  m_pMoveEnactor;
   IAISoundEnactor * m_pSoundEnactor;

   ////////////////////////////////////
   //
   // Pathfinder
   //

   IAIPathfinder *   m_pPathfinder;

   ////////////////////////////////////
   //
   // Generic components
   //

   cDynArray_<IAIComponent *, 1> m_OtherComponents;

   ////////////////////////////////////
   //
   // The current AI goals/activities
   //

   cAIGoal *    m_pCurGoal;
   eAIResult    m_CurGoalLastResult;
   cAIActions   m_CurActions;
   eAIResult    m_CurActionLastResult;

   cAITimer     m_NullActionTimer;

   ////////////////////////////////////
   //
   // Visited cell history
   //

   cAICellStack m_CellStack;

   ////////////////////////////////////
   //
   // The creature behavior set
   //

   IAIBehaviorSet * m_pBehaviorSet;

#ifndef SHIP
   ////////////////////////////////////
   //
   // Debugging options
   //

   const cAIMoveSuggestions * m_pDebugMoveSuggestions;
   const sAIMoveGoal *        m_pDebugMoveGoal;

   STDMETHOD_(void, DebugDraw)();
   STDMETHOD_(void, DebugSetPrimitiveMove)(const cAIMoveSuggestions *, const sAIMoveGoal *);
   STDMETHOD_(void, DebugGetPrimitiveMove)(const cAIMoveSuggestions **, const sAIMoveGoal **);
#endif

   ////////////////////////////////////

   cAIRandomTimer m_SimStartTimer;

};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAI, inline functions
//

inline ObjID cAI::GetID()
{
   return m_state.GetID();
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AI_H */
