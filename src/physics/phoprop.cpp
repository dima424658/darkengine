/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phoprop.cpp,v 1.15 1999/06/18 16:38:59 JON Exp $
//
// Physics other properties/relations
//

#include <matrix.h>
#include <rand.h>

#include <port.h>
#include <wrtype.h>

#include <property.h>
#include <propbase.h>
#include <propface.h>
#include <propfac_.h>
#include <propert_.h>
#include <dataops_.h>
#include <prophash.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <posprop.h>
#include <mnamprop.h>
#include <objpos.h>

#include <iobjsys.h>
#include <osysbase.h>
#include <objnotif.h>

#include <simstate.h>
#include <simflags.h>

#include <relation.h>
#include <linkbase.h>

#include <creatext.h>
#include <cretprop.h>

#include <physapi.h>
#include <phmods.h>
#include <phmod.h>
#include <phcore.h>
#include <phconst.h>
#include <phprop.h>

#include <phoprop.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>


///////////////////////////////////////////////////////////////////////////////
//
// Pressure plates
//

static sPropertyConstraint phys_pplate_constraints[] =
{
   { kPropertyNullConstraint }
};

static sPropertyDesc PhysPPlatePropDesc =
{
   PROP_PHYS_PPLATE,
   kPropertyNoInherit | kPropertyInstantiate,
   phys_pplate_constraints,
   1, 
   0, 
   { "Physics: Misc", "Pressure Plate" },
};

class cPPlateOps : public cClassDataOps<sPhysPPlateProp>
{
public:
   STDMETHOD_(int,Version)() { return 1000; };
};

typedef cHashPropertyStore<cPPlateOps> cPhysPPlateStore;
typedef cSpecificProperty<IPhysPPlateProperty, &IID_IPhysPPlateProperty, sPhysPPlateProp *, cPhysPPlateStore> cPhysPPlatePropertyBase;

class cPhysPPlateProperty : public cPhysPPlatePropertyBase
{
public:
   cPhysPPlateProperty(const sPropertyDesc *desc) : cPhysPPlatePropertyBase(desc) {};

   STANDARD_DESCRIBE_TYPE(sPhysPPlateProp);
};

static sFieldDesc PPlateFields[] =
{
   { "Activation Weight",  kFieldTypeFloat,  FieldLocation(sPhysPPlateProp, activation_weight),  kFieldFlagNone },
   { "Travel",             kFieldTypeFloat,  FieldLocation(sPhysPPlateProp, travel),             kFieldFlagNone },
   { "Speed",              kFieldTypeFloat,  FieldLocation(sPhysPPlateProp, speed),              kFieldFlagNone },
   { "Pause",              kFieldTypeFloat,  FieldLocation(sPhysPPlateProp, pause),              kFieldFlagNone },
   { "Blocks Vision",      kFieldTypeBool,   FieldLocation(sPhysPPlateProp, blocks_vision),      kFieldFlagNone },
};

static sStructDesc PPlateDesc = StructDescBuild(sPhysPPlateProp, kStructFlagNone, PPlateFields);

IPhysPPlateProperty *g_pPhysPPlateProp = NULL;

BOOL PhysPPlateListenerLock = FALSE;

void LGAPI PhysPPlateListener(sPropertyListenMsg * msg, PropListenerData data)
{
   if (PhysPPlateListenerLock)
      return;

   cPhysTypeProp *pTypeProp;

   if (OBJ_IS_CONCRETE(msg->obj) && (msg->type & (kListenPropSet | kListenPropModify)) &&
       !(msg->type & kListenPropLoad))
   {
      // Make sure it has physics
      if (!g_pPhysTypeProp->Get(msg->obj, &pTypeProp))
      {
         g_pPhysTypeProp->Create(msg->obj);
         g_pPhysTypeProp->Get(msg->obj, &pTypeProp);
      }

      if (pTypeProp->type != kOBBProp)
      {
         pTypeProp->type = kOBBProp;
         pTypeProp->num_submodels = 6;
 
         g_pPhysTypeProp->Set(msg->obj, pTypeProp);
      }

      PhysSetGravity(msg->obj, 0.0);
      PhysSetBaseFriction(msg->obj, 1.);

      // Set PPlate flags
      cPhysModel *pModel = g_PhysModels.Get(msg->obj);
      Assert_(pModel);
      if (!pModel->IsPressurePlate())
      {
         pModel->SetPressurePlate(TRUE);
         pModel->SetPPlateState(kPPS_Inactive);
      } 
 
      // Location control it
      PhysControlLocation(msg->obj, &ObjPosGet(msg->obj)->loc.vec);
      PhysControlRotation(msg->obj, &ObjPosGet(msg->obj)->fac);
   }

   if (msg->type & kListenPropUnset)
   {
      cPhysModel *pModel = g_PhysModels.Get(msg->obj);

      if (pModel)
         pModel->SetPressurePlate(FALSE);
   }
}

