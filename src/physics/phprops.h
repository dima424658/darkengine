/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phprops.h,v 1.23 2000/01/31 09:52:05 adurant Exp $
//
// Physics property structures
//
#pragma once

#ifndef __PHPROPS_H
#define __PHPROPS_H

#include <matrixs.h>
#include <sdesc.h>
#include <phmod.h>

////////////////////////////////////////

#define MAX_SUBMODELS      (8)  // max submodels any obj can have
#define MAX_PROP_SUBMODELS (2)  // max submodels shown in the property

////////////////////////////////////////

enum eModelType
{
   kOBBProp,
   kSphereProp,
   kSphereHatProp,
   kNoneProp
};

////////////////////////////////////////

struct sPhysTypeProp
{
   eModelType type;
   int        num_submodels;
   BOOL remove_on_sleep;
   BOOL special;
};

struct sPhysAttrProp
{
   mxs_real gravity;
   mxs_real mass;
   mxs_real density;
   mxs_real elasticity;
   mxs_real base_friction;

   mxs_vector cog_offset;

   int  rot_axes;
   int  rest_axes;

   int  climbable_sides;
   BOOL edge_trigger;
   mxs_real pore_size; //apparently BOOL is 4 bytes, so 
                       //this struct is still aligned properly.

};

struct sPhysStateProp
{
   mxs_vector location;
   mxs_vector facing;
   mxs_vector velocity;
   mxs_vector rot_velocity;
};

enum eControlPropTypes
{
   kCPT_AxisVelocity = 0x0001,
   kCPT_Velocity     = 0x0002,
   kCPT_RotVelocity  = 0x0004,
   kCPT_Location     = 0x0008,
   kCPT_Rotation     = 0x0010
};

struct sPhysControlProp
{
   int control_prop_types;

   mxs_vector axis_vel;
   mxs_vector vel;
   mxs_vector rot_vel;
};

struct sPhysDimsProp
{
   mxs_real   radius[MAX_PROP_SUBMODELS];
   mxs_vector offset[MAX_PROP_SUBMODELS];

   mxs_vector size;

   BOOL pt_vs_terrain;
   BOOL pt_vs_not_special;
};

////////////////////////////////////////

EXTERN sStructDesc *GetPhysTypeDesc();
EXTERN sStructDesc *GetPhysAttrDesc(ePhysModelType type = kPMT_Invalid);
EXTERN sStructDesc *GetPhysStateDesc();
EXTERN sStructDesc *GetPhysControlDesc();
EXTERN sStructDesc *GetPhysDimsDesc(ePhysModelType type = kPMT_Invalid);

////////////////////////////////////////

#endif








