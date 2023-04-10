/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <comtools.h>
#include <appagg.h>

#include <reaction.h>
#include <reactype.h>
#include <reacbase.h>

#include <dpcreact.h>
#include <propreac.h>

#include <objquery.h>

#include <label.h>
#include <mnamprop.h>
#include <objpos.h>
#include <objshape.h>
#include <matrixs.h>
#include <wr.h>

#include <sdesc.h>
#include <sdesbase.h>

// turning animated lights on and off
#include <animlgt.h>
#include <litprop.h>
#include <litprops.h>
#include <litnet.h>

#include <dmgmodel.h>

#include <frobctrl.h>

#include <dpchazpr.h>
#include <dpcplayr.h>
#include <dpcarmpr.h>
#include <dpcovrly.h>
#include <dpcovcst.h>
#include <dpcplcst.h>

#include <dpcstnrc.h>

// Demo hacks
#include <stimuli.h>
#include <stimul8r.h>
#include <stimsens.h>
#include <objtype.h>
#include <stimtype.h>
#include <stimbase.h>
#include <label.h>
#include <physapi.h>
#include <aiapi.h>
#include <dmgbase.h>
#include <chevkind.h>
#include <aiapiiai.h>
#include <aistnapi.h>
#include <culpable.h>
#include <aiaware.h>

//------------------------------------------------------------
// Model change
//

struct sSetModelParam
{
   Label modelname;
   char pad[sizeof(sReactParamData)-sizeof(Label)];
};

static eReactionResult LGAPI set_model_func(sReactionEvent* event, 
                                            const sReactionParam* p, 
                                            tReactionFuncData )
{
   sSetModelParam* param = (sSetModelParam*)&p->data;
   ObjID obj = p->obj[kReactDirectObj];

   if (obj == OBJ_NULL) obj = event->sensor_obj;

   mxs_vector oldbb,dummy;
   ObjPos* pos = ObjPosGet(obj);
   ObjGetObjOffsetBBox(obj,&oldbb,&dummy);
   ObjSetModelName(obj,param->modelname.text);
   if (pos != NULL)
   {
      mxs_vector newbb;
      ObjGetObjOffsetBBox(obj,&newbb,&dummy);
      pos->loc.vec.z -= newbb.z - oldbb.z;
      ObjTranslate(obj,&pos->loc.vec);
   }
   return kReactionNormal;
}

static sReactionDesc set_model_desc =
{
   REACTION_SET_MODEL,
   "Change Object Model",
   REACTION_PARAM_TYPE(sSetModelParam), 
   kReactionHasDirectObj, 
};

static sFieldDesc model_set_fields[] = 
{
   { "Model Name", kFieldTypeString, FieldLocation(sSetModelParam,modelname), },
};

static sStructDesc model_set_sdesc = StructDescBuild(sSetModelParam,kStructFlagNone,model_set_fields);



static ReactionID create_set_model(IReactions* pReactions)
{
   StructDescRegister(&model_set_sdesc);
   return pReactions->Add(&set_model_desc,set_model_func,NULL);
}

//------------------------------------------------------------
// Turn off a light
//

static eReactionResult LGAPI light_off_func(sReactionEvent* event, 
                                            const sReactionParam* param, 
                                            tReactionFuncData )
{
   sAnimLightProp *anim_light;
   ObjID obj = param->obj[kReactDirectObj];
   if (obj == OBJ_NULL) obj = event->sensor_obj; // back compatible
   
   if (ObjAnimLightGet(obj, &anim_light))
   {
      BroadcastAnimLightMode(obj, ANIM_LIGHT_MODE_EXTINGUISH);
      anim_light->animation.mode = ANIM_LIGHT_MODE_EXTINGUISH;
      anim_light->animation.refresh = TRUE;
      ObjAnimLightSet(obj, anim_light);
   }

   return kReactionNormal;
}

