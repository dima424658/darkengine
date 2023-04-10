/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiapimov.h,v 1.11 1999/06/15 18:23:21 dc Exp $
//
//
//

#ifndef __AIAPIMOV_H
#define __AIAPIMOV_H

#ifndef __cplusplus
#error "Private AI APIs require C++"
#endif

#include <dynarray.h>
#include <aiapicmp.h>

#pragma once
#pragma pack(4)

class IMotor;

F_DECLARE_INTERFACE(IAIMoveEnactor);

typedef unsigned eAIMotionType;
struct sAIMotionRequest;
struct sAIMoveGoal;

typedef unsigned eAIMoveSuggKind;
struct sAIMoveSuggestion;
class cAIMoveSuggestions;

class cAIMoveAction;
class cAIMotionAction;
class cAIOrientAction;

struct sAIImpulse;

///////////////////////////////////////////////////////////////////////////////
//
// Constants
//

//
// Move enactor status
//
enum eAIMEStatus
{
   // The enactor is idle
   kAIME_Idle,

   // The enactor is active, but safely interruptable
   kAIME_ActiveInterrupt,

   // The enactor is active, and not interruptable
   kAIME_ActiveBusy,

   // The enactor is in an error state
   kAIME_Error,

   kAIME_IntMax = 0xffffffff
};

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAIMoveEnactor
//

DECLARE_INTERFACE_(IAIMoveEnactor, IAIComponent)
{
   ////////////////////////////////////
   //
   // Control flow
   //

   //
   // Add a movement regulator
   //
   STDMETHOD (InstallRegulator)(IAIMoveRegulator *) PURE;

   //
   // Query the current status
   //
   STDMETHOD_(eAIMEStatus, GetStatus)() PURE;

   //
   // Set the current action, if any
   //
   STDMETHOD (NoAction)(ulong deltaTime) PURE;
   STDMETHOD (EnactAction)(cAIMoveAction * pAction, ulong deltaTime) PURE;
   STDMETHOD (EnactAction)(cAIMotionAction * pAction, ulong deltaTime) PURE;
   STDMETHOD (EnactAction)(cAIOrientAction * pAction, ulong deltaTime) PURE;

   ////////////////////////////////////
   //
   // Arc-suggestion low-level operation
   //
   // These functions are intentionally provided as broken-out
   // steps of decision
   //

   //
   //
   //
   STDMETHOD_(BOOL, NewRegulations)() PURE;

   //
   //
   //
   STDMETHOD (AddMovementRegulations)(cAIMoveSuggestions & suggestions) PURE;

   //
   //
   //
   STDMETHOD (ResolveSuggestions)(const cAIMoveSuggestions & suggestions,
                                  const sAIMoveSuggestion ** ppBestSuggestion,
                                  sAIMoveGoal * pResultGoal) PURE;

   ////////////////////////////////////
   //
   // Motion functions
   //
   // (Note that a specific AI may not support motions)
   //

   STDMETHOD_(BOOL, SupportsMotionActions)() PURE;
   STDMETHOD (SetMotor)(IMotor * pMotor) PURE;
   STDMETHOD (ResetMotionTags)() PURE;

#ifdef INCORRECT_AI_NETWORKING
   ////////////////////////////////////
   //
   // Functions for networking
   //
   // Enact a move or motion received off the network.  No action is created for it.
   STDMETHOD_(void, HandleMoveMessage)(void *pMsg, ObjID fromPlayer) PURE;
   STDMETHOD_(void, HandleMotionMessage)(void *pMsg, ObjID fromPlayer) PURE;
#endif

   //
   // Query what the last movement desire was, in the form of an
   // impulse. Returns TRUE if there actually was one
   //
   STDMETHOD_(BOOL, GetTargetVel)(sAIImpulse * pResult) PURE;
};

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAIMoveRegulator
//

DECLARE_INTERFACE_(IAIMoveRegulator, IAIComponent)
{
   //
   //
   //
   STDMETHOD_(BOOL, NewRegulations)() PURE;

   //
   //
   //
   STDMETHOD (SuggestRegulations)(cAIMoveSuggestions &) PURE;

   //
   //
   //
   STDMETHOD_(BOOL, AssistGoal)(const sAIMoveGoal & goal,
                                sAIMoveGoal * pResultGoal) PURE;

   //
   //
   //
   STDMETHOD_(BOOL, WantsInterrupt)() PURE;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()
#endif /* !__AIAPIMOV_H */



