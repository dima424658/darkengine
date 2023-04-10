/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/fasprop.cpp,v 1.5 1999/03/23 11:39:13 Justin Exp $

#include <property.h>
#include <propert_.h>
#include <propface.h>
#include <propfac_.h>
#include <dataops_.h>
#include <propsprs.h>
#include <propbase.h>
#include <prophash.h>
#include <propbase.h>
#include <objsys.h>
#include <objtype.h>

#if 0
#include <linkbase.h>
#include <lnkquery.h>
#include <relation.h>

#include <traitman.h>
#include <trait.h>
#include <traitbas.h>
#include <iobjsys.h>
#include <objnotif.h>
#endif

#include <fas.h>
#include <fastype.h>
#include <fasprop.h>

#include <sdesbase.h>
#include <sdestool.h>

// must be last header
#include <dbmem.h>


/////////////////////////////
//
//  bitmap animation properties
//


struct sFrameStateProp : public FrameAnimationState
{
   sFrameStateProp(const sFrameStateProp& state) 
   {
      FrameAnimationStateStart((FrameAnimationState *) this);
   }

   sFrameStateProp() 
   {
      FrameAnimationStateStart((FrameAnimationState *) this);
   }

}; 

class cFAStateOps    : public cClassDataOps<sFrameStateProp>
{
public:
   // override default flags
   cFAStateOps() :cClassDataOps<sFrameStateProp>(kNoFlags){};

}; 

class cFAStateStore  : public cSparseHashPropertyStore<cFAStateOps> { }; 
class cFAConfigOps   : public cClassDataOps<FrameAnimationConfig> { }; 
class cFAConfigStore : public cSparseHashPropertyStore<cFAConfigOps> { }; 

typedef cSpecificProperty<IFrameAnimationStateProperty,
                       &IID_IFrameAnimationStateProperty,
                       FrameAnimationState*,cFAStateStore>
         cFAStateParent; 

typedef cSpecificProperty<IFrameAnimationConfigProperty,
                       &IID_IFrameAnimationConfigProperty,
                       FrameAnimationConfig*,cFAConfigStore>
         cFAConfigParent; 

class cFrameAnimationStateProperty: public cFAStateParent
{

public: 
   cFrameAnimationStateProperty(const sPropertyDesc* desc)
      : cFAStateParent(desc)
   { }
   STANDARD_DESCRIBE_TYPE(FrameAnimationState);
}; 

class cFrameAnimationConfigProperty: public cFAConfigParent
{

public: 
   cFrameAnimationConfigProperty(const sPropertyDesc* desc)
      : cFAConfigParent(desc)
   { }

   STANDARD_DESCRIBE_TYPE(FrameAnimationConfig);
};

#if 0
static sPropertyConstraint state_constraint[] =
{
   { kPropertyRequires, PROP_FAS_CONFIG_NAME },
   { kPropertyNullConstraint }
};
#endif

static sPropertyConstraint config_constraint[] =
{
   { kPropertyAutoCreate, PROP_FAS_STATE_NAME },
   { kPropertyNullConstraint }
};

int x;

static sPropertyDesc fas_state_prop_desc=
{
   PROP_FAS_STATE_NAME,
   kPropertyInstantiate|kPropertyNoInherit, //| kPropertyNoEdit,
   NULL, // state_constraint,
   0,0, // version
   { "SFX", "FrameAnimationState" },
   kPropertyChangeLocally
};

int y;

static sPropertyDesc fas_config_prop_desc =
{
   PROP_FAS_CONFIG_NAME,
   0,
   config_constraint,
   0,0, // version
   { "SFX", "FrameAnimationConfig" }
};

#define FL(x)    FieldLocation(FrameAnimationConfig, x)

// structure descriptor fun
static sFieldDesc config_fields [] =
{
   { "frames per second", kFieldTypeFloat, FL(fps) },
   { "one-shot", kFieldTypeBool, FL(clamp) },
   { "bounce", kFieldTypeBool, FL(bounce) },
   { "frame-limit", kFieldTypeBool, FL(frame_limit) },
   { "kill when finished", kFieldTypeBool, FL(client_data) },
};
#undef FL

#define FL(x)    FieldLocation(FrameAnimationState, x)

// structure descriptor fun
static sFieldDesc state_fields [] =
{
   { "current frame", kFieldTypeInt, FL(cur_frame) },
};
#undef FL

static sStructDesc fas_config_desc = StructDescBuild(FrameAnimationConfig,
                              kStructFlagNone,config_fields);
static sStructDesc fas_state_desc  = StructDescBuild(FrameAnimationState,
                              kStructFlagNone,state_fields);

static IFrameAnimationStateProperty  *fas_state_prop = NULL;
static IFrameAnimationConfigProperty *fas_config_prop = NULL;

BOOL FASPropInit(void)
{
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&fas_state_desc);
   pTools->Register(&fas_config_desc);

   fas_state_prop  = new cFrameAnimationStateProperty(&fas_state_prop_desc);
   fas_config_prop = new cFrameAnimationConfigProperty(&fas_config_prop_desc);

   return TRUE;
}

void FASPropShutdown(void)
{
   SafeRelease(fas_state_prop);
   SafeRelease(fas_config_prop);
}

FrameAnimationState *ObjGetFrameAnimationState(ObjID obj)
{
   FrameAnimationState *res = NULL;
   fas_state_prop->Get(obj, &res);
   return res;
}

FrameAnimationConfig *ObjGetFrameAnimationConfig(ObjID obj)
{
   FrameAnimationConfig *res = NULL;
   fas_config_prop->Get(obj, &res);
   return res;
}

void ObjSetFrameAnimationState(ObjID obj, FrameAnimationState *fas)
{
   fas_state_prop->Set(obj, fas);
}

void ObjSetFrameAnimationConfig(ObjID obj, FrameAnimationConfig *cfg)
{
   fas_config_prop->Set(obj, cfg);
}

