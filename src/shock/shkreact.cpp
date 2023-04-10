/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkreact.cpp,v 1.14 2000/02/19 13:26:04 toml Exp $

#include <comtools.h>
#include <appagg.h>

#include <reaction.h>
#include <reactype.h>
#include <reacbase.h>

#include <shkreact.h>
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

#include <shkhazpr.h>
#include <shkplayr.h>
#include <shkarmpr.h>
#include <shkovrly.h>
#include <shkovcst.h>
#include <shkplcst.h>

#include <shkstnrc.h>

// Demo hacks
#include <stimuli.h>
#include <stimul8r.h>
#include <stimsens.h>
#include <objtype.h>
#include <stimtype.h>
#include <stimbase.h>
#include <label.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

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
   if (obj == OBJ_NULL) obj = event->sensor_obj;

   float curval, maxval;

   // just slam the rad level of the target
   // if it is lower than our intensity
   maxval = event->stim->intensity;
   // apply armor
   AutoAppIPtr(ShockPlayer);
   ObjID armor = pShockPlayer->GetEquip(obj, kEquipArmor);
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

   if (!gPropRadLevel->Get(obj, &curval))
      curval = 0;
   if (curval < maxval)
   {
      gPropRadLevel->Set(obj, maxval);
      ShockOverlayChange(kOverlayRadiation,kOverlayModeOn);
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
   ObjID obj = param->obj[kReactDirectObj];
   if (obj == OBJ_NULL) obj = event->sensor_obj;

   float newval, curval;
   newval = event->stim->intensity;
   if (!gPropToxin->Get(obj, &curval))
      curval = 0;

   // reduce incoming value based on anti-toxin armor
   AutoAppIPtr(ShockPlayer);
   ObjID armor;
   struct sArmor *pArmor;
   armor = pShockPlayer->GetEquip(obj,kEquipArmor);
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
         ShockOverlayChange(kOverlayPoison, kOverlayModeOn);
      }
   }

   return kReactionNormal;
}

static sReactionDesc toxin_desc =
{
   "toxin",
   "Toxin",
   NO_REACTION_PARAM,
   kReactionHasDirectObj, 
};


////////////////////////////////////////

void init_hacks(void);

void ShockReactionsInit(void)
{
   InitPropReactions();
   InitDamageReactions();

   AutoAppIPtr(Reactions);
   create_set_model(pReactions);
   create_light_off(pReactions);
   create_light_on(pReactions);
   create_frob_obj(pReactions);

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

void ShockReactionsPostLoad(void)
{

}

