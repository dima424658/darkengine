/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phprops.cpp,v 1.9 2000/01/09 18:09:24 adurant Exp $
//
// Physics property structures code
//


#include <sdesbase.h>
#include <sdesc.h>

#include <phmod.h>
#include <phprops.h>
#include <phprop.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////

static const char *model_types[] =
{
   "OBB",
   "Sphere",
   "Sphere Hat",
   "None"
};

static const char *rot_bits[] =
{
   "X Axis",
   "Y Axis", 
   "Z Axis",
};

static const char *rest_bits[] =
{
   "+X Axis",
   "+Y Axis", 
   "+Z Axis",
   "-X Axis",
   "-Y Axis", 
   "-Z Axis",
}; 

static const char *control_bits[] =
{
   "Axis Velocity",
   "Velocity",
   "Rot Velocity",
   "Location",
   "Rotation"
};

////////////////////////////////////////

static sFieldDesc phys_type_fields[] =
{
   { "Type",        kFieldTypeEnum, FieldLocation(cPhysTypeProp, type),          kFieldFlagNone, 0, 4, 4, model_types },
   { "# Submodels", kFieldTypeInt,  FieldLocation(cPhysTypeProp, num_submodels), kFieldFlagNone },
   { "Remove on Sleep", kFieldTypeBool, FieldLocation(cPhysTypeProp, remove_on_sleep), kFieldFlagNone },
   { "Special",         kFieldTypeBool, FieldLocation(cPhysTypeProp, special), kFieldFlagNone },
};

static sFieldDesc phys_attr_fields[] =
{
   { "Gravity %",       kFieldTypeFloat,  FieldLocation(cPhysAttrProp, gravity),         kFieldFlagNone },
   { "Mass",            kFieldTypeFloat,  FieldLocation(cPhysAttrProp, mass),            kFieldFlagNone },
   { "Density",         kFieldTypeFloat,  FieldLocation(cPhysAttrProp, density),         kFieldFlagNone },
   { "Elasticity",      kFieldTypeFloat,  FieldLocation(cPhysAttrProp, elasticity),      kFieldFlagNone },
   { "Base Friction",   kFieldTypeFloat,  FieldLocation(cPhysAttrProp, base_friction),   kFieldFlagNone },

   { "COG Offset",      kFieldTypeVector, FieldLocation(cPhysAttrProp, cog_offset),      kFieldFlagNone },

   { "Climbable Sides", kFieldTypeBits,   FieldLocation(cPhysAttrProp, climbable_sides), kFieldFlagNone, 0, 6, 6, rest_bits },
   { "Is Edge Trigger", kFieldTypeBool,   FieldLocation(cPhysAttrProp, edge_trigger), kFieldFlagNone },

   { "Rotation Axes", kFieldTypeBits,   FieldLocation(cPhysAttrProp, rot_axes),      kFieldFlagUnsigned, 0, 3, 3, rot_bits },
   { "Rest Axes",     kFieldTypeBits,   FieldLocation(cPhysAttrProp, rest_axes),     kFieldFlagUnsigned, 0, 6, 6, rest_bits },
};

static sFieldDesc phys_attr_fields_obb[] = 
{
   { "Gravity %",       kFieldTypeFloat,  FieldLocation(cPhysAttrProp, gravity),         kFieldFlagNone },
   { "Mass",            kFieldTypeFloat,  FieldLocation(cPhysAttrProp, mass),            kFieldFlagNone },
   { "Density",         kFieldTypeFloat,  FieldLocation(cPhysAttrProp, density),         kFieldFlagNone },
   { "Elasticity",      kFieldTypeFloat,  FieldLocation(cPhysAttrProp, elasticity),      kFieldFlagNone },
   { "Base Friction",   kFieldTypeFloat,  FieldLocation(cPhysAttrProp, base_friction),   kFieldFlagNone },

   { "COG Offset",      kFieldTypeVector, FieldLocation(cPhysAttrProp, cog_offset),      kFieldFlagNone },

   { "Climbable Sides", kFieldTypeBits,   FieldLocation(cPhysAttrProp, climbable_sides), kFieldFlagNone, 0, 6, 6, rest_bits },
   { "Is Edge Trigger", kFieldTypeBool,   FieldLocation(cPhysAttrProp, edge_trigger), kFieldFlagNone },
   { "Pore Size", kFieldTypeFloat, FieldLocation(cPhysAttrProp, pore_size), kFieldFlagNone},
};

