/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkcmbat.cpp,v 1.26 2000/02/01 16:40:38 adurant Exp $

// For backstab
#include <aiprcore.h>
#include <aialert.h>
#include <contain.h>
#include <playrobj.h>

#include <esnd.h>
#include <esndprop.h>
#include <ctagset.h>

#include <dmgmodel.h>
#include <dmgbase.h>
#include <objhp.h>

#include <objsys.h>
#include <objdef.h>
#include <objpos.h>
#include <wrtype.h>
#include <rand.h>

// For blood
#include <phcollev.h>

#include <config.h>

#include <property.h>
#include <propbase.h>
#include <propface.h>
#include <propert_.h>
#include <dataops_.h>
#include <prophash.h>

// For collision bashing 
#include <filevar.h>
#include <stimtype.h>
#include <stimbase.h>
#include <stimsens.h>
#include <stimul8r.h>
#include <sdesc.h>
#include <sdesbase.h>
#include <sdestool.h>
#include <simtime.h>
#include <physapi.h>
#include <dmgmodel.h>
#include <dmgbase.h>

#include <prjctile.h>
#include <rendprop.h>
#include <parttype.h>
#include <pgrpprop.h>

// For in_combat
#include <autolink.h>
#include <playrobj.h>
#include <vocore.h>
#include <linkbase.h>
#include <matrix.h>
#include <drkcmbat.h>
#include <drkstats.h>
#include <drkwswd.h>
#include <culpable.h>
#include <aiapi.h>

#ifdef NEW_NETWORK_ENABLED
#include <netman.h>
#include <iobjnet.h>
#include <netmsg.h>
#include <netprops.h>
#endif

// Include this last
#include <dbmem.h>

static IObjectSystem* gpObjSys; 

////////////////////////////////////////////////////////////
// Particle Type property 

static sPropertyDesc ptype_desc = 
{
   "ParticleType",
   0,  //kPropertyNoInherit|kPropertyInstantiate,
   NULL, 0, 0, 
   { "SFX", "Particle Type"    },  
};

static IStringProperty* gpPartTypeProp = NULL; 

static void create_ptype_prop()
{
   gpPartTypeProp = CreateStringProperty(&ptype_desc,kPropertyImplSparseHash); 
}

////////////////////////////////////////////////////////////
// COMBAT GAMESYS VARS 
//


//
// TYPE
// 
struct sCombatVars
{
   int backstab_bonus; 
   float in_combat_dist; 
}; 


//
// SDESC
//

static sFieldDesc combat_fields[] = 
{
   { "backstab bonus",  kFieldTypeInt, FieldLocation(sCombatVars,backstab_bonus) }, 
   { "'in combat' min distance",  kFieldTypeFloat, FieldLocation(sCombatVars,in_combat_dist) }, 

};


static sStructDesc combat_sdesc = StructDescBuild(sCombatVars,kStructFlagNone,combat_fields); 

//
// VAR DESCRIPTOR
//


sFileVarDesc gCombatVarDesc = 
{
   kGameSysVar,         // Where do I get saved?
   "DARKCOMBAT",          // Tag file tag
   "Dark Combat Vars",     // friendly name
   FILEVAR_TYPE(sCombatVars),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version 
   "dark"
}; 

//
// Defaults 
//

static sCombatVars def_vars = 
{
   10000, 
   25.0,
};

//
// Actually use the defaults when resetting
//

class cCombatVars : public cFileVar<sCombatVars,&gCombatVarDesc>
{
   void Reset() 
   {
      sCombatVars& vars = *this; 
      vars = def_vars; 
   }
};

//
// The variable itself 
//
static cCombatVars gCombatVars; 

#ifdef NEW_NETWORK_ENABLED
////////////////////////////////////////////////////////////
// NETWORK DAMAGE FILTER 
//

static IObjectNetworking *g_pObjNet = NULL;
static INetManager *g_pNetMan = NULL;
static cNetMsg *g_pDamageMsg = NULL;

// TRUE iff we're handling some damage that was filtered elsewhere:
static BOOL g_bRemoteDamage = FALSE;

static void handleDamage(ObjID victim, ObjID culprit, sDamage *damage)
{
   // Okay, deal out the damage. We don't actually send a cause; hopefully
   // everyone below can cope?
   AutoAppIPtr(DamageModel);
   g_bRemoteDamage = TRUE;
   pDamageModel->DamageObject(victim, culprit, damage, NULL);
   g_bRemoteDamage = FALSE;
}