static sReactionDesc light_off_desc =
{
   REACTION_LIGHT_OFF,
   "Animating Light Off",
   NO_REACTION_PARAM,
   kReactionHasDirectObj, 
};

static ReactionID create_light_off(IReactions* pReactions)
{
   return pReactions->Add(&light_off_desc,light_off_func,NULL);
}

//------------------------------------------------------------
// Turn on a light
//

static eReactionResult LGAPI light_on_func(sReactionEvent* event, 
                                           const sReactionParam* param, 
                                           tReactionFuncData )
{
   sAnimLightProp *anim_light;
   ObjID obj = param->obj[kReactDirectObj];
   if (obj == OBJ_NULL) obj = event->sensor_obj;

   if (ObjAnimLightGet(obj, &anim_light))
   {
      BroadcastAnimLightMode(obj, ANIM_LIGHT_MODE_RANDOM);
      anim_light->animation.mode = ANIM_LIGHT_MODE_RANDOM;
      anim_light->animation.refresh = TRUE;
      ObjAnimLightSet(obj, anim_light);
   }

   return kReactionNormal;
}

static sReactionDesc light_on_desc =
{
   REACTION_LIGHT_ON,
   "Animating Light On",
   NO_REACTION_PARAM,
   kReactionHasDirectObj, 
};

static ReactionID create_light_on(IReactions* pReactions)
{
   return pReactions->Add(&light_on_desc,light_on_func,NULL);
}

//------------------------------------------------------------
// Frob an object
//

static eReactionResult LGAPI frob_obj_func(sReactionEvent* event, 
                                           const sReactionParam* param, 
                                           tReactionFuncData )
{
   sFrobActivate frob_info;
   ObjID obj = param->obj[kReactDirectObj];
   if (obj == OBJ_NULL) obj = event->sensor_obj;

   frob_info.frobber=OBJ_NULL;
   frob_info.ms_down=1; // since we dont have time data...
   frob_info.src_obj=obj;
   frob_info.src_loc=kFrobLocWorld;
   frob_info.dst_obj=OBJ_NULL;
   frob_info.dst_loc=kFrobLocNone;
   FrobInstantExecute(&frob_info);
//   do_world_frob_cmd(obj);   

   return kReactionNormal;
}

static sReactionDesc frob_obj_desc =
{
   "frob_obj",
   "Frob Object",
   NO_REACTION_PARAM,
   kReactionHasDirectObj, 
};

static ReactionID create_frob_obj(IReactions* pReactions)
{
   return pReactions->Add(&frob_obj_desc,frob_obj_func,NULL);
}


//------------------------------------------------------------
// Alter ambient radiation level
//

// only works on the player!!
struct sRadParam
{
   int increm;
   char pad[sizeof(sReactParamData)-sizeof(int)];
};


static eReactionResult LGAPI radiate_func(sReactionEvent* event, 
                                           const sReactionParam* p, 
                                           tReactionFuncData )
{
   sRadParam* param = (sRadParam*)&p->data;
   ObjID obj = p->obj[kReactDirectObj];
   if (obj == OBJ_NULL)
   {
       obj = event->sensor_obj;
   }

   float curval = 0.0f;
   // just slam the rad level of the target
   // if it is lower than our intensity
   float maxval = event->stim->intensity;
   // apply armor
   AutoAppIPtr(DPCPlayer);
   ObjID armor = pDPCPlayer->GetEquip(obj, kEquipArmor);
   struct sArmor *pArmor;
   if ((armor != OBJ_NULL) && (ObjHasArmor(armor)))
   {
      if (ArmorGet(armor, &pArmor) && (pArmor->m_radiation != 0))
         maxval = maxval * ((100 - pArmor->m_radiation) / 100);
   }

   if (ArmorGet(obj, &pArmor))
   {
      maxval = maxval * ((100 - pArmor->m_radiation) / 100);
   }

   if (curval < maxval)
   {
      DPCOverlayChange(kOverlayRadiation,kOverlayModeOn);
   }

   return kReactionNormal;
}

