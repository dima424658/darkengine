/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/cretprop.cpp,v 1.49 1999/10/18 14:57:04 BFarquha Exp $

#include <appagg.h>

// property stuff
#include <propbase.h>
#include <propert_.h>
#include <property.h>
#include <propsprs.h>
#include <propcary.h>
#include <propface.h>
#include <sdesc.h>
#include <sdesbase.h>

#include <aiapi.h>
#include <puppet.h>   // for puppet listener
#include <plycbllm.h> // for player listener

#include <schema.h>   // attempting to get tag stuff working
#include <motmngr.h>
#include <ctagset.h>
#include <motprop.h>
#include <tagdbin.h>

#include <mnamprop.h>
#include <posprop.h>

#include <cretprop.h>
#include <creature.h>
#include <creatur_.h>

#include <iobjsys.h>
#include <objnotif.h>

// must be last header
#include <dbmem.h>

EXTERN int g_SimRunning; // from biploop.c

////////////////////////
// Creature property value funcs.

class cCreatureDataOps: public cClassDataOps<sCreatureHandle>
{
public:
   // override default flags
   cCreatureDataOps() : cClassDataOps<sCreatureHandle>(kNoFlags) {};

   STDMETHOD(Delete)(sDatum dat)
   {
      sCreatureHandle* handle = (sCreatureHandle*)dat.value;
      if (handle)
      {
         if(handle->pCreature)
            delete handle->pCreature;
         delete handle;
      }
      return S_OK;
   }

   STDMETHOD(Read)(sDatum* pdat, IDataOpsFile* file, int v)
   {
      if (!pdat->value)
         *pdat = New();

      sCreatureHandle *pCHandle = (sCreatureHandle *)pdat->value;

      file->Read(&pCHandle->type,sizeof(pCHandle->type));
      return S_OK;
   }

   STDMETHOD(Write)(sDatum dat, IDataOpsFile* file)
   {
      sCreatureHandle *pCHandle = (sCreatureHandle *)dat.value;

      // write creature handle
      file->Write((void *)&(pCHandle->type), sizeof(pCHandle->type));
      return S_OK;
   }
};

//
// wsf: 10/18/99: code added to check for presence of physics. Before, physics was just deleted if
// pCreature existed. It is hoped this will fix the T2 rope arrow rope bug without affecting other
// creatures.
//
#if 0 // well, this doesn't work because after destruction, this isn't a registered physics model anymore.
static void MakeCreature(ObjID obj_id, sCreatureHandle *pHandle)
{
   BOOL bIsPhysical;
   BOOL bUpdatePhysics = FALSE;
   AutoAppIPtr(AIManager);

   if(pHandle->pCreature)
      {
      bIsPhysical = pHandle->pCreature->IsPhysical();
      delete pHandle->pCreature;
      bUpdatePhysics = TRUE;
      }

   pHandle->pCreature=CreatureCreate(pHandle->type,obj_id);

   // Update physics
   if (bUpdatePhysics && bIsPhysical)
      pHandle->pCreature->MakePhysical();

   // notify AI about new motor
   if (!!pAIManager)
      pAIManager->SetMotor(obj_id,pHandle->pCreature);
}
#else
static void MakeCreature(ObjID obj_id, sCreatureHandle *pHandle, BOOL bDestroy = TRUE)
{
   AutoAppIPtr(AIManager);
   if (pHandle->pCreature && bDestroy)
      delete pHandle->pCreature;
   pHandle->pCreature=CreatureCreate(pHandle->type,obj_id);
   // notify AI about new motor
   if (!!pAIManager)
      pAIManager->SetMotor(obj_id,pHandle->pCreature);
}
#endif


//
// Creature Property Store.
//

class cCreatureStore : public cCompactArrayPropertyStore<cArrayObjIndexMap,cCreatureDataOps>
{
   void OnFinalRelease() {}; // don't self-delete

public:
   STDMETHOD_(BOOL,GetCopy)(ObjID obj, sDatum* dat)
   {
      sDatum real;
      if (!Get(obj,&real)) return FALSE;
      sCreatureHandle* handle = (sCreatureHandle*)real.value;
      sCreatureHandle* out = new sCreatureHandle(*handle);
      dat->value = out;
      return TRUE;
   }

   STDMETHOD(ReleaseCopy)(ObjID obj, sDatum dat)
   {
      sCreatureHandle* handle = (sCreatureHandle*)dat.value;
      handle->pCreature = NULL;
      delete handle;
      return S_OK;
   }

};

