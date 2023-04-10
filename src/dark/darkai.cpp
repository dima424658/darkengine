/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/dark/darkai.cpp,v 1.12 1999/11/19 14:48:33 adurant Exp $
//
//
//

#include <appagg.h>
#include <aggmemb.h>

#include <aiapi.h>
#include <aibasbhv.h>

#include <darkai.h>
#include <daicam.h>
#include <aiturbs.h> //turret
#include <daielem.h>
#include <daifrog.h>

#include <drkmiss.h>


// @TBD (toml 11-08-98): hack include of murus things. clean up after ship
#include <ai.h>
#include <aibasabl.h>
#include <aibasact.h>
#include <aibasbhv.h>
#include <aibascmp.h>
#include <aibasctm.h>
#include <aibasmrg.h>
#include <aibassns.h>
#include <aiactfol.h>
#include <aiactfrb.h>
#include <aiactinv.h>
#include <aiactloc.h>
#include <aiactmot.h>
#include <aiactmov.h>
#include <aiactori.h>
#include <aiactps.h>
#include <aiactsnd.h>
#include <aiacttyp.h>
#include <aiactwnd.h>
#include <aiactwt.h>
#include <aicbhtoh.h>
#include <aicbrngd.h>
#include <aicombat.h>
#include <aidbgcmp.h>
#include <aideath.h>
#include <aidefend.h>
#include <aifollow.h>
#include <aigests.h>
#include <aimove.h>
#include <aiidle.h>
#include <aiinvest.h>
#include <aipatrol.h>
#include <aiscrabl.h>
#include <aisound.h>
#include <aistdmrg.h>
#include <aitest.h>
#include <aicdummy.h>
#include <aistun.h>
#include <aidetect.h>
#include <aiflee.h>
#include <aiwatch.h>
#include <ailisten.h>
#include <aithreat.h>
#include <aialtrsp.h>
#include <aimplyr.h>
#include <aidoormr.h>
#include <aincbdmg.h>
#include <aiinform.h>
#include <aicbtrsp.h>
#include <aicnvabl.h>
#include <aipthfnd.h>
#include <ainet.h>
#include <playrobj.h>

// Must be last header
#include <dbmem.h>


///////////////////////////////////////////////////////////////////////////////

class cDarkAIAppServices : public cCTDelegating<IAIAppServices>,
                           public cCTAggregateMemberControl<kCTU_Default>
{
public:
   cDarkAIAppServices(IUnknown * pOuter)
   {
      MI_INIT_AGGREGATION_1(pOuter, IAIAppServices, kPriorityApp, NULL);
   }
   
   STDMETHOD (AddSoundTags)(THIS_ cTagSet * pTagSet)
   {
      const sMissionData * pMissionData = GetMissionData();
      
      if (pMissionData)
         pTagSet->Add(cTag("Mission", pMissionData->num));

      return S_OK;
   }
};

///////////////////////////////////////////////////////////////////////////////

void DarkAICreateAppServices()
{
    IUnknown * pAppUnknown = AppGetObj(IUnknown);
    new cDarkAIAppServices(pAppUnknown);
    COMRelease(pAppUnknown);
}

///////////////////////////////////////////////////////////////////////////////
//
// The names of Dark-specific behavior sets
//

///////////////////////////////////////////////////////////////////////////////
//
// Macro to temporarily allow fast creation of a behavior set that is 
// identical to a standard one except in name. Eventually each will be
// replaced, most likely, with a true custom behavior set
//

#define DECL_DARK_BSET_(tag, name, base) \
   class cAIDark##tag##BehaviorSet : public base \
   { \
   public: \
      STDMETHOD_(const char *, GetName)() \
      { \
         return name; \
      } \
   }

#define DECL_DARK_BSET(tag, base) DECL_DARK_BSET_(tag, #tag, base)

DECL_DARK_BSET(Haunt,      cAIBehaviorSet);
DECL_DARK_BSET(Zombie,     cAIBehaviorSet);
//DECL_DARK_BSET(Murus,      cAIBehaviorSet);
DECL_DARK_BSET(Apparition, cAIRangedBehaviorSet);
DECL_DARK_BSET(Ape,        cAIBehaviorSet);
DECL_DARK_BSET(Bug,        cAIRangedBehaviorSet);
DECL_DARK_BSET(Constantine,cAIRangedBehaviorSet);
DECL_DARK_BSET(Viktoria,   cAIBehaviorSet);
DECL_DARK_BSET(Clockwork,  cAIBehaviorSet);
DECL_DARK_BSET(Burrick,    cAIRangedBehaviorSet);
DECL_DARK_BSET(Crayman,    cAIBehaviorSet);
DECL_DARK_BSET(Spider,     cAIBehaviorSet);
DECL_DARK_BSET_(SpiderRanged, "Ranged spider",  cAIRangedBehaviorSet);
DECL_DARK_BSET(Sweel,      cAIBehaviorSet);
DECL_DARK_BSET(Turret, cAITurretBehaviorSet);
DECL_DARK_BSET_(Unarmed,   "Human unarmed",        cAIBehaviorSet);
DECL_DARK_BSET_(Sword,     "Human with sword",     cAIBehaviorSet);
DECL_DARK_BSET_(Hammer,    "Human with hammer",    cAIBehaviorSet);
DECL_DARK_BSET_(Bow,       "Human with bow",       cAIRangedBehaviorSet);
DECL_DARK_BSET_(Spellcaster, "Human spellcaster",  cAIRangedBehaviorSet);

///////////////////////////////////////////////////////////////////////////////
//
// cAIElemental behavior set
//

class cAIDarkElementalBehaviorSet : public cAIRangedBehaviorSet
{ 
public: 
   STDMETHOD_(const char *, GetName)()
   {
      return "Elemental";
   }
   
