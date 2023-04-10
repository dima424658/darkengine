/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phdyn.cpp,v 1.20 2000/02/19 12:32:31 toml Exp $
//
//
//

#include <lg.h>
#include <matrixs.h>
#include <matrix.h>

#ifndef SHIP
#include <config.h>
#endif

#include <creatext.h>

#include <phmod.h>
#include <phdyn.h>
#include <phutils.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysDynData
//

void cPhysDynData::Init(void)
{
   m_currentTime = 0;

   mx_zero_vec(&m_translation.velocity);
   mx_zero_vec(&m_translation.acceleration);
   mx_zero_vec(&m_rotation.velocity);
   mx_zero_vec(&m_rotation.acceleration);

   m_pModel = NULL;
   m_submodel = -1;
}

///////////////////////////////////////

cPhysDynData::cPhysDynData(void)
 : m_flags(0),
   m_mass(1),
   m_elasticity(1),
   m_density(1)
{
   Init();
}

///////////////////////////////////////

cPhysDynData::cPhysDynData(unsigned flags, mxs_real mass, mxs_real elasticity, mxs_real density)
 : m_flags(flags),
   m_mass(mass),
   m_elasticity(elasticity),
   m_density(density)
{
   Init();
}

///////////////////////////////////////////////////////////////////////////////

void cPhysDynData::LoadV11(PhysReadWrite func)
{
   func(&m_pModel, sizeof(cPhysModel *), 1);
   func(&m_submodel, sizeof(int), 1);
   func(&m_flags, sizeof(unsigned), 1);
   func(&m_translation, sizeof(sDynamics), 1);
   func(&m_rotation, sizeof(sDynamics), 1);
   func(&m_mass, sizeof(mxs_real), 1);
   m_density = 1.0;
   func(&m_elasticity, sizeof(mxs_real), 1);
   func(&m_terrainPassThruTime, sizeof(mxs_real), 1);
   func(&m_currentTime, sizeof(mxs_real), 1);
   func(&m_collisionCount, sizeof(int), 1);
}

///////////////////////////////////////////////////////////////////////////////


void cPhysDynData::SetVelocity(const mxs_vector & velocity)
{
   if ((m_translation.velocity.x != velocity.x) ||
       (m_translation.velocity.y != velocity.y) ||
       (m_translation.velocity.z != velocity.z))
   {
      SetFlagState(kDynamicsVelocityChanged, 1);
   }

#ifndef SHIP
   if ((config_is_defined("highvelcheck")) && (m_pModel->IsPlayer()) &&
       (mx_mag2_vec(&velocity)>1000))
     AssertMsg(FALSE,"Very high player velocity achieved.\n");
#endif

   mx_copy_vec(&m_translation.velocity, (mxs_vector *) & velocity);

   if ((m_submodel == -1) && (!m_pModel->IsLocationControlled() && (m_pModel->GetType(0) != kPMT_OBB))) 
   {
      for (int i=0; i<m_pModel->NumSubModels(); i++)
      {
         if (m_pModel->GetSpringTension(i) == 0)
            m_pModel->GetDynamics(i)->SetVelocity(velocity);
      }
   }
   if (!IsZeroVector(velocity))
      m_pModel->SetSleep(FALSE);
}

///////////////////////////////////////

void cPhysDynData::SetRotationalVelocity(const mxs_vector &velocity)
{
   mx_copy_vec(&m_rotation.velocity, (mxs_vector *) &velocity);
   if (!IsZeroVector(velocity))
   {
      m_pModel->SetSleep(FALSE);
      m_pModel->SetRest(FALSE);
   }
}

///////////////////////////////////////


