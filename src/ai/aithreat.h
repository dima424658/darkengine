/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aithreat.h,v 1.2 1999/05/10 19:10:35 JON Exp $
//
//
//

#ifndef __AITHREAT_H
#define __AITHREAT_H

#include <aiapicmp.h>
#include <aitrig.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitThreatResponseAbility(IAIManager *);
BOOL AITermThreatResponseAbility();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIThreatResponse
//

enum eAIThreat
{
   kAIT_Weapon,
   kAIT_Damage,
   
   kAIT_NumThreats,
   
   kAIT_TypeMax = 0xffffffff
};

///////////////////////////////////////

class cAIThreatResponse : public cAITriggeredPScripted
{
public:
   cAIThreatResponse();
   ~cAIThreatResponse();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   STDMETHOD_(void, OnDamage)(const sDamageMsg * pMsg, ObjID realCulpritID);
   STDMETHOD_(void, OnWeapon)(eWeaponEvent ev, ObjID victim, ObjID culprit);

private:
   void CheckThreat(eAIThreat);
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AITHREAT_H */
