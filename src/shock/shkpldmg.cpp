/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkpldmg.cpp,v 1.11 1999/08/05 17:58:39 Justin Exp $

#include <shkpldmg.h>

#include <math.h>

#include <cfgdbg.h>
#include <str.h>

#include <appagg.h>
#include <autolink.h>
#include <dmgbase.h>
#include <iobjsys.h>
#include <linkbase.h>
#include <relation.h>
#include <physapi.h>
#include <playrobj.h>
#include <propbase.h>
#include <traitman.h>
#include <bintrait.h>
#include <objhp.h>
#include <questapi.h>

#include <shkarmpr.h>
#include <shkimcst.h>
#include <shkmelee.h>
#include <shkparam.h>
#include <shkplayr.h>
#include <shkplcst.h>
#include <shkpsapi.h>
#include <shkstcst.h>
#include <shktrcst.h>
#include <shkrsrch.h>

// For damage networking:
#include <netman.h>
#include <iobjnet.h>
#include <netmsg.h>
#include <netprops.h>

// must be last header
#include <dbmem.h>

// this is stupid 
#define min(x, y) ((x<y)?(x):(y))

////////////////////////////////////////
//
// NETWORKING CODE
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
#define CUTOFF_DAMAGE {damage->amount = 0; return kDamageStatusQuo;}

