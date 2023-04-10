/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phprop.cpp,v 1.72 2000/03/07 19:56:19 toml Exp $
//
// Physics model properties
//

#include <property.h>
#include <propbase.h>
#include <propface.h>
#include <propert_.h>
#include <dataops_.h>
#include <prophash.h>
#include <sdesbase.h>
#include <sdesc.h>
#include <simpwrap.h>

#include <posprop.h>
#include <objpos.h>
#include <objshape.h>
#include <objscale.h>
#include <mnumprop.h>

#include <relation.h>
#include <linkbase.h>

#include <physapi.h>
#include <phmods.h>
#include <phmod.h>
#include <phdyn.h>
#include <phcore.h>
#include <phmodobb.h>
#include <phmodsph.h>

#include <vernum.h>
#include <tagfile.h>
#include <objnotif.h>

#include <phprop.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>

BOOL PhysicsListenerLock;

////////////////////////////////////////////////////////////////////////////////

void LGAPI PhysTypeListener(sPropertyListenMsg *msg, PropListenerData data);
void LGAPI PhysAttrListener(sPropertyListenMsg *msg, PropListenerData data);
void LGAPI PhysStateListener(sPropertyListenMsg *msg, PropListenerData data);
void LGAPI PhysControlListener(sPropertyListenMsg *msg, PropListenerData data);
void LGAPI PhysDimsListener(sPropertyListenMsg *msg, PropListenerData data);

void PhysAuxListener(sPropertyListenMsg *msg, PropListenerData data, int type);

#ifdef EDITOR
extern void CreateAuxPhysPropEditor(IProperty *prop, int type);
#else
#define CreateAuxPhysPropEditor(x, y)
#endif

IPhysTypeProperty    *g_pPhysTypeProp = NULL;
IPhysAttrProperty    *g_pPhysAttrProp = NULL;
IPhysStateProperty   *g_pPhysStateProp = NULL;
IPhysControlProperty *g_pPhysControlProp = NULL;
IPhysDimsProperty    *g_pPhysDimsProp = NULL;

IBoolProperty        *g_pPhysCanMantleProp = NULL;


//////////////////////////////////////////////////
// physics system tag file structures

TagFileTag PhysicsSystemTag = { "PHYS_SYSTEM" };
TagVersion PhysicsSystemVersion = { 0, 1 };

static ITagFile* tagfile = NULL;
static void movefunc(void *buf, size_t elsize, size_t nelem)
{
   ITagFile_Move(tagfile,(char*)buf,elsize*nelem);
}

static BOOL setup_tagfile(ITagFile* file, TagFileTag *tag,
                          TagVersion *version)
{
   HRESULT result;
   TagVersion found_version = *version;
   tagfile = file;

   result = ITagFile_OpenBlock(file, tag, &found_version);
   return (result == S_OK
        && found_version.major == version->major
        && found_version.minor == version->minor);
}


static void cleanup_tagfile(ITagFile* file)
{
   ITagFile_CloseBlock(file);
}


////////////////////////////////////////

static sPropertyConstraint phys_type_constraints[] =
{
   { kPropertyRequires, PROP_POSITION_NAME },
   { kPropertyNullConstraint }
};

static sPropertyDesc PhysTypePropDesc =
{
   PHYS_TYPE_NAME,
   kPropertyNoInherit | kPropertyInstantiate,
   phys_type_constraints,
   1,  // version
   0,  // last ok version (0 means no other ok versions)
   { "Physics: Model", "Type"},
   kPropertyChangeLocally,  // net_flags
};

class cPhysTypeOps : public cClassDataOps<cPhysTypeProp>
{
public:
   cPhysTypeOps() : cClassDataOps<cPhysTypeProp>(kNoFlags) {};

   STDMETHOD_(int, Version)() { return 1002; };
   STDMETHOD(Read)(sDatum* pdat, IDataOpsFile* file, int version);
};

STDMETHODIMP cPhysTypeOps::Read(sDatum* pdat, IDataOpsFile* file, int version)
{
   if (!pdat->value)
      *pdat = New();

   sPhysTypeProp *pTypeProp = (sPhysTypeProp *)pdat->value;

   file->Read(&pTypeProp->type, sizeof(eModelType));
   file->Read(&pTypeProp->num_submodels, sizeof(int));

   if (version < 1001)
   {
      BOOL dummy;
      file->Read(&dummy, sizeof(BOOL));
   }

   if (version >= 1000)
      file->Read(&pTypeProp->remove_on_sleep, sizeof(BOOL));

   // fixup num submodels
   if (version < 1000)
   {
      switch (pTypeProp->type)
      {
         case kOBBProp:       pTypeProp->num_submodels = 6; break;
         case kSphereHatProp: pTypeProp->num_submodels = 2; break;
      }
   }

   pTypeProp->remove_on_sleep = !!(pTypeProp->remove_on_sleep);

   if (version < 1002)
      pTypeProp->special = FALSE;
   else
      file->Read(&pTypeProp->special, sizeof(BOOL));

   return S_OK;
}

typedef cHashPropertyStore<cPhysTypeOps> cPhysTypeStore;
typedef cSpecificProperty<IPhysTypeProperty, &IID_IPhysTypeProperty, cPhysTypeProp *, cPhysTypeStore> cBasePhysTypeProperty;

