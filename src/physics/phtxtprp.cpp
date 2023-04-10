/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/physics/phtxtprp.cpp,v 1.6 2000/02/19 12:32:24 toml Exp $
#include <phtxtprp.h>
#include <propface.h>
#include <propbase.h>
#include <sdesbase.h>
#include <sdestool.h>
#include <appagg.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//
// Friction
//

static IFloatProperty* gpFricProp = NULL;

struct sPropertyDesc fric_desc =
{
   PROP_FRICTION_NAME,
   0, // flags
   NULL,  // constraints
   0, 0, // version
   {
      "Physics: Terrain",
      "Friction"
   }
};

static void setup_friction(void)
{
   gpFricProp = CreateFloatProperty(&fric_desc,kPropertyImplLlist);
}

static void cleanup_friction(void)
{
   SafeRelease(gpFricProp);
}

float GetObjFriction(ObjID obj)
{
   float value = 1.0; // DEFAULT FRICTION
   gpFricProp->Get(obj,&value);
   return value;
}

void SetObjFriction(ObjID obj, float val)
{
   gpFricProp->Set(obj,val);
}


//
// Elasticity
//

static IFloatProperty* gpElasticityProp = NULL;

struct sPropertyDesc elasticity_desc =
{
   PROP_ELASTICITY_NAME,
   0, // flags
   NULL,  // constraints
   0, 0, // version
   {
      "Physics: Terrain",
      "Elasticity"
   }
};

static void setup_elasticity(void)
{
   gpElasticityProp = CreateFloatProperty(&elasticity_desc,kPropertyImplLlist);
}

static void cleanup_elasticity(void)
{
   SafeRelease(gpElasticityProp);
}

float GetObjElasticity(ObjID obj)
{
   float value = 1.0; // DEFAULT ELASTICITY
   gpElasticityProp->Get(obj,&value);
   return value;
}

void SetObjElasticity(ObjID obj, float val)
{
   gpElasticityProp->Set(obj,val);
}

//
// Climbability
//

static IFloatProperty* gpClimbabilityProp = NULL;

struct sPropertyDesc climbability_desc =
{
   PROP_CLIMBABILITY_NAME,
   0, // flags
   NULL,  // constraints
   0, 0, // version
   {
      "Physics: Terrain",
      "Climbability"
   }
};

static void setup_climbability(void)
{
   gpClimbabilityProp = CreateFloatProperty(&climbability_desc,kPropertyImplLlist);
}

static void cleanup_climbability(void)
{
   SafeRelease(gpClimbabilityProp);
}

float GetObjClimbability(ObjID obj)
{
   float value = 0.0; // DEFAULT CLIMBABILITY
   gpClimbabilityProp->Get(obj,&value);
   return value;
}

void SetObjClimbability(ObjID obj, float val)
{
   gpClimbabilityProp->Set(obj,val);
}


//
// CanAttach
//



static sPropertyDesc CanAttach_desc =
{
   PROP_CAN_ATTACH_NAME,
   0, // flags
   NULL,  // constraints
   0, 0, // version
   {
      "Physics: Terrain",
      "Can Attach"
   }
};

static char* CanAttach_flags[] =
{
   "Rope",
   "Vine",
};

#define CA_TYPENAME "tCanAttachType"

static sFieldDesc ca_field[] =
{
   { "Flags", kFieldTypeBits, sizeof(int), 0, FullFieldNames(CanAttach_flags) },
};

static sStructDesc ca_sdesc =
{
   CA_TYPENAME,
   sizeof(int),
   kStructFlagNone,
   sizeof(ca_field)/sizeof(ca_field[0]),
   ca_field,
};

static sPropertyTypeDesc ca_tdesc =
{
   CA_TYPENAME,
   sizeof(int),
};

static IIntProperty* gpCanAttachProp = NULL;


static void setup_CanAttach(void)
{
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&ca_sdesc);
   gpCanAttachProp = CreateIntegralProperty(&CanAttach_desc,&ca_tdesc,kPropertyImplLlist);
}

static void cleanup_CanAttach(void)
{
   SafeRelease(gpCanAttachProp);
}

int GetObjCanAttach(ObjID obj)
{
   int value = 0; // DEFAULT CanAttach
   gpCanAttachProp->Get(obj,&value);
   return value;
}

void SetObjCanAttach(ObjID obj, int val)
{
   gpCanAttachProp->Set(obj,val);
}

////////////////////////////////////////////////////////////

void PhysTexturePropsInit()
{
   setup_friction();
   setup_elasticity();
   setup_climbability();
   setup_CanAttach();
}

void PhysTexturePropsTerm()
{
   cleanup_friction();
   cleanup_elasticity();
   cleanup_climbability();
   cleanup_CanAttach();
}

