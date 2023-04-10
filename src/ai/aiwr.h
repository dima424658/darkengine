/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiwr.h,v 1.4 2000/01/31 09:34:14 adurant Exp $
//
//
#pragma once

#ifndef __AIWR_H
#define __AIWR_H

#include <objtype.h>
#include <wrtype.h>

float AIGetObjectLighting(ObjID id);

BOOL AIIsInWorld(ObjID id);

enum eAIRaycastFlags
{
   kAIR_ZeroEpsilon = 0x01,
   kAIR_NoHintWarn  = 0x02,
   kAIR_VsObjects   = 0x04,
};

BOOL AIRaycast(const Location *start_loc, Location *end_loc,
              Location *hit_loc, unsigned flags = 0);


#endif /* !__AIWR_H */