class cPhysTypeProperty : public cBasePhysTypeProperty
{
public:
   cPhysTypeProperty(const sPropertyDesc* desc) : cBasePhysTypeProperty(desc) {};

   STDMETHOD_(void,Notify)(ePropertyNotifyMsg msg, PropNotifyData data)
   {
      uObjNotifyData objdata;
      objdata.raw = data;

      switch (NOTIFY_MSG(msg))
      {
         case kObjNotifyLoad:
            if (msg & kObjPartConcrete)
            {
               if (setup_tagfile(objdata.db.load, &PhysicsSystemTag,
                                 &PhysicsSystemVersion))
               {
                  PhysRead(movefunc,DB_OBJ_PARTITION(msg));
                  cleanup_tagfile(objdata.db.load);
               }
            }
            break;

         case kObjNotifySave:
            if (msg & kObjPartConcrete)
               if (setup_tagfile(objdata.db.save, &PhysicsSystemTag,
                                 &PhysicsSystemVersion))
               {
                  PhysWrite(movefunc, DB_OBJ_PARTITION(msg));
                  cleanup_tagfile(objdata.db.save);
               }
            break;
      }
      cBasePhysTypeProperty::Notify(msg,data);
   }

   STANDARD_DESCRIBE_TYPE(cPhysTypeProp);
};


////////////////////////////////////////

static sPropertyConstraint phys_attr_constraints[] =
{
   { kPropertyRequires, PHYS_TYPE_NAME },
   { kPropertyNullConstraint }
};

static sPropertyDesc PhysAttrPropDesc =
{
   PHYS_ATTR_NAME,
   kPropertyNoInherit | kPropertyInstantiate,
   phys_attr_constraints,
   2,  // version
   1,  // last ok version (0 means no other ok versions)
   { "Physics: Model", "Attributes"},
   kPropertyChangeLocally,  // net_flags
};

class cPhysAttrOps : public cClassDataOps<cPhysAttrProp>
{
public:
   cPhysAttrOps() : cClassDataOps<cPhysAttrProp>(kNoFlags) {};

   STDMETHOD_(int,Version)() { return 1001; };
   STDMETHOD(Read)(sDatum* pdat, IDataOpsFile* file, int version);
};

STDMETHODIMP cPhysAttrOps::Read(sDatum* pdat, IDataOpsFile* file, int version)
{
   if (!pdat->value)
      *pdat = New();

   sPhysAttrProp *pAttrProp = (sPhysAttrProp *)pdat->value;

   file->Read(&pAttrProp->gravity, sizeof(mxs_real));
   file->Read(&pAttrProp->mass, sizeof(mxs_real));
   file->Read(&pAttrProp->density, sizeof(mxs_real));
   file->Read(&pAttrProp->elasticity, sizeof(mxs_real));
   file->Read(&pAttrProp->base_friction, sizeof(mxs_real));
   file->Read(&pAttrProp->cog_offset, sizeof(mxs_vector));
   file->Read(&pAttrProp->rot_axes, sizeof(int));
   file->Read(&pAttrProp->rest_axes, sizeof(int));
   file->Read(&pAttrProp->climbable_sides, sizeof(int));
   file->Read(&pAttrProp->edge_trigger, sizeof(BOOL));
   if (version >= 1001)
     file->Read(&pAttrProp->pore_size, sizeof(mxs_real));
   else
     pAttrProp->pore_size = 0.0;

   pAttrProp->edge_trigger = !!(pAttrProp->edge_trigger);

   return S_OK;
}

typedef cHashPropertyStore<cPhysAttrOps> cPhysAttrStore;
typedef cSpecificProperty<IPhysAttrProperty, &IID_IPhysAttrProperty, cPhysAttrProp *, cPhysAttrStore> cBasePhysAttrProperty;

class cPhysAttrProperty : public cBasePhysAttrProperty
{
public:
   cPhysAttrProperty(const sPropertyDesc* desc) : cBasePhysAttrProperty(desc) {};

   STANDARD_DESCRIBE_TYPE(cPhysAttrProp);

protected:
   virtual void CreateEditor()
   {
       CreateAuxPhysPropEditor(this, PHYS_ATTR);
   }
};

////////////////////////////////////////

static sPropertyConstraint phys_state_constraints[] =
{
   { kPropertyRequires, PHYS_TYPE_NAME },
   { kPropertyNullConstraint }
};

static sPropertyDesc PhysStatePropDesc =
{
   PHYS_STATE_NAME,
   kPropertyNoInherit | kPropertyInstantiate,
   phys_state_constraints,
   1,  // version
   0,  // last ok version (0 means no other ok versions)
   { "Physics: Model", "State"},
   kPropertyChangeLocally,  // net_flags
};

class cPhysStateOps : public cClassDataOps<cPhysStateProp>
{
public:
   cPhysStateOps() : cClassDataOps<cPhysStateProp>(kNoFlags) {};
};

typedef cHashPropertyStore<cPhysStateOps> cPhysStateStore;
typedef cSpecificProperty<IPhysStateProperty, &IID_IPhysStateProperty, cPhysStateProp *, cPhysStateStore> cBasePhysStateProperty;

