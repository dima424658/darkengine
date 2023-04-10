/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/doorprop.cpp,v 1.23 2000/03/07 19:54:00 toml Exp $
//
//
//
#include <lg.h>

#include <objpos.h>
#include <matrix.h>
#include <osysbase.h>

#include <property.h>
#include <propbase.h>
#include <propert_.h>
#include <dataops_.h>
#include <propface.h>
#include <prophash.h>
#include <sdesbase.h>
#include <sdesc.h>
#include <simpwrap.h>

#include <physapi.h>
#include <phcore.h>
#include <phmods.h>
#include <phmod.h>
#include <doorprop.h>
#include <doorphys.h>
#include <phprop.h>

#include <objnotif.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////////////////////////

IRotDoorProperty   *g_pRotDoorProperty;
ITransDoorProperty *g_pTransDoorProperty;

////////////////////////////////////////////////////////////////////////////////

sDoorProp *GetDoorProperty(int o_id)
{
   sRotDoorProp   *pRotDoorProp;
   sTransDoorProp *pTransDoorProp;

   if (g_pRotDoorProperty->Get(o_id, &pRotDoorProp))
      return (sDoorProp *)pRotDoorProp;

   if (g_pTransDoorProperty->Get(o_id, &pTransDoorProp))
      return (sDoorProp *)pTransDoorProp;

   return NULL;
}

////////////////////////////////////////////////////////////////////////////////

// Constraints
static sPropertyConstraint door_constraints[] = 
{
   { kPropertyRequires, PHYS_TYPE_NAME }, 
   { kPropertyNullConstraint }
};

////////////////////////////////////////////////////////////////////////////////
//
// Rotating doors
//

// Description
static sPropertyDesc RotPropDesc = 
{
   PROP_ROTDOOR_NAME,
   kPropertyNoInherit | kPropertyInstantiate,
   door_constraints,
   1,  // version
   0,  // last ok version (0 means no other ok versions)
   { "Door", "Rotating" },
   kPropertyChangeLocally,
}; 

//
// Hey kids, lets get our own notify 

typedef cGenericProperty<IRotDoorProperty,&IID_IRotDoorProperty, sRotDoorProp*> cRotDoorPropertyBase;

class cRotDoorProperty : public cRotDoorPropertyBase
{
   cClassDataOps< sRotDoorProp> mOps; 

public: 
   cRotDoorProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cRotDoorPropertyBase(desc,store)
   {
      SetOps(&mOps); 
   }

   cRotDoorProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cRotDoorPropertyBase(desc,NULL)
   {
      IPropertyStore* store = CreateGenericPropertyStore(impl);
      SetStore(store); 
      SetOps(&mOps); 
   }

   STDMETHOD_(void,Notify)(ePropertyNotifyMsg msg, PropNotifyData data)
   {
      switch(NOTIFY_MSG(msg))
      {
         case kObjNotifyPostLoad:
            GenerateAllDoorRooms();
            break; 
      }
      cRotDoorPropertyBase::Notify(msg,data);
   }

   STANDARD_DESCRIBE_TYPE( sRotDoorProp);
};

////////////////////////////////////////

static const char *pDoorStates[5] = 
{
   "Closed",
   "Open",
   "Closing",
   "Opening",
   "Halted",
};

static const char *pDoorAxes[3] = 
{
  "X-Axis",
  "Y-Axis",
  "Z-Axis",
};

static sFieldDesc RotDoorPropFields[] = 
{
   { "Closed Angle",   kFieldTypeFloat,  FieldLocation(sRotDoorProp, closed),          kFieldFlagNone },
   { "Open Angle",     kFieldTypeFloat,  FieldLocation(sRotDoorProp, open),            kFieldFlagNone },
   { "Base Speed",     kFieldTypeFloat,  FieldLocation(sRotDoorProp, base_speed),      kFieldFlagNone },
   { "Axis",           kFieldTypeEnum,   FieldLocation(sRotDoorProp, axis),            kFieldFlagNone, 0, 3, 3, pDoorAxes },
   { "Status",         kFieldTypeEnum,   FieldLocation(sRotDoorProp, status),          kFieldFlagNone, 0, 5, 5, pDoorStates },
   { "Hard Limits?",   kFieldTypeBool,   FieldLocation(sRotDoorProp, hard_limits),     kFieldFlagNone },
   { "Blocks Vision?", kFieldTypeBool,   FieldLocation(sRotDoorProp, vision_blocking), kFieldFlagNone },
   { "Blocks Sound %", kFieldTypeFloat,  FieldLocation(sRotDoorProp, sound_blocking),  kFieldFlagNone },
   { "Push Mass",      kFieldTypeFloat,  FieldLocation(sRotDoorProp, push_mass),       kFieldFlagNone },
   { "Clockwise?",     kFieldTypeBool,   FieldLocation(sRotDoorProp, clockwise),       kFieldFlagNone },
   { "Room ID #1",     kFieldTypeInt,    FieldLocation(sRotDoorProp, room1),           kFieldFlagNotEdit },
   { "Room ID #2",     kFieldTypeInt,    FieldLocation(sRotDoorProp, room2),           kFieldFlagNotEdit },
};

