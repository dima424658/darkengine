/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkdiff.h,v 1.2 2000/01/31 09:55:34 adurant Exp $
#pragma once

#ifndef __SHKDIFF_H
#define __SHKDIFF_H

enum eShockDifficulty
{
   kShockDiffPlaytest,
   kShockFirstDiff,
   kShockDiffEasy = kShockFirstDiff, 
   kShockDiffNormal,
   kShockDiffHard,
   kShockDiffImpossible,
   kShockLimDiff, 
   kNumShockDiffs = kShockLimDiff - kShockFirstDiff,
   kShockDiffMultiplay = kShockLimDiff,
}; 

#endif // __SHKDIFF_H
