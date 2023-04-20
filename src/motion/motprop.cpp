/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/motprop.cpp,v 1.9 1999/01/13 13:57:20 KATE Exp $

#include <motprop.h>
#include <property.h>
#include <propbase.h>
#include <propface.h>
#include <propfac_.h>
#include <dataops_.h>
#include <propsprs.h>
#include <matrix.h>
#include <fix.h>

#include <mschbase.h>
#include <sdesbase.h>
#include <sdestool.h>

#include <dbmem.h> // must be last included header


static IMotActorTagListProperty* actorTagListProp = NULL;

static IMotPhysLimitsProperty* motPhysLimitsProp = NULL;

static IMotGaitDescProperty* motGaitDescProp = NULL;

static IIntProperty* motorControllerProp = NULL;

static IIntProperty* motSwordActionProp = NULL;

// @DIPPY, but hopefully functional
#if 0
static IBoolProperty* motBlockProp = NULL;
static IBoolProperty* motRecoverProp = NULL;
#endif

static IFloatProperty* timeWarpProp = NULL;

static IMotPlayerLimbOffsetsProperty* motPlayerLimbOffsetsProp = NULL;

////////////////////////////////////////////////
//
//    ACTOR TAG LIST PROPERTY CREATION
//

class cActorTagListOps : public cClassDataOps<sMotActorTagList>
{
}; 

class cActorTagListStore : public cSparseHashPropertyStore<cActorTagListOps>
{
}; 

class cMotActorTagListProperty : public cSpecificProperty<IMotActorTagListProperty,&IID_IMotActorTagListProperty,sMotActorTagList*,cActorTagListStore>
{
   typedef cSpecificProperty<IMotActorTagListProperty,&IID_IMotActorTagListProperty,sMotActorTagList*,cActorTagListStore> cParent; 

public: 
   cMotActorTagListProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
      
   }

   STANDARD_DESCRIBE_TYPE(sMotActorTagList); 

}; 



static sPropertyDesc actorTagListPropDesc =
{
   PROP_ACTOR_TAG_LIST_NAME,
   0,
   NULL,
   0,1, // verison
   { "Motions", "ActorTagList" }, 
};

// structure descriptor fun
static sFieldDesc actorTagListFields [] =
{
   { "tags", kFieldTypeString, FieldLocation(sMotActorTagList, m_TagStrings) },
};

static sStructDesc actorTagListStruct = StructDescBuild(sMotActorTagList,kStructFlagNone,actorTagListFields);


////////////////////////////////////////////////
//
//    MOTOR CONTROLLER PROPERTY CREATION
//

static sPropertyDesc motControlPropDesc =
{
   PROP_MOTOR_CONTROLLER_NAME,
   0,  // flags
   NULL, // constraints
   0, 0, // Version
   { "Motions", "Motor Controller" }, // ui strings
};

#define MOTCONTROL_TYPENAME "tMotorControllerType"

static sFieldDesc motControllerField[] = 
{
   { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, 0, 0, NULL },
}; 

static sStructDesc motControlSDesc = 
{
   MOTCONTROL_TYPENAME, 
   sizeof(int),
   kStructFlagNone,
   sizeof(motControllerField)/sizeof(motControllerField[0]),
   motControllerField,
}; 

static sPropertyTypeDesc motControlTDesc = 
{
   MOTCONTROL_TYPENAME,
   sizeof(int),
};

#define MOTCONTROLPROP_IMPL kPropertyImplSparseHash


////////////////////////////////////////////////
//
//    TIMEWARP PROPERTY
//

static sPropertyDesc timeWarpPropDesc = 
{
   PROP_TIMEWARP_NAME,
   0,  // flags
   NULL, // constraints
   0, 0, // Version
   { "Creature", "Time Warp" }, // ui strings
}; 

#define TIMEWARP_PROP_IMPL kPropertyImplSparseHash


////////////////////////////////////////////////
//
//    SWORD ACTION PROPERTY CREATION
//

