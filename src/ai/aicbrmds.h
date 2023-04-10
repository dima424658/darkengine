/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrmds.h,v 1.2 1999/03/29 08:44:46 JON Exp $
//
//
//

#ifndef __AICBRMDS_H
#define __AICBRMDS_H

#pragma once
#pragma pack(4)

#include <aicbrtyp.h>

///////////////////////////////////////////////////////////////////////////////

class cAIRangedMode;

///////////////////////////////////////////////////////////////////////////////

typedef int eAIRangedModeID;

enum eAIRangedModeID_ 
{
   kAIRC_IdleMode  =    0,
   kAIRC_ShootMode =    1, 
   kAIRC_CloseMode =    2, 
   kAIRC_BackupMode =   3, 
   kAIRC_WoundMode =    4, 
   kAIRC_VantageMode =  5, 
   kAIRC_LeftMode =     6, 
   kAIRC_RightMode =    7, 
   kAIRC_FleeMode =     8, 
   kAIRC_NumModes
};

///////////////////////////////////////////////////////////////////////////////

typedef int eAIRangedModeFlags;

enum eAIRangedModeFlags_ 
{
   kAIRC_MoveMode =     0x0001,
};

///////////////////////////////////////////////////////////////////////////////

typedef int eAIRangedModeApplicability;

enum eAIRangedModeApplicability_
{
   kAIRC_AppNone =      0,
   kAIRC_AppMinimum =   1,
   kAIRC_AppVeryLow =   2,
   kAIRC_AppLow =       3,
   kAIRC_AppNormal =    4,
   kAIRC_AppHigh =      5,
   kAIRC_AppVeryHigh =  6,
   kAIRC_NumApplicabilities,
};

///////////////////////////////////////////////////////////////////////////////

typedef struct sAIRangedApplicabilities
{
   eAIRangedModeApplicability m_modeApplicability[kAIRC_NumModes];
} sAIRangedApplicabilities;

///////////////////////////////////////////////////////////////////////////////

typedef int eAIRangedModePriority;

enum eAIRangedModePriority_
{
   kAIRC_PriMinimum = 0,
   kAIRC_PriLow = 1,
   kAIRC_PriNormal = 2,
   kAIRC_PriHigh = 3,
};

#pragma pack()

#endif /* !__AICBRMDS_H */