class cPhysStateProperty : public cBasePhysStateProperty
{
public:
   cPhysStateProperty(const sPropertyDesc* desc) : cBasePhysStateProperty(desc) {};

   STANDARD_DESCRIBE_TYPE(cPhysStateProp);

protected:
   virtual void CreateEditor()
   {
       CreateAuxPhysPropEditor(this, PHYS_STATE);
   }
};

////////////////////////////////////////

static sPropertyConstraint phys_control_constraints[] =
{
   { kPropertyRequires, PHYS_TYPE_NAME },
   { kPropertyNullConstraint }
};

static sPropertyDesc PhysControlPropDesc =
{
   PHYS_CONTROL_NAME,
   kPropertyNoInherit | kPropertyInstantiate,
   phys_control_constraints,
   1,  // version
   0,  // last ok version (0 means no other ok versions)
   { "Physics: Model", "Controls"},
   kPropertyChangeLocally,  // net_flags
};

class cPhysControlOps : public cClassDataOps<cPhysControlProp>
{
public:
   cPhysControlOps() : cClassDataOps<cPhysControlProp>(kNoFlags) {};

   STDMETHOD_(int,Version)() { return 1000; };
   STDMETHOD(Read)(sDatum* pdat, IDataOpsFile* file, int version);
};

STDMETHODIMP cPhysControlOps::Read(sDatum* pdat, IDataOpsFile* file, int version)
{
   if (!pdat->value)
      *pdat = New();

   sPhysControlProp *pControlProp = (sPhysControlProp *)pdat->value;

   if (version < 1000)
   {
      BOOL state;

      pControlProp->control_prop_types = 0;

      file->Read(&state, sizeof(BOOL));
      if (state) pControlProp->control_prop_types |= kCPT_AxisVelocity;
      file->Read(&pControlProp->axis_vel, sizeof(mxs_vector));

      file->Read(&state, sizeof(BOOL));
      if (state) pControlProp->control_prop_types |= kCPT_Velocity;
      file->Read(&pControlProp->vel, sizeof(mxs_vector));

      file->Read(&state, sizeof(BOOL));
      if (state) pControlProp->control_prop_types |= kCPT_RotVelocity;
      file->Read(&pControlProp->rot_vel, sizeof(mxs_vector));

      file->Read(&state, sizeof(BOOL));
      if (state) pControlProp->control_prop_types |= kCPT_Location;

      file->Read(&state, sizeof(BOOL));
      if (state) pControlProp->control_prop_types |= kCPT_Rotation;
   }
   else
   {
      file->Read(&pControlProp->control_prop_types, sizeof(int));
      file->Read(&pControlProp->axis_vel, sizeof(mxs_vector));
      file->Read(&pControlProp->vel, sizeof(mxs_vector));
      file->Read(&pControlProp->rot_vel, sizeof(mxs_vector));
   }

   return S_OK;
}

typedef cHashPropertyStore<cPhysControlOps> cPhysControlStore;
typedef cSpecificProperty<IPhysControlProperty, &IID_IPhysControlProperty, cPhysControlProp *, cPhysControlStore> cBasePhysControlProperty;

class cPhysControlProperty : public cBasePhysControlProperty
{
public:
   cPhysControlProperty(const sPropertyDesc* desc) : cBasePhysControlProperty(desc) {};

   STANDARD_DESCRIBE_TYPE(cPhysControlProp);

protected:
   virtual void CreateEditor()
   {
       CreateAuxPhysPropEditor(this, PHYS_CONTROL);
   }
};

////////////////////////////////////////

static sPropertyConstraint phys_dims_constraints[] =
{
   { kPropertyRequires, PHYS_TYPE_NAME },
   { kPropertyNullConstraint }
};

static sPropertyDesc PhysDimsPropDesc =
{
   PHYS_DIMS_NAME,
   kPropertyNoInherit | kPropertyInstantiate,
   phys_dims_constraints,
   1,  // version
   0,  // last ok version (0 means no other ok versions)
   { "Physics: Model", "Dimensions"},
   kPropertyChangeLocally,  // net_flags
};

class cPhysDimsOps : public cClassDataOps<cPhysDimsProp>
{
public:
   cPhysDimsOps() : cClassDataOps<cPhysDimsProp>(kNoFlags) {};

   STDMETHOD_(int,Version)() { return 1001; };
   STDMETHOD(Read)(sDatum* pdat, IDataOpsFile* file, int version);
};

STDMETHODIMP cPhysDimsOps::Read(sDatum* pdat, IDataOpsFile* file, int version)
{
   if (!pdat->value)
      *pdat = New();

   sPhysDimsProp *pDimsProp = (sPhysDimsProp *)pdat->value;

   file->Read(&pDimsProp->radius, sizeof(mxs_real) * MAX_PROP_SUBMODELS);
   file->Read(&pDimsProp->offset, sizeof(mxs_vector) * MAX_PROP_SUBMODELS);
   file->Read(&pDimsProp->size, sizeof(mxs_vector));
   file->Read(&pDimsProp->pt_vs_terrain, sizeof(BOOL));
   if (version < 1001)
      pDimsProp->pt_vs_not_special = FALSE;
   else
      file->Read(&pDimsProp->pt_vs_not_special, sizeof(BOOL));

   return S_OK;
}

