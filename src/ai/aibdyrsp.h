/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aibdyrsp.h,v 1.1 1998/11/10 15:25:18 TOML Exp $
//
//
//

#ifndef __AIBDYRSP_H
#define __AIBDYRSP_H

#include <aiapicmp.h>
#include <aitrig.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitBodyResponseAbility(IAIManager *);
BOOL AITermBodyResponseAbility();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIBodyResponse
//

class cAIBodyResponse : public cAITriggeredPScripted
{
public:
   cAIBodyResponse();
   ~cAIBodyResponse();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   STDMETHOD_(void, OnFoundBody)(ObjID body);
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIBDYRSP_H */