static sPropertyDesc motSwordActionPropDesc =
{
   PROP_SWORD_ACTION_NAME,
   0,  // flags
   NULL, // constraints
   0, 0, // Version
   { "Motions", "Sword Action Type" }, // ui strings
};

#define SWORDACTION_TYPENAME "eMSwordActionType"

static const char *motSwordActionNames[] =
{
   "Swing",
   "General Block",
   "Directed Block",
};

static sFieldDesc motSwordActionField[] = 
{
   { "", kFieldTypeEnum, sizeof(int), 0, FullFieldNames(motSwordActionNames) },
}; 

static sStructDesc motSwordActionSDesc = 
{
   SWORDACTION_TYPENAME, 
   sizeof(int),
   kStructFlagNone,
   sizeof(motSwordActionField)/sizeof(motSwordActionField[0]),
   motSwordActionField,
}; 

static sPropertyTypeDesc motSwordActionTDesc = 
{
   SWORDACTION_TYPENAME,
   sizeof(int),
};

#define MOTSWORDACTIONPROP_IMPL kPropertyImplSparseHash




////////////////////////////////////////////////
//
//    PHYS LIMITS PROPERTY CREATION
//


class cMotPhysLimitsOps : public cClassDataOps<sMotPhysLimits>
{
}; 

class cMotPhysLimitsStore : public cSparseHashPropertyStore<cMotPhysLimitsOps>
{

}; 

class cMotPhysLimitsProperty : public cSpecificProperty<IMotPhysLimitsProperty,&IID_IMotPhysLimitsProperty,sMotPhysLimits*,cMotPhysLimitsStore>
{
   typedef cSpecificProperty<IMotPhysLimitsProperty,&IID_IMotPhysLimitsProperty,sMotPhysLimits*,cMotPhysLimitsStore> cParent; 

public: 
   cMotPhysLimitsProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
      
   }

   STANDARD_DESCRIBE_TYPE(sMotPhysLimits); 

}; 

static sPropertyDesc motPhysLimitsPropDesc =
{
   PROP_MOT_PHYS_LIMITS_NAME,
   NULL,
   NULL, // constraints
   0,0, // version
   { "Motions", "Phys Limits" },
};

// structure descriptor fun
static sFieldDesc physLimitFields [] =
{
   { "Foo 1", kFieldTypeInt, FieldLocation(sMotPhysLimits,foo[0]) },
   { "Foo 2", kFieldTypeInt, FieldLocation(sMotPhysLimits,foo[1]) },
};

static sStructDesc physLimitsStruct = StructDescBuild(sMotPhysLimits,kStructFlagNone,physLimitFields);


////////////////////////////////////////////////
//
//    PLAYER LIMB OFFSETS PROPERTY CREATION
//


class cMotPlayerLimbOffsetsOps : public cClassDataOps<sMPlayerLimbOffsets>
{
}; 

class cMotPlayerLimbOffsetsStore : public cSparseHashPropertyStore<cMotPlayerLimbOffsetsOps>
{
}; 

class cMotPlayerLimbOffsetsProperty : public cSpecificProperty<IMotPlayerLimbOffsetsProperty,&IID_IMotPlayerLimbOffsetsProperty,sMPlayerLimbOffsets*,cMotPlayerLimbOffsetsStore>
{
   typedef cSpecificProperty<IMotPlayerLimbOffsetsProperty,&IID_IMotPlayerLimbOffsetsProperty,sMPlayerLimbOffsets*,cMotPlayerLimbOffsetsStore> cParent; 

public: 
   cMotPlayerLimbOffsetsProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
      
   }

   STANDARD_DESCRIBE_TYPE(sMPlayerLimbOffsets); 

}; 

static sPropertyDesc motPlayerLimbOffsetsPropDesc =
{
   PROP_PLAYER_LIMB_OFFSETS_NAME,
   NULL,
   NULL, // constraints
   0,0, // version
   { "Motions", "Player Limb Offsets" },
};

// structure descriptor fun
static sFieldDesc limbOffsetsFields [] =
{
   { "Arm Pos Offset", kFieldTypeVector, FieldLocation(sMPlayerLimbOffsets,armPosOff) },
   { "Arm Ang Offset", kFieldTypeVector, FieldLocation(sMPlayerLimbOffsets,armAngDegrees) },
};

