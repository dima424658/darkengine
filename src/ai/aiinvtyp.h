/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiinvtyp.h,v 1.1 1998/07/15 18:12:18 TOML Exp $
//
// Types for tuning investigation ability
//

#ifndef __AIINVTYP_H
#define __AIINVTYP_H

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// ENUM: eAIInvestKind, basic do/don't control
//

enum eAIInvestKindEnum
{
   kAIIK_Always,
   kAIIK_Never,
   
   kAIIK_Num,
   
   kAIIK_TypeMax = 0xffffffff
};

typedef unsigned eAIInvestKind;

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIINVTYP_H */
