/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaisho.h,v 1.1 1999/03/04 12:08:42 JON Exp $
//
//
//

#ifndef __SHKAISHO_H
#define __SHKAISHO_H

#include <aicombat.h>
#include <aiprrngd.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

EXTERN void ShockAIInitShodan();
EXTERN void ShockAITermShodan();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIProtocolCombat
//

typedef int eAIShodanCombatMode;
enum _eAIShodanCombatMode {kAIShodanClosing, kAIShodanStartExplode, kAIShodanExploding};

class cAIShodanCombat : public cAICombat
{
public: 
   cAIShodanCombat();
      
   STDMETHOD_(const char *, GetName)();

   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   
   // Ability flow
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

private:
   void SetMode(eAIShodanCombatMode mode);
   BOOL GetProjectile(ObjID* pProjectileID, sAIProjectileRel** ppProjData);
   void TestTarget(void);

   eAIShodanCombatMode m_mode;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__SHKAISHO_H */