static sFieldDesc phys_attr_fields_sph[] = 
{
   { "Gravity %",     kFieldTypeFloat,  FieldLocation(cPhysAttrProp, gravity),       kFieldFlagNone },
   { "Mass",          kFieldTypeFloat,  FieldLocation(cPhysAttrProp, mass),          kFieldFlagNone },
   { "Density",       kFieldTypeFloat,  FieldLocation(cPhysAttrProp, density),       kFieldFlagNone },
   { "Elasticity",    kFieldTypeFloat,  FieldLocation(cPhysAttrProp, elasticity),    kFieldFlagNone },
   { "Base Friction", kFieldTypeFloat,  FieldLocation(cPhysAttrProp, base_friction), kFieldFlagNone },

   { "COG Offset",    kFieldTypeVector, FieldLocation(cPhysAttrProp, cog_offset),    kFieldFlagNone },

   { "Rotation Axes", kFieldTypeBits,   FieldLocation(cPhysAttrProp, rot_axes),      kFieldFlagUnsigned, 0, 3, 3, rot_bits },
   { "Rest Axes",     kFieldTypeBits,   FieldLocation(cPhysAttrProp, rest_axes),     kFieldFlagUnsigned, 0, 6, 6, rest_bits },
};

static sFieldDesc phys_state_fields[] = 
{
   { "Location",     kFieldTypeVector, FieldLocation(cPhysStateProp, location),     kFieldFlagNone },
   { "Facing",       kFieldTypeVector, FieldLocation(cPhysStateProp, facing),       kFieldFlagNone },
   { "Velocity",     kFieldTypeVector, FieldLocation(cPhysStateProp, velocity),     kFieldFlagNone },
   { "Rot Velocity", kFieldTypeVector, FieldLocation(cPhysStateProp, rot_velocity), kFieldFlagNone },
};

static sFieldDesc phys_control_fields[] = 
{
   { "Controls Active",      kFieldTypeBits,   FieldLocation(cPhysControlProp, control_prop_types), kFieldFlagUnsigned, 0, 5, 5, control_bits },
   { "AxisVelocity",         kFieldTypeVector, FieldLocation(cPhysControlProp, axis_vel),    kFieldFlagNone },
   { "Velocity",             kFieldTypeVector, FieldLocation(cPhysControlProp, vel),         kFieldFlagNone },
   { "RotationalVelocity",   kFieldTypeVector, FieldLocation(cPhysControlProp, rot_vel),     kFieldFlagNone },
};

static sFieldDesc phys_dims_fields[] =
{
   { "Size", kFieldTypeVector, FieldLocation(cPhysDimsProp, size), kFieldFlagNone },

   { "Radius 1", kFieldTypeFloat, FieldLocation(cPhysDimsProp, radius[0]), kFieldFlagNone },
   { "Radius 2", kFieldTypeFloat, FieldLocation(cPhysDimsProp, radius[1]), kFieldFlagNone },

   { "Offset 1", kFieldTypeVector, FieldLocation(cPhysDimsProp, offset[0]), kFieldFlagNone },
   { "Offset 2", kFieldTypeVector, FieldLocation(cPhysDimsProp, offset[1]), kFieldFlagNone },

   { "Point vs Terrain",     kFieldTypeBool, FieldLocation(cPhysDimsProp, pt_vs_terrain),     kFieldFlagNone },
   { "Point vs Not Special", kFieldTypeBool, FieldLocation(cPhysDimsProp, pt_vs_not_special), kFieldFlagNone },
};

static sFieldDesc phys_dims_fields_obb[] =
{
   { "Offset", kFieldTypeVector, FieldLocation(cPhysDimsProp, offset[0]), kFieldFlagNone },
   { "Size",   kFieldTypeVector, FieldLocation(cPhysDimsProp, size),      kFieldFlagNone },
};