static sNetMsgDesc sDamageDesc =
{
   kNMF_SendToObjOwner,
   "Damage",
   "Damage By Local Object",
   NULL,
   handleDamage,
   {{kNMPT_ReceiverObjID, kNMPF_None, "Victim"},
    // The culprit may be local-only, so we can't count upon it
    // actually being hosted here
    // @TBD: Is this going to cause us problems down the line? If the
    // victim's owner doesn't know about the culprit, will it get
    // confused?
    {kNMPT_SenderObjID, kNMPF_NoAssertHostedHere, "Culprit"},
    {kNMPT_Block, kNMPF_None, "Damage", sizeof(sDamage)},
    {kNMPT_End}}
};

//
// The network filter. This deals with the fairly complex ways we have to
// deal with damage, because the circumstances decide who "calls" the
// damage. In general, players always call their own damage -- you can't
// be hurt unless you saw it coming. Other than that, damage is generally
// decided by the culprit. So if a player shoots an AI that is hosted on
// a different machine, and the player thinks that he hit, but the AI
// thought that it had missed, it *did* hit.
//
// Basically, this code is responsible for deciding what we should do with
// this damage. If it's our problem to deal with it, we let it through
// untouched. If it's not our problem at all, then we simply cut it off.
// And if we need to deal with it, but the object is owned elsewhere, we
// need to tell the owner to deal the damage, and then cut it off here.
//
// It's all a tad kludgy, but should do what we need.
//
// @HACK: The above is how things worked in Shock. In Thief, we modify
// this slightly, so that players get no additional benefit -- the culprit
// calls the shot, even if the victim is a player. This allows you to
// blackjack the other players. This will need to be enhanced before
// ship; on a laggy line, a simpleminded shooter-calls will be quite
// annoying. But it'll do for prototyping on the LAN.
//
#define CUTOFF_DAMAGE {damage->amount = 0; return kDamageStatusQuo;}

eDamageResult LGAPI net_damage_filter(ObjID victim, 
                                      ObjID culprit, 
                                      sDamage* damage, 
                                      tDamageCallbackData /* data */)
{
   if (g_bRemoteDamage) {
      // Someone already handed this damage to us, because we owned the
      // victim. Do we, in fact, *still* own the victim?
      if (g_pObjNet->ObjIsProxy(victim)) {
         // Nope; pass it on...
         // @NOTE: we're not bothering to check hops here. If we can
         // ever get into a state where two machines each think the
         // other owns the victim, we're in deep kimchee here. But we'll
         // assume the best, on the theory that we have deeper problems
         // if things get that corrupted...
         g_pDamageMsg->Send(OBJ_NULL, victim, culprit, damage);
         CUTOFF_DAMAGE;
      } else {
         // All is cool
         return kDamageNoOpinion;
      }
   }

   if (!g_pNetMan->Networking())
      return kDamageNoOpinion;

#if 0
   // First, make sure that players call their own damage
   if (IsAPlayer(victim)) {
      if (IsPlayerObj(victim)) {
         // Okay, it's this player, so let it through untouched
      } else {
         // It's another player, so let them make their own decision
         CUTOFF_DAMAGE;
      }
   } else 
#endif
   if (g_pObjNet->ObjLocalOnly(victim)) {
      // Since no one else knows about the victim, we'd better deal
      // the damage ourselves
   } else if (culprit == OBJ_NULL) {
      // Terrain damage, or something like that; only let it happen
      // to our own victims:
      if (g_pObjNet->ObjIsProxy(victim)) {
         CUTOFF_DAMAGE;
      }
   } else if (g_pObjNet->ObjHostedHere(culprit)) {
      // We dealt the damage, so it's our responsibility to make sure
      // it happens
      if (g_pObjNet->ObjIsProxy(victim)) {
         // Tell the victim's owner he's been hurt
         g_pDamageMsg->Send(OBJ_NULL, victim, culprit, damage);
         CUTOFF_DAMAGE;
      } else {
         // Culprit and victim both live here, so just do it
      }
   } else if (g_pObjNet->ObjIsProxy(culprit)) {
      // Since we didn't cause this mess, it's not our problem to deal with it
      CUTOFF_DAMAGE;
   } else {
      // Sound like it's a local-only culprit, such as a bullet
      AssertMsg1(g_pObjNet->ObjLocalOnly(culprit),
                 "Damage culprit %d in weird network state!", culprit);
      BOOL copy;
      if (!gLocalCopyProp->Get(culprit, &copy) || !copy) {
         // We have the original of the culprit
         if (g_pObjNet->ObjIsProxy(victim)) {
            // Tell the victim's owner he's been hurt
            // We can't transmit the culprit, since it's a local-only
            // Hopefully, this won't cause horrible problems
            g_pDamageMsg->Send(OBJ_NULL, victim, OBJ_NULL, damage);
            CUTOFF_DAMAGE;
         } else {
            // Just deal with it ourselves
         }
      } else {
         // We have a copy of the culprit, so it's not out problem
         CUTOFF_DAMAGE;
      }
   }

   // So if we've gotten to this point, we're letting the damage pass
   // through:
   return kDamageNoOpinion;
}