typedef cHashPropertyStore<cPhysDimsOps> cPhysDimsStore;
typedef cSpecificProperty<IPhysDimsProperty, &IID_IPhysDimsProperty, cPhysDimsProp *, cPhysDimsStore> cBasePhysDimsProperty;

class cPhysDimsProperty : public cBasePhysDimsProperty
{
public:
   cPhysDimsProperty(const sPropertyDesc* desc) : cBasePhysDimsProperty(desc) {};

   STANDARD_DESCRIBE_TYPE(cPhysDimsProp);

protected:
   virtual void CreateEditor()
   {
       CreateAuxPhysPropEditor(this, PHYS_DIMS);
   }
};

////////////////////////////////////////

static sPropertyDesc _g_PhysCanMantleProp =
{
  PHYS_CANMANTLE_NAME,
  0,
  NULL, 0, 0,
  { "Physics: Model", "Mantleable" },
  kPropertyChangeLocally,
};

////////////////////////////////////////
// conveyor belt velocity (offset to control velocity for AIs and player)

IVectorProperty *g_pPhysConveyorVelProp = NULL;
static sPropertyDesc ConveyorVelDesc =
{ "ConveyorVel",
  0,
  NULL, 0, 0,
  { "Physics: Model", "ConveyorVelocity" },
  kPropertyChangeLocally
};


////////////////////////////////////////


void InitPhysicsProperties()
{
   g_pPhysTypeProp = new cPhysTypeProperty(&PhysTypePropDesc);
   g_pPhysAttrProp = new cPhysAttrProperty(&PhysAttrPropDesc);
   g_pPhysStateProp = new cPhysStateProperty(&PhysStatePropDesc);
   g_pPhysControlProp = new cPhysControlProperty(&PhysControlPropDesc);
   g_pPhysDimsProp = new cPhysDimsProperty(&PhysDimsPropDesc);

   g_pPhysCanMantleProp = CreateBoolProperty(&_g_PhysCanMantleProp, kPropertyImplHash);

   g_pPhysTypeProp->Listen(kListenPropModify | kListenPropSet | kListenPropUnset, PhysTypeListener, NULL);
   g_pPhysAttrProp->Listen(kListenPropModify | kListenPropSet | kListenPropUnset, PhysAttrListener, NULL);
   g_pPhysStateProp->Listen(kListenPropModify | kListenPropSet | kListenPropUnset, PhysStateListener, NULL);
   g_pPhysControlProp->Listen(kListenPropModify | kListenPropSet | kListenPropUnset, PhysControlListener, NULL);
   g_pPhysDimsProp->Listen(kListenPropModify | kListenPropSet | kListenPropUnset, PhysDimsListener, NULL);

   // Register type sdesc
   AutoAppIPtr_(StructDescTools, pSDT);
   pSDT->Register(GetPhysTypeDesc());

   // Register default other sdescs (for property getting/setting froms scripts)
   pSDT->Register(GetPhysAttrDesc());
   pSDT->Register(GetPhysStateDesc());
   pSDT->Register(GetPhysControlDesc());
   pSDT->Register(GetPhysDimsDesc());

   // create and register conveyor velocity property
   g_pPhysConveyorVelProp = CreateVectorProperty( &ConveyorVelDesc, kPropertyImplSparse );
}

void TermPhysicsProperties()
{
   SafeRelease(g_pPhysTypeProp);
   SafeRelease(g_pPhysAttrProp);
   SafeRelease(g_pPhysStateProp);
   SafeRelease(g_pPhysControlProp);
   SafeRelease(g_pPhysDimsProp);
   SafeRelease(g_pPhysCanMantleProp);
   SafeRelease(g_pPhysConveyorVelProp);
}

////////////////////////////////////////////////////////////////////////////////

BOOL PhysListenerLock = FALSE;

