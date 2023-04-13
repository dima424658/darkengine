/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/*
 * $Source: x:/prj/tech/libsrc/random/RCS/random.h $
 * $Revision: 1.9 $
 * $Author: ccarollo $
 * $Date: 1999/03/16 13:06:38 $
 */
#ifndef __RANDOM_H
#define __RANDOM_H
#pragma once

#define RANDOM_MAX 0x7FFFFFFF

// Call this first, or you're doomed to die a horrible death
EXTERN void RandInit(long seed);

// Call this if you want to free every last bit of memory.
EXTERN void RandShutdown(void);

// Only call this if you want, RandomInit does it anyway
EXTERN void RandSeed(long seed);

// Call this for a good one
EXTERN long RandLong(void);

// Call this for a good float between in the range [0 and 1)
EXTERN float RandFloat(void);

// Returns a normalized random variable,
// stdev 1, mean 0
// Not fast, takes log, sqrt, cos, and 2 uniform random variables.
EXTERN float RandNorm(void);

// good one, in the range [0,n)
EXTERN long RandRangeLong(int n);


// These are called Q for Quick, but really they shouldn't be
// all that much faster than the above, it's a mul and add vs 
// 2 adds, 2 branches, and an xor.  So the difference should be very
// slight.  Here they are anyway.

// Only call this if you want, RandomInit does it anyway
EXTERN void RandSeedQ(long seed);

// Call this for a quickee
EXTERN long RandLongQ(void);

// Fast, bad float between [0 and 1)
EXTERN float RandFloatQ(void);

// fast one, in the range [0,n)
EXTERN long RandRangeLongQ(int n);


#if defined(__cplusplus) && !defined(USE_RANDOM_HELPER_FUNCTIONS)

#include <rng.h>
extern RNG *gRNGCongruential;
extern RNG *gRNGFibonacci;
#define RandLong() (gRNGFibonacci->GetLong())
#define RandFloat() (gRNGFibonacci->GetFloat())
#define RandNorm() (gRNGFibonacci->GetNorm())
#define RandRangeLong(x) (gRNGFibonacci->GetRange(x))

#define RandLongQ() (gRNGCongruential->GetLong())
#define RandFloatQ() (gRNGCongruential->GetFloat())
#define RandNormQ() (gRNGCongruential->GetNorm())
#define RandRangeLongQ(x) (gRNGCongruential->GetRange(x))

#endif // !cplusplus



#endif