// build by hand
static sStructDesc RotDoorDesc = StructDescBuild(sRotDoorProp, kStructFlagNone, RotDoorPropFields);

////////////////////////////////////////

void LGAPI RotDoorListener(sPropertyListenMsg *msg, PropListenerData data);
static void SetupRotDoorProperty(void);

////////////////////////////////////////

class cRotDoorOps : public cClassDataOps<sRotDoorProp>
{
public:
   STDMETHOD_(int,Version)();
   STDMETHOD(Read)(sDatum* dat, IDataOpsFile* file, int version);
};

typedef cHashPropertyStore<cRotDoorOps> cRotDoorStore;

typedef cSpecificProperty<IRotDoorProperty, &IID_IRotDoorProperty, sRotDoorProp*, cRotDoorStore> cBaseRotDoorProp;

class cRotDoorProp : public cBaseRotDoorProp
{
public:
   cRotDoorProp(const sPropertyDesc* desc) : cBaseRotDoorProp(desc) {};

   STANDARD_DESCRIBE_TYPE(sRotDoorProp);
};

STDMETHODIMP_(int) cRotDoorOps::Version()
{
   return 1001;
}

STDMETHODIMP cRotDoorOps::Read(sDatum* pdat, IDataOpsFile* file, int v)
{
   if (!pdat->value)
      *pdat = New();

   sRotDoorProp *pRotDoorProp = (sRotDoorProp *)pdat->value;

   file->Read(&pRotDoorProp->type, sizeof(int));
   file->Read(&pRotDoorProp->closed, sizeof(mxs_real));
   file->Read(&pRotDoorProp->open, sizeof(mxs_real));
   file->Read(&pRotDoorProp->base_speed, sizeof(mxs_real));
   file->Read(&pRotDoorProp->axis, sizeof(int));
   file->Read(&pRotDoorProp->status, sizeof(int));
   file->Read(&pRotDoorProp->hard_limits, sizeof(BOOL));
   file->Read(&pRotDoorProp->sound_blocking, sizeof(mxs_real));
   file->Read(&pRotDoorProp->vision_blocking, sizeof(BOOL));
   if (v < 1000)
      pRotDoorProp->push_mass = 25.0;
   else
      file->Read(&pRotDoorProp->push_mass, sizeof(mxs_real)); 
   file->Read(&pRotDoorProp->base_closed_location, sizeof(mxs_vector));
   file->Read(&pRotDoorProp->base_open_location, sizeof(mxs_vector));
   file->Read(&pRotDoorProp->base_location, sizeof(mxs_vector));
   file->Read(&pRotDoorProp->base_angle, sizeof(mxs_angvec));
   file->Read(&pRotDoorProp->base, sizeof(mxs_real));
   file->Read(&pRotDoorProp->room1, sizeof(int));
   file->Read(&pRotDoorProp->room2, sizeof(int));
   file->Read(&pRotDoorProp->clockwise, sizeof(BOOL));
   if (v >= 1001)
   {
      file->Read(&pRotDoorProp->base_closed_facing, sizeof(mxs_angvec));
      file->Read(&pRotDoorProp->base_open_facing, sizeof(mxs_angvec));
   }

   return S_OK;
}

////////////////////////////////////////

IRotDoorProperty *RotDoorPropertyInit(void)
{
   SetupRotDoorProperty();

   g_pRotDoorProperty = new cRotDoorProp(&RotPropDesc);

   // Install listener
   g_pRotDoorProperty->Listen(kListenPropModify | kListenPropSet | kListenPropUnset | kListenPropLoad,
                              RotDoorListener, NULL);

   return g_pRotDoorProperty;
}

////////////////////////////////////////