static sReactionDesc radiate_desc =
{
   "radiate",
   "Radiate",
   REACTION_PARAM_TYPE(sRadParam), 
   kReactionHasDirectObj, 
};


static sFieldDesc rad_fields[] = 
{
   { "Increm", kFieldTypeFloat, FieldLocation(sRadParam,increm), },
};

static sStructDesc rad_sdesc = StructDescBuild(sRadParam,kStructFlagNone,rad_fields);

//------------------------------------------------------------
// Alter toxin level (poisoning)
//

// only works on the player!!
static eReactionResult LGAPI toxin_func(sReactionEvent* event, 
                                           const sReactionParam* param, 
                                           tReactionFuncData )
{
// @NOTE:  This is here for reference only.  - Jeff
#if 0
   ObjID obj = param->obj[kReactDirectObj];
   if (obj == OBJ_NULL) obj = event->sensor_obj;

   float newval = event->stim->intensity;
   float curval;
   if (!gPropToxin->Get(obj, &curval))
      curval = 0.0f;

   // reduce incoming value based on anti-toxin armor
   AutoAppIPtr(DPCPlayer);
   ObjID armor;
   struct sArmor *pArmor;
   armor = pDPCPlayer->GetEquip(obj,kEquipArmor);
   if (armor != OBJ_NULL)
   {
      if (ArmorGet(armor,&pArmor))
      {
         if (pArmor->m_toxic != 0)
         {
            newval = newval * ((100 - pArmor->m_toxic) / 100);
         }
      }
   }
   
   // psi powers, which set the armor property on the player
   // directly.
   if (ArmorGet(obj, &pArmor))
   {
      if (pArmor->m_toxic != 0)
      {
          newval = newval * ((100 - pArmor->m_toxic) / 100);
      }
   }

   // go poke the property
   if (newval > curval)
   {
      gPropToxin->Set(obj, newval);
      if (newval > 0)
      {
         DPCOverlayChange(kOverlayPoison, kOverlayModeOn);
      }
   }
#endif // 0
   return kReactionNormal;
}

static sReactionDesc toxin_desc =
{
   "toxin",
   "Toxin",
   NO_REACTION_PARAM,
   kReactionHasDirectObj, 
};



//------------------------------------------------------------
// Knock out a guy
//


static eReactionResult LGAPI KO_func(sReactionEvent* event,
                                            const sReactionParam* p,
                                            tReactionFuncData )
{
   ObjID victim = p->obj[kReactDirectObj];
   ObjID culprit = p->obj[kReactIndirectObj];

   // HACK: need to dephysicalize the KO object
   PhysDeregisterModel(culprit);

   // Only AI's can get knocked out.  We do this by telling the
   // AI it's dead, but not telling the damage model.
   AutoAppIPtr_(AIManager,pAIMan);
   cAutoIPtr<IAI> p_AI = pAIMan->GetAI(victim);
   if ((IAI*)p_AI == NULL)
      return kReactionNormal;

   // I'm not the damage model, but I play one on TV
   sDamageMsgData data = { kDamageMsgTerminate, victim, culprit, NULL };
   sDamageMsg msg(kEventKindTerminate, &data, event->stim);

   cAutoIPtr<IInternalAI> pAI(IID_IInternalAI,p_AI);
   pAI->NotifyDamage(&msg);

#if 0
   // Not keeping this stuff for Deep Cover
   // really, only want to do this if the guy isnt already down
   BOOL held=FALSE;
   if (player_is_culprit_for_weapon(culprit, &held))
      if (held)
      {
#if 0
         if (AIGetMode(victim)==kAIM_Dead)
            mprintf("Hey! im (%s %s) already dead, dont knock me out\n",
                     ObjWarnName(victim),ObjWarnName(culprit));
#endif

         DarkStatInc(kDarkStatKnockouts);
         if (!PhysObjOnGround(PlayerObject()))
            DarkStatInc(kDarkStatAerials);
      }
      else
         DarkStatInc(kDarkStatGassed);

#endif
   return kReactionNormal;
}


