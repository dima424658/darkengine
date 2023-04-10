/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiapiabl.h,v 1.7 1999/11/29 12:27:30 BFarquha Exp $
//
// Specification of the AI ability interface
//

#ifndef __AIAPIABL_H
#define __AIAPIABL_H

#ifndef __cplusplus
#error "Private AI APIs require C++"
#endif

#include <aiapiatr.h>

#pragma once
#pragma pack(4)

class cAIActions;
struct sAIModeSuggestion;

#define kAIMaxAbilsPerAI 32

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAIAbility
//

DECLARE_INTERFACE_(IAIAbility, IAIActor)
{
   //
   // Initialize the ability over and above normal component intialization
   //
   STDMETHOD_(void, ConnectAbility)(unsigned ** ppSignals) PURE;

   //
   // Goal save/load
   //
   STDMETHOD_(BOOL, SaveGoal)(ITagFile * pTagFile, cAIGoal * pGoal) PURE;
   STDMETHOD_(BOOL, LoadGoal)(ITagFile * pTagFile, cAIGoal ** ppGoal) PURE;

   //
   // Current action list save/load (pTagFile cursor should be prepositioned).
   //
   STDMETHOD_(BOOL, SaveActions)(ITagFile * pTagFile, cAIActions * pActions) PURE;
   STDMETHOD_(BOOL, LoadActions)(ITagFile * pTagFile, cAIActions * pActions) PURE;

   //
   // Set/Get whether this is the ability currently running the AI
   //
   STDMETHOD_(void, SetControl)(BOOL) PURE;
   STDMETHOD_(BOOL, InControl)() PURE;

   //
   // Update the status of the current goal
   //
   STDMETHOD_(eAIResult, UpdateGoal)(cAIGoal *) PURE;

   //
   // Ability flow
   //
   STDMETHOD (SuggestMode)(sAIModeSuggestion *) PURE;
   STDMETHOD (SuggestGoal)(cAIGoal * pPrevious, cAIGoal ** ppNew) PURE; // Ability should not release pPrevious!
   STDMETHOD_(BOOL, FinalizeGoal)(cAIGoal *pGoal) PURE;
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew) PURE;

};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIAPIABL_H */
