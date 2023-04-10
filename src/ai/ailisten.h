/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/ailisten.h,v 1.4 1998/08/11 11:03:23 TOML Exp $
//
// Signal listener
//

#ifndef __AILISTEN_H
#define __AILISTEN_H

#include <aiapicmp.h>
#include <aitrig.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitSignalResponseAbility(IAIManager *);
BOOL AITermSignalResponseAbility();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISignalResponse
//

class cAISignalResponse : public cAITriggeredPScripted
{
public:
   cAISignalResponse();
   ~cAISignalResponse();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   STDMETHOD_(void, OnSignal)(const sAISignal *);
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AILISTEN_H */