#endif

////////////////////////////////////////////////////////////
// GAME DAMAGE FILTER 
//

BOOL player_is_culprit_for_weapon(ObjID culprit, BOOL *held)
{
   if (culprit == OBJ_NULL)
      return FALSE; 

   if (!PlayerObjectExists())
      return FALSE; 

   if (IsPlayerObj(culprit))
   {
      if (held) *held=TRUE;
      return TRUE;
   }

   if (GetProjectileFirer(culprit) == PlayerObject())
      return TRUE; 
   
   AutoAppIPtr(ContainSys);
   if (pContainSys->IsHeld(PlayerObject(),culprit) != ECONTAIN_NULL)
   {
      if (held) *held=TRUE;
      return TRUE;
   }
   
   return FALSE; 

}

static eDamageResult LGAPI damage_filter(ObjID victim, ObjID culprit, sDamage* damage, tDamageCallbackData /* data */ )
{
   eDamageResult dmgRes=kDamageNoOpinion;
   BOOL held=FALSE;
   // Thieves get double damage for backstabbing :)
   sAIAlertness* alert = AIGetAlertness(victim);
   if (alert && alert->level < kAIAL_Moderate && damage->amount > 0 &&
       player_is_culprit_for_weapon(culprit,&held)) // I'm not alert, players fault
      if (!WeaponIsBlackjack(PlayerObject(),culprit))
      {
         damage->amount = damage->amount*(100+gCombatVars.backstab_bonus)/100; 
         dmgRes=kDamageStatusQuo;
         if (held)
            DarkStatInc(kDarkStatBackStabs);
      }

	// don't let things heal past max hp
   if (damage->amount < 0)
   {
      int hp, maxhp; 
      if (ObjGetHitPoints(victim,&hp) && ObjGetMaxHitPoints(victim,&maxhp))
         if (hp - damage->amount > maxhp)
         {
            damage->amount = hp - maxhp; 
            dmgRes=kDamageStatusQuo; 
         }
   }         

   return dmgRes;
}

////////////////////////////////////////////////////////////
// BLOOD TYPE UTILITY CODE

static sPropertyDesc    BloodTypePropDesc= { "BloodType", 0, NULL, 0, 0, { "Game: Dark", "BloodType" } };
static IStringProperty *pBloodTypeProp = NULL;

static sPropertyDesc    BloodCausePropDesc= { "BloodCause", 0, NULL, 0, 0, { "Game: Dark", "BloodCause" } };
static IBoolProperty  *pBloodCauseProp = NULL;

static sPropertyDesc    BloodMaxDmgPropDesc = { "BloodMaxDmg", 0, NULL, 0, 0, { "Game: Dark", "BloodMaxDamage" } };
static IIntProperty   *pBloodMaxDmgProp = NULL;

static void BloodTypeInit(void)
{
   pBloodTypeProp   = CreateStringProperty(&BloodTypePropDesc,kPropertyImplSparse);
   pBloodCauseProp  = CreateBoolProperty(&BloodCausePropDesc,kPropertyImplSparse);
   pBloodMaxDmgProp = CreateIntProperty(&BloodMaxDmgPropDesc,kPropertyImplSparse);
}

static void BloodTypeTerm(void)
{
   SafeRelease(pBloodTypeProp);
   SafeRelease(pBloodCauseProp);
   SafeRelease(pBloodMaxDmgProp);
}