void LGAPI PhysTypeListener(sPropertyListenMsg * msg, PropListenerData data)
{
   if (PhysListenerLock)
      return;

   // Delete
   if (msg->type & kListenPropUnset)
   {
      g_pPhysAttrProp->Delete(msg->obj);
      g_pPhysStateProp->Delete(msg->obj);
      g_pPhysControlProp->Delete(msg->obj);
      g_pPhysDimsProp->Delete(msg->obj);

      PhysDeregisterModel(msg->obj);
      return;
   }

   // Force a model load
   if (OBJ_IS_CONCRETE(msg->obj))
      ObjLoadModel(msg->obj);

   cPhysModel *pModel;
   ePhysModelType new_type;
   BOOL rebuild = FALSE;

   cPhysTypeProp  typeProp((cPhysTypeProp *)msg->value.ptrval);
   cPhysDimsProp *pDimsProp;

   if ((pModel = g_PhysModels.Get(msg->obj)) == NULL)
      rebuild = TRUE;

   if (typeProp.type == kNoneProp)
   {
      if (OBJ_IS_CONCRETE(msg->obj))
      {
         g_pPhysTypeProp->Delete(msg->obj);

         g_pPhysAttrProp->Delete(msg->obj);
         g_pPhysStateProp->Delete(msg->obj);
         g_pPhysControlProp->Delete(msg->obj);
         g_pPhysDimsProp->Delete(msg->obj);
      }

      return;
   }

   // Convert sdesc enum into model type
   switch (typeProp.type)
   {
      case kOBBProp:
      {
         new_type = kPMT_OBB;
         if (pModel && (pModel->GetType(0) != kPMT_OBB))
            rebuild = TRUE;
         break;
      }

      case kSphereProp:
      {
         new_type = kPMT_Sphere;
         if (pModel && ((pModel->GetType(0) != kPMT_Sphere) && (pModel->GetType(0) != kPMT_Point)))
            rebuild = TRUE;
         break;
      }

      case kSphereHatProp:
      {
         new_type = kPMT_SphereHat;
         if (pModel && (pModel->GetType(0) != kPMT_SphereHat))
            rebuild = TRUE;
         break;
      }

      default:
         Warning(("PhysTypeListener: Unknown type: %d\n", (int)typeProp.type));
         return;
   }

   // Are we changing the number of submodels?
   if (pModel && (pModel->GetType(0) != kPMT_OBB) && (pModel->NumSubModels() != typeProp.num_submodels))
      rebuild = TRUE;

   if (rebuild && OBJ_IS_CONCRETE(msg->obj))
   {
      // @NOTE: We want to make sure that the Dims property is getting
      // instantiated before the type property, so that if there isn't
      // a dims property at this point, then we really don't have one
      // on any ancestor.  Curently, we're not sure whether it's going
      // to be instantiated or not.

      PhysListenerLock = TRUE;

      // Remove old physics
      PhysDeregisterModel(msg->obj);

      // Initialize the type property
      switch (typeProp.type)
      {
         case kOBBProp:        typeProp.num_submodels = 6; break;
         case kSphereHatProp:  typeProp.num_submodels = 2; break;
      }

      g_pPhysTypeProp->Set(msg->obj, &typeProp);

      g_pPhysAttrProp->Create(msg->obj);
      g_pPhysStateProp->Create(msg->obj);
      g_pPhysControlProp->Create(msg->obj);

      g_pPhysDimsProp->Create(msg->obj);
      g_pPhysDimsProp->Get(msg->obj, &pDimsProp);

      // Add new physics
      switch (typeProp.type)
      {
         case kOBBProp: PhysRegisterOBB(msg->obj, kPMCF_Default);
                        break;

         case kSphereProp: PhysRegisterSphere(msg->obj, typeProp.num_submodels, kPMCF_Default, pDimsProp->radius[0]);
                           break;

         case kSphereHatProp: PhysRegisterSphereHat(msg->obj, kPMCF_Default, pDimsProp->radius[0]);
                              break;
      }

      PhysListenerLock = FALSE;

      // Refresh physics with the property
      UpdatePhysModel(msg->obj, PHYS_ATTR | PHYS_STATE | PHYS_CONTROL | PHYS_DIMS);
   }

   if (OBJ_IS_CONCRETE(msg->obj))
   {
      pModel = g_PhysModels.Get(msg->obj);
      pModel->SetRemovesOnSleep(typeProp.remove_on_sleep);
      pModel->SetSpecial(typeProp.special);

      if (!g_pPhysAttrProp->IsRelevant(msg->obj))
         g_pPhysAttrProp->Create(msg->obj);
      if (!g_pPhysStateProp->IsRelevant(msg->obj))
         g_pPhysStateProp->Create(msg->obj);
      if (!g_pPhysControlProp->IsRelevant(msg->obj))
         g_pPhysControlProp->Create(msg->obj);
      if (!g_pPhysDimsProp->IsRelevant(msg->obj))
         g_pPhysDimsProp->Create(msg->obj);
   }
}

static BOOL PhysAuxListenerLock = FALSE;

void LGAPI PhysAttrListener(sPropertyListenMsg *msg, PropListenerData data)
{
   if (!g_pPhysTypeProp->IsRelevant(msg->obj) && !(msg->type & kListenPropUnset))
   {
      PhysAuxListenerLock = TRUE;
      g_pPhysAttrProp->Delete(msg->obj);
      PhysAuxListenerLock = FALSE;
      return;
   }

   PhysAuxListener(msg, data, PHYS_ATTR);
}

void LGAPI PhysStateListener(sPropertyListenMsg *msg, PropListenerData data)
{
   if (!g_pPhysTypeProp->IsRelevant(msg->obj) && !(msg->type & kListenPropUnset))
   {
      PhysAuxListenerLock = TRUE;
      g_pPhysStateProp->Delete(msg->obj);
      PhysAuxListenerLock = FALSE;
      return;
   }

   PhysAuxListener(msg, data, PHYS_STATE);
}

void LGAPI PhysControlListener(sPropertyListenMsg *msg, PropListenerData data)
{
   if (!g_pPhysTypeProp->IsRelevant(msg->obj) && !(msg->type & kListenPropUnset))
   {
      PhysAuxListenerLock = TRUE;
      g_pPhysControlProp->Delete(msg->obj);
      PhysAuxListenerLock = FALSE;
      return;
   }

   PhysAuxListener(msg, data, PHYS_CONTROL);
}

void LGAPI PhysDimsListener(sPropertyListenMsg *msg, PropListenerData data)
{
   if (!g_pPhysTypeProp->IsRelevant(msg->obj) && !(msg->type & kListenPropUnset))
   {
      PhysAuxListenerLock = TRUE;
      g_pPhysDimsProp->Delete(msg->obj);
      PhysAuxListenerLock = FALSE;
      return;
   }

   PhysAuxListener(msg, data, PHYS_DIMS);
}

