/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/tweqprop.cpp,v 1.35 1999/12/10 15:40:46 BFarquha Exp $
// tweaq property

#include <propert_.h>
#include <dataops_.h>
#include <propbase.h>
#include <sdesc.h>
#include <sdesbase.h>

#include <tweqbase.h>
#include <tweqprop.h>
#include <tweqrep.h>
#include <tweqexp.h>
#include <twqprop_.h>

#include <command.h>

// Must be last header
#include <dbmem.h>

//////////////////////
// global strings, much match the enums

// has to match the TweqType enum ordering
char *tweq_type_names[] =
 { "TweqScale", "TweqRotate", "TweqJoints", "TweqModels", "TweqDelete",
   "TweqEmitter", "TweqFlicker", "TweqLock", "TweqAll", "TweqNull" };

static char *tweq_do_ops[] =
 { "DoDefault", "DoActivate", "DoHalt", "DoReset", "DoContinue", "DoForward", "DoReverse" };

//////////////////////
// bitfield names

static char *animc_bit_names[] = { "NoLimit", "Sim", "Wrap", "OneBounce", "SimSmallRad", "SimLargeRad", "OffScreen" };
static char *miscc_bit_names[] = { "Anchor", "Scripts", "Random", "Grav", "ZeroVel", "TellAi", "PushOut", "NegativeLogic", "Relative Velocity", "NoPhysics", "AnchorVhot", "HostOnly" };
static char *curve_bit_names[] = { "JitterLow", "JitterHi", "Mul" };

static char *halt_act_names[]  = { "Destroy Obj", "Remove Prop", "Stop Tweq", "Continue", "Slay Obj" };
static char *anims_bit_names[] = { "On", "Reverse", "ReSynch", "GoEdge", "LapOne" };

static char *miscs_bit_names[] = { "Null" };

//////////////////////
// horrible macros for common structures for flags and stuff

#define SDESC_TWEQ_BASE_CONFIG(pre_str,struct_n,cfg) \
 { pre_str"Halt",     kFieldTypeEnum,  FieldLocation(struct_n,cfg.halt_act),  FullFieldNames(halt_act_names) }, \
 { pre_str"AnimC",    kFieldTypeBits,  FieldLocation(struct_n,cfg.flg_anim),  FullFieldNames(animc_bit_names) }, \
 { pre_str"MiscC",    kFieldTypeBits,  FieldLocation(struct_n,cfg.flg_misc),  FullFieldNames(miscc_bit_names) }, \
 { pre_str"CurveC",   kFieldTypeBits,  FieldLocation(struct_n,cfg.flg_curve), FullFieldNames(curve_bit_names) }
#define SDESC_TWEQ_BASE_CONFIG_RATE(pre_str,struct_n,field_label) \
 { pre_str"Rate",     kFieldTypeShort, FieldLocation(struct_n,cfg.rate) }
#define SDESC_TWEQ_BASE_STATE(pre_str,struct_n,cfg) \
 { pre_str"AnimS",    kFieldTypeBits,  FieldLocation(struct_n,cfg.flg_anim),  FullFieldNames(anims_bit_names) }, \
 { pre_str"MiscS",    kFieldTypeBits,  FieldLocation(struct_n,cfg.flg_misc),  FullFieldNames(miscs_bit_names) }
#define SDESC_TWEQ_TIME_STATE(pre_str,struct_n,cfg) \
 { pre_str"Cur Time", kFieldTypeShort, FieldLocation(struct_n,cfg.cur_time) }, \
 { pre_str"Frame #",  kFieldTypeShort, FieldLocation(struct_n,cfg.cur_frame) }

// for joints
#define SDESC_TWEQ_JOINT_BASE_CONFIG(pre_str,struct_n,cfg) \
 { pre_str"AnimC",    kFieldTypeBits,  FieldLocation(struct_n,cfg.flg_anim),  FullFieldNames(animc_bit_names) }, \
 { pre_str"CurveC",   kFieldTypeBits,  FieldLocation(struct_n,cfg.flg_curve), FullFieldNames(curve_bit_names) }
