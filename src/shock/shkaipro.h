/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaipro.h,v 1.4 1999/06/25 20:46:05 JON Exp $
//
//
//

#ifndef __SHKAIPRO_H
#define __SHKAIPRO_H

#include <aicombat.h>
#include <aisndtyp.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

EXTERN void ShockAIInitProtocolDroid();
EXTERN void ShockAITermProtocolDroid();

///////////////////////////////////////////////////////////////////////////////

typedef int eAIProtocolCombatState;
enum eAIProtocolCombatState_ {kAIPC_Normal, kAIPC_StartExplosion, kAIPC_EndExplosion};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIProtocolCombat
//

class cAIProtocolCombat : public cAICombat
{
public: 

   cAIProtocolCombat();

   STDMETHOD_(void, Init)();

   STDMETHOD_(const char *, GetName)();

   STDMETHOD_(void, OnDamage)(const sDamageMsg *pMsg, ObjID realCulpritID);

   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   
   // Ability flow
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

private:
   eAIProtocolCombatState m_state;

   void SetState(eAIProtocolCombatState state);
   BOOL PlaySound(eAISoundConcept CombatSound);

   BOOL m_pathFailed; 
   tSimTime m_lastPathFailTime;
};

///////////////////////////////////////////////////////////////////////////////

inline cAIProtocolCombat::cAIProtocolCombat():
   m_state(kAIPC_Normal)
{
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__SHKAIPRO_H */