void PhysAuxListener(sPropertyListenMsg *msg, PropListenerData data, int type)
{
   if (PhysAuxListenerLock)
      return;

   BOOL has_physics = PhysObjHasPhysics(msg->obj);

   // Created w/o archetype
   if ((msg->type & kListenPropSet) && !(msg->type & kListenPropModify))
   {
      if (has_physics)
         UpdatePhysProperty(msg->obj, type);
      else
         InitPhysProperty(msg->obj, type);
   }

   if ((msg->type & kListenPropModify) && has_physics)
   {
      UpdatePhysModel(msg->obj, type);
   }
}

////////////////////////////////////////////////////////////////////////////////

//
// Initialize instance-specific portions of the physics property that are
// outside of physics's domain.  (location, facing, size, etc)
//
void InitPhysProperty(ObjID objID, int type)
{
   cPhysTypeProp *pTypeProp;

   if (!g_pPhysTypeProp->Get(objID, &pTypeProp))
      return;

   PhysAuxListenerLock = TRUE;

   if (type & PHYS_ATTR)
   {


   }

   if (type & PHYS_STATE)
   {
      cPhysStateProp *pStateProp;

      if (!g_pPhysStateProp->Get(objID, &pStateProp))
      {
         g_pPhysStateProp->Create(objID);
         g_pPhysStateProp->Get(objID, &pStateProp);
      }

      pStateProp->location = ObjPosGet(objID)->loc.vec;
      ANGVEC_TO_DEGVEC(&pStateProp->facing, ObjPosGet(objID)->fac);

      g_pPhysStateProp->Set(objID, pStateProp);
   }

   if (type & PHYS_CONTROL)
   {


   }

   if (type & PHYS_DIMS)
   {
      cPhysDimsProp *pDimsProp;

      if (!g_pPhysDimsProp->Get(objID, &pDimsProp))
      {
         g_pPhysDimsProp->Create(objID);
         g_pPhysDimsProp->Get(objID, &pDimsProp);
      }

      switch (pTypeProp->type)
      {
         case kOBBProp:
         {
            mxs_vector scale;

            ObjGetUnscaledDims(objID, &pDimsProp->size);
            if (ObjGetScale(objID, &scale))
            {
               pDimsProp->size.x *= scale.x;
               pDimsProp->size.y *= scale.y;
               pDimsProp->size.z *= scale.z;
            }

            break;
         }

         case kSphereProp:
         {
            mxs_vector size, scale;

            ObjGetUnscaledDims(objID, &size);
            if (ObjGetScale(objID, &scale))
            {
               size.x *= scale.x;
               size.y *= scale.y;
               size.z *= scale.z;
            }
            for (int i=0; i<pTypeProp->num_submodels && i<MAX_PROP_SUBMODELS; i++)
               pDimsProp->radius[i] = size.z / 2;
            break;
         }

         case kSphereHatProp:
         {
            mxs_vector size, scale;

            ObjGetUnscaledDims(objID, &size);
            if (ObjGetScale(objID, &scale))
            {
               size.x *= scale.x;
               size.y *= scale.y;
               size.z *= scale.z;
            }

            pDimsProp->radius[0] = size.z / 2;
            break;
         }
      }

      g_pPhysDimsProp->Set(objID, pDimsProp);
   }

   PhysAuxListenerLock = FALSE;
}

////////////////////////////////////////////////////////////////////////////////