eDamageResult LGAPI ShockNetDamageFilter(ObjID victim, 
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

   // First, make sure that players call their own damage
   if (IsAPlayer(victim)) {
      if (IsPlayerObj(victim)) {
         // Okay, it's this player, so let it through untouched
      } else {
         // It's another player, so let them make their own decision
         CUTOFF_DAMAGE;
      }
   } else if (g_pObjNet->ObjLocalOnly(victim)) {
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

////////////////////////////////////////
//
// Armor Effect Relation
//

IRelation *g_pArmorEffectRel;

static IRelation *g_pOrganRel = NULL;
static ITrait *g_pOrganTrait = NULL;

static sRelationDesc ArmorEffectRDesc = {"Armor Effect", 0, 0, 0}; 
static sRelationDataDesc noDataDesc = { "None", 0 };

////////////////////////////////////////
//
// Armor Filter
// Actually filters player armor & damage inflicted by player
//

eDamageResult LGAPI ShockPlayerDamageFilter(ObjID victim, ObjID culprit, sDamage* damage, tDamageCallbackData data)
{
   if (g_bRemoteDamage)
      // We got sent this damage from someone else, who should have
      // already done this filtering for himself
      return kDamageNoOpinion;

	// don't let things heal past max hp
   if (damage->amount < 0)
   {
      int hp, maxhp; 
      if (ObjGetHitPoints(victim,&hp) && ObjGetMaxHitPoints(victim,&maxhp))
         if (hp - damage->amount > maxhp)
         {
            damage->amount = hp - maxhp; 
            //dmgRes=kDamageStatusQuo; 
         }
   }         

   // Player traits & implants that reduce damage
   if (victim == PlayerObject())
   {
      AutoAppIPtr(ShockPlayer);
      /*
      if (pShockPlayer->HasTrait(PlayerObject(), kTraitNimble))
      {
         if (PhysIsProjectile(culprit))
         {
            ConfigSpew("ArmorSpew", ("Damage obj %d, reduced from %d to %d because nimble\n", 
               victim, damage->amount, int(floor(float(damage->amount)*kTraitNimbleDamageFrac+0.5))));
            // argh, where is the round function?
            damage->amount = int(floor(float(damage->amount)*kTraitNimbleDamageFrac+0.5));
         }
      }
      */
      BOOL slim_mode;
      AutoAppIPtr(QuestData);
      slim_mode = pQuestData->Get("HideInterface");
      if (slim_mode)
      {
         // The player doesn't ever take damage during slim_mode:
         damage->amount = 0;
      }
      else if (pShockPlayer->HasImplant(PlayerObject(), kImplantWormMind))
      {
         int psiDamage = int(floor(float(damage->amount)*kImplantWormMindDamageFrac+0.5));
         AutoAppIPtr(PlayerPsi);
         int psi = pPlayerPsi->GetPoints();
         psiDamage = min(psiDamage, psi);
         
         pPlayerPsi->SetPoints(psi-psiDamage);
         ConfigSpew("ArmorSpew", ("Damage obj %d, reduced from %d to %d (%d to psi)\n", 
            victim, damage->amount, damage->amount-psiDamage, psiDamage));
         damage->amount -= psiDamage;
      }
   }
   // Player traits that affect melee weapon damage
   // This shouldn't really be here, but easier than defining a new filter....
   else 
   {
      if (IsMelee(culprit)) // && (victim != PlayerObject())
      { 
         AutoAppIPtr(ShockPlayer);
         ConfigSpew("ArmorSpew", ("Damage obj %d, changed from %d to %d because strength %d\n", 
            victim, damage->amount, damage->amount+GetMeleeStrengthParams()->m_meleeMods[pShockPlayer->GetStat(kStatStrength)-1], 
            pShockPlayer->GetStat(kStatStrength)-1));
         damage->amount += GetMeleeStrengthParams()->m_meleeMods[pShockPlayer->GetStat(kStatStrength)-1];
         if (damage->amount<0)
            damage->amount = 0;
      }

      // do we have any "organ" links which represent research vulnerabilities?
      if (g_pOrganRel == NULL)
      {
         AutoAppIPtr(LinkManager);
         g_pOrganRel = pLinkManager->GetRelationNamed("Organ");
      }
      if (g_pOrganTrait == NULL)
         g_pOrganTrait = MakeTraitFromRelation(g_pOrganRel);

      ILinkQuery *query;
      sLink slink;
      ObjID organarch;
      int research;

      query = QueryInheritedLinksSingle(g_pOrganTrait,g_pOrganRel, victim, LINKOBJ_WILDCARD); 
      if (!query->Done())
      {
         query->Link(&slink);
         organarch = slink.dest;
         // is our organ researched?
         research = GetResearchQBArch(organarch,"ResearchState");
         if (research == 1)
         {
            sSkillParams *params;
            params = GetSkillParams();
            damage->amount = damage->amount * params->m_researchdmg;
         }
      }
      SafeRelease(query); 
            
   }
   return kDamageNoOpinion;
}

void ShockUnequipArmor(ObjID equipperID, ObjID armorID)
{
   if (armorID != OBJ_NULL)
   {
      AutoAppIPtr(TraitManager);
      cAutoLinkQuery linkQuery(g_pArmorEffectRel, pTraitManager->GetArchetype(armorID), LINKOBJ_WILDCARD);
      while (!linkQuery->Done())
      {
         pTraitManager->RemoveObjMetaProperty(equipperID, linkQuery.GetDest());
         linkQuery->Next();
      }
   }
}

void ShockEquipArmor(ObjID equipperID, ObjID armorID)
{
   //ObjID currentArmorID;

   AutoAppIPtr(ShockPlayer);
   /*
   if ((currentArmorID = pShockPlayer->GetEquip(equipperID, kEquipArmor)) != OBJ_NULL)
      ShockUnequipArmor(equipperID, currentArmorID);
   */   
   if (armorID != OBJ_NULL)
   {
      AutoAppIPtr(TraitManager);
      cAutoLinkQuery linkQuery(g_pArmorEffectRel, pTraitManager->GetArchetype(armorID), LINKOBJ_WILDCARD);
      while (!linkQuery->Done())
      {
         pTraitManager->AddObjMetaProperty(equipperID, linkQuery.GetDest());
         linkQuery->Next();
      }
   }
}

void ShockPlayerDamageInit(void)
{
   // Armor prop & rel
   ArmorPropertyInit();
   g_pArmorEffectRel = CreateStandardRelation(&ArmorEffectRDesc, &noDataDesc, kQCaseSetSourceKnown); 

   // Set up networking:
   g_pObjNet = AppGetObj(IObjectNetworking);
   g_pNetMan = AppGetObj(INetManager);
   g_pDamageMsg = new cNetMsg(&sDamageDesc, NULL);

   // Install damage listener/filters
   IDamageModel* pDamageModel = AppGetObj(IDamageModel);
   // Note that the order here is *essential* -- networking should come
   // after the ordinary filter! This way, we modify the damage for the
   // player's stats before we try to network it:
   pDamageModel->Filter(ShockPlayerDamageFilter,NULL);
   pDamageModel->Filter(ShockNetDamageFilter,NULL);
   SafeRelease(pDamageModel);
}
 
void ShockPlayerDamageTerm(void)
{
   ArmorPropertyTerm();
   SafeRelease(g_pArmorEffectRel);
   SafeRelease(g_pObjNet);
   SafeRelease(g_pNetMan);
   delete g_pDamageMsg;
}