static sFieldDesc phys_dims_fields_sph[] =
{
   { "Radius 1", kFieldTypeFloat, FieldLocation(cPhysDimsProp, radius[0]), kFieldFlagNone },
   { "Radius 2", kFieldTypeFloat, FieldLocation(cPhysDimsProp, radius[1]), kFieldFlagNone },

   { "Offset 1", kFieldTypeVector, FieldLocation(cPhysDimsProp, offset[0]), kFieldFlagNone },
   { "Offset 2", kFieldTypeVector, FieldLocation(cPhysDimsProp, offset[1]), kFieldFlagNone },

   { "Point vs Terrain",     kFieldTypeBool, FieldLocation(cPhysDimsProp, pt_vs_terrain),     kFieldFlagNone },
   { "Point vs Not Special", kFieldTypeBool, FieldLocation(cPhysDimsProp, pt_vs_not_special), kFieldFlagNone },
};

static sFieldDesc phys_dims_fields_sphhat[] = 
{
   { "Radius", kFieldTypeVector, FieldLocation(cPhysDimsProp, radius[0]), kFieldFlagNone },
   { "Offset", kFieldTypeVector, FieldLocation(cPhysDimsProp, offset[0]), kFieldFlagNone },

   { "Point vs Terrain",     kFieldTypeBool, FieldLocation(cPhysDimsProp, pt_vs_terrain),     kFieldFlagNone },
   { "Point vs Not Special", kFieldTypeBool, FieldLocation(cPhysDimsProp, pt_vs_not_special), kFieldFlagNone },
};

////////////////////////////////////////

static sStructDesc PhysTypeDesc = StructDescBuild(cPhysTypeProp, kStructFlagNone, phys_type_fields);

static sStructDesc PhysAttrDesc = StructDescBuild(cPhysAttrProp, kStructFlagNone, phys_attr_fields);
static sStructDesc PhysAttrDescOBB = StructDescBuild(cPhysAttrProp, kStructFlagNone, phys_attr_fields_obb);
static sStructDesc PhysAttrDescSph = StructDescBuild(cPhysAttrProp, kStructFlagNone, phys_attr_fields_sph);

static sStructDesc PhysStateDesc = StructDescBuild(cPhysStateProp, kStructFlagNone, phys_state_fields);

static sStructDesc PhysControlDesc = StructDescBuild(cPhysControlProp, kStructFlagNone, phys_control_fields);

static sStructDesc PhysDimsDesc = StructDescBuild(cPhysDimsProp, kStructFlagNone, phys_dims_fields);
static sStructDesc PhysDimsDescOBB = StructDescBuild(cPhysDimsProp, kStructFlagNone, phys_dims_fields_obb);
static sStructDesc PhysDimsDescSph = StructDescBuild(cPhysDimsProp, kStructFlagNone, phys_dims_fields_sph);
static sStructDesc PhysDimsDescSphHat = StructDescBuild(cPhysDimsProp, kStructFlagNone, phys_dims_fields_sphhat);

////////////////////////////////////////

sStructDesc *GetPhysTypeDesc()
{
   return &PhysTypeDesc;
}

sStructDesc *GetPhysAttrDesc(ePhysModelType type)
{
   switch (type)
   {
      case kPMT_Sphere:
      case kPMT_SphereHat:
      case kPMT_Point:
         return &PhysAttrDescSph;

      case kPMT_OBB:
         return &PhysAttrDescOBB;

      case kPMT_Invalid:
         return &PhysAttrDesc;

      default:
         Warning(("GetPhysAttrDesc: Unknown type: %d\n", (int)type));
         return NULL;
   }
}

sStructDesc *GetPhysStateDesc()
{
   return &PhysStateDesc;
}

sStructDesc *GetPhysControlDesc()
{
   return &PhysControlDesc;
}

sStructDesc *GetPhysDimsDesc(ePhysModelType type)
{
   switch (type)
   {
      case kPMT_Sphere:
      case kPMT_Point:
         return &PhysDimsDescSph;

      case kPMT_SphereHat:
         return &PhysDimsDescSphHat;

      case kPMT_OBB:
         return &PhysDimsDescOBB;

      case kPMT_Invalid:
         return &PhysDimsDesc;
     
      default:
         Warning(("GetPhysDimsDesc: Unknown type: %d\n", (int)type));
         return NULL;
   }
}