   STDMETHOD_(cAI *, CreateAI)(ObjID id)
   {
      cAI * pAI = new cAIElemental;
      if (pAI->Init(id, this) != S_OK)
         SafeRelease(pAI);
      return pAI;
   }

   void CreateGenericAbilities(cAIComponentPtrs * pComponents)
   {
      cAIRangedBehaviorSet::CreateGenericAbilities(pComponents);
      pComponents->Append(new cAIElementalLightAbility);
   }
};

///////////////////////////////////////////////////////////////////////////////
//
// cAIElemental behavior set
//

class cAIDarkFrogBehaviorSet : public cAIBehaviorSet
{ 
public: 
   STDMETHOD_(const char *, GetName)()
   {
      return "Frog";
   }
   
   void CreateCombatAbilities(cAIComponentPtrs * pComponents)
   {
      pComponents->Append(new cAIFrogCombat);
   }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Hack senses for murus so we can ship this thing (toml 11-08-98)
//

class cAIMurusSenses : public cAISenses
{
public:

   STDMETHOD_(const char *, GetName)()
   {
      return "Murus senses";
   }

   STDMETHOD_(void, OnSound)(const sSoundInfo *, const sAISoundType * pType)
   {
      return;
   }

   STDMETHOD_(void, OnFoundBody)(ObjID body)
   {
      return;
   }
   STDMETHOD_(void, OnDamage)(const sDamageMsg * pMsg)
   {
      return;
   }
   
   STDMETHOD_(void, OnFastObj)(ObjID firer, ObjID projectile, const mxs_vector & velocity)
   {
      return;
   }
   
   STDMETHOD (Update)(ObjID objId, unsigned flags = 0)
   {
      if (objId != PlayerObject())
         return S_OK;
      return cAISenses::Update(objId, flags);
   }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// cAIDarkMurusBehaviorSet behavior set
//

class cAIDarkMurusBehaviorSet : public cAIBehaviorSet
{ 
public: 
   STDMETHOD_(const char *, GetName)()
   {
      return "Murus";
   }
   
   void CreateGenericAbilities(cAIComponentPtrs * pComponents)
   {
      pComponents->Append(new cAIDeath);
      pComponents->Append(new cAIBasicScript);
      pComponents->Append(new cAIStun);
      // pComponents->Append(new cAIDetect);
   }

   void CreateNonCombatAbilities(cAIComponentPtrs * pComponents)
   {
      pComponents->Append(new cAIInvestigate);
      pComponents->Append(new cAIPatrol);
      pComponents->Append(new cAIIdle);
      pComponents->Append(new cAIFlee);
      //pComponents->Append(new cAINonCombatDamageResponse);
      pComponents->Append(new cAIFollow);
      pComponents->Append(new cAISignalResponse);
      pComponents->Append(new cAIAlertResponse);
      //pComponents->Append(new cAIThreatResponse);
      pComponents->Append(new cAIWatch);
      pComponents->Append(new cAIConverse);
      //pComponents->Append(new cAIInform);
      //pComponents->Append(new cAISenseCombatResponse);
   }

   void CreateNonAbilityComponents(cAIComponentPtrs * pComponents)
   {
      // Debugging/development tools
   #ifndef SHIP
      pComponents->Append(new cAIFlowDebugger);
   #endif
      
   #ifdef TEST_ABILITY
      pComponents->Append(new cAITest);
   #endif

      // Enactors
      pComponents->Append(new cAIMoveEnactor);
      pComponents->Append(new cAISoundEnactor);

      // Pathfinder
      pComponents->Append(new cAIPathfinder);
      
      // Movement regulators
      pComponents->Append(new cAIObjectsMovReg);
      pComponents->Append(new cAIWallsCliffsMovReg);
      pComponents->Append(new cAIDoorMovReg);
      
      // Senses
      pComponents->Append(new cAIMurusSenses);
      
      // Defend point watcher
      pComponents->Append(new cAIDefend);

   #ifdef NEW_NETWORK_ENABLED
      // Networking interface
      pComponents->Append(new cAINetwork);
   #endif
   }

};

///////////////////////////////////////////////////////////////////////////////

#define InstallDarkBSet(tag) \
   pBehaviorSet = new cAIDark##tag##BehaviorSet; \
   pAIManager->InstallBehaviorSet(pBehaviorSet); \
   SafeRelease(pBehaviorSet)

EXTERN void DarkAIInit()
{
   AutoAppIPtr(AIManager);
   IAIBehaviorSet * pBehaviorSet;
   
   if (!pAIManager)
      return;
      
   InstallDarkBSet(Haunt);
   InstallDarkBSet(Zombie);
   InstallDarkBSet(Murus);
   InstallDarkBSet(Apparition);
   InstallDarkBSet(Ape);
   InstallDarkBSet(Bug);
   InstallDarkBSet(Frog);
   InstallDarkBSet(Constantine);
   InstallDarkBSet(Viktoria);
   InstallDarkBSet(Elemental);
   InstallDarkBSet(Clockwork);
   InstallDarkBSet(Burrick);
   InstallDarkBSet(Crayman);
   InstallDarkBSet(Spider);
   InstallDarkBSet(SpiderRanged);
   InstallDarkBSet(Sweel);
   InstallDarkBSet(Camera);
   InstallDarkBSet(Turret);
   InstallDarkBSet(Unarmed);
   InstallDarkBSet(Sword);
   InstallDarkBSet(Hammer);
   InstallDarkBSet(Bow);
   InstallDarkBSet(Spellcaster);
   
   DarkAIInitFrogBeast();
}

///////////////////////////////////////

EXTERN void DarkAITerm()
{
   DarkAITermFrogBeast();
}

///////////////////////////////////////////////////////////////////////////////
