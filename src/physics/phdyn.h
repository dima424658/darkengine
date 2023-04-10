/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phdyn.h,v 1.15 2000/01/29 13:32:45 adurant Exp $
//
// Moveable objects dynamics data
//
// NOTE: !!! Do not add a virtual function to this data structure.  It will 
//           cause the saving and loading of dynamics data to break. !!!
#pragma once

#ifndef __PHDYN_H
#define __PHDYN_H

#include <lg.h>

#include <matrixs.h>
#include <matrix.h>

#ifdef DBG_ON
#include <mprintf.h>
#endif

///////////////////////////////////////

#include <objtype.h>
#include <wr.h>
// @Note (toml 08-05-97): This is to get at "Position." At some point,
//                        portal should expose Postion and Location in a
//                        separate header so that physics doesn't need to be
//                        100 0ependent on all of wr.h

#include <phystyp2.h>

#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

// For translational and/or rotational data
typedef struct sDynamics
{
   Location   padding; 
   mxs_vector velocity;
   mxs_vector acceleration;
} sDynamics;

enum ePhysDynFlags
{
   kDynamicsTranslatable     = 0x0001,  // we can translate
   kDynamicsRotatable        = 0x0002,  // we can rotate
   kDynamicsTerrainPassThru  = 0x0004,  // we can pass through terrain
   kDyanmicsSubModelPassThru = 0x0008,  // we can pass through another model (identified seperately)
   kDynamicsFrozen           = 0x0010,  // the model is frozen because we can't deal with it
   kDynamicsVelocityChanged  = 0x0020,  // the velocity has changed since flag last cleared
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysDynData
//
// For any moveable object
//

class cPhysDynData
{
public:
   ////////////////////////////////////
   //
   // Constructors
   //
   cPhysDynData();
   cPhysDynData(unsigned flags, mxs_real mass, mxs_real elasticity, mxs_real density);

   void LoadV11(PhysReadWrite func);

   ////////////////////////////////////
   //
   // Translation
   //
   BOOL CanTranslate() const;
   void SetTranslatable(BOOL state);

   ////////////////////////////////////
   //
   // Get/Set velocity
   //
   const mxs_vector & GetVelocity() const;
   mxs_vector *       GetVelocityPtr();
   void               SetVelocity(const mxs_vector & velocity);
   void               ClearVelocityChanged(void);
   BOOL               VelocityChanged(void) const;

   ////////////////////////////////////
   //
   // Get/Set acceleration
   //
   const mxs_vector & GetAcceleration() const;
   void               SetAcceleration(const mxs_vector & acceleration);
   void               ZeroAcceleration();

   ////////////////////////////////////
   //
   // Get/Set rotation
   //
   BOOL               CanRotate() const;
   void               SetRotatable(BOOL state);

   const mxs_vector & GetRotationalVelocity() const;
   void               SetRotationalVelocity(const mxs_vector & velocity);

   const mxs_vector & GetRotationalAcceleration() const;
   void               SetRotationalAcceleration(const mxs_vector & acceleration);

   ////////////////////////////////////
   //
   // Halt the model
   //
   void Stop();

   ////////////////////////////////////
   //
   // per frame count of collisions
   //
   void ZeroCollisionCount();
   int  IncCollisionCount();
   int  CollisionCount() const;

   ////////////////////////////////////
   //
   // True if I'm remote or I've been told to
   //
   BOOL TerrainPassThru() const;
   void SetTerrainPassThru(BOOL);

   ////////////////////////////////////
   //
   // i'm frozen because i've been bad
   //
   BOOL Frozen() const;
   void SetFrozen(BOOL state);

   ////////////////////////////////////
   //
   // for avoiding multiple collisions with the same object
   //
   BOOL SubModelPassThru() const;
   void EnableSubModelPassThru(ObjID objID, tPhysSubModId subModId, mxs_real time);

   ////////////////////////////////////

   mxs_real GetElasticity() const;
   void SetElasticity(mxs_real elasticity);

   mxs_real GetMass() const;
   void SetMass(mxs_real mass);

   mxs_real GetDensity() const;
   void SetDensity(mxs_real density);

   ////////////////////////////////////
   //
   // Sub-frame stuff
   //
   mxs_real           GetCurrentTime() const;
   void               SetCurrentTime(mxs_real time);


   ////////////////////////////////////

   cPhysModel *  GetModel() const;
   void          SetModel(cPhysModel *pModel);
   tPhysSubModId GetSubModId() const;
   void          SetSubModId(tPhysSubModId);

   ////////////////////////////////////
#ifdef DBG_ON
   void MonoPrintAcceleration() const;
   void MonoPrintVelocity() const;
#endif

private:
   void Init();
   void SetFlagState(unsigned flag, BOOL state);

   cPhysModel  *m_pModel;
   int          m_submodel;

   unsigned     m_flags;

   sDynamics    m_translation;
   sDynamics    m_rotation;

   // for determining bounce results
   mxs_real     m_mass;
   mxs_real     m_elasticity;
   mxs_real     m_density;

   // terrain pass thru until this time
   mxs_real     m_terrainPassThruTime;

   // sub-frame data
   mxs_real     m_currentTime;