#define SDESC_TWEQ_JOINT_BASE_STATE(pre_str,struct_n,cfg) \
 { pre_str"AnimS",    kFieldTypeBits,  FieldLocation(struct_n,cfg.flg_anim),  FullFieldNames(anims_bit_names) }

//////////////////////

typedef cGenericProperty<ITweqSimpleProperty,&IID_ITweqSimpleProperty,sTweqSimpleConfig*> cTweqSimplePropertyBase;

typedef cAutoIPtr<IPropertyStore> cPS;
static inline cPS GenStore(ePropertyImpl impl)
{
   return CreateGenericPropertyStore(impl);
}


class cTweqSimpleProperty : public cTweqSimplePropertyBase
{
   cClassDataOps<sTweqSimpleConfig> mOps;


public:
   cTweqSimpleProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cTweqSimplePropertyBase(desc,store)
   {
      SetOps(&mOps);
   }

   cTweqSimpleProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cTweqSimplePropertyBase(desc,GenStore(impl))
   {
      SetOps(&mOps);
   }

   STANDARD_DESCRIBE_TYPE(sTweqSimpleConfig);
};

typedef cGenericProperty<ITweqVectorProperty,&IID_ITweqVectorProperty,sTweqVectorConfig*> cTweqVectorPropertyBase;

class cTweqVectorProperty : public cTweqVectorPropertyBase
{
   cClassDataOps<sTweqVectorConfig> mOps;

public:
   cTweqVectorProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cTweqVectorPropertyBase(desc,store)
   {
      SetOps(&mOps);
   }

   cTweqVectorProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cTweqVectorPropertyBase(desc,GenStore(impl))
   {
      SetOps(&mOps);
   }

   STANDARD_DESCRIBE_TYPE(sTweqVectorConfig);
};

typedef cGenericProperty<ITweqJointsProperty,&IID_ITweqJointsProperty,sTweqJointsConfig*> cTweqJointsPropertyBase;

class cTweqJointsProperty : public cTweqJointsPropertyBase
{
   cClassDataOps<sTweqJointsConfig> mOps;

public:
   cTweqJointsProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cTweqJointsPropertyBase(desc,store)
   {
      SetOps(&mOps);
   }

   cTweqJointsProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cTweqJointsPropertyBase(desc,GenStore(impl))
   {
      SetOps(&mOps);
   }

   STANDARD_DESCRIBE_TYPE(sTweqJointsConfig);
};

typedef cGenericProperty<ITweqModelsProperty,&IID_ITweqModelsProperty,sTweqModelsConfig*> cTweqModelsPropertyBase;

class cTweqModelsProperty : public cTweqModelsPropertyBase
{
   cClassDataOps<sTweqModelsConfig> mOps;

public:
   cTweqModelsProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cTweqModelsPropertyBase(desc,store)
   {
      SetOps(&mOps);
   }

   cTweqModelsProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cTweqModelsPropertyBase(desc,GenStore(impl))
   {
      SetOps(&mOps);
   }

   STANDARD_DESCRIBE_TYPE(sTweqModelsConfig);
};

typedef cGenericProperty<ITweqEmitterProperty,&IID_ITweqEmitterProperty,sTweqEmitterConfig*> cTweqEmitterPropertyBase;

class cTweqEmitterProperty : public cTweqEmitterPropertyBase
{
   cClassDataOps<sTweqEmitterConfig> mOps;

public:
   cTweqEmitterProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cTweqEmitterPropertyBase(desc,store)
   {
      SetOps(&mOps);
   }

   cTweqEmitterProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cTweqEmitterPropertyBase(desc,GenStore(impl))
   {
      SetOps(&mOps);
   }

   STANDARD_DESCRIBE_TYPE(sTweqEmitterConfig);
};

typedef cGenericProperty<ITweqLockProperty,&IID_ITweqLockProperty,sTweqLockConfig*> cTweqLockPropertyBase;

class cTweqLockProperty : public cTweqLockPropertyBase
{
   cClassDataOps<sTweqLockConfig> mOps;

public:
   cTweqLockProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cTweqLockPropertyBase(desc,store)
   {
      SetOps(&mOps);
   }

   cTweqLockProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cTweqLockPropertyBase(desc,GenStore(impl))
   {
      SetOps(&mOps);
   }

   STANDARD_DESCRIBE_TYPE(sTweqLockConfig);
};

// delete a flicker both use base config

typedef cGenericProperty<ITweqSimpleStateProperty,&IID_ITweqSimpleStateProperty,sTweqSimpleState*> cTweqSimpleStatePropertyBase;

class cTweqSimpleStateProperty : public cTweqSimpleStatePropertyBase
{
   cClassDataOps<sTweqSimpleState> mOps;

public:
   cTweqSimpleStateProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cTweqSimpleStatePropertyBase(desc,store)
   {
      SetOps(&mOps);
   }

   cTweqSimpleStateProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cTweqSimpleStatePropertyBase(desc,GenStore(impl))
   {
      SetOps(&mOps);
   }

   STANDARD_DESCRIBE_TYPE(sTweqSimpleState);
};

typedef cGenericProperty<ITweqVectorStateProperty,&IID_ITweqVectorStateProperty,sTweqVectorState*> cTweqVectorStatePropertyBase;

class cTweqVectorStateProperty : public cTweqVectorStatePropertyBase
{
   cClassDataOps<sTweqVectorState> mOps;

public:
   cTweqVectorStateProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cTweqVectorStatePropertyBase(desc,store)
   {
      SetOps(&mOps);
   }

   cTweqVectorStateProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cTweqVectorStatePropertyBase(desc,GenStore(impl))
   {
      SetOps(&mOps);
   }

   STANDARD_DESCRIBE_TYPE(sTweqVectorState);
};

typedef cGenericProperty<ITweqJointsStateProperty,&IID_ITweqJointsStateProperty,sTweqJointsState*> cTweqJointsStatePropertyBase;

class cTweqJointsStateProperty : public cTweqJointsStatePropertyBase
{
   cClassDataOps<sTweqJointsState> mOps;

public:
   cTweqJointsStateProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cTweqJointsStatePropertyBase(desc,store)
   {
      SetOps(&mOps);
   }

   cTweqJointsStateProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cTweqJointsStatePropertyBase(desc,GenStore(impl))
   {
      SetOps(&mOps);
   }

   STANDARD_DESCRIBE_TYPE(sTweqJointsState);
};

typedef cGenericProperty<ITweqLockStateProperty,&IID_ITweqLockStateProperty,sTweqLockState*> cTweqLockStatePropertyBase;

class cTweqLockStateProperty : public cTweqLockStatePropertyBase
{
   cClassDataOps<sTweqLockState> mOps;

public:
   cTweqLockStateProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cTweqLockStatePropertyBase(desc,store)
   {
      SetOps(&mOps);
   }

   cTweqLockStateProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cTweqLockStatePropertyBase(desc,GenStore(impl))
   {
      SetOps(&mOps);
   }

   STANDARD_DESCRIBE_TYPE(sTweqLockState);
};

// model, delete, emit, and flicker states all use base state

////////////
// shared common vectors

// argh - cant use the #def, since we have no subfield!
static sFieldDesc tweq_simple_config_fields [] =
{
   SDESC_TWEQ_BASE_CONFIG("",sTweqSimpleConfig,cfg),
   SDESC_TWEQ_BASE_CONFIG_RATE("",sTweqSimpleConfig,cfg),
};

static sFieldDesc tweq_simple_state_fields [] =
{
   SDESC_TWEQ_BASE_STATE("",sTweqSimpleState,state),
   SDESC_TWEQ_TIME_STATE("",sTweqSimpleState,tm_info),
};

////////////
// vector

static sFieldDesc tweq_vector_config_fields [] =
{
   SDESC_TWEQ_BASE_CONFIG("",sTweqVectorConfig,cfg),
   { "Primary Axis", kFieldTypeInt, FieldLocation(sTweqVectorConfig,primary_axis) },
   { "x rate-low-high", kFieldTypeVector, FieldLocation(sTweqVectorConfig,limits[0]) },
   { "y rate-low-high", kFieldTypeVector, FieldLocation(sTweqVectorConfig,limits[1]) },
   { "z rate-low-high", kFieldTypeVector, FieldLocation(sTweqVectorConfig,limits[2]) },
};

