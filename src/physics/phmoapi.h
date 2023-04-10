/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmoapi.h,v 1.14 2000/01/29 13:32:50 adurant Exp $
//
// Physics motion api
//
#pragma once

#ifndef __PHMOAPI_H
#define __PHMOAPI_H

#include <matrixs.h>

enum ePlayerMotion
{
   kMoInvalid = -1,

   kMoNormal,
   kMoStrideLeft,
   kMoStrideRight,

   kMoCrouch,
   kMoCrawlLeft,
   kMoCrawlRight,

   kMoWithBody,
   kMoWithBodyLeft,
   kMoWithBodyRight,

   kMoJumpLand,
   kMoWeaponSwing,
   kMoWeaponSwingCrouch,

   kMoLeanLeft,
   kMoLeanRight,
   kMoLeanForward,

   kMoCrouchLeanLeft,
   kMoCrouchLeanRight,
   kMoCrouchLeanForward,

   kMoDisable = 1000,
   kMoEnable,
   kBigNum = 0x10000,
};

typedef enum ePlayerMotion ePlayerMotion;

////////////////////////////////////////////////////////////////////////////////

EXTERN void PhysAdjustPlayerHead(mxs_vector *pos, mxs_angvec *ang);

EXTERN void InitPlayerMotion();
EXTERN void TermPlayerMotion();
EXTERN void ResetPlayerMotion();

EXTERN void          PlayerMotionActivate(ePlayerMotion motion);
EXTERN ePlayerMotion PlayerMotionGetActive();

EXTERN void          PlayerMotionActivateList(ePlayerMotion *motions, int size);

EXTERN void          PlayerMotionSetRest(ePlayerMotion restMotion);
EXTERN ePlayerMotion PlayerMotionGetRest();

EXTERN void PlayerMotionSetOffset(short submod, mxs_vector *offset);         
EXTERN void PlayerMotionGetOffset(short submod, mxs_vector *offset);

EXTERN void PlayerMotionUpdate(mxs_real dt);

EXTERN mxs_vector PlayerGetEyeOffset();

////////////////////////////////////////////////////////////////////////////////

#endif // __PHMOAPI_H