static sStructDesc playerLimbOffsetsStruct = StructDescBuild(sMPlayerLimbOffsets,kStructFlagNone,limbOffsetsFields);


////////////////////////////////////////////////
//
//    GAIT DESC PROPERTY CREATION
//

class cMotGaitDescOps : public cClassDataOps<sMGaitSkillData>
{
}; 

class cMotGaitDescStore : public cSparseHashPropertyStore<cMotGaitDescOps>
{

}; 

class cMotGaitDescProperty : public cSpecificProperty<IMotGaitDescProperty,&IID_IMotGaitDescProperty,sMGaitSkillData*,cMotGaitDescStore>
{
   typedef cSpecificProperty<IMotGaitDescProperty,&IID_IMotGaitDescProperty,sMGaitSkillData*,cMotGaitDescStore> cParent; 

public: 
   cMotGaitDescProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
      
   }

   STANDARD_DESCRIBE_TYPE(sMGaitSkillData); 

}; 

static sPropertyDesc motGaitPropDesc =
{
   PROP_MOT_GAIT_DESC_NAME,
   NULL,
   NULL, // constraints
   0,0, // version
   { "Motions", "Gait Desc" },
};

// structure descriptor fun
static sFieldDesc gaitDescFields[] =
{
   { "LeftMotOff", kFieldTypeInt, FieldLocation(sMGaitSkillData,leftStride) },
   { "RightMotOff", kFieldTypeInt, FieldLocation(sMGaitSkillData,rightStride) },
   { "MaxAscend", kFieldTypeFloat, FieldLocation(sMGaitSkillData,maxAscend) },
   { "MaxDescend", kFieldTypeFloat, FieldLocation(sMGaitSkillData,maxDescend) },
   { "TimeWarp", kFieldTypeFloat, FieldLocation(sMGaitSkillData,timeWarp) },
   { "Stretch", kFieldTypeFloat, FieldLocation(sMGaitSkillData,stretch) },
   { "MaxAngVel", kFieldTypeInt, FieldLocation(sMGaitSkillData,maxAngVel) },
   { "TurnTolerance", kFieldTypeInt, FieldLocation(sMGaitSkillData,turnTolerance) },
   { "MaxVel", kFieldTypeFloat, FieldLocation(sMGaitSkillData,maxVel) },
   { "MinVel", kFieldTypeFloat, FieldLocation(sMGaitSkillData,minVel) },
   { "NumTurns", kFieldTypeInt, FieldLocation(sMGaitSkillData,numTurns) },
   { "Turn 1 Angle", kFieldTypeInt, FieldLocation(sMGaitSkillData,turns[0].angle)},
   { "Turn 1 MotOff", kFieldTypeInt, FieldLocation(sMGaitSkillData,turns[0].mot)},
   { "Turn 2 Angle", kFieldTypeInt, FieldLocation(sMGaitSkillData,turns[1].angle)},
   { "Turn 2 MotOff", kFieldTypeInt, FieldLocation(sMGaitSkillData,turns[1].mot)},
   { "Turn 3 Angle", kFieldTypeInt, FieldLocation(sMGaitSkillData,turns[2].angle)},
   { "Turn 3 MotOff", kFieldTypeInt, FieldLocation(sMGaitSkillData,turns[2].mot)},
   { "Stride Duration Noise",kFieldTypeFloat,FieldLocation(sMGaitSkillData,noise)},
};

static sStructDesc gaitDescStruct = StructDescBuild(sMGaitSkillData,kStructFlagNone,gaitDescFields);


////////////////////////////////////////////////
//
//    INITIALIZATION
//

EXTERN void MotorControllerPropInit(const char **motControllerNames,int nMotControllers);