void UpdatePhysProperty(ObjID objID, int type)
{
   cPhysModel *pModel = g_PhysModels.Get(objID);

   if (!pModel)
      return;

   PhysAuxListenerLock = TRUE;

   if (type & PHYS_TYPE)
   {
      cPhysTypeProp *pTypeProp;
      if (!g_pPhysTypeProp->Get(objID, &pTypeProp))
      {
         g_pPhysTypeProp->Create(objID);
         g_pPhysTypeProp->Get(objID, &pTypeProp);
      }
      switch (pModel->GetType(0))
      {
      case kPMT_Sphere: case kPMT_Point:
         pTypeProp->type = kSphereProp;
         break;
      case kPMT_OBB:
         pTypeProp->type = kOBBProp;
         break;
      case kPMT_SphereHat:
         pTypeProp->type = kSphereHatProp;
         break;
      case kPMT_BSP:
      default:
         Warning(("UpdatePhysProperty: bad or missing physics type for obj %d\n", objID));
         // hell, why not...
         pTypeProp->type = kSphereProp;
         break;
      }
      pTypeProp->num_submodels = pModel->NumSubModels();
      pTypeProp->remove_on_sleep = pModel->RemovesOnSleep();
      pTypeProp->special = pModel->IsSpecial();
      g_pPhysTypeProp->Set(objID, pTypeProp);
   }

   if (type & PHYS_ATTR)
   {
      cPhysAttrProp *pAttrProp;

      if (!g_pPhysAttrProp->Get(objID, &pAttrProp))
      {
         g_pPhysAttrProp->Create(objID);
         g_pPhysAttrProp->Get(objID, &pAttrProp);
      }

      pAttrProp->gravity = pModel->GetGravity() * 100;
      pAttrProp->mass = pModel->GetDynamics()->GetMass();
      pAttrProp->density = pModel->GetDynamics()->GetDensity();
      pAttrProp->elasticity = pModel->GetDynamics()->GetElasticity();
      pAttrProp->base_friction = pModel->GetBaseFriction();

      pAttrProp->cog_offset = pModel->GetCOGOffset();

      switch (pModel->GetType(0))
      {
         case kPMT_OBB:
         {
            pAttrProp->climbable_sides = ((cPhysOBBModel *)pModel)->GetClimbableSides();
            pAttrProp->edge_trigger = ((cPhysOBBModel *)pModel)->IsEdgeTrigger();
	    pAttrProp->pore_size = ((cPhysOBBModel *)pModel)->GetPoreSize();
            break;
         }

         case kPMT_Sphere:
         case kPMT_Point:
         case kPMT_SphereHat:
         {
            pAttrProp->rot_axes = pModel->GetRotAxes();
            pAttrProp->rest_axes = pModel->GetRestAxes();
            break;
         }
      }

      g_pPhysAttrProp->Set(objID, pAttrProp);
   }

   if (type & PHYS_STATE)
   {
      cPhysStateProp *pStateProp;

      if (!g_pPhysStateProp->Get(objID, &pStateProp))
      {
         g_pPhysStateProp->Create(objID);
         g_pPhysStateProp->Get(objID, &pStateProp);
      }

      pStateProp->location = pModel->GetLocationVec();
      ANGVEC_TO_DEGVEC(&pStateProp->facing, pModel->GetRotation());
      pStateProp->velocity = pModel->GetVelocity();
      pStateProp->rot_velocity = pModel->GetDynamics()->GetRotationalVelocity();

      g_pPhysStateProp->Set(objID, pStateProp);
   }

   if (type & PHYS_CONTROL)
   {
      cPhysControlProp *pControlProp;

      if (!g_pPhysControlProp->Get(objID, &pControlProp))
      {
         g_pPhysControlProp->Create(objID);
         g_pPhysControlProp->Get(objID, &pControlProp);
      }

      if (pModel->IsAxisVelocityControlled())
         pControlProp->control_prop_types |= kCPT_AxisVelocity;
      else
         pControlProp->control_prop_types &= ~kCPT_AxisVelocity;
      pControlProp->axis_vel.x = pModel->GetControls()->GetControlAxisVelocity(0);
      pControlProp->axis_vel.y = pModel->GetControls()->GetControlAxisVelocity(1);
      pControlProp->axis_vel.z = pModel->GetControls()->GetControlAxisVelocity(2);

      if (pModel->IsVelocityControlled())
         pControlProp->control_prop_types |= kCPT_Velocity;
      else
         pControlProp->control_prop_types &= ~kCPT_Velocity;
      pControlProp->vel = pModel->GetControls()->GetControlVelocity();

      if (pModel->IsRotationalVelocityControlled())
         pControlProp->control_prop_types |= kCPT_RotVelocity;
      else
         pControlProp->control_prop_types &= ~kCPT_RotVelocity;
      pControlProp->rot_vel = pModel->GetControls()->GetControlRotationalVelocity();

      if (pModel->IsLocationControlled())
         pControlProp->control_prop_types |= kCPT_Location;
      else
         pControlProp->control_prop_types &= ~kCPT_Location;

      if (pModel->IsRotationControlled())
         pControlProp->control_prop_types |= kCPT_Rotation;
      else
         pControlProp->control_prop_types &= ~kCPT_Rotation;

      g_pPhysControlProp->Set(objID, pControlProp);
   }

   if (type & PHYS_DIMS)
   {
      cPhysDimsProp *pDimsProp;

      if (!g_pPhysDimsProp->Get(objID, &pDimsProp))
      {
         g_pPhysDimsProp->Create(objID);
         g_pPhysDimsProp->Get(objID, &pDimsProp);
      }

      switch (pModel->GetType(0))
      {
         case kPMT_OBB:
         {
            pDimsProp->offset[0] = ((cPhysOBBModel *)pModel)->GetOffset();
            pDimsProp->size = ((cPhysOBBModel *)pModel)->GetEdgeLengths();
            break;
         }

         case kPMT_Sphere:
         case kPMT_Point:
         {
            for (int i=0; (i<pModel->NumSubModels()) && (i<MAX_PROP_SUBMODELS); i++)
            {
               pDimsProp->radius[i] = ((cPhysSphereModel *)pModel)->GetRadius(i);
               pDimsProp->offset[i] = pModel->GetSubModOffset(i);
            }
            pDimsProp->pt_vs_terrain = pModel->IsPointVsTerrain();
            pDimsProp->pt_vs_not_special = pModel->IsPointVsNotSpecial();
            break;
         }

         case kPMT_SphereHat:
         {
            pDimsProp->radius[0] = ((cPhysSphereModel *)pModel)->GetRadius(0);
            pDimsProp->offset[0] = pModel->GetSubModOffset(0);
            pDimsProp->pt_vs_terrain = pModel->IsPointVsTerrain();
            pDimsProp->pt_vs_not_special = pModel->IsPointVsNotSpecial();
            break;
         }
      }

      g_pPhysDimsProp->Set(objID, pDimsProp);
   }

   PhysAuxListenerLock = FALSE;
}

