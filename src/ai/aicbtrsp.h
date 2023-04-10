/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbtrsp.h,v 1.2 1998/10/10 14:01:42 TOML Exp $
//
//
//

#ifndef __AICBTRSP_H
#define __AICBTRSP_H

#include <aiapicmp.h>
#include <aitrig.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitSenseCombatResponseAbility(IAIManager *);
BOOL AITermSenseCombatResponseAbility();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISenseCombatResponse
//

class cAISenseCombatResponse : public cAITriggeredPScripted
{
public:
   cAISenseCombatResponse();
   ~cAISenseCombatResponse();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   STDMETHOD_(void, OnWitnessCombat)();
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AICBTRSP_H */