void PhysPPlatePropInit()
{
   Assert_(g_pPhysPPlateProp == NULL);

   g_pPhysPPlateProp = new cPhysPPlateProperty(&PhysPPlatePropDesc);
   g_pPhysPPlateProp->Listen(kListenPropSet | kListenPropModify | kListenPropUnset | kListenPropLoad, 
                             PhysPPlateListener, NULL);

   StructDescRegister(&PPlateDesc);
}

void PhysPPlatePropTerm()
{
   Assert_(g_pPhysPPlateProp != NULL);

   SafeRelease(g_pPhysPPlateProp);
   g_pPhysPPlateProp = NULL;
}


///////////////////////////////////////////////////////////////////////////////
//
// Rope
//

static sPropertyConstraint phys_rope_constraints[] = 
{
   { kPropertyRequires, PROP_POSITION_NAME },
   { kPropertyRequires, PROP_MODELNAME_NAME },
   { kPropertyNullConstraint }
};

static sPropertyDesc PhysRopePropDesc = 
{
   PROP_PHYS_ROPE,
   kPropertyNoInherit | kPropertyInstantiate,
   phys_rope_constraints,
   1,  // version
   0,  // last ok version (0 means no other ok versions)
   { "Physics: Misc", "Rope"},
}; 

class cRopeOps : public cClassDataOps<sPhysRopeProp>
{
public:
   STDMETHOD_(int,Version)() { return 1001; };
};
 
typedef cHashPropertyStore<cRopeOps> cPhysRopeStore;
typedef cSpecificProperty<IPhysRopeProperty, &IID_IPhysRopeProperty, sPhysRopeProp *, cPhysRopeStore> cPhysRopePropertyBase;

class cPhysRopeProperty : public cPhysRopePropertyBase
{
public:
   cPhysRopeProperty(const sPropertyDesc* desc) : cPhysRopePropertyBase(desc) {}; 

   STANDARD_DESCRIBE_TYPE(sPhysRopeProp);
};

static sFieldDesc RopeFields[] = 
{
   { "Desired Length",  kFieldTypeFloat,  FieldLocation(sPhysRopeProp, desired_length),  kFieldFlagNone },
   { "Length",          kFieldTypeFloat,  FieldLocation(sPhysRopeProp, length),          kFieldFlagNone },
   { "Deployed",        kFieldTypeBool,   FieldLocation(sPhysRopeProp, deployed),        kFieldFlagNone },
};

static sStructDesc RopeDesc = StructDescBuild(sPhysRopeProp, kStructFlagNone, RopeFields);

IPhysRopeProperty *g_pPhysRopeProp = NULL;

