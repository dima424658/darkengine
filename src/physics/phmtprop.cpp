/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmtprop.cpp,v 1.17 1999/08/05 17:22:14 Justin Exp $
//
// Physics moving terrain properties/links
//

#include <lg.h>
#include <matrixs.h>
#include <comtools.h>
#include <appagg.h>

#include <linkbase.h> // for flags and structures
#include <relation.h> // for factory function
#include <linkstor.h> // for link store factory
#include <linkdraw.h>
#include <linkman.h>
#include <autolink.h>

#include <property.h>
#include <propface.h>
#include <propbase.h>
#include <prophash.h>
#include <propert_.h>
#include <dataops_.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <physapi.h>
#include <phcore.h>
#include <phmods.h>
#include <phmod.h>
#include <phconst.h>
#include <phmterr.h>
#include <phmtprop.h>
#include <phprop.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>


////////////////////////////////////////

//
// Relation Descriptor Structure
//
static sRelationDesc terrain_path_desc = 
{
   TERRAIN_PATH_NAME,
   kRelationNetworkLocalOnly,
};

//
// Relation data descriptor
//
static sRelationDataDesc terrain_path_data_desc = 
   LINK_DATA_DESC_FLAGS(sTerrainPath, kRelationDataAutoCreate);

//
// Sdesc field descriptor
//
static sFieldDesc TerrainPathDataFields[] = 
{
   { "Speed",         kFieldTypeFloat,   FieldLocation(sTerrainPath, speed) },
   { "Pause (ms)",    kFieldTypeInt,     FieldLocation(sTerrainPath, pause) },
   { "Path Limit?",   kFieldTypeBool,    FieldLocation(sTerrainPath, path_limit) },
};

//
// Build the data descriptor for the structure
//
static sStructDesc TerrainPathDataDesc = StructDescBuild(sTerrainPath, kStructFlagNone, TerrainPathDataFields);

////////////////////////////////////////

static sRelationDesc terrain_path_init_desc = 
{
   TERRAIN_PATH_INIT_NAME,
   kRelationNetworkLocalOnly,
};

static sRelationDataDesc terrain_path_init_data_desc = 
{
   "None",
   0,
};

////////////////////////////////////////

static sRelationDesc terrain_path_next_desc = 
{
   TERRAIN_PATH_NEXT_NAME,
   kRelationNoEdit | kRelationNetworkLocalOnly,
};

static sRelationDataDesc terrain_path_next_data_desc = 
{
   "None",
   0,
};

////////////////////////////////////////////////////////////////////////////////

void CreateTerrainPathRelations(void)
{
   IRelation *rel;

   rel = CreateStandardRelation(&terrain_path_desc, &terrain_path_data_desc, kQCaseSetAll);
   SafeRelease(rel); 

   rel = CreateStandardRelation(&terrain_path_init_desc, &terrain_path_init_data_desc, kQCaseSetAll);
   SafeRelease(rel);

   rel = CreateStandardRelation(&terrain_path_next_desc, &terrain_path_next_data_desc, kQCaseSetAll);
   SafeRelease(rel);
}

