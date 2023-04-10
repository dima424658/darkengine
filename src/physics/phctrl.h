/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phctrl.h,v 1.17 2000/01/29 13:32:43 adurant Exp $
//
// Physics controls
//
// NOTE: !!! Do not add a virtual function to this data structure.  It will
//           cause the saving and loading of control data to break. !!!
//
// @TODO: not much of this is implemented yet...
//
#pragma once

#ifndef __PHCTRL_H
#define __PHCTRL_H

#pragma pack(4)

class cPhysModel;

///////////////////////////////////////////////////////////////////////////////
//
// Control
//

// translational control types
enum eControlTranslationTypeEnum
{
   kControlVelocity = 0x01,
   kControlLocation = 0x02,
};

// rotational control types
enum eControlRotationTypeEnum
{
   kControlRotationalVelocity = 0x10,
   kControlRotationalLocation = 0x20,
};

///////////////////////////////////////////////////////////////////////////////

enum ePhysicsCtrlFlagsEnum
{
   kPCF_Valid = 0x0100,                       // control values are set
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysCtrlData
//
// for any controlled subobject
//

class cPhysCtrlData
{
public:
   cPhysCtrlData();
   ~cPhysCtrlData();

   void SetControlVelocity(const int axis, const mxs_real speed, mxs_real rate = 1.0);
   void SetControlVelocity(const mxs_vector & velocity, mxs_real rate = 1.0);
   void StopControlVelocity(const int axis);
   void StopControlVelocity();
   void ControlVelocity();


   void SetControlRotationalVelocity(const mxs_vector & velocity, mxs_real rate = 1.0);
   void StopControlRotationalVelocity();
   void ControlRotationalVelocity();

   void ControlLocation(const mxs_vector & location);
   void StopControlLocation();

   void ControlRotation(const mxs_angvec & rotation);
   void StopControlRotation();

   mxs_vector & GetControlVelocity();
   mxs_real     GetControlAxisVelocity(int axis);
   mxs_vector & GetControlRotationalVelocity();

   int  GetType() const;
   BOOL AxisControlled() const;

   void SetModel(cPhysModel *pModel);
   void SetSubModId(int submod_id);

private:

   cPhysModel   *m_pModel;         // Pointer to containing model
   int           m_submodel;       // Index of controlled submodel

   int           m_type;           // @TBD: Should these be flags? 
                                   //       Multiple controls at once?

   BOOL          m_axis[3];        // Controlling along this axis? (update via axis)
   mxs_real      m_speed[3];       // Controlling speed along axis

   unsigned      m_flags;          // @TBD: Needed?
   mxs_real      m_time;           // When to reach position

   mxs_vector    m_velocity;       // For velocity control
   mxs_real      m_velocity_rate;

   mxs_vector    m_rotation;       // For rotational velocity control
   mxs_real      m_rotation_rate;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysCtrlData, inline functions
//

inline void cPhysCtrlData::StopControlRotationalVelocity()
{
   m_type &= ~kPCT_RotVelControl;
}

///////////////////////////////////////

inline void cPhysCtrlData::StopControlRotation()
{
   m_type &= ~kPCT_RotControl;
}

///////////////////////////////////////

inline mxs_vector & cPhysCtrlData::GetControlVelocity()
{
   return m_velocity;
}

///////////////////////////////////////

inline mxs_real cPhysCtrlData::GetControlAxisVelocity(int axis)
{
   return (m_axis[axis] ? m_speed[axis] : 0.0);
}

///////////////////////////////////////

inline mxs_vector & cPhysCtrlData::GetControlRotationalVelocity()
{
   return m_rotation;
}

///////////////////////////////////////

inline BOOL cPhysCtrlData::AxisControlled() const
{
   return (m_axis[0] | m_axis[1] | m_axis[2]);
}

///////////////////////////////////////

inline int cPhysCtrlData::GetType() const
{
   return m_type;
}

///////////////////////////////////////

inline void cPhysCtrlData::SetModel(cPhysModel *pModel)
{
   m_pModel = pModel;
}

///////////////////////////////////////

inline void cPhysCtrlData::SetSubModId(int submod_id)
{
   m_submodel = submod_id;
}

///////////////////////////////////////


#pragma pack()

#endif /* !__PHCTRL_H */







 
