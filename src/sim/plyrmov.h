/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/plyrmov.h,v 1.5 2000/01/31 10:00:25 adurant Exp $
//
// Player movement header
//
#pragma once

#ifndef __PLYRMOV_H
#define __PLYRMOV_H

#include <matrixs.h>
#include <objtype.h>

class cPlayerMovement;

////////////////////////////////////////

EXTERN void PlayerMovementInit();
EXTERN void PlayerMovementTerm();
EXTERN void PlayerMovementReset();

EXTERN void PlayerMovementUpdate();

EXTERN cPlayerMovement *g_pPlayerMovement;

////////////////////////////////////////

class cPlayerMovement
{
public:

   cPlayerMovement();
   ~cPlayerMovement();

   void Update();

   void LeaveGround();
   void LandOnGround(ObjID ground_obj);
   
   void  SetGroundObj(ObjID ground_obj);
   ObjID GetGroundObj() const;

   void SetLastFoot(const mxs_vector &last_foot_loc, mxs_real foot_time);

   // only 0 or 1 supported currently
   void SetVolume(float volume);

private:

   // State
   mxs_vector m_LastFootLoc;
   int        m_LastFootTime;

   BOOL       m_RightFootNext;
   
   ObjID      m_GroundObj;

   // Config
   int m_LandingMinMs;
   int m_LandingCutoffVel;
   int m_LandingBaseVol;
   int m_LandingVolMul;

   int m_SteppingBaseVol;
   int m_SteppingVolMul;
   int m_SteppingMaxMs;

   float m_volume;
};

////////////////////////////////////////

inline void cPlayerMovement::SetGroundObj(ObjID ground_obj)
{
   m_GroundObj = ground_obj;
}

inline ObjID cPlayerMovement::GetGroundObj() const
{
   return m_GroundObj;
}

inline void cPlayerMovement::SetLastFoot(const mxs_vector &last_foot_loc, mxs_real foot_time)
{
   m_LastFootLoc = last_foot_loc;
   m_LastFootTime = foot_time;
}

inline void cPlayerMovement::SetVolume(float volume)
{
   m_volume = volume;
}

////////////////////////////////////////

#endif
