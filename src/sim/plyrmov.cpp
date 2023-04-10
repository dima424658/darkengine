/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/plyrmov.cpp,v 1.7 1998/11/18 10:41:10 JON Exp $
//
// Player movement
//

#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <config.h>
#include <matrix.h>
#include <simtime.h>

#include <propman.h>
#include <flowarch.h>

#include <ctagset.h>
#include <esnd.h>

#include <objmedia.h>
#include <physapi.h>
#include <phcore.h>
#include <phmods.h>
#include <phmod.h>
#include <phmoapi.h>
#include <phmterr.h>

#include <playrobj.h>
#include <plyrmov.h>
#include <plyrmode.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////

cPlayerMovement *g_pPlayerMovement = NULL;

////////////////////////////////////////

void PlayerMovementInit()
{
   Assert_(g_pPlayerMovement == NULL);

   g_pPlayerMovement = new cPlayerMovement;
}

void PlayerMovementTerm()
{
   Assert_(g_pPlayerMovement != NULL);

   delete g_pPlayerMovement;
   g_pPlayerMovement = NULL;
}

void PlayerMovementReset()
{
   if (g_pPlayerMovement != NULL)
      PlayerMovementTerm();
   PlayerMovementInit();
}

void PlayerMovementUpdate()
{
   Assert_(g_pPlayerMovement != NULL);

   g_pPlayerMovement->Update();
}

////////////////////////////////////////

cPlayerMovement::cPlayerMovement()
{
   // State
   mx_zero_vec(&m_LastFootLoc);
   m_LastFootTime = -1;

   m_RightFootNext = FALSE;

   m_GroundObj = OBJ_NULL;

   // Config
   m_LandingMinMs = 200;
   m_LandingCutoffVel = 2;
   m_LandingBaseVol = -1000;
   m_LandingVolMul = 250;

   m_SteppingBaseVol = -1800;
   m_SteppingVolMul = 150;
   m_SteppingMaxMs = 800;
   
   m_volume = 1.0;

#ifndef SHIP
   config_get_int("landing_min_ms", &m_LandingMinMs);
   config_get_int("landing_cutoff_vel", &m_LandingCutoffVel);
   config_get_int("landing_base_vol", &m_LandingBaseVol);
   config_get_int("landing_vol_mul" ,&m_LandingVolMul);

   config_get_int("stepping_base_vol", &m_SteppingBaseVol);
   config_get_int("stepping_vol_mul", &m_SteppingVolMul);
   config_get_int("stepping_max_ms", &m_SteppingMaxMs);
#endif
}

cPlayerMovement::~cPlayerMovement()
{

}

////////////////////////////////////////

void cPlayerMovement::LeaveGround()
{
   if (!PlayerObjectExists())
      return;

   m_GroundObj = OBJ_NULL;

   PhysGetSubModLocation(PlayerObject(), PLAYER_FOOT, &m_LastFootLoc);
   m_LastFootTime = GetSimTime();
}

void cPlayerMovement::LandOnGround(ObjID ground_obj)
{
   if (!PlayerObjectExists())
      return;

   m_GroundObj = ground_obj;

   // Has there been enough time since our last landing?
   if ((GetSimTime() - m_LastFootTime) > m_LandingMinMs)
   {
      mxs_vector foot_loc;
      mxs_vector velocity;
      mxs_real   velocity_mag;

      PhysGetVelocity(PlayerObject(), &velocity);
      velocity_mag = mx_mag_vec(&velocity);

      PhysGetSubModLocation(PlayerObject(), PLAYER_FOOT, &foot_loc);

      // Are we moving fast enough?
      if (velocity.z < -m_LandingCutoffVel)
      {
         g_pPlayerMode->ActivateLandingMotion();

         if (m_volume>0)
         {
            int play_vol = m_LandingBaseVol - m_LandingVolMul * (m_LandingCutoffVel + velocity.z);
            if (play_vol >= 0)
               play_vol = -1;
            g_pPlayerMode->DoLandingSound(play_vol, foot_loc);
         }
      }
   }
}

////////////////////////////////////////

#define sq(x)  ((x)*(x))

void cPlayerMovement::Update()
{
   if (!PlayerObjectExists())
      return;

   if (IsMovingTerrain(m_GroundObj))
      return;

   mxs_vector velocity;
   mxs_real   velocity_mag;

   PhysGetVelocity(PlayerObject(), &velocity);
   velocity_mag = mx_mag_vec(&velocity);

   float footstep_dist;
   if (velocity_mag < 5.0)
      footstep_dist = 2.5;
   else
   if (velocity_mag > 15.0)
      footstep_dist = 4.0;
   else
      footstep_dist = 2.5 + 3.0 * ((velocity_mag - 5.0) / 10.0);

   mxs_vector foot_loc;
   PhysGetSubModLocation(PlayerObject(), PLAYER_FOOT, &foot_loc);

   // Have we moved enough to step?
   mxs_real cur_dist_2 = mx_dist2_vec(&m_LastFootLoc, &foot_loc);

   if ((velocity_mag > 1.0) && ((m_LastFootTime < 0) || (cur_dist_2 > sq(footstep_dist))))
   {
      BOOL on_ground = PhysObjOnGround(PlayerObject());
      ePlayerMode mode = g_pPlayerMode->GetMode();

      // Update our last footfall location/time
      if ((mode != kPM_Jump) && ((mode != kPM_Stand) || on_ground))
      {
         m_LastFootLoc = foot_loc;
         m_LastFootTime = GetSimTime();
      }

      // Update head movement
      if (velocity_mag > 1.5)
      {
         BOOL do_stride = TRUE;

         // Stand must be on ground to do head-bob
         if ((g_pPlayerMode->GetMode() == kPM_Stand) || 
             (g_pPlayerMode->GetMode() == kPM_Crouch) ||
             (g_pPlayerMode->GetMode() == kPM_BodyCarry))
         {
            if (!PhysObjOnGround(PlayerObject()))
               do_stride = FALSE;
         }

         if (do_stride)
         {
            g_pPlayerMode->ActivateStrideMotion(!m_RightFootNext);

            // Switch which foot is next
            m_RightFootNext = !m_RightFootNext;

            if (m_volume>0)
            {
               int play_vol = m_SteppingBaseVol + (m_SteppingVolMul * velocity_mag);
               if (play_vol >= 0)
                  play_vol = -1;
               
               g_pPlayerMode->DoStrideSound(play_vol, foot_loc);
            }
         }
      }
   }

   // See if we're coming to a stop
   if (((GetSimTime() - m_LastFootTime) > 100) && (velocity_mag > 0.1) && 
       ((cur_dist_2 > sq(footstep_dist / 2.0)) || (velocity_mag < 1.5)))
   {
      g_pPlayerMode->ActivateRestMotion();
   }

   // Have we stopped moving?      
   if (velocity_mag < 1.0)
      m_LastFootTime = -1.0;
}

////////////////////////////////////////











