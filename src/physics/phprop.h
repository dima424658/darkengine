/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phprop.h,v 1.23 2000/01/31 09:52:04 adurant Exp $
//
// Physics model properties
//
#pragma once

#ifndef __PHPROP_H
#define __PHPROP_H

#include <objtype.h>

#include <property.h>
#include <propface.h>
#include <phprops.h>

#include <relation.h>

#ifndef __cplusplus
#error "Physics property header requires C++"
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Model properties
//

#define ANGVEC_TO_DEGVEC(dv, av) mx_mk_vec((dv), (mxs_real)(av).tx * 180 / MX_ANG_PI, \
                                                 (mxs_real)(av).ty * 180 / MX_ANG_PI, \
                                                 (mxs_real)(av).tz * 180 / MX_ANG_PI);

#define DEGVEC_TO_ANGVEC(av, dv) mx_mk_angvec((av), (short)((dv).x * MX_ANG_PI / 180), \
                                                    (short)((dv).y * MX_ANG_PI / 180), \
                                                    (short)((dv).z * MX_ANG_PI / 180));

#define MXVEC_TO_DEGVEC(dv, mv)  mx_scale_vec((dv), (mv), 180 / MX_REAL_PI);
#define DEGVEC_TO_MXVEC(mv, dv)  mx_scale_vec((mv), (dv), MX_REAL_PI / 180);

// bitfield of auxiliary physics properties
#define PHYS_ATTR    (0x0001)
#define PHYS_STATE   (0x0002)
#define PHYS_CONTROL (0x0004)
#define PHYS_DIMS    (0x0008)
#define PHYS_TYPE    (0x0010)

#define PHYS_ALL  (PHYS_ATTR | PHYS_STATE | PHYS_CONTROL | PHYS_DIMS)

EXTERN void InitPhysicsProperties();
EXTERN void TermPhysicsProperties();

extern void InitPhysProperty(ObjID objID, int type = PHYS_ALL);
extern void UpdatePhysProperty(ObjID objID, int type = PHYS_ALL);
extern void UpdatePhysModel(ObjID objID, int type);

#define PHYS_TYPE_NAME    "PhysType"
#define PHYS_ATTR_NAME    "PhysAttr"
#define PHYS_STATE_NAME   "PhysState"
#define PHYS_CONTROL_NAME "PhysControl"
#define PHYS_DIMS_NAME    "PhysDims"
#define PHYS_CANMANTLE_NAME "PhysCanMant"

class cPhysTypeProp : public sPhysTypeProp
{
public:
   cPhysTypeProp()
   {
      Assert_(sizeof(sPhysTypeProp) == 16);  // reminder to update initializers

      type          = kSphereProp;
      num_submodels = 1;

      remove_on_sleep = FALSE;
      special = FALSE;
   }
   cPhysTypeProp(cPhysTypeProp *prop)
   {
      *this = *prop;
   }
};

#undef  INTERFACE
#define INTERFACE IPhysTypeProperty
DECLARE_PROPERTY_INTERFACE(IPhysTypeProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(cPhysTypeProp *);
};

class cPhysAttrProp : public sPhysAttrProp
{
public:
   cPhysAttrProp()
   {
      Assert_(sizeof(sPhysAttrProp) == 52);  // reminder to update initializers

      gravity = 100.0;
      mass = 30.0;
      density = 1.0;
      elasticity = 1.0;
      base_friction = 0.0;

      mx_zero_vec(&cog_offset);

      rot_axes = XAxis | YAxis | ZAxis;
      rest_axes = XAxis | YAxis | ZAxis | NegXAxis | NegYAxis | NegZAxis;

      climbable_sides = 0;
      edge_trigger = 0;
      pore_size = 0.0;
   }
};

#undef  INTERFACE
#define INTERFACE IPhysAttrProperty
DECLARE_PROPERTY_INTERFACE(IPhysAttrProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(cPhysAttrProp *);
};

class cPhysStateProp : public sPhysStateProp
{
public:
   cPhysStateProp()
   {
      Assert_(sizeof(sPhysStateProp) == 48);  // reminder to update initializers

      mx_zero_vec(&location);
      mx_zero_vec(&facing);
      mx_zero_vec(&velocity);
      mx_zero_vec(&rot_velocity);
   }
};

#undef  INTERFACE
#define INTERFACE IPhysStateProperty
DECLARE_PROPERTY_INTERFACE(IPhysStateProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(cPhysStateProp *);
};

class cPhysControlProp : public sPhysControlProp
{
public:
   cPhysControlProp()
   {
      Assert_(sizeof(sPhysControlProp) == 40);  // reminder to update initializers

      control_prop_types = 0;

      mx_zero_vec(&axis_vel);
      mx_zero_vec(&vel);
      mx_zero_vec(&rot_vel);
   }
};

#undef  INTERFACE
#define INTERFACE IPhysControlProperty
DECLARE_PROPERTY_INTERFACE(IPhysControlProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(cPhysControlProp *);
};

class cPhysDimsProp : public sPhysDimsProp
{
public:
   cPhysDimsProp()
   {
      Assert_(sizeof(sPhysDimsProp) == 52);  // reminder to update initializers

      for (int i=0; i<MAX_PROP_SUBMODELS; i++)
      {
         radius[i] = 0.0;
         mx_zero_vec(&offset[i]);
      }

      mx_zero_vec(&size);
      pt_vs_terrain = FALSE;
      pt_vs_not_special = FALSE;
   }
};

#undef  INTERFACE
#define INTERFACE IPhysDimsProperty
DECLARE_PROPERTY_INTERFACE(IPhysDimsProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(cPhysDimsProp *);
};

EXTERN IPhysTypeProperty    *g_pPhysTypeProp;
EXTERN IPhysAttrProperty    *g_pPhysAttrProp;
EXTERN IPhysStateProperty   *g_pPhysStateProp;
EXTERN IPhysControlProperty *g_pPhysControlProp;
EXTERN IPhysDimsProperty    *g_pPhysDimsProp;
EXTERN IBoolProperty        *g_pPhysCanMantleProp;
EXTERN IVectorProperty      *g_pPhysConveyorVelProp;

#endif /* !__PHPROP_H */