BOOL DeployRope(ObjID objID)
{
   sPhysRopeProp *pRopeProp;
   mxs_vector base_loc;
   int i;

   if (!g_pPhysRopeProp->Get(objID, &pRopeProp))
   {
      Warning(("DeployRope: obj %d has no rope property\n", objID));
      return FALSE;
   }

   mx_copy_vec(&base_loc, &ObjPosGet(objID)->loc.vec);
   PhysSetModLocation(objID, &base_loc);

   // Check if we're in the world
   Location start;

   MakeLocationFromVector(&start, &base_loc);
   if (ComputeCellForLocation(&start) == CELL_INVALID)
      return FALSE;

   // Raycast down to floor to check length
   mxs_real true_length;

   Location end, hit;

   MakeLocationFromVector(&end, &base_loc);
   end.vec.z -= pRopeProp->desired_length + 1.0;

   if (!PortalRaycast(&start, &end, &hit, TRUE))
      true_length = (start.vec.z - hit.vec.z) - 1.0;
   else
      true_length = pRopeProp->desired_length;

   pRopeProp->length = true_length;

   PhysRopeListenerLock = TRUE;
   g_pPhysRopeProp->Set(objID, pRopeProp);
   PhysRopeListenerLock = FALSE;

   // Initialize submod positions
   mxs_vector submod_loc;
   mxs_vector submod_dir;

   if (pRopeProp->deployed)
   {
      for (i=0; i<8; i++)
      {
         mx_scale_add_vec(&submod_loc, &base_loc, &kGravityDir, true_length / 8);
         PhysSetSubModLocation(objID, i, &submod_loc);
      }
   }
   else
   {
      // Search for a direction to release from
      BOOL found_dir = FALSE;

      for (i=0; i<8; i++)
      {
         submod_dir.x = (((mxs_real)RandRange(0, 2000)) / 1000) - 1.0;  
         submod_dir.y = (((mxs_real)RandRange(0, 2000)) / 1000) - 1.0;
         submod_dir.z = 0;
        
         mx_normeq_vec(&submod_dir);
         mx_addeq_vec(&submod_dir, &base_loc);
        
         MakeLocationFromVector(&end, &submod_dir);
         if (PortalRaycast(&start, &end, &hit, FALSE))
         {
            found_dir = TRUE;
            break;
         }
      }

      if (!found_dir)
         mx_zero_vec(&submod_dir);
      else
         mx_subeq_vec(&submod_dir, &base_loc);

      for (i=0; i<8; i++)
      {
         mx_scale_vec(&submod_loc, &submod_dir, ((float)i) / 8.0);
         mx_addeq_vec(&submod_loc, &base_loc);

         PhysSetSubModLocation(objID, i, &submod_loc);
      }
   }

   return TRUE;
}

BOOL PhysRopeListenerLock = FALSE;

void LGAPI PhysRopeListener(sPropertyListenMsg * msg, PropListenerData data)
{
   if (PhysRopeListenerLock)
      return;

   sPhysRopeProp *pRopeProp = (sPhysRopeProp *)msg->value.ptrval;

   if (msg->type & (kListenPropSet | kListenPropModify))
   {
      // Init to default name
      char name[16];

      ObjGetModelName(msg->obj, name);
      if (strlen(name) == 0)
         ObjSetModelName(msg->obj, "rope");

      if (OBJ_IS_CONCRETE(msg->obj))
      {
         if (!PhysObjHasPhysics(msg->obj))
         {
            int i;
 
            PhysRegisterSphere(msg->obj, 8, kPMCF_Rope, 0.0);
  
            for (i=1; i<8; i++)
            {
               PhysSetSubModSpringTension(msg->obj, i, DEFAULT_SPRING_TENSION / sqrt((mxs_real)i));
               PhysSetSubModSpringDamping(msg->obj, i, 1.0); 
            }
         }

         if (!CreatureExists(msg->obj))
         {
            CreatureCreate(10, msg->obj);
            ObjSetCreatureType(msg->obj, 10);
         }

         // Minimum mass
         if (PhysGetMass(msg->obj) < 5.0)
            PhysSetMass(msg->obj, 5.0);

#if 0
         DeployRope(msg->obj);
#endif
      }
   }

   // Deletion
   if (msg->type & kListenPropUnset)
   {
      PhysDeregisterModel(msg->obj);
   }
}

void PhysRopePropInit()
{
   Assert_(g_pPhysRopeProp == NULL);

   g_pPhysRopeProp = new cPhysRopeProperty(&PhysRopePropDesc);

   g_pPhysRopeProp->Listen(kListenPropModify | kListenPropSet | kListenPropUnset, PhysRopeListener, NULL);

   StructDescRegister(&RopeDesc);
}

void PhysRopePropTerm()
{
   Assert_(g_pPhysRopeProp != NULL);

   SafeRelease(g_pPhysRopeProp);
   g_pPhysRopeProp = NULL;
}

////////////////////////////////////////////////////////////
//
// Explosion
//

static sPropertyConstraint explode_constraints[] = 
{
   { kPropertyRequires, PROP_POSITION_NAME },
   { kPropertyNullConstraint }
};

static sPropertyDesc PropExplodeDesc =
{
   PROP_PHYS_EXPLODE,
   kPropertyInstantiate,
   explode_constraints,
   0, 0,
   { "Physics: Projectile", "Explode Me"},
};

// data ops
class cExplodeDataOps: public cClassDataOps<sPhysExplodeProp>
{
public:
   STDMETHOD_(int,Version)() { return 1000; };
   STDMETHOD(Read)(sDatum* pdat, IDataOpsFile* file, int version);
}; 