void RotDoorPropertyTerm(void)
{
   SafeRelease(g_pRotDoorProperty);
}

////////////////////////////////////////

static void SetupRotDoorProperty(void)
{
   StructDescRegister(&RotDoorDesc);
}

////////////////////////////////////////

BOOL rot_door_lock = FALSE;

void LGAPI RotDoorListener(sPropertyListenMsg *msg, PropListenerData data)
{
   sRotDoorProp *pDoorProp;
   
   if (!msg->value.ptrval || rot_door_lock)
      return;

   pDoorProp = (sRotDoorProp *)msg->value.ptrval;

#if 0
   if (OBJ_IS_CONCRETE(msg->obj) && !PhysObjHasPhysics(msg->obj) && !(msg->type & kListenPropUnset))
      PhysRegisterOBBDefault(msg->obj);
#endif

   // Load
   if ((msg->type & kListenPropLoad) && (pDoorProp->status != kDoorClosed))
   {
      cPhysModel *pModel;

      if ((pModel = g_PhysModels.Get(msg->obj)) != NULL)
      {
         pModel->ClearAngleLimits();

         mxs_angvec cur_ang = ObjPosGet(msg->obj)->fac;
         ObjRotate(msg->obj, &pDoorProp->base_angle);

         pModel->AddAngleLimit(pDoorProp->axis, pDoorProp->open, RotDoorOpenCallback);
         pModel->AddAngleLimit(pDoorProp->axis, 0, RotDoorCloseCallback);

         ObjRotate(msg->obj, &cur_ang);
      }

      return;
   }         


   // Creation
   if (msg->type & kListenPropSet)
   {
      if (!(msg->type & kListenPropModify))
      {
         // default to the z-axis and 1.0 radians/sec
         pDoorProp->axis = 2;
         pDoorProp->base_speed = 1.0;

         pDoorProp->type = ROTATING_DOOR;

         pDoorProp->vision_blocking = TRUE;
         pDoorProp->sound_blocking = 60.0;

         pDoorProp->push_mass = 25.0;

         pDoorProp->room1 = -1;
         pDoorProp->room2 = -1;
      }

      if (OBJ_IS_CONCRETE(msg->obj))
      {
         UpdateDoorPhysics(msg->obj);
         UpdateDoorBrush(msg->obj);
      }

      ROT_DOOR_LOCK;
      g_pRotDoorProperty->Set(msg->obj, pDoorProp);
      ROT_DOOR_UNLOCK;

      return;
   }

   // Modification
   if (msg->type & kListenPropModify)
   {
      // check for valid axis
      if ((pDoorProp->axis < 0) || (pDoorProp->axis > 2))
         pDoorProp->axis = 2;

      // check for valid status
      if ((pDoorProp->status < 0) || (pDoorProp->status > 3))
         pDoorProp->status = 0;

      // check for valid angle
      if (pDoorProp->open < 0)
      {
         while (pDoorProp->open < 0)
            pDoorProp->open += 360.0;
         while (pDoorProp->open >= 360)
            pDoorProp->open -= 360.0;
      }
      if (pDoorProp->closed < 0)
      {
         while (pDoorProp->closed < 0)
            pDoorProp->closed += 360.0;
         while (pDoorProp->closed >= 360)
            pDoorProp->closed -= 360.0;
      }

      if (OBJ_IS_CONCRETE(msg->obj))
      {
         UpdateDoorPhysics(msg->obj);
         GenerateBaseDoorLocations(msg->obj);
      }

      ROT_DOOR_LOCK;
      g_pRotDoorProperty->Set(msg->obj, pDoorProp);
      ROT_DOOR_UNLOCK;

      return;
   }

   // Deletion
   if (msg->type & kListenPropUnset)
   {
      return;
   }
}

////////////////////////////////////////////////////////////////////////////////
//
// Translating doors
//

// Description
static sPropertyDesc TransPropDesc = 
{
   PROP_TRANSDOOR_NAME,
   kPropertyNoInherit | kPropertyInstantiate,
   door_constraints,
   1,  // version
   0,  // last ok version (0 means no other ok versions)
   { "Door", "Translating" },
   kPropertyChangeLocally,
}; 

typedef cGenericProperty<ITransDoorProperty,&IID_ITransDoorProperty, sTransDoorProp*> cTransDoorPropertyBase;