// im not sure how to get the real collision location for blood?
static void ReleaseBlood(const sDamageMsg* msg)
{
   ObjID victim = msg->victim;
   ObjID hitter = msg->data.damage->kind; 
   const char *blood_arch;
   BOOL cause_bleeding=FALSE;
   
   if (hitter==OBJ_NULL)
      return;
   pBloodCauseProp->Get(hitter,&cause_bleeding); 

   if (cause_bleeding)
      if (pBloodTypeProp->Get(victim,&blood_arch))
      {
         IObjectSystem* pOS = gpObjSys; 
         ObjID arch = pOS->GetObjectNamed(blood_arch);

         mxs_vector dir; 
         dir.x=0.2; dir.y=0.0; dir.z=0.0;

         // Look in the event history for the raw collision event
         sPhysClsnEvent* ev = (sPhysClsnEvent*)msg->Find(kEventKindCollision); 
         if (ev) // It's there, find point of contact
         {
            cPhysClsn* coll = ev->collision; 
               
            mxs_vector pt; 
            pt = coll->GetClsnPt();
             
            ObjID blood = pOS->BeginCreate(arch,kObjectConcrete); 
            ObjTranslate(blood,&pt); 
            pOS->EndCreate(blood);

	    //ok, make sure blood has physics... or delete it if not. 
	    //Should prevent "Bloodstorm" effect.
	    if (!PhysObjHasPhysics(blood))
	      {
		mprintf("Blood %d tried to create out of world.  Destroying it.\n",blood);
 	        pOS->Destroy(blood);
	      }

            int maxDmg;
            if (pBloodMaxDmgProp->Get(blood,&maxDmg))
            {  // get the particle property, tweak the data here
               ParticleGroup *pPGroup = ObjGetParticleGroup(blood);
               float scale=(float)msg->data.damage->amount/(float)maxDmg;
               if (scale<0.33) scale=0.33; else if (scale>1.0) scale=1.0;
               pPGroup->n*=scale;
               ObjSetParticleGroup(blood,pPGroup);
            }
         }
         else
            launchProjectile(victim,arch,0.1,PRJ_FLG_PUSHOUT|PRJ_FLG_GRAVITY,NULL,&dir,NULL);

         SafeRelease(pOS);
      }
}

////////////////////////////////////////////////////////////
// GAME DAMAGE LISTENER
//

#ifdef E398_HACK
EXTERN mxs_vector g_terr_coll_normal;
#endif

#define NUM_PROJ_PARTICLES 5
static eDamageResult LGAPI damage_listener(const sDamageMsg* msg, tDamageCallbackData /* data */ )
{
   switch(msg->kind)
   {
      // I took damage
      case kDamageMsgDamage:
      {
         // msg->data.damage->amount is damage quantity
         // msg->data.damage->kind is damage type, usually stim archetype

         // Play an environmental sound 
         cTagSet Event("Event Damage");

         int damage = msg->data.damage->amount; 

         // stats
         if (damage>0 && (GetRealCulprit(msg->culprit)==PlayerObject()))
         {
            if (msg->victim==PlayerObject())
               DarkStatIntAdd(kDarkStatDamageSelf,damage);              
            else if (ObjIsAI(msg->victim))
               DarkStatIntAdd(kDarkStatDamageDealt,damage);
            else
               DarkStatIntAdd(kDarkStatObjDmgDealt,damage);
         }
         else 
            if (msg->victim==PlayerObject())
               if (damage<0)
                  DarkStatIntAdd(kDarkStatHealingTaken,-damage);
               else
                  DarkStatIntAdd(kDarkStatDamageTaken,damage);

         int hp = 100, maxhp = 100; 
         ObjGetHitPoints(msg->victim,&hp);
         ObjGetMaxHitPoints(msg->victim,&maxhp); 

         if (maxhp <= 0) 
            maxhp = 1; 
         if (damage > maxhp) 
            damage = maxhp; 
         if (hp < 0)
            hp = 0; 

         Event.Append(cTag("Damage",damage*100/maxhp));
         Event.Append(cTag("Health",hp*100/maxhp)); 

         // Add class tags for damage type
         ObjID dmgtype = (ObjID)msg->data.damage->kind; 
         sESndTagList* pDamageTags = NULL; 
         if (ObjGetESndClass(dmgtype,&pDamageTags))
            Event.Append(*pDamageTags->m_pTagSet); 

         ESndPlayLoc(&Event,msg->victim,msg->culprit,&ObjPosGet(msg->victim)->loc.vec); 

         ReleaseBlood(msg);
      }
      break;

      // I got slain 
      case kDamageMsgSlay:
      {
         const char* partname;

         // Play an environmental sound 
         cTagSet Event("Event Death");

         ESndPlayLoc(&Event,msg->victim,(ObjID) msg->data.slay, &ObjPosGet(msg->victim)->loc.vec); 

         // stats
         if (msg->victim==PlayerObject()&&msg->culprit==PlayerObject())
            DarkStatInc(kDarkStatSuicides);
         else if ((GetRealCulprit(msg->culprit)==PlayerObject())||
                  (GetRealCulprit(msg->victim)==PlayerObject()))
            if (ObjIsAI(msg->victim))
               if (DarkStatCheckBit(msg->victim,kDarkStatBitInnocent))
                  DarkStatInc(kDarkStatInnocents);
               else
		 if (DarkStatCheckBit(msg->victim,kDarkStatBitRobot))
		   DarkStatInc(kDarkStatRobotsKilled);
	         else
		   DarkStatInc(kDarkStatKills);
            else
               DarkStatInc(kDarkStatObjsKilled);


         if (gpPartTypeProp->Get(msg->victim,&partname))
         {
            ObjPos* pos = ObjPosGet(msg->victim);
            ObjID arch = gpObjSys->GetObjectNamed(partname);
            // @TODO: move this to a more general place

            // we want to create an object of this archetype, but
            // displace from the wall appropriately... and we might
            // want to tweak its tweq emitters to be oriented away
            // from the wall as well
            ObjID obj = gpObjSys->Create(arch, kObjectConcrete);
            Position p;
            p.fac.tx = Rand();
            p.fac.ty = Rand();
            p.fac.tz = Rand();
            p.loc = pos->loc;
#ifdef E398_HACK
            mx_scale_addeq_vec(&p.loc.vec, &g_terr_coll_normal, ObjGetRadius(obj));
#endif
            // get the bounding box of this object
            UpdateChangedPosition(&p);
            ObjPosCopyUpdate(obj, &p);
         } 
         
      }
   }
   return kDamageNoOpinion;
}
#pragma on(unreferenced)