static sReactionDesc KO_desc =
{
   "Knockout",
   "Knockout",
   NO_REACTION_PARAM,
   kReactionHasDirectObj|kReactionHasIndirectObj,
};

static ReactionID create_KO(IReactions* pReactions)
{
   return pReactions->Add(&KO_desc,KO_func,NULL);
}

////////////////////////////////////////
//------------------------------------------------------------
// Filter on awareness
//


struct sAware
{
   ulong flags; 
   char pad[sizeof(sReactParamData)-sizeof(ulong)];
};

enum eKnockoutFlags
{
   kNotSeen       = 1 << 0,
   kNotHeard      = 1 << 1, 
   kStunned       = 1 << 2, 
   kNoAI          = 1 << 3,
};

static eReactionResult LGAPI aware_func(sReactionEvent* event, 
                                            const sReactionParam* p, 
                                            tReactionFuncData )
{
   sAware* param = (sAware*)&p->data;
   ObjID victim = p->obj[kReactDirectObj];
   ObjID culprit = p->obj[kReactIndirectObj]; 

   if ((param->flags & kStunned) && AIGetStun(victim))
      return kReactionNormal; 

   // Only AI's can be aware
   // AI it's dead, but not telling the damage model. 
   AutoAppIPtr_(AIManager,pAIMan); 
   cAutoIPtr<IAI> p_AI = pAIMan->GetAI(victim); 
   if ((IAI*)p_AI == NULL) 
   {
      return (param->flags & kNoAI) ? kReactionNormal : kReactionAbort; 
   }

   cAutoIPtr<IInternalAI> pAI(IID_IInternalAI,p_AI); 
   ObjID real_culprit = GetRealCulprit(culprit); 

   const sAIAwareness* aware = pAI->GetAwareness(real_culprit); 

   if ((param->flags & kNotSeen) && !(aware->flags & kAIAF_Seen))
      return kReactionNormal; 

   if ((param->flags & kNotHeard) && !(aware->flags & kAIAF_Heard))
      return kReactionNormal; 


   
   return kReactionAbort;
}

static sReactionDesc aware_desc =
{
   "AwareFilter",
   "Awareness Filter",
   REACTION_PARAM_TYPE(sAware), 
   kReactionHasDirectObj|kReactionHasIndirectObj, 
};

static const char* aware_flags[] = 
{
   "Can't see culprit",
   "Can't hear culprit",
   "Stunned",
   "Not an AI",
};

#define NUM_AWARE_FLAGS (sizeof(aware_flags)/sizeof(aware_flags[0]))

static sFieldDesc aware_fields[] = 
{
   { "Abort Unless", kFieldTypeBits, FieldLocation(sAware,flags), kFieldFlagNone, 0, NUM_AWARE_FLAGS, NUM_AWARE_FLAGS,  aware_flags},
};

static sStructDesc aware_sdesc = StructDescBuild(sAware,kStructFlagNone,aware_fields);



static ReactionID create_aware(IReactions* pReactions)
{
   StructDescRegister(&aware_sdesc);
   return pReactions->Add(&aware_desc,aware_func,NULL);
}


void init_hacks(void);

void DPCReactionsInit(void)
{
   InitPropReactions();
   InitDamageReactions();

   AutoAppIPtr(Reactions);
   create_set_model(pReactions);
   create_light_off(pReactions);
   create_light_on(pReactions);
   create_frob_obj(pReactions);
   create_KO(pReactions);
   create_aware(pReactions);

   // rad & toxin
   pReactions->Add(&radiate_desc, radiate_func, NULL);
   StructDescRegister(&rad_sdesc);
   pReactions->Add(&toxin_desc, toxin_func, NULL);

   StunReactionsInit();
}

////////////////////////////////////////

//
// Post load
// This is basically a hack, where we add all stimuli to the contact model
//

void DPCReactionsPostLoad(void)
{

}
