//		Rnd.H		Random stream header file (see rnd.c for more info)
//		Rex E. Bradford (REX)
/*
* $Header: x:/prj/tech/libsrc/rnd/RCS/rnd.h 1.6 1997/02/20 15:44:33 REX Exp $
* $Log: rnd.h $
 * Revision 1.6  1997/02/20  15:44:33  REX
 * Added RndGetInternalState() and RndPutInternalState()
 * 
 * Revision 1.5  1997/01/31  13:38:04  TOML
 * w106 C++ friendly
 * 
 * Revision 1.4  1996/09/10  17:19:29  BODISAFA
 * Added recording support for seeds.
 *
 * Revision 1.3  1996/02/20  13:56:41  DAVET
 * Added cplusplus stuff
 *
 * Revision 1.2  1993/04/06  10:33:57  rex
 * Fixed RndSeed() macro to pass seed!
 *
 * Revision 1.1  1993/04/06  09:56:35  rex
 * Initial revision
 *
*/

#ifndef RND_H
#define RND_H

#include "types.h"
#include "fix.h"

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus


//	A random stream

typedef struct RndStream_ {
	ulong curr;
	ulong (*f_Next)(struct RndStream_ *prs);
	void (*f_Seed)(struct RndStream_ *prs, ulong seed);
} RndStream;

//	To use a random stream, instantiate one (usually statically),
//	seed it, and then make calls to get rnums, like so:
//
//		static RNDSTREAM_STD(rs);			// declare a stream
//		RndSeed(&rs,22);						// or maybe 23
//		rval = Rnd(&rs);						// get any old rnum
//		rval = RndRange(&rs,1,6);			// throw dice
//		rfix = RndFix(&rs);					// maybe you'd like 0 to .9999
//		rfix = RndRangeFix(&rs,fl,fh);	// or fixed point in a range

//	Here are the random stream type declaration macros

#define RNDSTREAM_LC16(name) RndStream name = {0,RndLc16,RndLc16Seed}
#define RNDSTREAM_GAUSS16(name) RndStream name = {0,RndGauss16,RndGauss16Seed}
#define RNDSTREAM_GAUSS16FAST(name) RndStream name = {0,RndGauss16Fast,RndGauss16FastSeed}

#define RNDSTREAM_STD(name) RNDSTREAM_LC16(name)

//	Seed a random stream

void RndSeed(RndStream *prs, ulong seed);

//	Get next random #

#define Rnd(prs) ((prs)->f_Next(prs))

//	Get next random # and scale into fixed range 0.0 to .9999

#define RndFix(prs) (fix_make(0,Rnd(prs)>>16))

//	Get next random # and scale into low->high range (high value included)

long RndRange(RndStream *prs, long low, long high);

//	Get next random # and scale into low->high range

fix RndRangeFix(RndStream *prs, fix low, fix high);

// Save and restore current random stream "internal state"

#define RndGetInternalState(prs) ((prs)->curr)
#define RndPutInternalState(prs,val) ((prs)->curr=(val))

//	Prototypes for current set of random stream classes

ulong RndLc16(RndStream *prs);
void  RndLc16Seed(RndStream *prs, ulong seed);

ulong RndGauss16(RndStream *prs);
void  RndGauss16Seed(RndStream *prs, ulong seed);

ulong RndGauss16Fast(RndStream *prs);
void  RndGauss16FastSeed(RndStream *prs, ulong seed);

#ifdef __cplusplus
}
#endif  // cplusplus

#endif