   // collisions/frame
   int          m_collisionCount;

};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysDynData, inline functions
//

inline void cPhysDynData::SetFlagState(unsigned flag, BOOL state)
{
   if (state)
      m_flags |= flag;
   else
      m_flags &= ~flag;
}

///////////////////////////////////////

inline BOOL cPhysDynData::CanTranslate() const
{
   return m_flags & kDynamicsTranslatable;
}

///////////////////////////////////////

inline void cPhysDynData::SetTranslatable(BOOL state)
{
   SetFlagState(kDynamicsTranslatable, state);
}

///////////////////////////////////////

inline void cPhysDynData::Stop()
{
   mx_zero_vec(&m_translation.velocity);
   mx_zero_vec(&m_rotation.velocity);
}

///////////////////////////////////////

inline const mxs_vector & cPhysDynData::GetVelocity() const
{
   return m_translation.velocity;
}

///////////////////////////////////////

inline mxs_vector *cPhysDynData::GetVelocityPtr()
{
   return &m_translation.velocity;
}

///////////////////////////////////////

inline void cPhysDynData::ClearVelocityChanged(void)
{
   SetFlagState(kDynamicsVelocityChanged, 0);
}

///////////////////////////////////////

inline BOOL cPhysDynData::VelocityChanged(void) const
{
   return !!(m_flags&kDynamicsVelocityChanged);
}

///////////////////////////////////////

inline const mxs_vector & cPhysDynData::GetAcceleration() const
{
   return m_translation.acceleration;
}

inline void cPhysDynData::SetAcceleration(const mxs_vector & acceleration)
{
   mx_copy_vec(&m_translation.acceleration, (mxs_vector *) & acceleration);
}

///////////////////////////////////////

inline void cPhysDynData::ZeroAcceleration()
{
   mx_zero_vec(&m_translation.acceleration);
   mx_zero_vec(&m_rotation.acceleration);
}

///////////////////////////////////////

inline mxs_real cPhysDynData::GetElasticity() const
{
   return m_elasticity;
}

///////////////////////////////////////

inline void cPhysDynData::SetElasticity(mxs_real elasticity)
{
   if (elasticity < 0.0)
   {
      Warning(("SetElasticity: invalid value: 0\n", elasticity));
      return;
   }
   m_elasticity = elasticity;
}

///////////////////////////////////////

inline mxs_real cPhysDynData::GetDensity() const
{
   return m_density;
}

///////////////////////////////////////

inline void cPhysDynData::SetDensity(mxs_real density)
{
   m_density = density;
}

///////////////////////////////////////

inline BOOL cPhysDynData::CanRotate() const
{
   return m_flags & kDynamicsRotatable;
}

///////////////////////////////////////

inline void cPhysDynData::SetRotatable(BOOL state)
{
   SetFlagState(kDynamicsRotatable, state);
}

///////////////////////////////////////

inline const mxs_vector &cPhysDynData::GetRotationalVelocity() const
{
   return m_rotation.velocity;
}

///////////////////////////////////////

inline const mxs_vector &cPhysDynData::GetRotationalAcceleration() const
{
   return m_rotation.acceleration;
}

///////////////////////////////////////

inline void cPhysDynData::SetRotationalAcceleration(const mxs_vector &acceleration)
{
   mx_copy_vec(&m_rotation.acceleration, (mxs_vector *) &acceleration);
}

///////////////////////////////////////

inline BOOL cPhysDynData::Frozen() const
{
   return m_flags & kDynamicsFrozen;
}

///////////////////////////////////////

inline void cPhysDynData::SetFrozen(BOOL state)
{
   SetFlagState(kDynamicsFrozen, state);
}

///////////////////////////////////////

inline BOOL cPhysDynData::TerrainPassThru() const
{
   return m_flags & kDynamicsTerrainPassThru;
}

///////////////////////////////////////

inline void cPhysDynData::SetCurrentTime(mxs_real time)
{
   m_currentTime = time;
}

///////////////////////////////////////

inline mxs_real cPhysDynData::GetCurrentTime() const
{
   return m_currentTime;
}

///////////////////////////////////////

inline mxs_real cPhysDynData::GetMass() const
{
   return m_mass;
}

///////////////////////////////////////

inline void cPhysDynData::SetMass(mxs_real mass)
{
   m_mass = mass;
}

///////////////////////////////////////

inline void cPhysDynData::ZeroCollisionCount()
{
   m_collisionCount = 0;
}

///////////////////////////////////////

inline int cPhysDynData::IncCollisionCount()
{
   return ++m_collisionCount;
}

///////////////////////////////////////

inline int cPhysDynData::CollisionCount() const
{
   return m_collisionCount;
}

///////////////////////////////////////

inline cPhysModel * cPhysDynData::GetModel() const
{
   return m_pModel;
}

///////////////////////////////////////

inline void cPhysDynData::SetModel(cPhysModel *pModel)
{
   m_pModel = pModel;
}

///////////////////////////////////////

inline tPhysSubModId cPhysDynData::GetSubModId() const
{
   return m_submodel;
}

///////////////////////////////////////

inline void cPhysDynData::SetSubModId(tPhysSubModId subModId)
{
   m_submodel = subModId;
}

///////////////////////////////////////

#ifdef DBG_ON

///////////////////////////////////////

inline void cPhysDynData::MonoPrintAcceleration() const
{
   mprintf("Acceleration: 0 0 0\n",
           m_translation.acceleration.x,
           m_translation.acceleration.y,
           m_translation.acceleration.z);
}


///////////////////////////////////////

inline void cPhysDynData::MonoPrintVelocity() const
{
   mprintf("Velocity: 0 0 0\n",
           m_translation.velocity.x,
           m_translation.velocity.y,
           m_translation.velocity.z);
}
#endif

///////////////////////////////////////////////////////////////////////////////
#pragma pack()

#endif /* !__PHDYN_H */
