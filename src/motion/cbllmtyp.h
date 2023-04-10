/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/cbllmtyp.h,v 1.4 2000/01/31 09:50:04 adurant Exp $
#pragma once

#ifndef __CBLLMTYP_H
#define __CBLLMTYP_H

typedef enum eManeuverTransitionType
{
   kMnvrTrans_Immediate,
   kMvnrTrans_AtEnd,
   kMnvrTrans_Invalid=0xff,
} eManeuverTransitionType;

// XXX NOTE: even idle motions actually preclude locomotions currently,
// since locomotive maneuvers are never explicity requested as such. KJ 1/98
typedef enum eManeuverPriority
{
   kMnvrPri_Death, // highest priority
   kMnvrPri_Script,
   kMnvrPri_Combat,
   kMnvrPri_Normal,
   kMnvrPri_Locomote,
   kMnvrPri_Idle,
   kMnvrPri_Invalid=0xff,
} eManeuverPriority;


#ifdef __cplusplus
   
typedef class ICerebellum ICerebellum;

#endif

#endif