class cTransDoorProperty : public cTransDoorPropertyBase
{
   cClassDataOps< sTransDoorProp> mOps; 

public: 
   cTransDoorProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cTransDoorPropertyBase(desc,store)
   {
      SetOps(&mOps); 
   }

   cTransDoorProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cTransDoorPropertyBase(desc,CreateGenericPropertyStore(impl))
   {
      SetOps(&mOps); 
   }

   STANDARD_DESCRIBE_TYPE( sTransDoorProp);
};

////////////////////////////////////////

static sFieldDesc TransDoorPropFields[] = 
{
   { "Closed Position",  kFieldTypeFloat,  FieldLocation(sTransDoorProp, closed),        kFieldFlagNone },
   { "Open Position",    kFieldTypeFloat,  FieldLocation(sTransDoorProp, open),          kFieldFlagNone },
   { "Base Speed",       kFieldTypeFloat,  FieldLocation(sTransDoorProp, base_speed),    kFieldFlagNone },
   { "Axis",             kFieldTypeEnum,   FieldLocation(sTransDoorProp, axis),          kFieldFlagNone, 0, 3, 3, pDoorAxes },
   { "Status",           kFieldTypeEnum,   FieldLocation(sTransDoorProp, status),        kFieldFlagNone, 0, 5, 5, pDoorStates },
   { "Hard Limits?",     kFieldTypeBool,   FieldLocation(sTransDoorProp, hard_limits),   kFieldFlagNone },
   { "Blocks Vision?",   kFieldTypeBool,   FieldLocation(sRotDoorProp, vision_blocking), kFieldFlagNone },
   { "Blocks Sound %",   kFieldTypeFloat,  FieldLocation(sRotDoorProp, sound_blocking),  kFieldFlagNone },
   { "Push Mass",        kFieldTypeFloat,  FieldLocation(sRotDoorProp, push_mass),       kFieldFlagNone },
   { "Room ID #1",       kFieldTypeInt,    FieldLocation(sRotDoorProp, room1),           kFieldFlagNotEdit },
   { "Room ID #2",       kFieldTypeInt,    FieldLocation(sRotDoorProp, room2),           kFieldFlagNotEdit },
};

// build by hand
static sStructDesc TransDoorDesc = StructDescBuild(sTransDoorProp, kStructFlagNone, TransDoorPropFields);

////////////////////////////////////////

void LGAPI TransDoorListener(sPropertyListenMsg *msg, PropListenerData data);
static void SetupTransDoorProperty(void);

////////////////////////////////////////

class cTransDoorOps : public cClassDataOps<sTransDoorProp>
{
public:
   STDMETHOD_(int,Version)();
   STDMETHOD(Read)(sDatum* dat, IDataOpsFile* file, int version);
};

typedef cHashPropertyStore<cTransDoorOps> cTransDoorStore;

typedef cSpecificProperty<ITransDoorProperty, &IID_ITransDoorProperty, sTransDoorProp*, cTransDoorStore> cBaseTransDoorProp;

class cTransDoorProp : public cBaseTransDoorProp
{
public:
   cTransDoorProp(const sPropertyDesc* desc) : cBaseTransDoorProp(desc) {};

   STANDARD_DESCRIBE_TYPE(sTransDoorProp);
};

STDMETHODIMP_(int) cTransDoorOps::Version()
{
   return 1000;
}

STDMETHODIMP cTransDoorOps::Read(sDatum* pdat, IDataOpsFile* file, int v)
{
   if (!pdat->value)
      *pdat = New();

   sTransDoorProp *pTransDoorProp = (sTransDoorProp *)pdat->value;

   file->Read(&pTransDoorProp->type, sizeof(int));
   file->Read(&pTransDoorProp->closed, sizeof(mxs_real));
   file->Read(&pTransDoorProp->open, sizeof(mxs_real));
   file->Read(&pTransDoorProp->base_speed, sizeof(mxs_real));
   file->Read(&pTransDoorProp->axis, sizeof(int));
   file->Read(&pTransDoorProp->status, sizeof(int));
   file->Read(&pTransDoorProp->hard_limits, sizeof(BOOL));
   file->Read(&pTransDoorProp->sound_blocking, sizeof(mxs_real));
   file->Read(&pTransDoorProp->vision_blocking, sizeof(BOOL));
   if (v < 1000)
      pTransDoorProp->push_mass = 25.0;
   else
      file->Read(&pTransDoorProp->push_mass, sizeof(mxs_real)); 
   file->Read(&pTransDoorProp->base_closed_location, sizeof(mxs_vector));
   file->Read(&pTransDoorProp->base_open_location, sizeof(mxs_vector));
   file->Read(&pTransDoorProp->base_location, sizeof(mxs_vector));
   file->Read(&pTransDoorProp->base_angle, sizeof(mxs_angvec));
   file->Read(&pTransDoorProp->base, sizeof(mxs_real));
   file->Read(&pTransDoorProp->room1, sizeof(int));
   file->Read(&pTransDoorProp->room2, sizeof(int));

   return S_OK;
}

