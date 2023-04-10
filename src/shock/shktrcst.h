/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shktrcst.h,v 1.7 2000/01/31 09:59:27 adurant Exp $
#pragma once

#ifndef __SHKTRCST_H
#define __SHKTRCST_H

typedef enum eTrait {
   kTraitEmpty,      // 00

   // 01
   kTraitMetabolism, 
   kTraitPharmo,
   kTraitPackRat,
   kTraitSpeedy,
   kTraitSharpshooter,

   // 06
   kTraitAble,
   kTraitCybernetic,
   kTraitTank,
   kTraitLethal,     
   kTraitSecurity,

   // 11
   kTraitSmasher,
   kTraitBorg,
   kTraitReplicator,
   kTraitPsionic,
   kTraitTinker,

   // 16
   kTraitAutomap,

   kTraitMax,
};

#define NUM_TRAIT_SLOTS 4

#endif