/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkimcst.h,v 1.5 2000/01/31 09:56:45 adurant Exp $
#pragma once

#ifndef __SHKIMCST_H
#define __SHKIMCST_H

typedef enum eImplant {
   
   kImplantStrength,
   kImplantEndurance,
   kImplantAgility,
   kImplantPsi,
   kImplantMaxHP,
   kImplantRun,
   kImplantAim,
   kImplantTech,
   kImplantResearch,
   kImplantWormMind,
   kImplantWormBlood,
   kImplantWormBlend,
   kImplantWormHeart,

   kImplantPad = 0xFFFFFFFF,
};

const float kImplantWormMindDamageFrac = 0.25;
const float kImplantAimKickFrac = 0.8;
const float kImplantWormBlendFrac = 1.0;

#endif