//
// Creature Property
//


typedef cGenericProperty<ICreatureProperty,&IID_ICreatureProperty,sCreatureHandle*>
cBaseCreatureProp;

static LazyAggMember(IObjectSystem) gpObjSys;

static BOOL g_DontListen=FALSE;

class cCreatureProp : public cBaseCreatureProp
{
protected:
   cCreatureStore mStore;

public:
   cCreatureProp(const sPropertyDesc* desc)
      : cBaseCreatureProp(desc,NULL)
   {
      SetStore(&mStore);
   }

   ~cCreatureProp()
   {
      SetStore(NULL);
   }

   void OnListenMsg(ePropertyListenMsg msg, ObjID obj, uPropListenerValue value)
   {
      cBaseCreatureProp::OnListenMsg(msg,obj,value);

      if(g_DontListen) return;

      AutoAppIPtr(AIManager);
      sCreatureHandle* handle = (sCreatureHandle*)value.ptrval;

      if (msg & kListenPropLoad)
      {
         if ((void *)pAIManager && OBJ_IS_CONCRETE(obj) && handle->pCreature != NULL)
            pAIManager->SetMotor(obj,handle->pCreature);
         return ;
      }

      // create the creature object if necessary
      if ((msg & (kListenPropSet|kListenPropModify))
          && OBJ_IS_CONCRETE(obj)
          && handle->type != kCreatureTypeInvalid )
      {
         // Get the real handle
         Get(obj,&handle);
         MakeCreature(obj,handle);
         g_DontListen=TRUE;
         Set(obj,handle);
         g_DontListen=FALSE;
      }

      if ((void *)pAIManager && (msg & (kListenPropUnset)) && OBJ_IS_CONCRETE(obj) && gpObjSys->Exists(obj))
         pAIManager->SetMotor(obj,NULL);
   }

   STDMETHOD_(void,Notify)(ePropertyNotifyMsg msg, PropNotifyData data)
   {
      cBaseCreatureProp::Notify(msg,data);
// wsf: #ifndef added so that ropes from rope arrows will work. Without this, a Creature gets created twice.
// The first time it has a physics interface. The second creation destroys the first, then creates a new
// one without a physics, which is bad. Apparently, this piece of code was ADDED by Jon for some reason.
// Be warned that the side effects of removing it are not fully understood!
// wsf: Additional note. It is now apparent that this fix for the rope bug creates a new bug -
//      creting a new creature results in it having no skeleton. Damn. Will find another solution.
#ifndef FIX_ROPE_BUG
      ObjID obj = ObjID(data);
      sCreatureHandle* pHandle;

      switch (msg)
      {
      case kObjNotifyCreate:
         // Get the real handle
         if (Get(obj,&pHandle))
         {
            sCreatureHandle newHandle = *pHandle;
            MakeCreature(obj, &newHandle, FALSE);
            g_DontListen=TRUE;
            Set(obj, &newHandle);
            g_DontListen=FALSE;
         }
         break;
      }
#endif
   }


   STANDARD_DESCRIBE_TYPE(sCreatureHandle);


   cCreatureStore& Store() { return mStore; };

};


#pragma off(unreferenced)

static cCreatureProp *pCreatureProp;

static sPropertyConstraint creature_constraints[] =
{
   { kPropertyRequires, PROP_POSITION_NAME },
   { kPropertyNullConstraint },
};

static sPropertyDesc creature_desc =
{
   PROP_CREATURE_NAME,
//   "CREATURE",
   kPropertyConcrete|kPropertyNoClone|kPropertySendEndCreate,
   creature_constraints,
   1,  // version
   0,
   { "Creature", "Creature Type", },
   kPropertyChangeLocally,
};

#define NUM_STRINGS(name) (sizeof(name)/sizeof(name[0]))

char *g_aCreatureTypeNames[] = {"Dummy"};

static sFieldDesc creatureFields[] =
{
   { "Type", kFieldTypeEnum, FieldLocation(sCreatureHandle,type), kFieldFlagUnsigned, 0, NUM_STRINGS(g_aCreatureTypeNames), NUM_STRINGS(g_aCreatureTypeNames), g_aCreatureTypeNames},
};

static sStructDesc creatureInfoStruct = StructDescBuild(sCreatureHandle,kStructFlagNone,creatureFields);



//
// CREATURE POSE PROPERTY
//

class cCreaturePoseOps : public cClassDataOps<sCreaturePose>
{
public:
   cCreaturePoseOps(): cClassDataOps<sCreaturePose>(kNoFlags) {};
};

