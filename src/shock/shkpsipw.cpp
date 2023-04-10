/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkpsipw.cpp,v 1.8 1999/05/20 18:05:08 XEMU Exp $
// psionics powers
//

#include <shkpsipw.h>

#include <appagg.h>
#include <comtools.h>
#include <cfgdbg.h>
#include <mprintf.h>

#include <property.h>
#include <propbase.h>

#include <iobjsys.h>
#include <linkman.h>
#include <relation.h>
#include <playrobj.h>
#include <lnkquery.h>
#include <linktype.h>
#include <linkbase.h>

#include <shkplayr.h>
#include <shkplcst.h>
#include <shkpsibs.h>
#include <shkpsipr.h>

// must be last header
#include <dbmem.h>

const char *psiPowerNames[kPsiMax+1] = 
{
   "Level1",    
   "FeatherFall",
   "StillHand",
   "Pull",
   "Quickness",    
   "Cyber", 
   "Cryokinesis",  
   "Codebreaker",

   "Level2",
   "Stability",
   "Berserk",
   "RadShield",  
   "Heal",      
   "Might",     
   "Psi",
   "Immolate",

   "Level3",
   "Fabricate",
   "Electro",
   "AntiPsi",
   "ToxinShield",  
   "Radar",    
   "Pyrokinesis",  
   "Terror",

   "Level4",
   "Invisibility",
   "Seeker",
   "Dampen",    
   "Vitality",
   "Alchemy",
   "CyberHack",
   "Sword",

   "Level5",
   "MajorHealing",
   "SomaDrain",    
   "Teleport",
   "Enrage",
   "ForceWall",    
   "Mines",
   "Shield",

   "None",
};

static sPsiPower powers[kPsiMax];
static sPsiShield shields[kPsiMax];
static ObjID psiPowerObjIDs[kPsiMax];

void PsiPowersReset(void)
{
   for (int i=0; i<kPsiMax; i++)
   {
      psiPowerObjIDs[i] = OBJ_NULL;
      shields[i] = defaultPsiShield;
   }
}

/////////////////////////////////////////////

void LGAPI PsiPowerListener(sPropertyListenMsg* pMsg, PropListenerData data)
{
   sPsiPower *pPower;

   g_pPsiPowerProperty->Get(pMsg->obj, &pPower);
   Assert_((pPower->m_power>=0) && (pPower->m_power<kPsiMax));
   if ((psiPowerObjIDs[pPower->m_power] != OBJ_NULL) && (psiPowerObjIDs[pPower->m_power] != pMsg->obj))
      Warning(("Multiple psi power objects for power %s\n", psiPowerNames[pPower->m_power]));
   // fill in table entry indexed by power number
   powers[pPower->m_power] = *pPower;
   psiPowerObjIDs[pPower->m_power] = pMsg->obj;
}

/////////////////////////////////////////////

void LGAPI PsiShieldListener(sPropertyListenMsg* pMsg, PropListenerData data)
{
   sPsiPower *pPower;

   if (g_pPsiPowerProperty->Get(pMsg->obj, &pPower))
   {
      sPsiShield *pShield;
      if (g_pPsiShieldProperty->Get(pMsg->obj, &pShield))
         shields[pPower->m_power] = *pShield;
   }
   else
      Warning(("Object %d is not a psi power (add psi power prop)\n", pMsg->obj));
}

/////////////////////////////////////////////

void PsiPowersInit()
{
   g_pPsiPowerProperty->Listen(kListenPropModify, PsiPowerListener, NULL);
   g_pPsiShieldProperty->Listen(kListenPropModify, PsiShieldListener, NULL);
}

/////////////////////////////////////////////

PsiPowerGet(ePsiPowers power, sPsiPower** ppPower)
{
   Assert_((power>=0) && (power<kPsiMax));
   if (psiPowerObjIDs[power] == OBJ_NULL)
   {
      Warning(("No psi power object for psi power %s\n", psiPowerNames[power]));
      return FALSE;
   }
   *ppPower = &(powers[power]);
   return TRUE;
}

//////////////////////////////////////////////

float PsiPowerGetData(ePsiPowers power, int dataNum)
{
   Assert_((power>=0) && (power<kPsiMax));
   Assert_((dataNum>=0) && (dataNum<kPsiDataNum));

   if (psiPowerObjIDs[power] == OBJ_NULL)
   {
      Warning(("No psi power object for psi power %s\n", psiPowerNames[power]));
      return 0;
   }
   return powers[power].m_data[dataNum];
}

/////////////////////////////////////////////

ObjID PsiPowerGetObjID(ePsiPowers power)
{
   Assert_((power>=0) && (power<kPsiMax));
   return psiPowerObjIDs[power];
}

/////////////////////////////////////////////

#define max(x, y) (((x)>(y))?(x):(y))

tSimTime PsiPowerGetTime(ePsiPowers power, ObjID userID)
{
   Assert_((power>=0) && (power<kPsiMax));
   if ((psiPowerObjIDs[power] == OBJ_NULL) || (powers[power].m_type != kPsiTypeShield))
   {
      Warning(("Psi power %s is not a power or not a shield type power\n", psiPowerNames[power]));
      return 0;
   }
   AutoAppIPtr(ShockPlayer);
   return shields[power].m_baseTime+max(pShockPlayer->GetStat(kStatPsi)-shields[power].m_baseInt, 0)*shields[power].m_addTime;
}


/////////////////////////////////////////////
void ShockTeleportClear()
{
   // go through the level and look for a teleport marker
   IRelation *pRel;
   ILinkQuery *pQuery;      
   ObjID marker;

   AutoAppIPtr(LinkManager);
   AutoAppIPtr(ObjectSystem);

   pRel = pLinkManager->GetRelationNamed("Teleport");
   pQuery = pRel->Query(PlayerObject(), LINKOBJ_WILDCARD);

   if (!pQuery->Done())
   {
      sLink link;
      pQuery->Link(&link);
      marker = link.dest;
   }
   SafeRelease(pQuery);

   pObjectSystem->Destroy(marker);
}
/////////////////////////////////////////////
