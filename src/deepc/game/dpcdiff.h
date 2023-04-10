/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCDIFF_H
#define __DPCDIFF_H

enum eDPCifficulty
{
   kDPCDiffPlaytest,
   kDPCFirstDiff,
   kDPCDiffEasy = kDPCFirstDiff, 
   kDPCDiffNormal,
   kDPCDiffHard,
   kDPCDiffImpossible,
   kDPCLimDiff, 
   kNumDPCDiffs = kDPCLimDiff - kDPCFirstDiff,
   kDPCDiffMultiplay = kDPCLimDiff,
}; 

#endif // __DPCDIFF_H