class cCreaturePoseStore : public cSparseHashPropertyStore<cCreaturePoseOps>
{
};

class cCreaturePoseProperty : public cSpecificProperty<ICreaturePoseProperty,&IID_ICreaturePoseProperty,sCreaturePose*,cCreaturePoseStore>
{
   typedef cSpecificProperty<ICreaturePoseProperty,&IID_ICreaturePoseProperty,sCreaturePose*,cCreaturePoseStore> cParent;

public:
   cCreaturePoseProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
      SetRebuildConcretes(TRUE);
   }

   STANDARD_DESCRIBE_TYPE(sCreaturePose);

private:
   // NOTE: don't really want creature module to have to deal with tags
   // and schemas and such, but I couldn't think of a better place to put
   // this.  At least it's all isolated within in function, which is only used
   // by pose property listener.  (KJ 8/98)
   // @NOTE: i moved it here, so blame me too
   void DoPose(const sCreaturePose *pPose, cCreature *pCreature)
   {
      int motHandle;

      if (g_SimRunning)
         return;

      if(pPose->type==kCPT_MotionName)
      {
         motHandle=g_pMotionSet->GetMotHandleFromName((Label *)&pPose->motion);
         pCreature->PoseAtMotionFrame(motHandle,pPose->frac*(g_pMotionSet->GetNumFrames(motHandle)-1));
         pCreature->SetPhysSubModScale(pPose->scale);
         if (pPose->ballistic != pCreature->IsBallistic())
         {
            if (pPose->ballistic)
               pCreature->MakeBallistic(kCMB_Posed);
            else
               pCreature->MakeNonBallistic();
         }
         pCreature->MoveButt(&ObjPosGet(pCreature->GetObjID())->loc.vec,
                             &ObjPosGet(pCreature->GetObjID())->fac, TRUE);
      }
      else if (pPose->type==kCPT_Tags)
      {
         cTagSet tags(pPose->motion);
         cTagDBInput tagDBin;
         cMotionSchema *pSchema;

         tagDBin.Clear();
         tagDBin.AppendTagSet(&tags);
         if (g_pMotionDatabase->GetBestMatch(pCreature->GetActorType(),&tagDBin,&pSchema))
         {
            if (pSchema->GetMotion(0,&motHandle))
            {
               pCreature->PoseAtMotionFrame(motHandle,pPose->frac*(g_pMotionSet->GetNumFrames(motHandle)-1));
               pCreature->SetPhysSubModScale(pPose->scale);

               if (pPose->ballistic != pCreature->IsBallistic())
               {
                  if (pPose->ballistic)
                     pCreature->MakeBallistic(kCMB_Posed);
                  else
                     pCreature->MakeNonBallistic();
               }

               pCreature->MoveButt(&ObjPosGet(pCreature->GetObjID())->loc.vec,
                                   &ObjPosGet(pCreature->GetObjID())->fac, TRUE);
            }
         }
      }
   }

   void UnPose(cCreature *pCreature)
   {
      pCreature->SetPhysSubModScale(1.0);
      if (pCreature->IsBallistic())
         pCreature->MakeNonBallistic();
   }

   void RebuildConcrete(ObjID obj, BOOL fIsRelevant, uPropListenerValue val, ObjID donor)
   {
      if (!gpObjSys->Exists(obj))
         return;

      if (fIsRelevant)
      {
         sCreaturePose *pPose=(sCreaturePose *)val.ptrval;
    	   cCreature *pCreature=CreatureFromObj(obj);
         if (pCreature == NULL)
         {
            AutoAppIPtr(PropertyManager);
            IProperty *pCreatureProp = pPropertyManager->GetPropertyNamed(PROP_CREATURE_NAME);
            pCreatureProp->Create(obj);
            pCreature = CreatureFromObj(obj);
            Assert_(pCreature != NULL);
         }
         // mprintf("rebuildconcrete for %d (frel %d)..",obj,fIsRelevant);
         DoPose(pPose,pCreature);
         cParent::RebuildConcrete(obj,fIsRelevant,val,donor);
      }
      else
      {
         cCreature *pCreature = CreatureFromObj(obj);

         if (pCreature == NULL)
         {
            Warning(("Attempt to unpose non-creature object!\n"));
            return;
         }

         UnPose(pCreature);
         cParent::RebuildConcrete(obj,fIsRelevant,val,donor);
      }
   }
};