void MotorControllerPropInit(const char **motControllerNames,int nMotControllers)
{
   AutoAppIPtr_(StructDescTools,pTools);

   // motor controller property
   AssertMsg(motControlSDesc.fields,"Motor Controller SDesc not set up properly");
   AssertMsg(nMotControllers&&motControllerNames,"Motor Controller names not adequate");
   motControlSDesc.fields->max=nMotControllers;
   motControlSDesc.fields->datasize=nMotControllers;
   motControlSDesc.fields->data=(void *)motControllerNames;
}

void CoreMotionPropsInit()
{
   AutoAppIPtr_(StructDescTools,pTools);

   // actor property
   pTools->Register(&actorTagListStruct);
   actorTagListProp=new cMotActorTagListProperty(&actorTagListPropDesc);

   pTools->Register(&motControlSDesc);
   motorControllerProp = CreateIntegralProperty(&motControlPropDesc,&motControlTDesc,MOTCONTROLPROP_IMPL);

   timeWarpProp = CreateFloatProperty(&timeWarpPropDesc,TIMEWARP_PROP_IMPL);

   pTools->Register(&motSwordActionSDesc);
   motSwordActionProp = CreateIntegralProperty(&motSwordActionPropDesc,&motSwordActionTDesc,MOTSWORDACTIONPROP_IMPL);

   // phys limits property
   pTools->Register(&physLimitsStruct);
   motPhysLimitsProp=new cMotPhysLimitsProperty(&motPhysLimitsPropDesc);

   // gait desc property
   pTools->Register(&gaitDescStruct);
   motGaitDescProp=new cMotGaitDescProperty(&motGaitPropDesc);

   // player pos offsets property
   pTools->Register(&playerLimbOffsetsStruct);
   motPlayerLimbOffsetsProp=new cMotPlayerLimbOffsetsProperty(&motPlayerLimbOffsetsPropDesc);
}

void CoreMotionPropsShutdown()
{
   SafeRelease(actorTagListProp);
   SafeRelease(motorControllerProp);
   SafeRelease(timeWarpProp);
   SafeRelease(motSwordActionProp);
   SafeRelease(motPhysLimitsProp);
   SafeRelease(motGaitDescProp);
   SafeRelease(motPlayerLimbOffsetsProp);
}

/////////////////////////////////////////////////
//
//    Access functions
//

BOOL ObjGetMotControllerID(ObjID obj, int *pID)
{
   Assert_(motorControllerProp);
   return motorControllerProp->Get(obj,pID);
}

// this should not be put on motion schema archetypes. rather, it belongs
// on creatures who should have all their motion playback speeds modified.  KJ 8/98
BOOL ObjGetTimeWarp(ObjID obj, float *pWarp)
{
   Assert_(timeWarpProp);
   return timeWarpProp->Get(obj,pWarp);
}

BOOL ObjGetSwordActionType(ObjID obj, int *pType)
{
   Assert_(motSwordActionProp);
   return motSwordActionProp->Get(obj,pType);
}

BOOL ObjGetGaitData(ObjID obj, sMGaitSkillData **pData)
{
   Assert_(motGaitDescProp);
   return motGaitDescProp->Get(obj,pData);
}

// @TODO: move this to AI property category? KJ 8/98
BOOL ObjGetActorTagList(ObjID obj, sMotActorTagList **pData)
{
   Assert_(actorTagListProp);
   return actorTagListProp->Get(obj, pData);
}

BOOL ObjGetPlayerPosOffsets(ObjID obj, mxs_vector *pPosOff, mxs_angvec *pAngOff)
{
   BOOL success;
   sMPlayerLimbOffsets *pLimbOff;
   int d;

   Assert_(motPlayerLimbOffsetsProp);
   if(TRUE==(success=motPlayerLimbOffsetsProp->Get(obj,&pLimbOff)))
   {
      mx_copy_vec(pPosOff,&pLimbOff->armPosOff);
      d=pLimbOff->armAngDegrees.x;
      pAngOff->tx=degrees_to_fixang(d);
      d=pLimbOff->armAngDegrees.y;
      pAngOff->ty=degrees_to_fixang(d);
      d=pLimbOff->armAngDegrees.z;
      pAngOff->tz=degrees_to_fixang(d);
   }
   return success;
}
