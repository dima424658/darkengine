/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aictlsig.h,v 1.1 1998/08/09 16:06:35 TOML Exp $
//
// Control flow signals
//

#ifndef __AICTLSIG_H
#define __AICTLSIG_H

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// ENUM: eAIAbilitySignalFlags -- Decision flow signals
//

enum eAIAbilitySignalFlags
{
  kAI_SigMode = 0x01,
  kAI_SigGoal = 0x02,
  kAI_SigAct  = 0x04,

  kAI_SigAll = 0xffffffff
};

typedef unsigned tAIAbilitySignals;

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AICTLSIG_H */
