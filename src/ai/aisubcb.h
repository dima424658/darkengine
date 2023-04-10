/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aisubcb.h,v 1.3 1999/05/10 19:10:25 JON Exp $
//
// Sub-combat abilities. They are abilities, but their decision process is
// in cAICombat, not in the core AI. As such, they reveal themselves to the
// core AI only as components, not as abilities.
//


#ifndef __AISUBCB_H
#define __AISUBCB_H

#include <aibasabl.h>

#pragma once
#pragma pack(4)

class cAICombat;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISubCombat
//

EXTERN CLSID CLSID_cAISubCombat;

///////////////////////////////////////

class cAISubCombat : public cAIAbility
{
public:
   cAISubCombat();

   STDMETHOD (QueryInterface) (THIS_ REFIID riid, void ** ppvObj);
   
   virtual void InitSubCombat(cAICombat * pCombat);

#if 0
   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();
   STDMETHOD_(void, Term)();

   // Notifications
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   STDMETHOD_(void, OnAlertness)(ObjID source,
                                 eAIAwareLevel previous, 
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw);
   STDMETHOD_(void, OnDamage)(const sDamageMsg *pMsg, ObjID realCulpritID);
   STDMETHOD_(void, OnWeapon)(eWeaponEvent ev, ObjID victim, ObjID culprit);
   STDMETHOD_(void, OnProperty)(IProperty *, const sPropertyListenMsg *);

   // Update the status of the current goal
   STDMETHOD_(eAIResult, UpdateGoal)(cAIGoal *);

   // Ability flow
   STDMETHOD (SuggestMode)   (sAIModeSuggestion *);
   STDMETHOD (SuggestGoal)   (cAIGoal * pPrevious, cAIGoal ** ppNew);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);
#endif

protected:
   cAICombat * m_pCombat;

private:
};

///////////////////////////////////////

inline cAISubCombat::cAISubCombat()
 : m_pCombat(NULL)
{
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()
#endif /* !__AISUBCB_H */
