/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/dmgreact.cpp,v 1.13 2000/02/05 19:38:26 adurant Exp $

#include <appagg.h>
#include <reaction.h>
#include <reacbase.h>
#include <stimbase.h>
#include <sensbase.h>

#include <osysbase.h>

#include <dmgbase.h>
#include <dmgmodel.h>
#include <rand.h>
#include <math.h>

#include <sdesc.h>
#include <sdesbase.h>

// must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// ACT/REACT REACTIONS FOR DAMAGE MODEL
//

//------------------------------------------------------------
// Damage an object
//

struct sDamageParam : public sDamage
{
   float coeff;
   BOOL use_stim;
};


static eReactionResult LGAPI damage_func(sReactionEvent* event, 
                                           const sReactionParam* param, 
                                           tReactionFuncData data)
{
   
   sDamageParam* dmg = (sDamageParam*)&param->data;
   ObjID victim = param->obj[kReactDirectObj];
   ObjID culprit = param->obj[kReactIndirectObj];
   IDamageModel* pDamage = (IDamageModel*)data;

   sDamage damage = *dmg; 
   float float_damage = event->stim->intensity*dmg->coeff; 
   
   // add a random fraction to stochastically preserve precision 
   float_damage += RandRange(0,99)/100.0;  // should this really be a power of 2? 

   damage.amount += (int)floor(float_damage); 

   if (dmg->use_stim)
      damage.kind = event->stim->kind; 

   eDamageResult result = pDamage->DamageObject(victim,culprit,&damage,event->stim);
   return (result >= kDamageDestroy) ? kReactionAbort : kReactionNormal;
}

static sReactionDesc damage_desc =
{
   "damage",
   "Damage Object",
   REACTION_PARAM_TYPE(sDamageParam), 
   kReactionHasDirectObj|kReactionHasIndirectObj|kReactionWorkOnProxy,
};



static sFieldDesc damage_fields[] = 
{
   { "Damage Type", kFieldTypeInt,  FieldLocation(sDamageParam,kind), }, 
   { "Use stimulus as type", kFieldTypeBool, FieldLocation(sDamageParam,use_stim), },
   { "Multiply By", kFieldTypeFloat, FieldLocation(sDamageParam,coeff), },
   { "Then Add", kFieldTypeInt, FieldLocation(sDamageParam,amount), },
};

static sStructDesc damage_sdesc = StructDescBuild(sDamageParam,kStructFlagNone,damage_fields);

static ReactionID create_damage(IReactions* pReactions)
{
   StructDescRegister(&damage_sdesc);
   IDamageModel* damage = AppGetObj(IDamageModel);
   return pReactions->Add(&damage_desc,damage_func,damage);
}

//------------------------------------------------------------
// Spoof Damage to an object.  "Tickle" it, so it takes no damage, but
// acts as if it had taken damage
//

struct sSpoofDamageParam : public sDamage
{
   BOOL use_stim;
};


static eReactionResult LGAPI spoof_damage_func(sReactionEvent* event, 
                                               const sReactionParam* param, 
                                               tReactionFuncData data)
{
   
   sDamageParam* dmg = (sDamageParam*)&param->data;
   ObjID victim = param->obj[kReactDirectObj];
   ObjID culprit = param->obj[kReactIndirectObj];
   IDamageModel* pDamage = (IDamageModel*)data;

   sDamage damage = *dmg; 
   float float_damage = event->stim->intensity*dmg->coeff; 
   
   // add a random fraction to stochastically preserve precision 
   float_damage += RandRange(0,99)/100.0;  // should this really be a power of 2? 

   damage.amount += (int)floor(float_damage); 

   if (dmg->use_stim)
      damage.kind = event->stim->kind; 

   eDamageResult result = pDamage->DamageObject(victim,culprit,&damage,event->stim,TRUE); //last arg says allow zero.
   return (result >= kDamageDestroy) ? kReactionAbort : kReactionNormal;
}

static sReactionDesc spoof_damage_desc =
{
   "spoofdamage",
   "Poke Object",
   REACTION_PARAM_TYPE(sSpoofDamageParam), 
   kReactionHasDirectObj|kReactionHasIndirectObj|kReactionWorkOnProxy,
};



static sFieldDesc spoof_damage_fields[] = 
{
   { "Use stimulus as type", kFieldTypeBool, FieldLocation(sDamageParam,use_stim), },
};

static sStructDesc spoof_damage_sdesc = StructDescBuild(sSpoofDamageParam,kStructFlagNone,damage_fields);

static ReactionID create_spoof_damage(IReactions* pReactions)
{
   StructDescRegister(&spoof_damage_sdesc);
   IDamageModel* damage = AppGetObj(IDamageModel);
   return pReactions->Add(&spoof_damage_desc,spoof_damage_func,damage);
}


//------------------------------------------------------------
// Kill Object
//


static eReactionResult LGAPI slay_func(sReactionEvent* event, 
                                           const sReactionParam* param, 
                                           tReactionFuncData data)
{
   
   ObjID victim = param->obj[kReactDirectObj];
   ObjID culprit = param->obj[kReactIndirectObj];
   IDamageModel* pDamage = (IDamageModel*)data;
   // @TODO: give the user a choice about stim kind? 

   eDamageResult result = kDamageStatusQuo; 
   if (victim != OBJ_NULL)
      pDamage->SlayObject(victim,culprit,event->stim);   

   return (victim == event->sensor_obj && result >= kDamageDestroy) ? kReactionAbort : kReactionNormal;
}

static sReactionDesc slay_desc =
{
   "slay",
   "Slay Object",
   NO_REACTION_PARAM, 
   kReactionHasDirectObj|kReactionHasIndirectObj,
};


static ReactionID create_slay(IReactions* pReactions)
{
   IDamageModel* damage = AppGetObj(IDamageModel);
   return pReactions->Add(&slay_desc,slay_func,damage);
}

////////////////////////////////////////////////////////////
// Create the effects 
//

void InitDamageReactions(void)
{
   AutoAppIPtr(Reactions);
   create_damage(pReactions);
   create_spoof_damage(pReactions);
   create_slay(pReactions);
}
