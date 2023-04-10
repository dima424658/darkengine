/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/ainonhst.h,v 1.1 1998/07/15 18:12:20 TOML Exp $
//
// Non-hostility controls
//

#ifndef __AINONHST_H
#define __AINONHST_H

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// ENUM: eAINonHostility, basic do/don't control
//

enum eAINonHostilityEnum
{
   kAINH_Never,
   kAINH_ToPlayer,
   kAINH_ToPlayerUntilDmg,
   kAINH_ToPlayerUntilThreat,
   kAINH_UntilDmg,
   kAINH_UntilThreat,
   kAINH_Always,
   
   kAINH_Num,
   
   kAINH_TypeMax = 0xffffffff
};

typedef unsigned eAINonHostility;

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AINONHST_H */