STDMETHODIMP cExplodeDataOps::Read(sDatum* pdat, IDataOpsFile* file, int version)
{
   if (!pdat->value)
      *pdat = New();

   sPhysExplodeProp *pExplodeProp = (sPhysExplodeProp *)pdat->value;

   file->Read(&pExplodeProp->magnitude, sizeof(int));
   if (version<Version() && Version() == 1000)
      pExplodeProp->radius_squared = 10000;
   else 
      file->Read(&pExplodeProp->radius_squared, sizeof(float));
   return S_OK;
}

typedef cHashPropertyStore<cExplodeDataOps> cPhysExplodeStore;
typedef cSpecificProperty<IPhysExplodeProperty, &IID_IPhysExplodeProperty, sPhysExplodeProp *, cPhysExplodeStore> cPhysExplodePropertyBase;

class cPhysExplodeProperty : public cPhysExplodePropertyBase
{
public:
   cPhysExplodeProperty(const sPropertyDesc* desc) : cPhysExplodePropertyBase(desc) {}; 

   STANDARD_DESCRIBE_TYPE(sPhysExplodeProp);
};

static sFieldDesc ExplodeFields[] = 
{
   { "Magnitude",       kFieldTypeInt,  FieldLocation(sPhysExplodeProp, magnitude),          kFieldFlagNone },
   { "Radius (squared)",        kFieldTypeFloat,   FieldLocation(sPhysExplodeProp, radius_squared),        kFieldFlagNone },
};

static sStructDesc ExplodeDesc = StructDescBuild(sPhysExplodeProp, kStructFlagNone, ExplodeFields);

IPhysExplodeProperty *g_pPhysExplodeProp = NULL;

void ObjExplodeListener(ObjID objID, eObjNotifyMsg msg, void *data)
{
   if (!SimStateCheckFlags(kSimPhysics))
      return; 

   if ((msg == kObjNotifyCreate) && (OBJ_IS_CONCRETE(objID)))
   {
      Assert_(g_pPhysExplodeProp);
      sPhysExplodeProp* pExplodeProp;

      if (g_pPhysExplodeProp->Get(objID, &pExplodeProp))
      {
         PhysExplode(&ObjPosGet(objID)->loc.vec, (mxs_real)pExplodeProp->magnitude, pExplodeProp->radius_squared);

         g_pPhysExplodeProp->Delete(objID);
      }
   }
}

void PhysExplodePropInit()
{
   Assert_(g_pPhysExplodeProp == NULL);

   g_pPhysExplodeProp = new cPhysExplodeProperty(&PropExplodeDesc);

   // Listen for EndCreate on objects
   AutoAppIPtr_(ObjectSystem, pObjSys);
   sObjListenerDesc obj_listen_desc = { ObjExplodeListener, NULL };
   pObjSys->Listen(&obj_listen_desc);

   StructDescRegister(&ExplodeDesc);
}

void PhysExplodePropTerm()
{
   Assert_(g_pPhysExplodeProp != NULL);

   SafeRelease(g_pPhysExplodeProp);
}

////////////////////////////////////////////////////////////
//
// Initial Velocity
//

static sPropertyDesc PropInitVelDesc =
{
   PROP_PHYS_INITVEL,
   kPropertyInstantiate,
   NULL,
   0, 0,
   { "Physics: Projectile", "Initial Velocity"},
};

IVectorProperty *g_pPhysInitVelProp = NULL;

void PhysInitVelPropInit()
{
   Assert_(g_pPhysInitVelProp == NULL);

   g_pPhysInitVelProp = CreateVectorProperty(&PropInitVelDesc, kPropertyImplVerySparse);
}

void PhysInitVelPropTerm()
{
   Assert_(g_pPhysInitVelProp != NULL);

   SafeRelease(g_pPhysInitVelProp);
}

////////////////////////////////////////////////////////////
//
// Face Velocity
//

static sPropertyDesc PropFaceVelDesc = 
{
   PROP_PHYS_FACEVEL,
   kPropertyInstantiate | kPropertyNoInherit,
   NULL,
   0, 0,
   { "Physics: Projectile", "Faces Velocity" },
};

IBoolProperty *g_pPhysFaceVelProp = NULL;

void LGAPI PhysFaceVelListener(sPropertyListenMsg * msg, PropListenerData data)
{
   if (msg->type & kListenPropModify)
   {
      if (OBJ_IS_CONCRETE(msg->obj))
      {
         cPhysModel *pModel = g_PhysModels.Get(msg->obj);

         if (pModel)
            pModel->SetFacesVel(msg->value.intval);
      }
   }
}