static cCreaturePoseProperty *pCreaturePoseProp;

static sPropertyDesc creaturePoseDesc =
{
   PROP_CREATUREPOSE_NAME,
   NULL,
   NULL,
   0,1, // version
   { "Creature", "Current Pose" },
   kPropertyChangeLocally,
};

static char *poseTypeNames[] =
{
   "Tags",
   "Motion Name",
};

// structure descriptor fun
static sFieldDesc creaturePoseFields [] =
{
   { "Type",        kFieldTypeEnum,   FieldLocation(sCreaturePose,type),FullFieldNames(poseTypeNames) },
   { "Motion",      kFieldTypeString, FieldLocation(sCreaturePose,motion) },
   { "Frac",        kFieldTypeFloat,  FieldLocation(sCreaturePose,frac) },
   { "Model scale", kFieldTypeFloat,  FieldLocation(sCreaturePose,scale) },
   { "Ballistic",   kFieldTypeBool,   FieldLocation(sCreaturePose,ballistic) },
};

static sStructDesc creaturePoseStruct = StructDescBuild(sCreaturePose,kStructFlagNone,creaturePoseFields);

////////////////////////////////////////////////
//
// MAKE CREATURE PHYSICAL PROP
//

static IBoolProperty *pCreatureNonPhysProp=NULL;

static sPropertyDesc creatureNonPhysDesc =
{
   PROP_CREATURENONPHYS_NAME,
   0,  // flags
   NULL, // constraints
   0, 0, // Version
   { "Creature", "Is Non-Physical" }, // ui strings
   kPropertyChangeLocally,
};

ICreatureProperty *CreaturePropertiesInit(int nCreatureTypes, const char **pCreatureTypeNames)
{
   cCreatureProp* prop = new cCreatureProp(&creature_desc);

   creatureInfoStruct.fields->max=nCreatureTypes;
   creatureInfoStruct.fields->datasize=nCreatureTypes;
   creatureInfoStruct.fields->data=(void *)pCreatureTypeNames;
   StructDescRegister(&creatureInfoStruct);

   pCreatureProp=prop;

   // install listeners
   prop->Listen(kListenPropModify | kListenPropSet | kListenPropUnset | kListenPropLoad, PuppetCreatureListener, NULL);
   prop->Listen(kListenPropModify | kListenPropSet | kListenPropUnset | kListenPropLoad, PlayerCreatureListener, NULL);

   // create non-phys property
   pCreatureNonPhysProp=CreateBoolProperty(&creatureNonPhysDesc,kPropertyImplSparseHash);

   // create pose property
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&creaturePoseStruct);
   pCreaturePoseProp = new cCreaturePoseProperty(&creaturePoseDesc);

   // return pointer to creature property
   return prop;
}

void CreaturePropertiesTerm()
{
   SafeRelease(pCreatureProp);
   SafeRelease(pCreaturePoseProp);
   SafeRelease(pCreatureNonPhysProp);
}

void ObjSetCreatureType(ObjID obj, int type)
{
   sCreatureHandle cHandle;

   Assert_(pCreatureProp);
   cHandle.type=type;
   cHandle.pCreature=NULL;
   pCreatureProp->Set(obj,&cHandle);
}

BOOL ObjGetCreatureType(ObjID obj, int *pType)
{
   sCreatureHandle *pHandle;

   Assert_(pCreatureProp);
   if(pCreatureProp->Get(obj,&pHandle))
   {
      *pType=pHandle->type;
      return TRUE;
   }
   return FALSE;
}

EXTERN BOOL ObjIsNonPhysicalCreature(ObjID obj)
{
   Assert_(pCreatureNonPhysProp);
   BOOL result=FALSE;
   pCreatureNonPhysProp->Get(obj,&result);
   return result;
}

EXTERN BOOL ObjIsPosed(ObjID obj)
{
   Assert_(pCreaturePoseProp);

   return pCreaturePoseProp->IsRelevant(obj);
}

//
// Fast accessors to the creature property.  They should inline nicely
//


ObjID chandle_obj_id(int idx)
{
   return pCreatureProp->Store().Idx2Obj(idx);
}

sCreatureHandle* CreatureHandle(int idx)
{
   return (sCreatureHandle*)pCreatureProp->Store()[idx].value;
}

int obj_chandle_id(ObjID obj)
{
   return pCreatureProp->Store().Obj2Idx(obj);
}

int max_chandle_id()
{
   return pCreatureProp->Store().MaxIdx();
}


