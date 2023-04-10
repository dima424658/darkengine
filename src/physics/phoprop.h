/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phoprop.h,v 1.7 2000/01/31 09:52:02 adurant Exp $
//
// Physics other properties/relations
//
#pragma once

#ifndef __PHOPROP_H
#define __PHOPROP_H

#include <objtype.h>

#include <property.h>
#include <propface.h>
#include <phprops.h>

#include <relation.h>


///////////////////////////////////////////////////////////////////////////////
//
// Pressure plates
//

enum ePPlateState
{
   kPPS_Inactive = 0,
   kPPS_Active,
   kPPS_Deactivating,
   kPPS_Activating,
};

struct sPhysPPlateProp
{
   mxs_real activation_weight;
   mxs_real travel;
   mxs_real speed;
   mxs_real pause;
   BOOL     blocks_vision;

   int      state;
   mxs_real cur_pause;
};

#undef  INTERFACE
#define INTERFACE IPhysPPlateProp
DECLARE_PROPERTY_INTERFACE(IPhysPPlateProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sPhysPPlateProp *); 
}; 

#define PROP_PHYS_PPLATE "PhysPPlate"

EXTERN void PhysPPlatePropInit();
EXTERN void PhysPPlatePropTerm();

EXTERN BOOL PhysPPlateListenerLock;

EXTERN IPhysPPlateProperty *g_pPhysPPlateProp;

///////////////////////////////////////////////////////////////////////////////
//
// Rope
//

struct sPhysRopeProp
{
   mxs_real length;
   BOOL     deployed;

   mxs_real desired_length;
};

#undef  INTERFACE
#define INTERFACE IPhysRopeProperty
DECLARE_PROPERTY_INTERFACE(IPhysRopeProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sPhysRopeProp *); 
}; 

#define PROP_PHYS_ROPE  "PhysRope"

EXTERN void PhysRopePropInit();
EXTERN void PhysRopePropTerm();

EXTERN BOOL DeployRope(ObjID objID); 

EXTERN BOOL PhysRopeListenerLock;

EXTERN IPhysRopeProperty *g_pPhysRopeProp;

///////////////////////////////////////////////////////////////////////////////
//
// Explosion
//

struct sPhysExplodeProp
{  
   int magnitude;
   float radius_squared;
};

#undef  INTERFACE
#define INTERFACE IPhysExplodeProperty
DECLARE_PROPERTY_INTERFACE(IPhysExplodeProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sPhysExplodeProp *); 
}; 

#define PROP_PHYS_EXPLODE  "PhysExplode"
 
EXTERN void PhysExplodePropInit();
EXTERN void PhysExplodePropTerm();

EXTERN IPhysExplodeProperty *g_pPhysExplodeProp;

///////////////////////////////////////////////////////////////////////////////
//
// Initial Velocity
//

#define PROP_PHYS_INITVEL  "PhysInitVel"

EXTERN void PhysInitVelPropInit();
EXTERN void PhysInitVelPropTerm();

EXTERN IVectorProperty *g_pPhysInitVelProp;

///////////////////////////////////////////////////////////////////////////////
//
// Faces Velocity
//

#define PROP_PHYS_FACEVEL  "PhysFaceVel"

EXTERN void PhysFaceVelPropInit();
EXTERN void PhysFaceVelPropTerm();

EXTERN IBoolProperty *g_pPhysFaceVelProp;

///////////////////////////////////////////////////////////////////////////////
//
// AIs collide check
//

#define PROP_PHYS_AI_COLL "PhysAIColl"

EXTERN void PhysAICollidePropInit();
EXTERN void PhysAICollidePropTerm();

EXTERN IBoolProperty *g_pPhysAICollideProp;

///////////////////////////////////////////////////////////////////////////////
//
// Attach Relation
//

#define PHYS_ATTACH_NAME "PhysAttach"

EXTERN void PhysAttachRelationInit();
EXTERN void PhysAttachRelationTerm();

EXTERN IRelation *g_pPhysAttachRelation;

struct sPhysAttachData
{
   mxs_vector offset;
};


#endif // __PHOPROP_H

