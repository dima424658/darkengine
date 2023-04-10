/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkarmor.cpp,v 1.7 1998/12/20 12:23:34 JON Exp $

#include <shkarmor.h>

#include <math.h>

#include <cfgdbg.h>
#include <str.h>

#include <appagg.h>
#include <dmgbase.h>
#include <iobjsys.h>
#include <relation.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <osetlnkq.h>
#include <playrobj.h>
#include <propbase.h>
#include <sdesc.h>
#include <sdesbase.h>
#include <trait.h>
#include <traitman.h>
#include <traitbas.h>

#include <shkarmpr.h>
#include <shkimcst.h>
#include <shkmelee.h>
#include <shkplayr.h>
#include <shkplcst.h>
#include <shkpsapi.h>
#include <shkstcst.h>
#include <shktrcst.h>

// must be last header
#include <dbmem.h>

// retarded
#define min(x, y) ((x<y)?(x):(y))

////////////////////////////////////////
//
// Armor Modifier Relation
//

IRelation *g_pArmorMods;

// cached ObjID to projectile archetype
ObjID g_projArch = OBJ_NULL;
const cStr kProjArchName("Projectiles");

static sRelationDesc ArmorModRDesc = 
{
   "Armor Modifier",
}; 

static sRelationDataDesc ArmorModDDesc = {"Armor Modifier", sizeof(float),}; 

static sFieldDesc armorModRawFields = 
   { "Armor Modifier", kFieldTypeFloat, sizeof(float), 0, 0 };
static sStructDesc armorModSDesc = 
   { "Armor Modifier", sizeof(float), kStructFlagNone, 1, &armorModRawFields};

float GetArmorMod(ObjID victim, ObjID source)
{
   float result = 1.0;

   // @TODO: use trait cache 
   AutoAppIPtr(TraitManager);
   IObjectQuery* donors = pTraitManager->Query(victim, kTraitQueryAllDonors); 
   cLinkQueryFactory* factory = CreateSourceSetQueryFactory(g_pArmorMods, source); 
   ILinkQuery* query = CreateObjSetLinkQuery(donors, factory); 

   if (!query->Done())
      result = *((float*)(query->Data()));
   SafeRelease(query); 
   SafeRelease(donors);
   return result;
}

////////////////////////////////////////
//
// Armor Filter
//

eDamageResult LGAPI ShockArmorFilter(ObjID victim, ObjID culprit, sDamage* damage, tDamageCallbackData data)
{
   sArmor *pArmor;

   if ((ArmorGet(victim, &pArmor)) && (pArmor->m_combat>0))
   {
      ConfigSpew("ArmorSpew", ("Damage obj %d, reduced from %d", victim, damage->amount));
      damage->amount -= pArmor->m_combat*GetArmorMod(victim, damage->kind);
      if (damage->amount<0)
         damage->amount = 0;
      ConfigSpew("ArmorSpew", (" to %d\n", damage->amount));
   }
   // Player traits & implants that reduce damage
   if (victim == PlayerObject())
   {
      AutoAppIPtr(ShockPlayer);
      if (pShockPlayer->HasTrait(PlayerObject(), kTraitNimble))
      {
         AutoAppIPtr(TraitManager);
         if (pTraitManager->ObjHasDonor(culprit, g_projArch))
         {
            ConfigSpew("ArmorSpew", ("Damage obj %d, reduced from %d to %d because nimble\n", 
               victim, damage->amount, int(floor(float(damage->amount)*kTraitNimbleDamageFrac+0.5))));
            // argh, where is the round function?
            damage->amount = int(floor(float(damage->amount)*kTraitNimbleDamageFrac+0.5));
         }
      }
      if (pShockPlayer->HasImplant(PlayerObject(), kImplantWormMind))
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
   else if (IsMelee(culprit)) // && (victim != PlayerObject())
   { 
      AutoAppIPtr(ShockPlayer);
      ConfigSpew("ArmorSpew", ("Damage obj %d, changed from %d to %d because strength %d\n", 
         victim, damage->amount, damage->amount+kStrengthMeleeDmgAdd[pShockPlayer->GetStat(kStatStrength)-1], 
         pShockPlayer->GetStat(kStatStrength)-1));
      damage->amount += kStrengthMeleeDmgAdd[pShockPlayer->GetStat(kStatStrength)-1];
      if (damage->amount<0)
         damage->amount = 0;
      if (pShockPlayer->HasTrait(PlayerObject(), kTraitLethal))
      {
         ConfigSpew("ArmorSpew", ("Damage obj %d, increased from %d to %d because player lethal\n", 
               victim, damage->amount, int(floor(float(damage->amount)*kTraitLethalDamageFrac+0.5))));
         damage->amount = int(floor(float(damage->amount)*kTraitLethalDamageFrac+0.5));
      }
   }
   return kDamageStatusQuo;
}

void ShockArmorStartGamemode(void)
{
   AutoAppIPtr(ObjectSystem);
   if ((g_projArch = pObjectSystem->GetObjectNamed(kProjArchName)) == OBJ_NULL)
      Warning(("ShockArmorInit: can't find projectile archetype named %s\n", kProjArchName));
}

void ShockArmorInit(void)
{
   ArmorPropertyInit();
   g_pArmorMods = CreateStandardRelation(&ArmorModRDesc, &ArmorModDDesc, kQCaseSetSourceKnown); 
   StructDescRegister(&armorModSDesc);
   // Install damage listener/filters
   IDamageModel* pDamageModel = AppGetObj(IDamageModel);
   pDamageModel->Filter(ShockArmorFilter,NULL);
   SafeRelease(pDamageModel);
}
 
void ShockArmorTerm(void)
{
   ArmorPropertyTerm();
   SafeRelease(g_pArmorMods);
   g_projArch = OBJ_NULL;
}

