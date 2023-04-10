/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aimove.h,v 1.22 1999/06/15 18:23:24 dc Exp $
//
// AI move enactor
//

#ifndef __AIMOVE_H
#define __AIMOVE_H

#include <dynarray.h>

#include <cbllmapi.h>

#include <aiapimov.h>
#include <aibascmp.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

class IMotionCoordinator;
struct sAIMotionRequest;
class cTagSet;
struct sMcMoveParams;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMoveEnactor
//
// Note: Until motion system is saved, we don't want to save
// flags, and since the impulse timer is non-critical, there
// is no save/load on this class right now (toml 10-05-98)
//

typedef cAIComponentBase<IAIMoveEnactor, &IID_IAIMoveEnactor> cAIMoveEnactorBase;

class cAIMoveEnactor : public cAIMoveEnactorBase
{
public:
   cAIMoveEnactor();
   ~cAIMoveEnactor();

   STDMETHOD_(const char *, GetName)();

   ////////////////////////////////////
   //
   // Control flow
   //

   // Initialize the component
   STDMETHOD_(void, Init)();

   // Notifications
   STDMETHOD_(void, OnModeChange)(eAIMode previous, eAIMode mode);
   STDMETHOD_(void, OnSimStart)();
   
   // Add a movement regulator
   STDMETHOD (InstallRegulator)(IAIMoveRegulator *);
   
   // Query the current status
   STDMETHOD_(eAIMEStatus, GetStatus)();

   // Enact set action
   STDMETHOD (NoAction)(ulong deltaTime);
   STDMETHOD (EnactAction)(cAIMoveAction * pAction, ulong deltaTime);
   STDMETHOD (EnactAction)(cAIMotionAction * pAction, ulong deltaTime);
   STDMETHOD (EnactAction)(cAIOrientAction * pAction, ulong deltaTime);

   ////////////////////////////////////
   //
   // Arc-suggestion low-level operation
   //
   // These functions are intentionally provided as broken-out
   // steps of decision
   //

   //
   STDMETHOD_(BOOL, NewRegulations)();

   //
   STDMETHOD (AddMovementRegulations)(cAIMoveSuggestions & suggestions);

   //
   STDMETHOD (ResolveSuggestions)(const cAIMoveSuggestions & suggestions,
                                  const sAIMoveSuggestion ** ppBestSuggestion,
                                  sAIMoveGoal * pResultGoal);


   ////////////////////////////////////
   //
   // Motion functions
   //
   // (Note that a specific AI may not support motions)
   //
   STDMETHOD_(BOOL, SupportsMotionActions)();
   STDMETHOD (SetMotor)(IMotor * pMotor);
   STDMETHOD (ResetMotionTags)();

   ////////////////////////////////////

#ifdef INCORRECT_AI_NETWORKING
   // Enact a move or motion received off the network.  No action is created for it.
   STDMETHOD_(void, HandleMoveMessage)(void *pMsg, ObjID fromPlayer);
   STDMETHOD_(void, HandleMotionMessage)(void *pMsg, ObjID fromPlayer);
#endif

   //
   // Query what the last movement desire was, in the form of an
   // impulse. Returns TRUE if there actually was one
   //
   STDMETHOD_(BOOL, GetTargetVel)(sAIImpulse * pResult);

   ////////////////////////////////////
   
   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   ////////////////////////////////////
   
protected:

   floatang ComputeFacing(const sAIMoveGoal & goal);

   //
   // Actual enactor functions
   //

   // Motions:
   void MotCleanup();
   void MotOnModeChange(eAIMode previous, eAIMode mode);
   
   static void MotorStateChangeCallback(void *);

   eAIMEStatus MotStatus();
   HRESULT MotNoAction(ulong deltaTime);
   HRESULT MotEnact(cAIMoveAction * pAction, ulong deltaTime);
   HRESULT MotEnact(cAIMotionAction * pAction, ulong deltaTime);
   HRESULT MotEnact(cAIOrientAction * pAction, ulong deltaTime);
   
   HRESULT MotEnactMoveGoal(const sAIMoveGoal & goal, const cTagSet & tags, ulong deltaTime);
   HRESULT MotEnactMotion(const sMcMoveParams &params);
   BOOL MotSave(ITagFile *);
   BOOL MotLoad(ITagFile *);

   // Impulse/custom
   eAIMEStatus NonMotStatus();
   HRESULT NonMotNoAction(ulong deltaTime);
   HRESULT NonMotEnact(cAIMoveAction * pAction, ulong deltaTime);
   HRESULT NonMotEnact(cAIMotionAction * pAction, ulong deltaTime);
   HRESULT NonMotEnact(cAIOrientAction * pAction, ulong deltaTime);
   
   HRESULT NonMotEnactMoveGoal(const sAIMoveGoal & goal, ulong deltaTime);
   
   void CalculateImpulse(const sAIMoveGoal & goal, ulong deltaTime, sAIImpulse * pResult);
   void CalculateTargetVel(const sAIMoveGoal & goal, ulong deltaTime, sAIImpulse * pResult);

#ifdef INCORRECT_AI_NETWORKING
   // Private networking functions

   // Broadcast a move for the given goal and tags (not based on an action object).
   void BroadcastMove(const sAIMoveGoal &goal, const cTagSet &tags, ulong deltaTime);

   // Broadcast a motion for the given params.
   void BroadcastMotion(const sMcMoveParams params);
#endif

   ////////////////

   enum eFlags
   {
      kLastWasNoAction = 0x02,
      
      kFlags_MaxInt = 0xffffffff
   };
   
   unsigned m_flags;
   
   //
   // Movement Regulators
   //
   cDynArray<IAIMoveRegulator *> m_MoveRegulators;

   //
   // Motion hooks
   //
   IMotionCoordinator * m_pMotionCoord;
   cAIRandomTimer       m_StandTimer;

   //
   // Impulse members
   //
   cAITimer m_ImpulsePeriod;
   unsigned m_StandCount;

   sAIImpulse * m_pTargetVel;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIMOVE_H */
