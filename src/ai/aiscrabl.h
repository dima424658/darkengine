/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiscrabl.h,v 1.12 2000/01/04 19:29:19 BFarquha Exp $
//
//
//

#ifndef __AISCRABL_H
#define __AISCRABL_H

#include <str.h>
#include <dynarray.h>
#include <ainoncbt.h>

#pragma once
#pragma pack(4)

class cAIScrSrv;
class cMultiParm;


//////////////////////////////////////////////////////////////////////////////
//
// Publish ability and all associated link and property types
//

BOOL AIInitScriptAbility(IAIManager *);
BOOL AITermScriptAbility();


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIBasicScript
//

class cAIBasicScript : public cAINonCombatAbility
{
public:
   cAIBasicScript();
   ~cAIBasicScript();

   // Find the script ability for an AI -- does not up reference
   static cAIBasicScript * AccessBasicScript(ObjID id);

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   // Save/load
   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);
   STDMETHOD_(BOOL, SaveGoal)(ITagFile * pTagFile, cAIGoal * pGoal);
   STDMETHOD_(BOOL, LoadGoal)(ITagFile * pTagFile, cAIGoal ** ppGoal);

   // Notifications
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   STDMETHOD_(void, OnAlertness)(ObjID source,
                                 eAIAwareLevel previous,
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw);
   STDMETHOD_(void, OnHighAlert)(ObjID source,
                                 eAIAwareLevel previous,
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw);
   STDMETHOD_(void, OnSignal)(const sAISignal *);
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);
   STDMETHOD_(void, OnActionChange)(IAIAction * pPrevious, IAIAction * pAction);
   STDMETHOD_(void, OnModeChange)(eAIMode prev, eAIMode mode);


   // Update the status of the current goal
   STDMETHOD_(eAIResult, UpdateGoal)(cAIGoal *);

   // Ability flow
   STDMETHOD (SuggestGoal)   (cAIGoal * pPrevious, cAIGoal ** ppNew);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

   //
   // Script goal commands
   //
   BOOL GotoObjLoc(ObjID              objIdTarget,
                   eAISpeed           speed,
                   eAIPriority        priority,
                   const cMultiParm & dataToSendOnReach);

   BOOL FrobObj(ObjID              objIdTarget,
                ObjID              objIdWith,
                eAIPriority        priority,
                const cMultiParm & dataToSendOnReach);

   BOOL PlayMotion(const char *name);

private:

   void DispacthMotionEnd() {}

   // Goal pending from script goal commands
   cAIGoal *          m_pPendingGoal;
   cMultiParm * const m_pPendingData;
   cMultiParm * const m_pCurrentData;
   cStr               m_Motion;

   enum
   {
      kTransitionGoal = kBaseFirstAvailFlag,
      kLocoFailed     = (kTransitionGoal << 1),

      kScriptFirstAvailFlag =  (kLocoFailed << 1)
   };

   cAITimer            m_TransitionTimer;

   // Quick access to all AIs with this ability
   static cDynArray<cAIBasicScript *> gm_ScriptAbilities;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AISCRABL_H */