void InitTerrainPathLinks()
{
   CreateTerrainPathRelations();

   StructDescRegister(&TerrainPathDataDesc);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class cMovingTerrainOps : public cClassDataOps<cMovingTerrainProp>
{
public:
   cMovingTerrainOps() : cClassDataOps<cMovingTerrainProp>(kNoFlags) {};
};

typedef cHashPropertyStore<cMovingTerrainOps> cMovingTerrainStore;

typedef cSpecificProperty<IMovingTerrainProperty, &IID_IMovingTerrainProperty, cMovingTerrainProp *, cMovingTerrainStore> cMovingTerrainPropertyBase;

class cMovingTerrainProperty : public cMovingTerrainPropertyBase
{
public: 
   cMovingTerrainProperty(const sPropertyDesc* desc)
      : cMovingTerrainPropertyBase(desc) {};

   STANDARD_DESCRIBE_TYPE(cMovingTerrainProp);
};

// Constraints
static sPropertyConstraint MovingTerrainProp_const[] = 
{
   { kPropertyNullConstraint, NULL }
}; 

// Description
static sPropertyDesc MovingTerrainProp_desc[] = 
{
   MOVING_TERRAIN_PROP_NAME,
   kPropertyNoInherit | kPropertyInstantiate,
   MovingTerrainProp_const,
   1,
   0,
   {"Physics: Misc", "MovingTerrain" },
   kPropertyChangeLocally,
};

////////////////////////////////////////

IMovingTerrainProperty *g_pMovingTerrainProperty = NULL;

////////////////////////////////////////

void LGAPI MovingTerrainListener(sPropertyListenMsg * msg, PropListenerData data);
void SetupMovingTerrainProperty(void);

IMovingTerrainProperty *CreateMovingTerrainProperty(const sPropertyDesc *desc)
{
   SetupMovingTerrainProperty();
   return new cMovingTerrainProperty(desc);
}

////////////////////////////////////////

void InitMovingTerrainProps()
{
   g_pMovingTerrainProperty = CreateMovingTerrainProperty(MovingTerrainProp_desc);

   // Install listener
   g_pMovingTerrainProperty->Listen(kListenPropModify | kListenPropSet | kListenPropUnset,
                                    MovingTerrainListener, NULL);
}

void TermMovingTerrainProps()
{
   Assert_(g_pMovingTerrainProperty);

   SafeRelease(g_pMovingTerrainProperty);
   g_pMovingTerrainProperty = NULL;
}

BOOL IsMovingTerrainActive(ObjID objID)
{
   Assert_(g_pMovingTerrainProperty);

   cMovingTerrainProp *pMTProp;

   if (g_pMovingTerrainProperty->Get(objID, &pMTProp))
      return (pMTProp->active == 1);
   else
      return FALSE;
}

void SetMovingTerrainActive(ObjID objID, BOOL status)
{
   Assert_(g_pMovingTerrainProperty);

   cMovingTerrainProp *pMTProp;

   g_pMovingTerrainProperty->Get(objID, &pMTProp);
   pMTProp->active = status;
   g_pMovingTerrainProperty->Set(objID, pMTProp);
}

BOOL IsMovingTerrainMoving(ObjID objID)
{
   Assert_(g_pMovingTerrainProperty);

   cMovingTerrainProp *pMTProp;

   if (g_pMovingTerrainProperty->Get(objID, &pMTProp))
   {
      AutoAppIPtr_(LinkManager, pLinkMan);

      IRelation *pNextPath = pLinkMan->GetRelationNamed(TERRAIN_PATH_NEXT_NAME);
      cAutoLinkQuery next_query(pNextPath, objID);
      SafeRelease(pNextPath);

      if (!next_query->Done())
      {
         IRelation *pPath = pLinkMan->GetRelationNamed(TERRAIN_PATH_NAME);
         cAutoLinkQuery path_query(pPath, LINKOBJ_WILDCARD, next_query.GetDest());
         SafeRelease(pPath);

         if (!path_query->Done())
         {
            sTerrainPath *pTerrainPath = (sTerrainPath *)pLinkMan->GetData(path_query->ID());

            if (pTerrainPath->cur_paused < 0)
               return TRUE;
         }
      }
   }

   return FALSE;
}

////////////////////////////////////////

// Structure descriptor
static sFieldDesc MovingTerrainFields[] = 
{
   { "Active",  kFieldTypeBool,  FieldLocation(cMovingTerrainProp, active), },
};

static sStructDesc MovingTerrainDesc = StructDescBuild(cMovingTerrainProp, kStructFlagNone, MovingTerrainFields);

static void SetupMovingTerrainProperty()
{
   StructDescRegister(&MovingTerrainDesc);
}

////////////////////////////////////////

static BOOL MT_Lock = FALSE;

void LGAPI MovingTerrainListener(sPropertyListenMsg * msg, PropListenerData data)
{
   if (MT_Lock)
      return;

   cMovingTerrainProp *pMTProp = (cMovingTerrainProp *)msg->value.ptrval;
   mxs_angvec rot;

   mx_mk_angvec(&rot, 0, 0, 0);

   // Initialize on creation
   if (msg->type & kListenPropSet)
   {
      if (pMTProp->active == -1)
      {
         pMTProp->active = TRUE;
         pMTProp->prev_state = FALSE;
      }

      MT_Lock = TRUE;
      g_pMovingTerrainProperty->Set(msg->obj, pMTProp);
      MT_Lock = FALSE;

      // Refresh the physics 
      if (OBJ_IS_CONCRETE(msg->obj))
      {
         cPhysTypeProp *pTypeProp;

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

         cPhysModel *pModel = g_PhysModels.Get(msg->obj);

         if (pModel)
            pModel->SetMovingTerrain(TRUE);
           
         PhysSetGravity(msg->obj, 0.0);
         PhysSetBaseFriction(msg->obj, kFrictionFactor * kGravityAmt / 3);
         PhysControlRotation(msg->obj, &rot);
      }
   }

   if ((msg->type & kListenPropSet) || (msg->type & kListenPropModify))
   {
      // Edge-trigger activation/deactivation
      if (pMTProp->prev_state != pMTProp->active)
      {
         if (OBJ_IS_CONCRETE(msg->obj))
         {
            if (pMTProp->active)
               ActivateMovingTerrain(msg->obj);
            else
               DeactivateMovingTerrain(msg->obj);
         }

         // Update the property
         pMTProp->prev_state = pMTProp->active;

         MT_Lock = TRUE;
         g_pMovingTerrainProperty->Set(msg->obj, pMTProp);
         MT_Lock = FALSE;
      }
   }
   else
   if (msg->type & kListenPropUnset)
   {
      // Remove the physics
      if (PhysObjHasPhysics(msg->obj))
         PhysDeregisterModel(msg->obj);

      // Deactivate
      DeactivateMovingTerrain(msg->obj);

      // Update the property
      pMTProp->prev_state = pMTProp->active;

      MT_Lock = TRUE;
      g_pMovingTerrainProperty->Set(msg->obj, pMTProp);
      MT_Lock = FALSE;
   }
}

////////////////////////////////////////