static sFieldDesc tweq_vector_state_fields [] =
{
   SDESC_TWEQ_BASE_STATE("",sTweqVectorState,state),
   SDESC_TWEQ_JOINT_BASE_STATE("Axis 1",sTweqVectorState,axis_flgs[0]),
   SDESC_TWEQ_JOINT_BASE_STATE("Axis 2",sTweqVectorState,axis_flgs[1]),
   SDESC_TWEQ_JOINT_BASE_STATE("Axis 3",sTweqVectorState,axis_flgs[2]),
};

///////////
// joint

#define SDESC_JOINT_ONLY(postfix,struct_name,joint_field) \
 { "    rate-low-high"##postfix, kFieldTypeVector, FieldLocation(struct_name,joint_field.limits) }

#define SDESC_JOINT_DATA(postfix,axis_prefix,struct_name,joint_field) \
 SDESC_TWEQ_JOINT_BASE_CONFIG(axis_prefix,struct_name,joint_field.flags), \
 SDESC_JOINT_ONLY(postfix,struct_name,joint_field)

//
// We use a postfix of "" and not "1" for the first joint, so as to remain backward compatable:
// previously, they had no postfix at all, so when referenced by name, only the first one was found!
//
static sFieldDesc tweq_joints_config_fields [] =
{
   SDESC_TWEQ_BASE_CONFIG("",sTweqJointsConfig,cfg),
   { "Primary Joint", kFieldTypeInt, FieldLocation(sTweqJointsConfig,primary_joint) },
   SDESC_JOINT_DATA("","Joint1",sTweqJointsConfig,jnt_data[0]),
   SDESC_JOINT_DATA("2","Joint2",sTweqJointsConfig,jnt_data[1]),
   SDESC_JOINT_DATA("3","Joint3",sTweqJointsConfig,jnt_data[2]),
   SDESC_JOINT_DATA("4","Joint4",sTweqJointsConfig,jnt_data[3]),
   SDESC_JOINT_DATA("5","Joint5",sTweqJointsConfig,jnt_data[4]),
   SDESC_JOINT_DATA("6","Joint6",sTweqJointsConfig,jnt_data[5]),
};

static sFieldDesc tweq_joints_state_fields [] =
{
   SDESC_TWEQ_BASE_STATE("",sTweqJointsState,state),
   SDESC_TWEQ_JOINT_BASE_STATE("Joint1",sTweqJointsState,jnt_flgs[0]),
   SDESC_TWEQ_JOINT_BASE_STATE("Joint2",sTweqJointsState,jnt_flgs[1]),
   SDESC_TWEQ_JOINT_BASE_STATE("Joint3",sTweqJointsState,jnt_flgs[2]),
   SDESC_TWEQ_JOINT_BASE_STATE("Joint4",sTweqJointsState,jnt_flgs[3]),
   SDESC_TWEQ_JOINT_BASE_STATE("Joint5",sTweqJointsState,jnt_flgs[4]),
   SDESC_TWEQ_JOINT_BASE_STATE("Joint6",sTweqJointsState,jnt_flgs[5]),
};

///////////
// locks

static sFieldDesc tweq_lock_config_fields [] =
{
   SDESC_TWEQ_BASE_CONFIG("",sTweqLockConfig,cfg),
   { "Lock Joint", kFieldTypeInt, FieldLocation(sTweqLockConfig,lock_joint) },
   SDESC_JOINT_ONLY("Joint",sTweqLockConfig,jnt_data),
};

static sFieldDesc tweq_lock_state_fields [] =
{
   SDESC_TWEQ_BASE_STATE("",sTweqLockState,state),
   { "Target Angle", kFieldTypeFloat, FieldLocation(sTweqLockState,targ_ang) },
   { "Cur Stage",    kFieldTypeInt,   FieldLocation(sTweqLockState,stage) },
};

///////////
// models

static sFieldDesc tweq_models_config_fields [] =
{
   SDESC_TWEQ_BASE_CONFIG("",sTweqModelsConfig,cfg),
   SDESC_TWEQ_BASE_CONFIG_RATE("",sTweqModelsConfig,cfg),
   { "Model 0", kFieldTypeString, FieldLocation(sTweqModelsConfig,names[0]) },
   { "Model 1", kFieldTypeString, FieldLocation(sTweqModelsConfig,names[1]) },
   { "Model 2", kFieldTypeString, FieldLocation(sTweqModelsConfig,names[2]) },
   { "Model 3", kFieldTypeString, FieldLocation(sTweqModelsConfig,names[3]) },
   { "Model 4", kFieldTypeString, FieldLocation(sTweqModelsConfig,names[4]) },
   { "Model 5", kFieldTypeString, FieldLocation(sTweqModelsConfig,names[5]) },
};

// model state is just default state

///////////
// emitter

static sFieldDesc tweq_emitter_config_fields [] =
{
   SDESC_TWEQ_BASE_CONFIG("",sTweqEmitterConfig,cfg),
   SDESC_TWEQ_BASE_CONFIG_RATE("",sTweqEmitterConfig,cfg),
   { "Max frames",      kFieldTypeInt,    FieldLocation(sTweqEmitterConfig,max_frames) },
   { "Emit what",       kFieldTypeString, FieldLocation(sTweqEmitterConfig,emittee) },
   { "Velocity",        kFieldTypeVector, FieldLocation(sTweqEmitterConfig,vel) },
   { "Angle Random",    kFieldTypeVector, FieldLocation(sTweqEmitterConfig,rand), kFieldFlagHex },
};

// emitter state is just default state

///////////
// delete and flicker

// uses both default config and state

//////////
// connection to A/R

static sFieldDesc tweq_control_fields [] =
{
   { "Type",     kFieldTypeEnum, FieldLocation(sTweqControl,type),      FullFieldNames(tweq_type_names) },
   { "Action",   kFieldTypeEnum, FieldLocation(sTweqControl,action),    FullFieldNames(tweq_do_ops) },
   { "XtraBits", kFieldTypeBits, FieldLocation(sTweqControl,xtra_bits), FullFieldNames(anims_bit_names) },
};

///////////////////////////////
// now actually create the sdesc setup config

static sStructDesc tweq_simple_config_struct  = StructDescBuild(sTweqSimpleConfig,  kStructFlagNone, tweq_simple_config_fields);
static sStructDesc tweq_vector_config_struct  = StructDescBuild(sTweqVectorConfig,  kStructFlagNone, tweq_vector_config_fields);
static sStructDesc tweq_joints_config_struct  = StructDescBuild(sTweqJointsConfig,  kStructFlagNone, tweq_joints_config_fields);
static sStructDesc tweq_models_config_struct  = StructDescBuild(sTweqModelsConfig,  kStructFlagNone, tweq_models_config_fields);
static sStructDesc tweq_emitter_config_struct = StructDescBuild(sTweqEmitterConfig, kStructFlagNone, tweq_emitter_config_fields);
static sStructDesc tweq_lock_config_struct    = StructDescBuild(sTweqLockConfig,    kStructFlagNone, tweq_lock_config_fields);

static sStructDesc tweq_simple_state_struct   = StructDescBuild(sTweqSimpleState,   kStructFlagNone, tweq_simple_state_fields);
static sStructDesc tweq_vector_state_struct   = StructDescBuild(sTweqVectorState,   kStructFlagNone, tweq_vector_state_fields);
static sStructDesc tweq_joints_state_struct   = StructDescBuild(sTweqJointsState,   kStructFlagNone, tweq_joints_state_fields);
static sStructDesc tweq_lock_state_struct     = StructDescBuild(sTweqLockState,     kStructFlagNone, tweq_lock_state_fields);

static sStructDesc tweq_control_struct        = StructDescBuild(sTweqControl,       kStructFlagNone, tweq_control_fields);

// array of the sdescs
static sStructDesc *tweq_sdescs[] =
 { &tweq_simple_config_struct, &tweq_vector_config_struct, &tweq_joints_config_struct, &tweq_lock_config_struct,
   &tweq_models_config_struct, &tweq_emitter_config_struct, &tweq_simple_state_struct, &tweq_lock_state_struct,
   &tweq_vector_state_struct, &tweq_joints_state_struct, &tweq_control_struct };

#define BUILD_PDESC_FOR_CONFIG_TWEQ(codeName,defineName,publicName) \
  static sPropertyConstraint tweq_##codeName##_constraint[] = \
       {{kPropertyAutoCreate, PROP_TWEQ##defineName##STATE_NAME}, \
        {kPropertyNullConstraint} }; \
  static sPropertyDesc tweq_##codeName##_propdesc = { PROP_TWEQ##defineName##CONFIG_NAME, \
       0, tweq_##codeName##_constraint, 0, 0, { "Tweq", publicName }, kPropertyChangeLocally }

#ifdef NO_REVERSE_CONSTRAINT
#define BUILD_PDESC_FOR_STATE_TWEQ(codeName,defineName,publicName) \
  static sPropertyDesc tweq_##codeName##_propdesc = \
       { PROP_TWEQ##defineName##_NAME, kPropertyInstantiate|kPropertyNoInherit, \
         NULL, 0, 0, { "Tweq", publicName }, kPropertyChangeLocally }
#endif

#define BUILD_PDESC_FOR_STATE_TWEQ(codeName,defineName,publicName) \
  static sPropertyConstraint tweq_##codeName##_constraint[] = \
       {{kPropertyRequires, PROP_TWEQ##defineName##CONFIG_NAME}, \
        {kPropertyNullConstraint} }; \
  static sPropertyDesc tweq_##codeName##_propdesc = \
       { PROP_TWEQ##defineName##STATE_NAME, kPropertyInstantiate|kPropertyNoInherit, \
         tweq_##codeName##_constraint, 0, 0, { "Tweq", publicName"State" }, kPropertyChangeLocally }

// config sdesc tweq elements
BUILD_PDESC_FOR_CONFIG_TWEQ(scale,       SCALE,    "Scale");
BUILD_PDESC_FOR_CONFIG_TWEQ(rotate,      ROTATE,   "Rotate");
BUILD_PDESC_FOR_CONFIG_TWEQ(joints,      JOINTS,   "Joints");
BUILD_PDESC_FOR_CONFIG_TWEQ(models,      MODELS,   "Models");
BUILD_PDESC_FOR_CONFIG_TWEQ(delete,      DELETE,   "Delete");
BUILD_PDESC_FOR_CONFIG_TWEQ(flicker,     FLICKER,  "Flicker");
BUILD_PDESC_FOR_CONFIG_TWEQ(emitter,     EMITTER,  "Emit");
BUILD_PDESC_FOR_CONFIG_TWEQ(emitter2,    EMITTER2, "Emit2");
BUILD_PDESC_FOR_CONFIG_TWEQ(emitter3,    EMITTER3, "Emit3");
BUILD_PDESC_FOR_CONFIG_TWEQ(emitter4,    EMITTER4, "Emit4");
BUILD_PDESC_FOR_CONFIG_TWEQ(emitter5,    EMITTER5, "Emit5");
BUILD_PDESC_FOR_CONFIG_TWEQ(lock,        LOCK,     "Lock");

static sPropertyDesc *tweq_extra_emitter_propdesc[NUM_EXTRA_EMITTERS] =
{
   &tweq_emitter2_propdesc,
   &tweq_emitter3_propdesc,
   &tweq_emitter4_propdesc,
   &tweq_emitter5_propdesc,
};

// state sdesc tweq elements
BUILD_PDESC_FOR_STATE_TWEQ(scalestate,    SCALE,         "Scale");
BUILD_PDESC_FOR_STATE_TWEQ(rotatestate,   ROTATE,        "Rotate");
BUILD_PDESC_FOR_STATE_TWEQ(jointsstate,   JOINTS,        "Joints");
BUILD_PDESC_FOR_STATE_TWEQ(modelsstate,   MODELS,        "Models");
BUILD_PDESC_FOR_STATE_TWEQ(deletestate,   DELETE,        "Delete");
BUILD_PDESC_FOR_STATE_TWEQ(flickerstate,  FLICKER,       "Flicker");
BUILD_PDESC_FOR_STATE_TWEQ(emitterstate,  EMITTER,       "Emitter");
BUILD_PDESC_FOR_STATE_TWEQ(emitter2state, EMITTER2,      "Emitter2");
BUILD_PDESC_FOR_STATE_TWEQ(emitter3state, EMITTER3,      "Emitter3");
BUILD_PDESC_FOR_STATE_TWEQ(emitter4state, EMITTER4,      "Emitter4");
BUILD_PDESC_FOR_STATE_TWEQ(emitter5state, EMITTER5,      "Emitter5");
BUILD_PDESC_FOR_STATE_TWEQ(lockstate,     LOCK,          "Lock");

static sPropertyDesc *tweq_extra_emitterstate_propdesc[NUM_EXTRA_EMITTERS] =
{
   &tweq_emitter2state_propdesc,
   &tweq_emitter3state_propdesc,
   &tweq_emitter4state_propdesc,
   &tweq_emitter5state_propdesc,
};

// config tweq properties
ITweqVectorProperty      *tweq_scale_prop;
ITweqVectorProperty      *tweq_rotate_prop;
ITweqJointsProperty      *tweq_joints_prop;
ITweqModelsProperty      *tweq_models_prop;
ITweqSimpleProperty      *tweq_delete_prop;
ITweqSimpleProperty      *tweq_flicker_prop;
ITweqEmitterProperty     *tweq_emitter_prop;
ITweqLockProperty        *tweq_lock_prop;
ITweqEmitterProperty     *tweq_extra_emitter_prop[NUM_EXTRA_EMITTERS];

// state tweq properties
ITweqVectorStateProperty *tweq_scalestate_prop;
ITweqVectorStateProperty *tweq_rotatestate_prop;
ITweqJointsStateProperty *tweq_jointsstate_prop;
ITweqSimpleStateProperty *tweq_modelsstate_prop;
ITweqSimpleStateProperty *tweq_deletestate_prop;
ITweqSimpleStateProperty *tweq_flickerstate_prop;
ITweqSimpleStateProperty *tweq_emitterstate_prop;
ITweqLockStateProperty   *tweq_lockstate_prop;
ITweqSimpleStateProperty *tweq_extra_emitterstate_prop[NUM_EXTRA_EMITTERS];

#define NUM_TWEQ_PROPS (8) // +NUM_EXTRA_EMITTERS

IProperty *tweq_prop_states[NUM_TWEQ_PROPS];
IProperty *tweq_prop_config[NUM_TWEQ_PROPS];

////////////////
// helpers/misc

#ifdef HAD_COMMANDS
static void tweq_rate_fix(int)
{
   ;
}

Command tweq_keys[]={{"tweq_fix_rates",FUNC_INT,tweq_rate_fix}};

#define _DO_SETUP_COMMANDS() COMMANDS(tweq_keys,HK_BRUSH_EDIT);
#else
#define _DO_SETUP_COMMANDS()
#endif

////////////////
// startup/shutdown

// let us attempt to initialize ourselves
BOOL TweqPropsInit(void)
{
   int i;

   _DO_SETUP_COMMANDS();

   for (i=0; i<sizeof(tweq_sdescs)/sizeof(tweq_sdescs[0]); i++)
      StructDescRegister(tweq_sdescs[i]);

   tweq_prop_config[kTweqTypeScale]   = tweq_scale_prop   =
      new cTweqVectorProperty      (&tweq_scale_propdesc,   kPropertyImplVerySparse);
   tweq_prop_config[kTweqTypeRotate]  = tweq_rotate_prop  =
      new cTweqVectorProperty      (&tweq_rotate_propdesc,  kPropertyImplVerySparse);
   tweq_prop_config[kTweqTypeJoints]  = tweq_joints_prop  =
      new cTweqJointsProperty      (&tweq_joints_propdesc,  kPropertyImplVerySparse);
   tweq_prop_config[kTweqTypeModels]  = tweq_models_prop  =
      new cTweqModelsProperty      (&tweq_models_propdesc,  kPropertyImplVerySparse);
   tweq_prop_config[kTweqTypeDelete]  = tweq_delete_prop  =
      new cTweqSimpleProperty      (&tweq_delete_propdesc,  kPropertyImplVerySparse);
   tweq_prop_config[kTweqTypeFlicker] = tweq_flicker_prop =
      new cTweqSimpleProperty      (&tweq_flicker_propdesc, kPropertyImplVerySparse);
   tweq_prop_config[kTweqTypeEmitter] = tweq_emitter_prop =
      new cTweqEmitterProperty     (&tweq_emitter_propdesc, kPropertyImplVerySparse);
   tweq_prop_config[kTweqTypeLock] = tweq_lock_prop =
      new cTweqLockProperty        (&tweq_lock_propdesc, kPropertyImplVerySparse);

   for (i=0; i < NUM_EXTRA_EMITTERS; ++i)
      tweq_extra_emitter_prop[i] =
         new cTweqEmitterProperty    (tweq_extra_emitter_propdesc[i], kPropertyImplVerySparse);

   // need a hash table so we can iterate and have decent lookup
   tweq_prop_states[kTweqTypeScale]   = tweq_scalestate_prop   =
      new cTweqVectorStateProperty (&tweq_scalestate_propdesc,   kPropertyImplLlist);
   tweq_prop_states[kTweqTypeRotate]  = tweq_rotatestate_prop  =
      new cTweqVectorStateProperty (&tweq_rotatestate_propdesc,  kPropertyImplLlist);
   tweq_prop_states[kTweqTypeJoints]  = tweq_jointsstate_prop  =
      new cTweqJointsStateProperty (&tweq_jointsstate_propdesc,  kPropertyImplLlist);
   tweq_prop_states[kTweqTypeModels]  = tweq_modelsstate_prop  =
      new cTweqSimpleStateProperty (&tweq_modelsstate_propdesc,  kPropertyImplLlist);
   tweq_prop_states[kTweqTypeDelete]  = tweq_deletestate_prop  =
      new cTweqSimpleStateProperty (&tweq_deletestate_propdesc,  kPropertyImplLlist);
   tweq_prop_states[kTweqTypeEmitter] = tweq_emitterstate_prop =
      new cTweqSimpleStateProperty (&tweq_emitterstate_propdesc, kPropertyImplLlist);
   tweq_prop_states[kTweqTypeFlicker] = tweq_flickerstate_prop =
      new cTweqSimpleStateProperty (&tweq_flickerstate_propdesc, kPropertyImplLlist);
   tweq_prop_states[kTweqTypeLock] = tweq_lockstate_prop =
      new cTweqLockStateProperty   (&tweq_lockstate_propdesc,    kPropertyImplLlist);

   for (i=0; i < NUM_EXTRA_EMITTERS; ++i)
      tweq_extra_emitterstate_prop[i] =
         new cTweqSimpleStateProperty (tweq_extra_emitterstate_propdesc[i], kPropertyImplLlist);

   create_tweq_control_reaction();

   TweqReportInit();

   return TRUE;
}

void TweqPropsTerm(void)
{
   int i;

   TweqReportTerm();

   SafeRelease(tweq_scale_prop);
   SafeRelease(tweq_rotate_prop);
   SafeRelease(tweq_joints_prop);
   SafeRelease(tweq_models_prop);
   SafeRelease(tweq_delete_prop);
   SafeRelease(tweq_emitter_prop);
   SafeRelease(tweq_flicker_prop);
   SafeRelease(tweq_lock_prop);

   SafeRelease(tweq_scalestate_prop);
   SafeRelease(tweq_rotatestate_prop);
   SafeRelease(tweq_jointsstate_prop);
   SafeRelease(tweq_modelsstate_prop);
   SafeRelease(tweq_deletestate_prop);
   SafeRelease(tweq_emitterstate_prop);
   SafeRelease(tweq_flickerstate_prop);
   SafeRelease(tweq_lockstate_prop);

   for (i=0; i < NUM_EXTRA_EMITTERS; ++i) {
      SafeRelease(tweq_extra_emitter_prop[i]);
      SafeRelease(tweq_extra_emitterstate_prop[i]);
   }
}
