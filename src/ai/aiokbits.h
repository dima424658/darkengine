/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ai/aiokbits.h,v 1.7 2000/01/29 12:45:33 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   aiokbits.h

   okBits tell you whether a given AI can follow a given link in the
   pathfinding database.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _AIOKBITS_H_
#define _AIOKBITS_H_

// As of 4/98 each path link has a uchar for okBits.  If this grows
// be sure to update kAIOK_All.

#define kAIOK_Walk      0x01
#define kAIOK_Fly       0x02
#define kAIOK_Swim      0x04

// wsf: this is not used, so we'll use it for "small creature" pathing.
// #define kAIOK_Climb     0x08

#define kAIOK_SmallCreature 0x08


// wsf: these are not used, so I'm making my own.
// These are or'd in to link database for compact storage.
// #define kAIOK_Hoist     0x10
// #define kAIOK_JumpDown  0x20
#define kAIOKCOND_Stressed     0x10

// When cell linked to higher cell, within striking distance, but can't path.
#define kAIOKCOND_HighStrike  0x20

#define kConditionMask 0x30
#define kNoConditionMask 0xcf

// These are installed by the app at runtime and evaluated through
// callbacks.  Which makes them much more expensive than the others.
#define kAIOK_App1      0x40
#define kAIOK_App2      0x80

// 'All' is everything except HighStrike, SmallCreature, and Stressed:
#define kAIOK_All       (~(kAIOKCOND_HighStrike|kAIOKCOND_Stressed|kAIOK_SmallCreature))
#define kAIOK_Normal    0x07    // all but App1 & App2 & SmallCreature
#define kAIOK_AppAll    (kAIOK_App1 | kAIOK_App2)

// wsf 10/25/99: Here's something new. Orthogonal to the okbits are the okcondbits. These are
// extra "condition" bits that further qualify the okbits. For example, only path to this cell
// if the ai is in such 'n such a state.
//
// Currently, there are only two states:
//    stressed: Probably, the AI will determine "Stressed" to be a high alert level.
//    was stressed: was stressed within the past 'n' seconds, but is not any more.
//
// #define kAIOKCOND_Stressed 0x01
// #define kAIOKCOND_WasStressed 0x02
// #define kAIOKCOND_All 0xff
//
// wsf: we made these part of kAIOK bits...


#endif // ~_AIOKBITS_H_
