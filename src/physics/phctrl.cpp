/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/physics/phctrl.cpp,v 1.36 2000/02/19 12:32:14 toml Exp $
//
//
//

#include <lg.h>
#include <matrixs.h>
#include <matrix.h>
#include <objpos.h>
#include <camera.h>
#include <portal.h>
#include <plyrspd.h>

#include <physapi.h>
#include <phystyp2.h>
#include <phconst.h>
#include <phmod.h>
#include <phctrl.h>
#include <phdyn.h>
#include <phclimb.h>
#include <phutils.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysCtrlData
//


cPhysCtrlData::cPhysCtrlData() : m_type(kPCT_NoControl), m_flags(0)
{
   m_time = 0.0;
   mx_zero_vec(&m_velocity);
   mx_zero_vec(&m_rotation);
   m_velocity_rate = 0.0;
   m_rotation_rate = 0.0;

   for(int i=0; i<3; i++)
   {
      m_axis[i] = FALSE;
      m_speed[i] = 0.0;
   }

   m_pModel = NULL;
   m_submodel = -1;
}


cPhysCtrlData::~cPhysCtrlData()
{

}

///////////////////////////////////////

void cPhysCtrlData::SetControlVelocity(const int axis, const mxs_real speed, mxs_real rate)
{
   m_pModel->SetSleep(FALSE);

   m_type |= kPCT_VelControl;
   m_type &= ~kPCT_LocControl;

   m_axis[axis] = TRUE;
   m_speed[axis] = speed;

   m_velocity_rate = rate;
}

void cPhysCtrlData::SetControlVelocity(const mxs_vector &velocity, mxs_real rate)
{
   m_pModel->SetSleep(FALSE);
  
   m_type |= kPCT_VelControl;
   m_type &= ~kPCT_LocControl;

   mx_copy_vec((mxs_vector *) &m_velocity, (mxs_vector *) &velocity);
   m_velocity_rate = rate;

   // Turn off all axis controls
   for (int i=0; i<3; i++)
   {
      m_axis[i]  = FALSE;
      m_speed[i] = 0.0;
   }
}

///////////////////////////////////////

void cPhysCtrlData::StopControlVelocity()
{
   mx_zero_vec(&m_velocity);
   
   for (int i=0; i<3; i++)
   {
      m_axis[i]  = FALSE;
      m_speed[i] = 0.0;
   }

   m_type &= ~kPCT_VelControl;
}

void cPhysCtrlData::StopControlVelocity(const int axis)
{
   BOOL all_off = TRUE;

   m_axis[axis] = FALSE;
   m_speed[axis] = 0.0;

   for (int i=0; i<3 && all_off; i++)
   {
      if (m_axis[i])
         all_off = FALSE;
   }

   if (all_off)
   {
      m_type &= ~kPCT_VelControl;
      mx_zero_vec(&m_velocity);
   }
}

///////////////////////////////////////

void cPhysCtrlData::ControlVelocity()
{
   cPhysDynData *pDyn;
   mxs_vector    ctrl_accel;
   mxs_vector    accel;
   mxs_real      friction;
   mxs_real      rate;

   pDyn = m_pModel->GetDynamics();

   // If it's axis controlled, update vector based on current axis position
   if (m_axis[0] || m_axis[1] || m_axis[2])
   {
      mxs_matrix m;

      // @TODO: change this to use player modes
      if (m_pModel->IsPlayer())
      {
         mxs_angvec facing;

         facing.tx = (short)(m_pModel->GetRotation(PLAYER_HEAD).tx + m_pModel->GetRotation().tx);
         facing.ty = (short)(m_pModel->GetRotation(PLAYER_HEAD).ty + m_pModel->GetRotation().ty);
         facing.tz = (short)(m_pModel->GetRotation(PLAYER_HEAD).tz + m_pModel->GetRotation().tz);

         mx_ang2mat(&m, &facing);
      }
      else
         mx_ang2mat(&m, &m_pModel->GetRotation());

      mx_zero_vec(&m_velocity);
      for (int i=0; i<3; i++)
      {
         if (!m_axis[i])
            continue;

         mx_scale_addeq_vec(&m_velocity, &m.vec[i], m_speed[i]);
      }
   }

   // If the object's the player and we're not flying or swimming,
   // don't allow control outside the gravity-perpendicular plane (for just head tilt)
   if (m_pModel->IsPlayer() && (m_pModel->GetBaseFriction() < .0001) && !m_axis[2] && (m_pModel->GetClimbingObj() == OBJ_NULL)) 
   {
      mxs_vector grav_comp;
      mxs_real mag;

      mag = mx_mag_vec(&m_velocity);

      mx_scale_vec(&grav_comp, &kGravityDir,  mx_dot_vec(&kGravityDir, &m_velocity));
      mx_subeq_vec(&m_velocity, &grav_comp);

      if (!IsZeroVector(m_velocity))
         mx_normeq_vec(&m_velocity);
      mx_scaleeq_vec(&m_velocity, mag);
   }

#if 0
   if (m_pModel->IsClimbing())
   {
      m_velocity.z += 3;
      if (m_velocity.z > MOVE_SPEED)
         m_velocity.z = MOVE_SPEED;

      mx_scaleeq_vec(&m_velocity, 0.5);
   }
#endif

   // Determine the delta that we need to apply to reach our
   // desired velocity
   mx_sub_vec(&ctrl_accel, (mxs_vector *)&m_velocity, &pDyn->GetVelocity());

   if (m_pModel->IsPlayer() && (m_pModel->GetBaseFriction() <= 0) && !m_axis[2] && (m_pModel->GetClimbingObj() == OBJ_NULL))
   {
      mxs_vector grav_comp;

      mx_scale_vec(&grav_comp, &kGravityDir, mx_dot_vec(&kGravityDir, &ctrl_accel));
      mx_subeq_vec(&ctrl_accel, &grav_comp);
   }

   // Determine the impulse acceleration for us to reach the velocity, with friction
   PhysGetFriction(m_pModel->GetObjID(), &friction);
   rate = 11 * pDyn->GetMass() * friction / m_velocity_rate;
   if (rate > 2000)
      rate = 2000;
   mx_scaleeq_vec(&ctrl_accel, rate);

   // Add in acceleration
   mx_copy_vec(&accel, (mxs_vector *) &pDyn->GetAcceleration());
   mx_addeq_vec(&accel, &ctrl_accel);
   pDyn->SetAcceleration(accel);
}

