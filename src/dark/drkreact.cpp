/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkreact.cpp,v 1.24 2000/02/18 22:32:11 adurant Exp $

#include <comtools.h>
#include <appagg.h>

#include <reaction.h>
#include <reactype.h>
#include <reacbase.h>

#include <drkreact.h>
#include <propreac.h>

#include <stimbase.h>

#include <objquery.h>
#include <objedit.h>
#include <playrobj.h>

#include <label.h>
#include <mnamprop.h>
#include <objpos.h>
#include <objshape.h>
#include <matrixs.h>
#include <wr.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <dmgmodel.h>
#include <dmgbase.h>

#include <frobctrl.h>
#include <mediag8r.h>

// For knockout
#include <aiprcore.h>
#include <aialert.h>
#include <chevkind.h>
#include <physapi.h>
#include <aiapi.h>
#include <aiapiiai.h>
#include <culpable.h>
#include <aiaware.h>
#include <aistnapi.h>
#include <physapi.h>

#include <drkcmbat.h>
#include <drkstats.h>

#include <dbmem.h>

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
   frob_info.flags=0;
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
// Knock out a guy
//




static eReactionResult LGAPI KO_func(sReactionEvent* event, 
                                            const sReactionParam* p, 
                                            tReactionFuncData )
{
   ObjID victim = p->obj[kReactDirectObj];
   ObjID culprit = p->obj[kReactIndirectObj]; 

   // @HACK: need to dephysicalize the KO object
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

	 if (DarkStatCheckBit(victim,kDarkStatBitRobot))
	   DarkStatInc(kDarkStatRobotsDeactivated);
	 else
	   DarkStatInc(kDarkStatKnockouts);
         if (!PhysObjOnGround(PlayerObject()))
            DarkStatInc(kDarkStatAerials);
      }
      else
		  if (DarkStatCheckBit(victim,kDarkStatBitRobot))
			  DarkStatInc(kDarkStatRobotsDeactivated);
		  else         
			  DarkStatInc(kDarkStatGassed);
   
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

////////////////////////////////////////

void init_hacks(void);

void InitDarkReactions(void)
{
   InitPropReactions();
   InitDamageReactions();

   AutoAppIPtr(Reactions);
   create_set_model(pReactions);
   create_frob_obj(pReactions);
   create_KO(pReactions); 
   create_aware(pReactions); 

}

////////////////////////////////////////

//
// Post load
// This is basically a hack, where we add all stimuli to the contact model
//

void DarkReactionsPostLoad(void)
{

}