void PhysFaceVelPropInit()
{
   Assert_(g_pPhysFaceVelProp == NULL);

   g_pPhysFaceVelProp = CreateBoolProperty(&PropFaceVelDesc, kPropertyImplVerySparse);
   g_pPhysFaceVelProp->Listen(kListenPropModify, PhysFaceVelListener, NULL);
}

void PhysFaceVelPropTerm()
{
   Assert_(g_pPhysFaceVelProp != NULL);

   SafeRelease(g_pPhysFaceVelProp);
}

////////////////////////////////////////////////////////////
//
// AI collides against
//

static sPropertyDesc PropAICollideDesc = 
{
   PROP_PHYS_AI_COLL,
   0,
   NULL,
   0, 0,
   { "Physics: Misc", "AI Collides With" },
};

IBoolProperty *g_pPhysAICollideProp = NULL;

class cPhysAICollideProperty : public cGenericBoolProperty
{
public:

   cPhysAICollideProperty(const sPropertyDesc *desc, ePropertyImpl impl)
      : cGenericBoolProperty(desc, impl)
   {
      SetRebuildConcretes(TRUE);
   }

private:

   void RebuildConcrete(ObjID obj, BOOL fIsRelevant, uPropListenerValue val, ObjID donor)
   {
      cPhysModel *pModel = g_PhysModels.Get(obj);

      if (pModel == NULL)
         return;

      pModel->SetAICollides(val.intval);
   }
};

void PhysAICollidePropInit()
{
   Assert_(g_pPhysAICollideProp == NULL);

   g_pPhysAICollideProp = new cPhysAICollideProperty(&PropAICollideDesc, kPropertyImplVerySparse);
}

void PhysAICollidePropTerm()
{
   Assert_(g_pPhysAICollideProp != NULL);

   SafeRelease(g_pPhysAICollideProp);
}

////////////////////////////////////////////////////////////
//
// Phys Attachments
//

IRelation *g_pPhysAttachRelation = NULL;

static sRelationDesc phys_attach_desc = 
{
   PHYS_ATTACH_NAME,
   0,
};

static sRelationDataDesc phys_attach_data_desc = 
   LINK_DATA_DESC_FLAGS(sPhysAttachData, kRelationDataAutoCreate);

static sFieldDesc sPhysAttachFieldDesc[] =
{
   { "Offset",  kFieldTypeVector,  FieldLocation(sPhysAttachData, offset) },
};

static sStructDesc sPhysAttachSDesc = StructDescBuild(sPhysAttachData, kStructFlagNone, sPhysAttachFieldDesc);

////////////////////////////////////////

void LGAPI PhysAttachListener(sRelationListenMsg* msg, RelationListenerData data)
{
   cPhysModel *pModelFrom = g_PhysModels.GetActive(msg->link.source);
   cPhysModel *pModelTo   = g_PhysModels.GetActive(msg->link.dest);

   if ((pModelFrom == NULL) || (pModelTo == NULL))
   {
      Warning(("Adding PhysAttach link between non-physical objects\n"));
      return;
   }

   if (msg->type & kListenLinkBirth)
   {
      pModelTo->AddAttachment();
      if (pModelFrom->IsAttached())
      {
         mprintf("Obj %d is already PhysAttached!\n", msg->link.source);
         g_pPhysAttachRelation->Remove(msg->id);
         return;
      }
      pModelFrom->SetAttached(TRUE);
   }

   if (msg->type & kListenLinkDeath)
   {
      pModelTo->RemoveAttachment();
      pModelFrom->SetAttached(FALSE);
   }
}

////////////////////////////////////////

void PhysAttachRelationInit()
{
   Assert_(g_pPhysAttachRelation == NULL);

   g_pPhysAttachRelation = CreateStandardRelation(&phys_attach_desc, &phys_attach_data_desc, kQCaseSetSourceKnown|kQCaseSetDestKnown);

   g_pPhysAttachRelation->Listen(kListenLinkBirth | kListenLinkDeath, PhysAttachListener, NULL);

   StructDescRegister(&sPhysAttachSDesc);
}

void PhysAttachRelationTerm()
{
   Assert_(g_pPhysAttachRelation != NULL);

   SafeRelease(g_pPhysAttachRelation);
}