///////////////////////////////////////

void cPhysCtrlData::SetControlRotationalVelocity(const mxs_vector &velocity, mxs_real rate)
{
   m_pModel->SetSleep(FALSE);

   m_type |= kPCT_RotVelControl;

   mx_copy_vec(&m_rotation, (mxs_vector *) &velocity);
   m_rotation_rate = rate;
}

///////////////////////////////////////

void cPhysCtrlData::ControlRotationalVelocity()
{
   cPhysModel   *pModel = m_pModel;
   cPhysDynData *pDyn;
   mxs_vector    delta;
   mxs_vector    ctrl_delta;
   mxs_vector    accel;
   mxs_real      friction;
   mxs_real      rate;

   if (m_submodel == -1)
      pDyn = m_pModel->GetDynamics();
   else
      pDyn = m_pModel->GetDynamics(m_submodel);

   // Calculate delta 
   mx_sub_vec(&delta, &m_rotation, (mxs_vector *) &pDyn->GetRotationalVelocity());

   // Determine the acceleration (factor in mass if using base friction)
   PhysGetFriction(pModel->GetObjID(), &friction);
   rate = 5.5 * pModel->GetDynamics()->GetMass() * friction / m_rotation_rate;
   if (rate > 1000)
      rate = 1000;
   mx_scale_vec(&ctrl_delta, &delta, rate);   

   // Add in acceleration
   mx_copy_vec(&accel, (mxs_vector *) &pDyn->GetRotationalAcceleration());
   mx_addeq_vec(&accel, &ctrl_delta);
   pDyn->SetRotationalAcceleration(accel);
}

///////////////////////////////////////

void cPhysCtrlData::ControlLocation(const mxs_vector &location)
{
   mxs_vector  zero;
   mx_zero_vec(&zero);

   m_type |= kPCT_LocControl;
   m_type &= ~kPCT_VelControl;

   // Set a base friction in case we're floating
   if (!m_pModel->IsCreature())
      m_pModel->SetBaseFriction(kFrictionFactor * kGravityAmt);

   // Zero our velocity
   m_pModel->GetDynamics()->SetVelocity(zero);

   // Set location and endlocation
   if (m_submodel == -1)
   {
      m_pModel->SetLocationVec(location);
      m_pModel->SetEndLocationVec(location);
   }
   else
   {
      m_pModel->SetLocationVec(m_submodel, location);
      m_pModel->SetEndLocationVec(m_submodel, location);
   }
}

///////////////////////////////////////

void cPhysCtrlData::StopControlLocation()
{
   m_type &= ~kPCT_LocControl;

   if ((m_submodel == -1) && (m_pModel->GetType(0) != kPMT_OBB))
   {
      for (int i=0; i<m_pModel->NumSubModels(); i++)
         m_pModel->GetControls(i)->StopControlLocation();

      m_pModel->SetSleep(FALSE);
   }
}

///////////////////////////////////////

void cPhysCtrlData::ControlRotation(const mxs_angvec &rotation)
{
   mxs_vector zero;

   m_type |= kPCT_RotControl;
   if (m_submodel != -1)
      m_pModel->SetRotation(m_submodel, rotation);

   mx_zero_vec(&zero);
   m_pModel->GetDynamics()->SetRotationalVelocity(zero);
   
}

///////////////////////////////////////