////////////////////////////////////////

ITransDoorProperty *TransDoorPropertyInit(void)
{
   SetupTransDoorProperty();
   g_pTransDoorProperty = new cTransDoorProp(&TransPropDesc);

   // Install listener
   g_pTransDoorProperty->Listen(kListenPropModify | kListenPropSet | kListenPropUnset | kListenPropLoad,
                              TransDoorListener, NULL);

   return g_pTransDoorProperty;
}

////////////////////////////////////////

void TransDoorPropertyTerm(void)
{
   SafeRelease(g_pTransDoorProperty);
}

////////////////////////////////////////

static void SetupTransDoorProperty(void)
{
   StructDescRegister(&TransDoorDesc);
}

////////////////////////////////////////

static BOOL trans_door_lock = FALSE;

#define TRANS_DOOR_LOCK    trans_door_lock = TRUE
#define TRANS_DOOR_UNLOCK  trans_door_lock = FALSE

void LGAPI TransDoorListener(sPropertyListenMsg *msg, PropListenerData data)
{
   sTransDoorProp *pDoorProp;
   
   if (!msg->value.ptrval || trans_door_lock)
      return;

   pDoorProp = (sTransDoorProp *)msg->value.ptrval;

   if (OBJ_IS_CONCRETE(msg->obj) && !PhysObjHasPhysics(msg->obj) && !(msg->type & kListenPropUnset))
      PhysRegisterOBBDefault(msg->obj);

   // Load
   if ((msg->type & kListenPropLoad) && (pDoorProp->status != kDoorClosed))
   {
      cPhysModel *pModel;

      if ((pModel = g_PhysModels.Get(msg->obj)) != NULL)
      {
         pModel->ClearTransLimits();
         pModel->AddTransLimit(pDoorProp->base_open_location, TransDoorOpenCallback);
         pModel->AddTransLimit(pDoorProp->base_closed_location, TransDoorClosedCallback);
      }

      return;
   }

   // Creation
   if (msg->type & kListenPropSet)
   {
      if (!(msg->type & kListenPropModify))
      {
         // default to the x-axis and 1.0 ft/sec
         pDoorProp->axis = 0;
         pDoorProp->base_speed = 1.0;

         pDoorProp->type = TRANSLATING_DOOR;

         pDoorProp->vision_blocking = TRUE;
         pDoorProp->sound_blocking = 60.0;

         pDoorProp->push_mass = 25.0;

         pDoorProp->room1 = -1;
         pDoorProp->room2 = -1;
      }

      if (OBJ_IS_CONCRETE(msg->obj))
      {
         UpdateDoorPhysics(msg->obj);
         UpdateDoorBrush(msg->obj);
      }

      TRANS_DOOR_LOCK;
      g_pTransDoorProperty->Set(msg->obj, pDoorProp);
      TRANS_DOOR_UNLOCK;

      return;
   }

   // Modification
   if (msg->type & kListenPropModify)
   {
      // check for valid axis
      if ((pDoorProp->axis < 0) || (pDoorProp->axis > 2))
      {
         pDoorProp->axis = 2;
         
         TRANS_DOOR_LOCK;
         g_pTransDoorProperty->Set(msg->obj, pDoorProp);
         TRANS_DOOR_UNLOCK;
      }

      // check for valid status
      if ((pDoorProp->status < 0) || (pDoorProp->status > 3))
      {
         pDoorProp->status = 0;

         TRANS_DOOR_LOCK;
         g_pTransDoorProperty->Set(msg->obj, pDoorProp);
         TRANS_DOOR_UNLOCK;
      }

      if (OBJ_IS_CONCRETE(msg->obj))
      {
         GenerateBaseDoorLocations(msg->obj);
         UpdateDoorPhysics(msg->obj);
      }

      return;
   }

   // Deletion
   if (msg->type & kListenPropUnset)
   {
      return;
   }
}











