/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/plyrspd.h,v 1.9 2000/03/09 22:56:13 adurant Exp $
//
//
//
#pragma once

#ifndef __PLYRSPD_H
#define __PLYRSPD_H

EXTERN float g_PlayerSlewSpeedScale;
EXTERN float g_PlayerRotateSpeedScale;

// misc magic numbers
#define RAW_SLEW_SPEED   11.0
#define RAW_ROTATE_SPEED 3.14    // 360 degrees/sec
#define RAW_JUMP_SPEED   14.0

#define MOVE_SPEED      (RAW_SLEW_SPEED)
#define SLOW_MOVE_SPEED (MOVE_SPEED / 2)
#define FAST_MOVE_SPEED (MOVE_SPEED * 2)

#define SIDESTEP_SPEED      (MOVE_SPEED * 0.7)
#define SLOW_SIDESTEP_SPEED (SIDESTEP_SPEED / 2)

#define BACK_SPEED       -(MOVE_SPEED / 2)
#define SLOW_BACK_SPEED   (BACK_SPEED / 2)
#define FAST_BACK_SPEED   (BACK_SPEED * 2)

#define ROTATE_SPEED      (RAW_ROTATE_SPEED)
#define SLOW_ROTATE_SPEED (ROTATE_SPEED / 2)

#define JUMP_SPEED (RAW_JUMP_SPEED)

#endif /* !__PLYRSPD_H */