void UpdatePhysModel(ObjID objID, int type)
{
   cPhysModel *pModel = g_PhysModels.Get(objID);

   if (pModel == NULL)
   {
      Warning(("UpdatePhysModel: obj %d has no physics!\n", objID));
      return;
   }

   if ((type & PHYS_ATTR) && g_pPhysAttrProp->IsRelevant(objID))
   {
      cPhysAttrProp *pAttrProp;

      g_pPhysAttrProp->Get(objID, &pAttrProp);

      pModel->SetGravity(pAttrProp->gravity / 100.0);
      pModel->GetDynamics()->SetMass(pAttrProp->mass);
      pModel->GetDynamics()->SetDensity(pAttrProp->density);
      pModel->GetDynamics()->SetElasticity(pAttrProp->elasticity);
      pModel->SetBaseFriction(pAttrProp->base_friction);

      pModel->SetCOGOffset(pAttrProp->cog_offset);

      switch (pModel->GetType(0))
      {
         case kPMT_OBB:
         {
            ((cPhysOBBModel *)pModel)->SetClimbableSides(pAttrProp->climbable_sides);
            ((cPhysOBBModel *)pModel)->SetEdgeTrigger(pAttrProp->edge_trigger);
	    ((cPhysOBBModel *)pModel)->SetPoreSize(pAttrProp->pore_size);
            break;
         }

         case kPMT_Sphere:
         case kPMT_Point:
         case kPMT_SphereHat:
         {
            pModel->SetRotAxes(pAttrProp->rot_axes);
            pModel->SetRestAxes(pAttrProp->rest_axes);
            break;
         }
      }
   }

   if ((type & PHYS_STATE) && g_pPhysStateProp->IsRelevant(objID))
   {
      cPhysStateProp *pStateProp;
      mxs_angvec rot;

      g_pPhysStateProp->Get(objID, &pStateProp);

      pModel->SetLocationVec(pStateProp->location);
      DEGVEC_TO_ANGVEC(&rot, pStateProp->facing);
      pModel->SetRotation(rot);
      pModel->GetDynamics()->SetVelocity(pStateProp->velocity);
      pModel->GetDynamics()->SetRotationalVelocity(pStateProp->rot_velocity);
   }

   if ((type & PHYS_CONTROL) && g_pPhysControlProp->IsRelevant(objID))
   {
      cPhysControlProp *pControlProp;
      cPhysCtrlData *pCtrl = pModel->GetControls();

      g_pPhysControlProp->Get(objID, &pControlProp);

      if (pControlProp->control_prop_types & kCPT_AxisVelocity)
      {
         for (int i=0; i<3; i++)
         {
            if (pControlProp->axis_vel.el[0] != 0)
               pModel->GetControls()->SetControlVelocity(i, pControlProp->axis_vel.el[0]);
         }
      }

      if (pControlProp->control_prop_types & kCPT_Velocity)
         pCtrl->SetControlVelocity(pControlProp->vel);
      else
         pCtrl->StopControlVelocity();

      if (pControlProp->control_prop_types & kCPT_RotVelocity)
         pCtrl->SetControlRotationalVelocity(pControlProp->rot_vel);
      else
         pCtrl->StopControlRotationalVelocity();

      if (pControlProp->control_prop_types & kCPT_Location)
         pCtrl->ControlLocation(pModel->GetLocationVec());
      else
         pCtrl->StopControlLocation();

      if (pControlProp->control_prop_types & kCPT_Rotation)
         pCtrl->ControlRotation(pModel->GetRotation());
      else
         pCtrl->StopControlRotation();
   }

   if ((type & PHYS_DIMS) && g_pPhysDimsProp->IsRelevant(objID))
   {
      cPhysDimsProp *pDimsProp;

      g_pPhysDimsProp->Get(objID, &pDimsProp);

      switch (pModel->GetType(0))
      {
         case kPMT_OBB:
         {
            ((cPhysOBBModel *)pModel)->SetOffset(pDimsProp->offset[0]);
            ((cPhysOBBModel *)pModel)->SetEdgeLengths(pDimsProp->size);
            break;
         }

         case kPMT_Sphere:
         case kPMT_Point:
         {
            for (int i=0; (i<pModel->NumSubModels()) && (i<MAX_PROP_SUBMODELS); i++)
            {
               ((cPhysSphereModel *)pModel)->SetRadius(i, pDimsProp->radius[i]);
               pModel->SetSubModOffset(i, pDimsProp->offset[i]);
            }
            pModel->SetPointVsTerrain(pDimsProp->pt_vs_terrain);
            pModel->SetPointVsNotSpecial(pDimsProp->pt_vs_not_special);
            break;
         }

         case kPMT_SphereHat:
         {
            ((cPhysSphereModel *)pModel)->SetRadius(0, pDimsProp->radius[0]);
            pModel->SetSubModOffset(0, pDimsProp->offset[0]);
            pModel->SetPointVsTerrain(pDimsProp->pt_vs_terrain);
            pModel->SetPointVsNotSpecial(pDimsProp->pt_vs_not_special);
            break;
         }
      }
   }
}

////////////////////////////////////////////////////////////////////////////////