////////////////////////////////////////////////////////////
// "In Combat" test for voice overs
//


static BOOL LGAPI in_combat(void)
{
   if (!PlayerObjectExists()) return FALSE; 
   
   float mindist = gCombatVars.in_combat_dist; 
   float cutoff = mindist*mindist; 
   ObjPos* pos = ObjPosGet(PlayerObject()); 
   if (!pos)  
      return FALSE; 

   // Find attack links to the player
   cAutoLinkQuery query("~AIAttack",PlayerObject()); 
   for (; !query->Done(); query->Next())
   {
      sLink link; 
      query->Link(&link); 
      
      // compute the distance between the two objects 
      ObjPos *aipos = ObjPosGet(link.dest); 
      if (!aipos)
         continue; 

      if (mx_dist2_vec(&pos->loc.vec,&aipos->loc.vec) < cutoff)
         return TRUE; 
   }

   return FALSE; 
   
}


////////////////////////////////////////////////////////////
// INIT/TERM
//

void DarkCombatInit(void)
{
   AutoAppIPtr_(DamageModel,pDamage);
   pDamage->Filter(damage_filter,NULL); 
   pDamage->Listen(kDamageMsgDamage|kDamageMsgSlay,damage_listener,NULL); 

#ifdef NEW_NETWORK_ENABLED
   // Set up networking:
   g_pObjNet = AppGetObj(IObjectNetworking);
   g_pNetMan = AppGetObj(INetManager);
   g_pDamageMsg = new cNetMsg(&sDamageDesc, NULL);
   // The network damage filter, which makes sure that things only get
   // damaged on their owner's machine. Note that this should happen
   // *after* any other filters are applied.
   pDamage->Filter(net_damage_filter,NULL);
#endif

   AutoAppIPtr_(StructDescTools,pTools); 
   pTools->Register(&combat_sdesc); 

   AutoAppIPtr_(VoiceOverSys,pVoice); 
   pVoice->SetCombatTest(in_combat); 

   gpObjSys = AppGetObj(IObjectSystem); 
   create_ptype_prop(); 
   BloodTypeInit();
}

void DarkCombatTerm(void)
{
   SafeRelease(gpObjSys); 
   SafeRelease(gpPartTypeProp); 
   BloodTypeTerm();
#ifdef NEW_NETWORK_ENABLED
   SafeRelease(g_pObjNet);
   SafeRelease(g_pNetMan);
   delete g_pDamageMsg;
#endif
}
